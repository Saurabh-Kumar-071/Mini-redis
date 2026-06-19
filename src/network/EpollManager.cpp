#include "EpollManager.h"
#include "../exception/SocketException.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
using namespace std;

EpollManager::EpollManager(){
    epoll_fd = epoll_create1(0);

    if(epoll_fd == -1){
        throw SocketException(string("epoll creation failed: ")+ strerror(errno));
    }
}

EpollManager::~EpollManager(){
    if(epoll_fd != -1){
        close(epoll_fd);
    }
}

void EpollManager::addFd(int fd){
    epoll_event ev{};

    ev.events = EPOLLIN |EPOLLET|EPOLLRDHUP;
    ev.data.fd = fd;

    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&ev) == -1){
        throw SocketException(string("epoll add failed: ")+ strerror(errno));
    }
}

void EpollManager::removeFd(int fd){
    if(epoll_ctl( epoll_fd,EPOLL_CTL_DEL,fd,nullptr) == -1) {
        throw SocketException(string("epoll delete failed: ")+ strerror(errno));
    }
}

int EpollManager::wait(epoll_event events[],int maxEvents){
    return epoll_wait(epoll_fd,events,maxEvents,-1);
}

