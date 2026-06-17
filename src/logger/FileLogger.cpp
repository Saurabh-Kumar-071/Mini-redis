#include "FileLogger.h"
#include<iostream>
using namespace std;

FileLogger::FileLogger(){
   file.open("server.log");
}

 void FileLogger ::info(const string& message){
   file<<"[INFO]"<<message<<endl;
 }

 void FileLogger::error(const string& message){
  file<<"[ERROR]"<<message<<endl;
 }


FileLogger::~FileLogger(){
    file.close();
}
