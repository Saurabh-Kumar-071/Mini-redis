#include "Database.h"
#include <iostream>
using namespace std;

void Database ::set( const string &key , const string& value){//::->Scope Resolution Operator means Which scope does this thing belong to?
          data[key] = value;
}

string Database :: get( const string&key) {

  if(data.find(key)==data.end()){
     return "not-found key in the database yet!";
  }
     return data[key];
}

void Database ::del(const  string &key){

   data.erase(key);
}

const unordered_map<string,string>& Database::getAllData() const{
    return data;
}
