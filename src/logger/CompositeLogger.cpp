
#include "CompositeLogger.h"
using namespace std;

void CompositeLogger::addLogger(ILogger* logger){
    loggers.push_back(logger);
}

void CompositeLogger::info(const string& message){
     for(auto logger : loggers) {
        logger->info(message);
    }
}

void CompositeLogger::error(const string& message){
      for(auto logger : loggers){
        logger->error(message);
    }
}
