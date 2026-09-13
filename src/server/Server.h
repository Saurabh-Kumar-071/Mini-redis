#pragma once
#include "../scheduler/Scheduler.h"
#include "../database/Database.h"
#include "../command/CommandParser.h"
#include "../command/CommandExecutor.h"
#include "../persistence/PersistenceManager.h"
#include "../exception/SocketException.h"
#include "../network/FileDescriptor.h"

#include <atomic>

class ILogger;

class Server
{
private:
    void handleClientEvent(epoll_event& event);

    EpollManager epollManager;
    Database db;
    CommandParser parser;
    PersistenceManager persistence;
    CommandExecutor executor;
    FileDescriptor server_fd;
    Scheduler scheduler;
    ILogger& logger;

    static std::atomic<bool> running;
    static void signalHandler(int signum);

public:
    Server(ILogger& logger);

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();
    void stop();
    void disconnectClient(int fd);
};
