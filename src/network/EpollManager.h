#include <sys/epoll.h>
#include <stdint.h>

class EpollManager{
private:
    int epoll_fd;
public:
    EpollManager();
    ~EpollManager();

    void addFd(int fd);//Start watching this fd.
    void removeFd(int fd);//Stop watching this fd.
    // void modifyFd(int fd , uint32_t events);//Continue watching same fd, but change its events. // implement in future

    int wait(epoll_event events[],int maxEvents);

};
