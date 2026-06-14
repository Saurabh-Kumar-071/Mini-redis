#include "./ClientManager.h"
using namespace std;


void ConnectionManager::addClient(unique_ptr<ClientConnection> client){
    clients[client->getFd()] = move(client);
}

void ConnectionManager::removeClient(int fd){
   clients.erase(fd);
}

ClientConnection* ConnectionManager:: getClient(int fd){
   auto it  = clients.find(fd);

   if(it == clients.end()){
    return nullptr;
   }

   return it->second.get(); // Give me the raw pointer. Don't give me ownership
}

