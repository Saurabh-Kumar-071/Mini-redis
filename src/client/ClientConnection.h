#include "../network/FileDescriptor.h"
#pragma once
using namespace std;
#include<string>

class ClientConnection{
private:
    FileDescriptor fd;
    string readBuffer;
    string writeBuffer;
    bool authenticated;      // true if client has passed AUTH (or no password set)
    string currentUsername;  // which user is logged in ("" if not yet authenticated)
public:
    ClientConnection(int fd, bool startAuthenticated = false);

    int getFd() const;

    string& getReadBuffer();

    string& getWriteBuffer();

    void appendToReadBuffer(const string& data);

    void appendToWriteBuffer(const string& data);

    void clearWriteBuffer();

    void consumeReadBuffer(size_t bytes);

    bool isAuthenticated() const;
    void setAuthenticated(bool value);

    // ACL: track which user is currently logged in on this connection
    const string& getCurrentUsername() const;
    void setCurrentUsername(const string& username);
};
