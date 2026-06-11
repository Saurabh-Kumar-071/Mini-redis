#include <iostream>
#include <sys/socket.h>//socket()
#include <netinet/in.h>//sockaddr_in(ip and port number)
#include <unistd.h>//close()
#include <cstring>
#include <sys/epoll.h>
#include<thread>
#include <cerrno>
#include <fcntl.h>
#include "database/Database.h"
#include <sstream>

using namespace std;

int main()
{
   Database db;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd== -1){
      cout<<"Socket Creation is failed!";
      close(server_fd);
      return 1;
    }
    cout<<"Socket Creation is Successfully"<<endl;

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(server_fd , (sockaddr*)&server_addr , sizeof(server_addr)) < 0){
      cout<<"Bind connection is failed!"<<strerror(errno)<<endl;
      close(server_fd);
      return 1;
    }
    cout<<"Bind connection is successfully"<<endl;

    if(listen(server_fd ,5) <0){
      cout<<"Listen failed!";
      close(server_fd);
    }

    cout<<"Waiting for client"<<endl;
    int epoll_fd = epoll_create1(0);

    if(epoll_fd ==-1){
      cout<<"epoll creation failed!";
      return 1;
    }


    epoll_event ev{};
     ev.events = EPOLLIN;
     ev.data.fd = server_fd;

    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev) == -1){
    cout<<"epoll_ctl failed"<<endl;
    return 1;
     }

     epoll_event events[10];

    while( true){
      int num_events = epoll_wait( epoll_fd , events ,10 ,-1);

      if(num_events ==-1){
        cout<<"epoll_wait failed!"<<endl;
      }

    for(int i = 0; i < num_events; i++){

         if(events[i].data.fd == server_fd){
            cout<<"New Client Arrived"<<endl;
            int client_fd =accept(server_fd,nullptr,nullptr);
            if(client_fd == -1){
             cout<<"Accepted Failed! "<<endl;
             continue;
        }
        epoll_event client_event{};

        client_event.events = EPOLLIN;
        client_event.data.fd = client_fd;

       if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&client_event)==-1){
        cout<<"Client Regestire unsuccessfull with epoll"<<endl;
       }

         cout<<"Accepted client FD:"<<client_fd<<endl;

    }else{
       char buffer[1024];
       int bytes_received = recv(events[i].data.fd , buffer , sizeof(buffer) ,0);

       if(bytes_received > 0){
        cout<<"bytes_received:"<<bytes_received<<endl;
        cout.write(buffer , bytes_received);

        string command(buffer, bytes_received);
        stringstream ss(command);
        string action;
        ss>>action;

        if(action =="PING"){
           string response ="PONG";

           send(events[i].data.fd , response.c_str() , response.size() ,0);
        }
        else if(action == "SET") {

          string key;
          string value;
          ss>>key>>value;
          db.set(key , value);
          string response ="ok\n";
             send(events[i].data.fd , response.c_str() , response.size() ,0);
        }
        else if( action =="GET"){
          string key;
          ss>>key;
          string response = db.get(key);
          send(events[i].data.fd , response.c_str() , response.size() , 0);
        }
        else if(action =="DEL"){
          string key;
          ss>>key;
          db.del(key);
          string response = "Delete successfully from database!";
          send(events[i].data.fd , response.c_str() , response.size() ,0);
        }
        else{
          send(events[i].data.fd , buffer , bytes_received ,0);
        }
       }

       else if (bytes_received ==0){
        cout<<"Client disconnect"<<endl;
         epoll_ctl( epoll_fd,EPOLL_CTL_DEL,events[i].data.fd,nullptr);
         close(events[i].data.fd);
       }

       else{
        cout<<"Error"<<errno<<endl;
       }

    }
 }
}

    close(epoll_fd);
    close(server_fd);
    return 0;
}
