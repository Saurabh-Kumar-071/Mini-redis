
#include "Scheduler.h"


Scheduler::Scheduler(EpollManager& epollManager): epollManager(epollManager){
}

int Scheduler::waitForEvents(epoll_event events[],int maxEvents){

    return epollManager.wait(events,maxEvents);
}
                                                 //somelamda
void Scheduler ::registerHandler(int fd,std::function<void(epoll_event&)> handler){
     contexts[fd].handler = handler;
}


void Scheduler::dispatch(epoll_event& event){ // dispatch function
    int fd = event.data.fd;

    auto it = contexts.find(fd);

    if(it != contexts.end() && it->second.handler)
    {
        it->second.handler(event);
    }
}

ClientConnection* Scheduler::getClient(int fd)
{
    auto it = contexts.find(fd);

    if(it == contexts.end())
        return nullptr;

    return it->second.client.get();
}


void Scheduler::registerContext(int fd,std::unique_ptr<ClientConnection> client,std::function<void(epoll_event&)> handler){
    contexts[fd].client = std::move(client);
    contexts[fd].handler = handler;
}


void Scheduler::removeContext(int fd){
    contexts.erase(fd);
}
