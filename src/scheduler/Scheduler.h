
#pragma once

#include "../network/EpollManager.h"
#include <sys/epoll.h>
#include<unordered_map>
#include <functional>

class Scheduler{
private:
    EpollManager& epollManager;

    std::unordered_map<int,std::function<void(epoll_event&)>> handlers; //handlers[fd](event);

public:
    Scheduler(EpollManager& epollManager);

    int waitForEvents(epoll_event events[],int maxEvents);

    void run();

    void registerHandler(int fd,std::function<void(epoll_event&)> handler);

    void removeHandler(int fd);

    void dispatch(epoll_event& event);

};
