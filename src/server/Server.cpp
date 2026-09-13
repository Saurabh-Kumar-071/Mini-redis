#include "Server.h"
#include "../logger/ILogger.h"
#include "../protocol/RESPEncoder.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <csignal>

using namespace std;

std::atomic<bool> Server::running{true};

void Server::signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        Server::running.store(false);
    }
}

void Server::stop() {
    running.store(false);
}

Server::Server(ILogger &logger)
    : logger(logger), executor(db, persistence), server_fd(-1), scheduler(epollManager) {
    persistence.load(db);
    // Load password from environment variable MINIREDIS_PASSWORD
    executor.loadPasswordFromEnv();
    // Show ACL / auth status on startup
    if (executor.hasPassword()) {
        logger.info("ACL ENABLED — default user loaded from MINIREDIS_PASSWORD. Use AUTH <user> <pass> or AUTH <pass>.");
    } else {
        logger.info("[WARN] ACL DISABLED — set MINIREDIS_PASSWORD env var or use ACL SETUSER to require authentication.");
    }
}

bool setNonBlocking(int fd);

void Server::handleClientEvent(epoll_event &event)
{
    ClientConnection *client = scheduler.getClient(event.data.fd);
    if (client == nullptr)
        return;

    // Handle writable event (drain write buffer)
    if (event.events & EPOLLOUT)
    {
        string &buffer = client->getWriteBuffer();

        while(!buffer.empty()){
            int sent = send(event.data.fd, buffer.c_str(), buffer.size(), 0);

            if (sent == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    logger.error(string("send failed: ") + strerror(errno));
                    disconnectClient(event.data.fd);
                    return;
                }
            }

            if (sent > 0)
            {
                buffer.erase(0, sent);
            }
        }

        if (!buffer.empty()){
            return;
        }

        if(scheduler.isPeerClosed(event.data.fd)){
            disconnectClient(event.data.fd);
            return;
        }

        epollManager.modifyFd(event.data.fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
        return;
    }

    // Handle readable event
    char buffer[4096];

    while (true)
    {
        int bytes_received = recv(event.data.fd, buffer, sizeof(buffer), 0);

        if (bytes_received > 0)
        {
            string incoming(buffer, bytes_received);
            client->appendToReadBuffer(incoming);

            while(true){
                ParseResult result;
                try {
                    result = parser.parseRESP(client->getReadBuffer());
                } catch (...) {
                    client->consumeReadBuffer(client->getReadBuffer().size());
                    break;
                }

                if(!result.complete) break;
                if (result.command.arguments.empty()) break;

                CommandResponse response;
                try {
                    response = executor.execute(result.command, *client);
                } catch (const exception& e) {
                    response = {ResponseType::Error, string("ERR ") + e.what()};
                } catch (...) {
                    response = {ResponseType::Error, "ERR internal server error"};
                }

                string encodedResponse;
                switch(response.type){
                    case ResponseType::SimpleString:
                        encodedResponse = RESPEncoder::simpleString(response.value);
                        break;
                    case ResponseType::BulkString:
                        encodedResponse = RESPEncoder::bulkString(response.value);
                        break;
                    case ResponseType::Integer:
                        try {
                            encodedResponse = RESPEncoder::integer(stoll(response.value));
                        } catch (...) {
                            encodedResponse = RESPEncoder::error("ERR integer overflow");
                        }
                        break;
                    case ResponseType::Error:
                        encodedResponse = RESPEncoder::error(response.value);
                        break;
                    case ResponseType::Null:
                        encodedResponse = RESPEncoder::nullBulkString();
                        break;
                    case ResponseType::Array:
                        encodedResponse = RESPEncoder::array(response.array);
                        break;
                }

                // If writeBuffer already has pending data, preserve order by appending
                if (!client->getWriteBuffer().empty()) {
                    client->appendToWriteBuffer(encodedResponse);
                    epollManager.modifyFd(event.data.fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT);
                } else {
                    int sent = send(event.data.fd, encodedResponse.c_str(), encodedResponse.size(), 0);

                    if (sent == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            sent = 0;
                        }
                        else
                        {
                            logger.error(string("send failed: ") + strerror(errno));
                            scheduler.markPeerClosed(event.data.fd);
                            disconnectClient(event.data.fd);
                            return;
                        }
                    }

                    if (static_cast<size_t>(sent) < encodedResponse.size())
                    {
                        client->appendToWriteBuffer(encodedResponse.substr(sent));
                        epollManager.modifyFd(event.data.fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT);
                    }
                }

                client->consumeReadBuffer(result.bytesConsumed);
            }
        }
        else if (bytes_received == 0)
        {
            scheduler.markPeerClosed(event.data.fd);

            if(client->getWriteBuffer().empty()){
                disconnectClient(event.data.fd);
            }
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // No more data to read for now
            }
            else
            {
                logger.error(string("Recv failed: ") + strerror(errno));
                scheduler.markPeerClosed(event.data.fd);
                if(client->getWriteBuffer().empty()){
                    disconnectClient(event.data.fd);
                }
            }
            break;
        }
    }
}

void Server::start()
{
    // Ignore SIGPIPE to avoid crashing when writing to closed sockets
    signal(SIGPIPE, SIG_IGN);

    // Register signal handlers for graceful shutdown
    signal(SIGINT, Server::signalHandler);
    signal(SIGTERM, Server::signalHandler);

    server_fd = FileDescriptor(socket(AF_INET, SOCK_STREAM, 0));

    if (server_fd.get() == -1)
    {
        throw SocketException("Socket Creation is failed!");
    }

    int opt = 1;
    setsockopt(server_fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (!setNonBlocking(server_fd.get()))
    {
        throw SocketException("Failed to make server socket non-blocking");
    }

    logger.info("Socket Creation Successfully");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd.get(), (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        throw SocketException(string("Bind connection failed! :") + strerror(errno));
    }

    logger.info("Bind connection is successfully");

    if (listen(server_fd.get(), 128) < 0)
    {
        throw SocketException(string("Listen failed!") + strerror(errno));
    }

    logger.info("Server listening on port 8080. Waiting for clients...");

    epollManager.addFd(server_fd.get());
    epollManager.createTimer();
    epollManager.addFd(epollManager.getTimerFd());

    // TimerFD handler for active TTL sweeps and RDB snapshot persistence
    scheduler.registerHandler(epollManager.getTimerFd(), [this](epoll_event&) {
        uint64_t expirations = 0;
        ssize_t bytes = read(epollManager.getTimerFd(), &expirations, sizeof(expirations));

        if (bytes != sizeof(expirations)){
            return;
        }
        if(db.cleanupExpiredKeys()){
            persistence.markDirty();
        }
        persistence.saveIfDirty(db);
    });

    // Accept handler for incoming client connections
    scheduler.registerHandler(server_fd.get(), [this](epoll_event &) {
        while(true){
            int client_fd = accept(server_fd.get(), nullptr, nullptr);

            if(client_fd == -1){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    break;
                }
                logger.error("Client connection accept failed!");
                continue;
            }

            if(!setNonBlocking(client_fd)){
                logger.error("Failed to make client socket non-blocking");
                close(client_fd);
                continue;
            }

            // If no password is configured, clients are authenticated immediately
            bool noPasswordSet = !executor.hasPassword();
            auto client = make_unique<ClientConnection>(client_fd, noPasswordSet);

            scheduler.registerContext(client_fd, move(client), [this](epoll_event& event){
                handleClientEvent(event);
            });

            epollManager.addFd(client_fd);
            logger.info(string("Accepted Client FD: ") + to_string(client_fd));
        }
    });

    epoll_event events[64];
    running.store(true);

    while (running.load())
    {
        int num_events = scheduler.waitForEvents(events, 64);

        if (num_events == -1)
        {
            if (errno == EINTR)
            {
                if (!running.load()) break;
                continue;
            }
            throw SocketException(string("epoll_wait failed! ") + strerror(errno));
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].events & EPOLLRDHUP)
            {
                scheduler.markPeerClosed(events[i].data.fd);
            }

            scheduler.dispatch(events[i]);
        }
    }

    logger.info("Graceful shutdown: persisting database snapshot to dump.rdb...");
    persistence.saveIfDirty(db);
    logger.info("Server shutdown cleanly.");
}

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
    {
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }

    return true;
}

void Server::disconnectClient(int fd)
{
    epollManager.removeFd(fd);
    scheduler.removeContext(fd);
}
