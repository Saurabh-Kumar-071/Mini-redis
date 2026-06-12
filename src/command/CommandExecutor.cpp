
#include "CommandExecutor.h"
using namespace std;

CommandExecutor::CommandExecutor( Database& database): db(database){ // ?

}

string CommandExecutor ::execute(const ParsedCommand& cmd){

  if(cmd.command =="PING"){
    return "PONG";
  }

  else if(cmd.command=="SET"){
     db.set(cmd.key , cmd.value);
     return "OK";
  }

  else if(cmd.command=="GET"){
    return db.get(cmd.key);
  }

  else if(cmd.command=="DEL"){
    db.del(cmd.key);
    return "delete from database!";
  }

  return "UNKNOWN COMMMAND!";

  }
