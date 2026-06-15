
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

  return "UNKNOWN COMMMAND!";

  }
