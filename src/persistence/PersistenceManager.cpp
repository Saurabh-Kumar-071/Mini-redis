

#include "PersistenceManager.h"
#include "../database/Database.h"
#include <fstream>

void  PersistenceManager::save(const Database& db){

  ofstream file("dump.rdb");  // open the file if not create yet then create it (for write )

  if(!file) return; // like it file is full then it open the you return

  const auto& data = db.getAllData();

  for( const auto& pair : data){
    file<<pair.first<<"="<<pair.second<<endl;
  }

}

void PersistenceManager ::load( Database& db){
   ifstream file("dump.rdb"); // open the file and read

   if(!file)return;
  string line;

  while(getline(file ,line)){

    size_t pos = line.find('=');

    if(pos == string::npos)continue;

    string key = line.substr(0,pos);
    string value = line.substr(pos+1);

    db.set(key ,value);
  }


}
