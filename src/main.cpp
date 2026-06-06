#include <iostream>
#include <sys/socket.h>//socket()
#include <netinet/in.h>//sockaddr_in(ip and port number)
#include <unistd.h>//close()
#include <cstring>
#include<thread>

using namespace std;

void handleClient(int client_fd){

    char buffer[1024]{};

    int bytes_received = recv(client_fd , buffer , sizeof(buffer),0);

    if(bytes_received >0){
       cout<<"Received:"<<buffer<<endl;

       send(client_fd , buffer , bytes_received ,0);
    }
    close(client_fd);
}

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

    while(true){
    int client_fd = accept(server_fd , nullptr , nullptr);

    if(client_fd ==-1){
      cout<<"Client connection is failed!"<<endl;
      continue;
    }

    thread t(handleClient ,client_fd);
    t.detach();

    }

    close(server_fd);
    return 0;
}
