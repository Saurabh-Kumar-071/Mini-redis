#pragma once

#include <string>

using namespace std;

class RESPEncoder{
public:
    static string simpleString(const string& str);

    static string bulkString(const string& str);

    static string integer(long long value);

    static string error(const string& str);

    static string nullBulkString();
};
