#include "../network/FileDescriptor.h"
#pragma once
using namespace std;
#include<string>

class ClientConnection{
private:
    FileDescriptor fd;
    string readBuffer;
    string writeBuffer;
public:
    ClientConnection(int fd);

    int getFd() const;

    string& getReadBuffer();

    string& getWriteBuffer();

    void appendToReadBuffer(const string& data);

    void clearReadBuffer();

    void appendToWriteBuffer(const string& data);

    void clearWriteBuffer();

};
