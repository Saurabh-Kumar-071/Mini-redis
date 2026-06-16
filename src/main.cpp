#include<iostream>
#include "server/Server.h"
using namespace std;

int main(){

try{
     Server server;
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
