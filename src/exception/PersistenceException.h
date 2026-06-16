
#pragma once

#include "MiniRedisException.h"

class PersistenceException: public MiniRedisException{
public:
    explicit PersistenceException(const std::string& message): MiniRedisException(message){
    }
};
