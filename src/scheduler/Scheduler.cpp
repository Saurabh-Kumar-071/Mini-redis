
#include "Scheduler.h"


Scheduler::Scheduler(EpollManager& epollManager): epollManager(epollManager){
}

int Scheduler::waitForEvents(epoll_event events[],int maxEvents){

    return epollManager.wait(events,maxEvents);
}
                                                 //somelamda
void Scheduler ::registerHandler(int fd,std::function<void(epoll_event&)> handler){
    handlers[fd] = handler; //Now fd  knows which function to call.
}

void Scheduler :: removeHandler(int fd){
    handlers.erase(fd);
}

void Scheduler::dispatch(epoll_event& event){ // dispatch function
    int fd = event.data.fd;

    auto it = handlers.find(fd);

    if(it != handlers.end())
    {
        it->second(event);
    }
}





