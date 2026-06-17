
#pragma once

#include "ILogger.h"
#include<fstream>

class FileLogger : public ILogger {

  private:
  ofstream file;

  public:
  FileLogger();

  void info(const string& message)override;
  void error(const string& message)override;
  ~FileLogger() override;
};
