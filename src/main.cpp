#include <iostream>
#include <sys/socket.h>//socket()
#include <netinet/in.h>//sockaddr_in(ip and port number)
#include <unistd.h>//close()
#include <cstring>
#include <sys/epoll.h>
#include<thread>
#include <cerrno>
#include <fcntl.h>

using namespace std;



int main()
{
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
      cout<<"Bind connection is failed!"<<endl;
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

         cout<<"Accepted client FD:"<<client_fd<<endl;
         close(client_fd);
    }
 }
}

    close(epoll_fd);
    close(server_fd);
    return 0;
}
