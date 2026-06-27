
#pragma once

#include <string>

using namespace std;

enum class ResponseType{ // ?
    SimpleString,
    BulkString,
    Integer,
    Error,
    Null
};

struct CommandResponse{
    ResponseType type; 
    string value;
};
