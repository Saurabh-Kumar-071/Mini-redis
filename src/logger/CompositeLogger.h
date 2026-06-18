
#pragma once
#include "ILogger.h"
#include<vector>
using namespace std;


class CompositeLogger: public ILogger{
  private:
    vector<ILogger*>loggers;

    public:
    void addLogger(ILogger* logger);

  void info(const string& message)override;
  void error(const string& message)override;
};
