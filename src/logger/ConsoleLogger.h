
#pragma once

#include "ILogger.h"

class ConsoleLogger : public ILogger{
public:

    void info(const std::string& message) override;

    void error(const std::string& message) override;
};
