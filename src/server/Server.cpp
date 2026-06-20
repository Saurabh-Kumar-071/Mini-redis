#include "Server.h"
#include "../logger/ILogger.h"
#include "../network/EpollManager.h"
#include <iostream>
#include <sys/socket.h>//socket()
#include <netinet/in.h>//sockaddr_in(ip and port number)
#include <unistd.h>//close()
#include <cstring>
#include<thread>
#include <cerrno>
#include <fcntl.h>
#include <sstream>
using namespace std;

Server::Server(ILogger& logger): logger(logger), executor(db ,persistence),server_fd(-1){
    persistence.load(db);
}

EpollManager epollManager;

bool setNonBlocking(int fd);
void Server::start(){

    server_fd = FileDescriptor(socket(AF_INET, SOCK_STREAM, 0));

   if(server_fd.get()== -1){
       throw SocketException("Socket Creation is failed!");
    }

   if(!setNonBlocking(server_fd.get())){
      throw SocketException("Failed to make server socket non-blocking");
     }

    logger.info("Socket Creation Successfully");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(server_fd.get() , (sockaddr*)&server_addr , sizeof(server_addr)) < 0){
      throw SocketException(string("Bind connection is failed!")+strerror(errno));
    }

    logger.info("Bind connection is successfully");

    if(listen(server_fd.get() ,5) <0){
       throw SocketException(string("Listen failed!")+strerror(errno));
    }

    logger.info("Waiting for client");

    epollManager.addFd(server_fd.get());

     epoll_event events[10];

    while( true){
      int num_events = epollManager.wait(events ,10);

    if(num_events ==-1){
         throw SocketException(string("epoll_wait failed!")+strerror(errno));
      }

    for(int i = 0; i < num_events; i++){

        if(events[i].events & EPOLLRDHUP){
          logger.info("Client disconnected");

          epollManager.removeFd(events[i].data.fd);

          connectionManager.removeClient(events[i].data.fd);

          continue;
        }

        if(events[i].events & EPOLLOUT){
         ClientConnection* client =connectionManager.getClient(events[i].data.fd);
         if(client == nullptr)continue;

         string& buffer =client->getWriteBuffer();

       int sent =send(events[i].data.fd,buffer.c_str(),buffer.size(),0);
    
        if(sent == -1){
              if(errno == EAGAIN ||errno == EWOULDBLOCK){
                   sent = 0;
                     }
               else{
                 logger.error(string("send failed: ") + strerror(errno));
                 epollManager.removeFd(events[i].data.fd );

                 connectionManager.removeClient(events[i].data.fd);

                  continue;
                }
              }

        if(sent > 0){
         buffer.erase(0, sent);
             if(buffer.empty()){
              epollManager.modifyFd(events[i].data.fd,EPOLLIN |EPOLLET |EPOLLRDHUP);
            }
               }
          continue;
     }

         if(events[i].data.fd == server_fd.get()){
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
            connectionManager.addClient(move(client));


        if(!setNonBlocking(client_fd)){
         logger.error("Failed to make client socket non-blocking" );
         close(client_fd);
          continue;
          }

         epollManager.addFd(client_fd);

        logger.info(string("Accepted Client FD:" )+ to_string(client_fd));
        }
    }else{
       char buffer[1024];

       while(true){
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
         int sent = send(events[i].data.fd , response.c_str(), response.size() ,0);

         if(sent == -1){
              if(errno == EAGAIN ||errno == EWOULDBLOCK){
                   sent = 0;
                     }
               else{
                 logger.error(string("send failed: ") + strerror(errno));
                }
              }

              if(sent < response.size()){
                 ClientConnection* client = connectionManager.getClient(events[i].data.fd);
                 client->appendToWriteBuffer( response.substr(sent));

                 epollManager.modifyFd(events[i].data.fd,EPOLLIN |EPOLLET | EPOLLRDHUP |EPOLLOUT);
                  cout << "Partial write detected"<< endl;
              }


         client->clearReadBuffer();
       }

       else if (bytes_received ==0){

        cout<<"Client disconnect"<<endl;
        epollManager.removeFd(events[i].data.fd);

         connectionManager.removeClient(events[i].data.fd);
         break;
       }

      else{

         if(errno == EAGAIN || errno == EWOULDBLOCK){
        cout << "No more data available" << endl;
            }
       else{
        logger.error(string("Recv failed: ") + strerror(errno));
         }
        break;
        }
    }
  }
 }
}

}
// this is the function for checking the socket is blcokig

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if(flags == -1){
        return false;
    }

    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
        return false;
    }

    return true;
}

