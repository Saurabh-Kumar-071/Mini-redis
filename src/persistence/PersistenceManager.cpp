

#include "PersistenceManager.h"
#include "../database/Database.h"
#include <fstream>
#include<iostream>
void  PersistenceManager::save(const Database& db){

  ofstream file("dump.rdb");  // open the file if not create yet then create it (for write )

  if(!file) {// like it file is full then it open the you return
      throw PersistenceException("Failed to open dump.rdb for writing");
  }
  const auto& data = db.getAllData();
  const auto& expiry = db.getAllExpiry();

  for( const auto& pair : data){

    auto it = expiry.find(pair.first);

    if(it!= expiry.end()){
      auto timestamp = chrono::system_clock::to_time_t(it->second);

      file<<pair.first<<"="<<pair.second<<"|"<<timestamp<<endl;
    }

    else{
      file<<pair.first<<"="<<pair.second<<endl;
    }
  }
}

void PersistenceManager ::load( Database& db){
   ifstream file("dump.rdb"); // open the file and read

   if(!file){
     cout<<"No dumb.rdb file is found and starting with empty database "<<endl;
     return;
   }
  string line;

  while(getline(file ,line)){

    size_t pos = line.find('=');

    if(pos == string::npos)continue;

    string key = line.substr(0,pos);
    string valuePart = line.substr(pos+1);

    size_t ttlpos = valuePart.find('|');

    if(ttlpos == string::npos){
      db.set(key,valuePart);
    }

    else{
       string value = valuePart.substr(0,ttlpos);
       string timestampStr = valuePart.substr(ttlpos+1);

       time_t timestamp = stoll(timestampStr);

      auto expiryTime = chrono::system_clock::from_time_t(timestamp);

      if(expiryTime <=chrono::system_clock::now()){
          continue;
         }

      db.set(key,value);

      db.setExpiryTime(key,expiryTime);
    }
  }
}
