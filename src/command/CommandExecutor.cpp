
#include "CommandExecutor.h"

using namespace std;

CommandExecutor::CommandExecutor( Database& database , PersistenceManager& persistence): db(database) ,persistence(persistence){ // constructor initializer list.

}

string CommandExecutor ::execute(const ParsedCommand& cmd){

  if(cmd.command =="PING"){
    return "PONG";
  }

  else if(cmd.command=="SET"){
     db.set(cmd.key , cmd.value);
     persistence.save(db);
     return "OK";
  }

  else if(cmd.command=="GET"){
    return db.get(cmd.key);
  }

  else if(cmd.command=="DEL"){
    db.del(cmd.key);
    persistence.save(db);
    return "delete from database!";
  }

  else if(cmd.command=="EXPIRE"){
    int val = stoi(cmd.value);
    db.expire(cmd.key ,val);
    persistence.save(db);
    return "ok";
  }

  else if(cmd.command=="TTL"){
    return to_string(db.ttl(cmd.key));
  }

  else if(cmd.command == "BIG"){
    return string(10000000, 'A'); // only for testing why EPOLLOUT needed
}

  return "UNKNOWN COMMMAND!";

  }
