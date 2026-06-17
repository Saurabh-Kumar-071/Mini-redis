
#include "ConsoleLogger.h"
#include<iostream>
using namespace std;

 void ConsoleLogger ::info(const string& message){
   cout<<"[INFO]"<<message<<endl;
 }

 void ConsoleLogger::error(const string& message){
  cout<<"[ERROR]"<<message<<endl;
 }

 