#pragma once

#include <stdexcept>
#include <string>
using namespace std;

class MiniRedisException: public runtime_error{
public:
    explicit MiniRedisException(const string& message): runtime_error(message){

    }
};
