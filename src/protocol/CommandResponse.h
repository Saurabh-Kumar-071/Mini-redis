
#pragma once

#include <string>

using namespace std;

enum class ResponseType{ // ?
    SimpleString,
    BulkString,
    Integer,
    Error,
    Null,
    Array
};

struct CommandResponse{
    ResponseType type;
    string value;
     vector<string> array;
};
