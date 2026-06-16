
#pragma once

#include "MiniRedisException.h"

class DatabaseException: public MiniRedisException{
public:
    explicit DatabaseException(const std::string& message): MiniRedisException(message){
    }
};
