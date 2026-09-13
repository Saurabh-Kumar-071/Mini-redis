
#include "ClientConnection.h"              //RAII (Resource Acquisition Is Initialization).
#include <unistd.h>

ClientConnection::ClientConnection(int fd, bool startAuthenticated)
    : fd(fd), authenticated(startAuthenticated), currentUsername("") {
}

int ClientConnection::getFd() const{
    return fd.get();
}


void ClientConnection::appendToReadBuffer(const string& data){
    readBuffer += data;
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

void ClientConnection::consumeReadBuffer(size_t bytes){
    readBuffer.erase(0, bytes);
}

bool ClientConnection::isAuthenticated() const {
    return authenticated;
}

void ClientConnection::setAuthenticated(bool value) {
    authenticated = value;
}

const string& ClientConnection::getCurrentUsername() const {
    return currentUsername;
}

void ClientConnection::setCurrentUsername(const string& username) {
    currentUsername = username;
}
