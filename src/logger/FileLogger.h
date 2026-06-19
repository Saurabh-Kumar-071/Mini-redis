
#pragma once

#include "ILogger.h"
#include<fstream>

class FileLogger : public ILogger {

  private:
  ofstream file;

  public:
  FileLogger();

  FileLogger(const FileLogger&) = delete;
  FileLogger& operator=(const FileLogger&) = delete;

  void info(const string& message)override;
  void error(const string& message)override;
  ~FileLogger() override;
};
