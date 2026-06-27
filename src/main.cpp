#include<iostream>
#include "server/Server.h"
#include "logger/ConsoleLogger.h"
#include "logger/FileLogger.h"
#include "logger/CompositeLogger.h"

using namespace std;

int main(){


try{
     FileLogger fileLogger;
     ConsoleLogger consoleLogger;
     CompositeLogger logger;

    logger.addLogger(&fileLogger);
    logger.addLogger(&consoleLogger);
     Server server(logger);
   server.start();
  }
  catch(const SocketException& e){
    cout<<e.what()<<endl;
  }
  catch(const MiniRedisException& e){
    cout<<e.what()<<endl;
  }
  catch(const exception& e){
    cout<<"Standard Error:"<<e.what()<<endl;
  }
  catch(...){
    cout << "Unknown Error" << endl;
  }

    return 0;
}
