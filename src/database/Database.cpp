#include "Database.h"
#include <iostream>
#include<vector>
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

   auto it  = data.find(key);
   if(it ==data.end()){
      return "not-found";
   }

   return it->second;
}

void Database ::del(const  string &key){

   data.erase(key);
}

const unordered_map<string,string>& Database::getAllData() const{
    return data;
}

 void Database::expire(const string& key,int seconds){
    auto it  = data.find(key);
    if(it ==data.end()) return;

    expiry[key] = chrono::system_clock::now() + chrono::seconds(seconds);
 }

   bool Database:: isExpired(const string& key){

    auto it  = expiry.find(key);
      if(it == expiry.end()) return false;

      auto now  = chrono::system_clock::now();
      if(now >= it->second) return true;
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

     auto it  = expiry.find(key);
       if(it == expiry.end()){
       return -1; //key exists but no TTL
      }

      auto remaining_time = it->second - chrono::system_clock::now();

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

 vector<string> Database::keys() const{
    vector<string> result;
    result.reserve(data.size());

    for (const auto& pair : data){  //const auto& pair → uses a reference without copying.
        result.push_back(pair.first);
    }
    return result;
}

void Database::clear(){  //Your Database API should describe what it does, not which Redis command called it.
    data.clear();
    expiry.clear();
}

bool Database::persist(const string& key) {
    auto it = expiry.find(key);
   if(it!=expiry.end() ){
     expiry.erase(it);
     return true;
   }
 return false;
}

 int Database::append(const string& key, const string& value){
    auto& str = data[key];
     str += value;
return str.size();
 }

  int Database::strlen(const string& key) const{

   auto it = data.find(key);
   if(it == data.end())
       return 0;

return it->second.size();
  }


string Database::getset(const string& key,const string& value){
    auto it  = data.find(key);
    if(it!=data.end()){
   string old_val = it->second;
    it->second = value;
   return old_val;
    }
return "(nil)";

}

int Database::incrby(const string& key, int increment){

    if (!exists(key)) {
        set(key, to_string(increment));
        return increment;
    }

     int value = stoi(get(key));
    value += increment;
    set(key, to_string(value));

    return value;
}

int Database::decrby(const string& key, int decrement){
    return incrby(key, -decrement);
}

size_t Database::size() const{
   return data.size();
}

string Database::info() const{
    return "MiniRedis\nKeys: " + to_string(data.size()) +"\nTTL Keys: " + to_string(expiry.size());
}

bool Database::rename(const string& oldKey,   const string& newKey){
    auto it = data.find(oldKey);

    if(it == data.end())
        return false;

    data[newKey] = std::move(it->second);
    data.erase(it);

    auto exp = expiry.find(oldKey);

    if(exp != expiry.end())
    {
        expiry[newKey] = exp->second;
        expiry.erase(exp);
    }

    return true;
}


