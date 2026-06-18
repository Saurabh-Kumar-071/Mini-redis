#include "Server.h"
#include "../logger/ILogger.h"
#include <iostream>
#include <sys/socket.h>//socket()
#include <netinet/in.h>//sockaddr_in(ip and port number)
#include <unistd.h>//close()
#include <cstring>
#include <sys/epoll.h>
#include<thread>
#include <cerrno>
#include <fcntl.h>
#include <sstream>
using namespace std;

Server::Server(ILogger& logger): logger(logger), executor(db ,persistence){
    persistence.load(db);
}

bool setNonBlocking(int fd);
void Server::start(){

   int server_fd = socket(AF_INET, SOCK_STREAM, 0);

   if(server_fd== -1){
       throw SocketException("Socket Creation is failed!");
    }

   if(!setNonBlocking(server_fd)){
      throw SocketException("Failed to make server socket non-blocking");
     }

    logger.info("Socket Creation Successfully");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(server_fd , (sockaddr*)&server_addr , sizeof(server_addr)) < 0){
      throw SocketException(string("Bind connection is failed!")+strerror(errno));
    }

    logger.info("Bind connection is successfully");

    if(listen(server_fd ,5) <0){
       throw SocketException(string("Listen failed!")+strerror(errno));
    }

    logger.info("Waiting for client");
    int epoll_fd = epoll_create1(0);

    if(epoll_fd ==-1){
     throw SocketException(string("epoll creation failed!")+strerror(errno));
    }

    epoll_event ev{};
     ev.events = EPOLLIN;
     ev.data.fd = server_fd;

    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev) == -1){
       throw SocketException(string("epoll_ctl failed")+strerror(errno));
     }

     epoll_event events[10];

    while( true){
      int num_events = epoll_wait( epoll_fd , events ,10 ,-1);

    if(num_events ==-1){
         throw SocketException(string("epoll_wait failed!")+strerror(errno));
      }

    for(int i = 0; i < num_events; i++){

         if(events[i].data.fd == server_fd){
            logger.info("New Client Arrived");

            int client_fd =accept(server_fd,nullptr,nullptr);

            if(client_fd == -1){
            logger.error("Client connection Accepted Failed!");
             continue;
             }

            auto client = make_unique<ClientConnection>(client_fd);
            connectionManager.addClient(move(client));


        if(!setNonBlocking(client_fd)){
         logger.error("Failed to make client socket non-blocking" );
         close(client_fd);
          continue;
          }

        epoll_event client_event{};

        client_event.events = EPOLLIN;
        client_event.data.fd = client_fd;

       if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&client_event)==-1){
         logger.info("Client Regestire unsuccessfull with epoll");
       }

        logger.info(string("Accepted Client FD:" )+ to_string(client_fd));

    }else{
       char buffer[1024];
       int bytes_received = recv(events[i].data.fd , buffer , sizeof(buffer) ,0);

       if(bytes_received > 0){
        ClientConnection* client = connectionManager.getClient(events[i].data.fd);

        cout<<"bytes_received:"<<bytes_received<<endl;
        cout.write(buffer , bytes_received);

         string incoming(buffer, bytes_received);
         client->appendToReadBuffer(incoming);

         ParsedCommand cmd = parser.parse(client->getReadBuffer());
        if(cmd.command.empty()) continue;

         string response = executor.execute(cmd);
         send(events[i].data.fd , response.c_str(), response.size() ,0);
         client->clearReadBuffer();
       }

       else if (bytes_received ==0){

        cout<<"Client disconnect"<<endl;
         epoll_ctl( epoll_fd,EPOLL_CTL_DEL,events[i].data.fd,nullptr);

         connectionManager.removeClient(events[i].data.fd);
       }

      else{

         if(errno == EAGAIN || errno == EWOULDBLOCK){
        cout << "No more data available" << endl;
            }
       else{
        logger.error(string("Recv failed: ") + strerror(errno));
         }
        }
    }
 }
}

    close(epoll_fd);
    close(server_fd);
}
// this is the function for checking the socket is blcokig

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if(flags == -1)
    {
        return false;
    }

    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }

    return true;
}
