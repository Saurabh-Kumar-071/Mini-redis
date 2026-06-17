#pragma once

#include <string>
using namespace std;

class ILogger{
public:

    virtual ~ILogger() = default;

    virtual void info(const string& message) = 0;

    virtual void error( const string& message) = 0;
};
