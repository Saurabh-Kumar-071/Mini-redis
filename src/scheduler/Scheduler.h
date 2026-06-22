
#pragma once

#include "../network/EpollManager.h"
#include "../client/ClientConnection.h"
#include <sys/epoll.h>
#include<unordered_map>
#include <functional>
#include <memory>

struct EventContext{
    std::function<void(epoll_event&)> handler; ////handlers[fd](event);
    std::unique_ptr<ClientConnection> client;
};

class Scheduler{
private:
    EpollManager& epollManager;

    std::unordered_map<int,EventContext> contexts;

public:
    Scheduler(EpollManager& epollManager);

    int waitForEvents(epoll_event events[],int maxEvents);

    void run();

     void registerContext(int fd,std::unique_ptr<ClientConnection> client,std::function<void(epoll_event&)> handler);

     void removeContext(int fd);

    void registerHandler(int fd,std::function<void(epoll_event&)> handler); // server socket only


     void dispatch(epoll_event& event);

     ClientConnection* getClient(int fd);

};
