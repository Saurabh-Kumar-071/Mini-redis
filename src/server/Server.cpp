#include "Server.h"
#include "../logger/ILogger.h"
#include <iostream>
#include <sys/socket.h> //socket()
#include <netinet/in.h> //sockaddr_in(ip and port number)
#include <unistd.h>     //close()
#include <cstring>
#include <thread>
#include <cerrno>
#include <fcntl.h>
#include <sstream>
using namespace std;
// ----------------member initializer list.----------------------------------//
Server::Server(ILogger &logger) : logger(logger), executor(db, persistence), server_fd(-1), scheduler(epollManager)
{
  persistence.load(db); //[ Construct executor    //[Create FileDescriptor  //[Give Scheduler a reference
                        // using this Server's db   //containing invalid fd.]  //to Server's epollManager.]
                        // and persistence objects.]
}

bool setNonBlocking(int fd);

void Server::handleClientEvent(epoll_event &event)
{
  cout << "handleClientEvent FD = "<< event.data.fd<< endl;

  if (event.events & EPOLLOUT)
  {
      // cout << "EPOLLOUT fired" << endl;
    ClientConnection *client = scheduler.getClient(event.data.fd);
    if (client == nullptr)
      return;

    string &buffer = client->getWriteBuffer();
    // cout<<"Edge triggered"<<endl;
    // cout<<"write buffer size:"<<buffer.size()<<endl;

     while(!buffer.empty()){  // write drain loop
       int sent = send(event.data.fd, buffer.c_str(), buffer.size(), 0);
    // cout << "EPOLLOUT send returned = "<< sent<< endl;



    if (sent == -1)
    {
      // cout << "send errno = "<< errno<< " "<< strerror(errno)<< endl;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
        break; // when the kernal is full then call again EPOLLOUT to send remaining data
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
      // cout<<"Buffer size after send: "<<buffer.size()<<endl;
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

  ClientConnection *client =
      scheduler.getClient(event.data.fd);

  if (client == nullptr)
    return;
  char buffer[1024];

  while (true)
  {
    int bytes_received = recv(event.data.fd, buffer, sizeof(buffer), 0);
    // cout << "recv returned = "<< bytes_received << endl;

    if (bytes_received > 0)
    {

      string incoming(buffer, bytes_received);
      // cout<<"Received :"<<incoming<<endl;
      client->appendToReadBuffer(incoming);

      ParsedCommand cmd = parser.parse(client->getReadBuffer());
      // cout<<"cmd command:"<<" ,"<<cmd.command<<"cmd key:"<<cmd.key<<" ,"<<"cmd val:"<<" "<<cmd.value<<endl;
      if (cmd.arguments.empty())
        continue;

      string response = executor.execute(cmd);

      // cout<<"Response:"<<response<<endl;
      int sent = send(event.data.fd, response.c_str(), response.size(), 0);

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

          if(client->getWriteBuffer().empty()){
           disconnectClient(event.data.fd);
          }

        }
      }

      if (sent < response.size())
      {
        //  cout << "PARTIAL WRITE" << endl;
        //  cout << "Sent = " << sent << endl;
        //  cout << "Total = " << response.size() << endl;
        client->appendToWriteBuffer(response.substr(sent));

        epollManager.modifyFd(event.data.fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT);
        // cout << "Partial write detected" << endl;
      }

      client->clearReadBuffer();
    }

    else if (bytes_received == 0)
    {
      cout << "Client disconnect" << endl;

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
        cout << "No more data available" << endl;
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

  server_fd = FileDescriptor(socket(AF_INET, SOCK_STREAM, 0));

  if (server_fd.get() == -1)
  {
    throw SocketException("Socket Creation is failed!");
  }

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
    throw SocketException(string("Bind connection is failed! :") + strerror(errno));
  }

  logger.info("Bind connection is successfully");

  if (listen(server_fd.get(), 5) < 0)
  {
    throw SocketException(string("Listen failed!") + strerror(errno));
  }

  logger.info("Waiting for client");

  epollManager.addFd(server_fd.get());
  ///

  scheduler.registerHandler(server_fd.get(), [this](epoll_event &)
                            {
            logger.info("New Client Arrived");

            while(true){

             int client_fd =accept(server_fd.get(),nullptr,nullptr);

            if(client_fd == -1){
              if(errno==EAGAIN ||errno == EWOULDBLOCK){
                break;
              }
            logger.error("Client connection Accepted Failed!");
             continue;
             }

            auto client = make_unique<ClientConnection>(client_fd);


        if(!setNonBlocking(client_fd)){
         logger.error("Failed to make client socket non-blocking" );
         close(client_fd);
          continue;
          }

   /////
        scheduler.registerContext(client_fd,move(client),[this](epoll_event& event){

          handleClientEvent(event);
        }
           );

         epollManager.addFd(client_fd);

        logger.info(string("Accepted Client FD:" )+ to_string(client_fd));
        } });

  epoll_event events[10];

  while (true)
  {

    int num_events = scheduler.waitForEvents(events, 10);

    if (num_events == -1)
    {
      throw SocketException(string("epoll_wait failed!") + strerror(errno));
    }

    for (int i = 0; i < num_events; i++)
    {



      // cout << "FD = "<< events[i].data.fd<< " EVENTS = "<< events[i].events<< endl;

      if (events[i].events & EPOLLRDHUP)
      {
        logger.info("Peer closed");
        // cout << "RDHUP received for fd " << events[i].data.fd << endl;
        scheduler.markPeerClosed(events[i].data.fd);
      }

     scheduler.dispatch(events[i]);
    }
  }
}
// this is the function for checking the socket is blcokig

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
    ClientConnection* client =
        scheduler.getClient(fd);

    // if(client)
    // {
    //     cout << "Disconnecting fd "
    //          << fd
    //          << " writeBuffer size = "
    //          << client->getWriteBuffer().size()
    //          << endl;
    // }

    epollManager.removeFd(fd);
    scheduler.removeContext(fd);
}
