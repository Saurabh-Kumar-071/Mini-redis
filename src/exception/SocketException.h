#pragma once

#include "MiniRedisException.h"

class SocketException: public MiniRedisException{
public:
    explicit SocketException(const string& message): MiniRedisException(message){
    }
};
