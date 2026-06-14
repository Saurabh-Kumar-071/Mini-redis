#include<unordered_map>
#include <memory>  // this header control the unique_ptr(this is not copied)
#include "./ClientConnection.h"
using namespace std;

#pragma once

class ConnectionManager{
  private:
  unordered_map<int,unique_ptr<ClientConnection>>clients;

  public:
  void addClient(unique_ptr<ClientConnection> client);

ClientConnection* getClient(int fd);

void removeClient(int fd);

};
