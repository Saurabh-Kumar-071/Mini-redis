#include "Database.h"
#include <iostream>
using namespace std;

void Database ::set( const string &key , const string& value){//::->Scope Resolution Operator means Which scope does this thing belong to?
          data[key] = value;
}


string Database :: get( const string&key) {


  if(isExpired(key)){
    data.erase(key);
    expiry.erase(key);
    return "Not-found";
   }

   if(data.find(key) ==data.end()){
      return "not-found";
   }

   return data[key];
}

void Database ::del(const  string &key){

   data.erase(key);
}

const unordered_map<string,string>& Database::getAllData() const{
    return data;
}

 void Database::expire(const string& key,int seconds){
   if(data.find(key) ==data.end()) return;

    expiry[key] = chrono::system_clock::now() + chrono::seconds(seconds);
 }

   bool Database:: isExpired(const string& key){

      if(expiry.find(key) == expiry.end()) return false;

      auto now  = chrono::system_clock::now();
      if(now >=expiry[key]) return true;
      return false;
   }

   int Database::ttl(const string& key){

      if(data.find(key) == data.end()){
        return -2; //key doesn't exist
      }

      if(isExpired(key)){
        data.erase(key);
        expiry.erase(key);
        return -2;
       }

       if(expiry.find(key) == expiry.end()){
       return -1; //key exists but no TTL
      }

      auto remaining_time = expiry[key] - chrono::system_clock::now();

       return chrono::duration_cast<chrono::seconds>(remaining_time).count();
  }

  bool Database::exists(const string& key){
    return data.find(key) != data.end();
}

  const unordered_map<string,chrono::system_clock::time_point>& Database:: getAllExpiry() const{
    return expiry;
}


void Database::setExpiryTime(const string&key , const chrono::system_clock::time_point& tp){
   expiry[key] = tp;
}
