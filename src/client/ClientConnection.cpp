
#include "ClientConnection.h"              //RAII (Resource Acquisition Is Initialization).
#include <unistd.h>

ClientConnection::ClientConnection(int fd): fd(fd){
}

ClientConnection::~ClientConnection(){
    if(fd != -1){
        close(fd);
    }
}

int ClientConnection::getFd() const{
    return fd;
}

void ClientConnection::appendToReadBuffer(const string& data){
    readBuffer += data;
}

void ClientConnection::clearReadBuffer(){
  readBuffer.clear();
}

string& ClientConnection::getReadBuffer(){
  return readBuffer;
}

void ClientConnection::appendToWriteBuffer(const string& data){
  writeBuffer+=data;
}

void ClientConnection::clearWriteBuffer(){
  writeBuffer.clear();
}

string& ClientConnection::getWriteBuffer(){
  return writeBuffer;
}
