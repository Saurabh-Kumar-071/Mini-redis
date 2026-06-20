
#include "FileDescriptor.h"

FileDescriptor::FileDescriptor(int fd): fd(fd){

}


  FileDescriptor::~FileDescriptor(){
    if(fd != -1){
        close(fd);
    }
}

int FileDescriptor::get() const{
    return fd;
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept: fd(other.fd){ // move constructor
    other.fd = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept{ // move assignment
    if(this != &other){
        if(fd != -1){
            close(fd);
        }
        fd = other.fd;
        other.fd = -1;
    }

    return *this;
}


