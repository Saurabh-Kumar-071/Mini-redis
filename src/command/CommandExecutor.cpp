
#include "CommandExecutor.h"
#include<stdbool.h>
using namespace std;

CommandExecutor::CommandExecutor( Database& database , PersistenceManager& persistence): db(database) ,persistence(persistence){ // constructor initializer list.

}

string CommandExecutor ::execute(const ParsedCommand& cmd){

  if(cmd.arguments.empty()) return "UNKNOWN COMMAND!";

  const string& command = cmd.arguments[0];

  if(command =="PING"){
    return "PONG";
  }

  else if(command=="SET"){
       if (cmd.arguments.size() != 3) return "ERR wrong number of arguments";

    const string& key = cmd.arguments[1];
    const string& value = cmd.arguments[2];
     db.set(key , value);
     persistence.save(db);
     return "OK";
  }

  else if(command=="GET"){
    if (cmd.arguments.size() != 2) return "ERR wrong number of arguments";

    const string& key = cmd.arguments[1];
    return db.get(key);
  }

  else if(command=="DEL"){
    if (cmd.arguments.size() != 2)  return "ERR wrong number of arguments";

    const string& key = cmd.arguments[1];
    db.del(key);
    persistence.save(db);
    return "delete from database!";
  }

  else if(command=="EXPIRE"){
       if (cmd.arguments.size() != 3)  return "ERR wrong number of arguments";

    const string& key = cmd.arguments[1];
    const string& value = cmd.arguments[2];
    int val = stoi(value);
    db.expire(key ,val);
    persistence.save(db);
    return "ok";
  }

  else if(command=="TTL"){
     if (cmd.arguments.size() != 2) return "ERR wrong number of arguments";

    const string& key = cmd.arguments[1];
    return to_string(db.ttl(key));
  }

  else if(command == "BIG"){
    return string(10000000, 'A'); // only for testing why EPOLLOUT needed
}

  else if(command == "MSET"){

    if(cmd.arguments.size() < 3  || cmd.arguments.size() %2 ==0) return "ERR wrong number of argument";

    for(size_t i = 1 ; i < cmd.arguments.size() ;i+=2){
      const string& key = cmd.arguments[i];
      const string& val = cmd.arguments[i+1];

      db.set(key ,val);
    }
     persistence.save(db);
     return "ok";
  }

  else if(command == "EXISTS"){
    if(cmd.arguments.size() !=2) return "ERR wrong number of argument";
     const string& key = cmd.arguments[1];
      return db.exists(key) ? "1" : "0";
  }

  if(command == "MGET"){
    if(cmd.arguments.size() < 2) return "ERR wrong number of arguments";

    string result;
    for(size_t i = 1; i < cmd.arguments.size(); i++){
        result += db.get(cmd.arguments[i]);

        if(i + 1 < cmd.arguments.size())
            result += " ";
    }
    return result;
}

 else if(command=="MDEL"){
    int deleted = 0;
    for(size_t  i =1 ; i<cmd.arguments.size() ;i++){

       const string& key = cmd.arguments[i];
       if(db.exists(key)){
        db.del(key);
         deleted++;
       }
    }
    persistence.save(db);
    return to_string(deleted);
 }

 else if(command=="INCR"){
    const string& key = cmd.arguments[1];

    try{
      if (!db.exists(key)){
       db.set(key, "1");
       persistence.save(db);
      return "1";
      }

      int value = stoi(db.get(key));
      value++;

      db.set(key , to_string(value));
      persistence.save(db);

    return to_string(value);
    }
    catch(const invalid_argument&){
       return "ERR value is not an integer";
    }
 }

 else if(command == "DECR"){
   const string& key = cmd.arguments[1];

   try{
      if (!db.exists(key)){
      db.set(key, "1");
      persistence.save(db);
      return "1";
     }

      int value = stoi(db.get(key));
      value--;

      db.set(key ,to_string(value));
      persistence.save(db);
    return to_string(value);
   }
   catch(const invalid_argument&){
     return "ERR value is not an integer";
   }
 }

 else if(command == "KEYS"){
   if (cmd.arguments.size() != 1) return "ERR wrong number of arguments";

   vector<string>keys  = db.keys();
   string response="";
   for (size_t i = 0; i < keys.size(); i++){
    response += keys[i];
    if (i + 1 < keys.size())
        response += " ";
      }
return response;
 }

 else if(command == "FLUSHDB"){
  if(cmd.arguments.size()!=1) return "ERR wrong number of arguments";

    db.clear();
    persistence.save(db);
  return "OK";
 }

 else if(command == "PERSIST"){
     if(cmd.arguments.size()!=2) return "ERR wrong number of arguments";

      const string& key = cmd.arguments[1];
     if(db.persist(key)){
      persistence.save(db);
      return "1";
     }
    return "0";
 }

 else if(command == "APPEND"){
    if(cmd.arguments.size() != 3) return "ERR wrong number of arguments";

    int len = db.append(cmd.arguments[1], cmd.arguments[2]);
    persistence.save(db);
    return to_string(len);
}

else if(command == "STRLEN"){
    if(cmd.arguments.size() != 2) return "ERR wrong number of arguments";

    const string&key = cmd.arguments[1];
    return to_string(db.strlen(key));
}

else if(command =="GETSET"){
   if(cmd.arguments.size() != 3) return "ERR wrong number of arguments";
   const string& key = cmd.arguments[1];
   const string& value = cmd.arguments[2];
    string old_val = db.getset(key,value);
   persistence.save(db);
  return old_val;
}

else if(command == "INCRBY"){
    if(cmd.arguments.size() != 3) return "ERR wrong number of arguments";

    const string& key = cmd.arguments[1];

    int increment;

    try{
        increment = stoi(cmd.arguments[2]);
    }
    catch(...){
        return "ERR increment is not an integer";
    }

    try{
        int ans = db.incrby(key, increment);

        persistence.save(db);

        return to_string(ans);
    }
    catch(...){
        return "ERR value is not an integer";
    }
}

else if(command == "DECRBY"){
    if(cmd.arguments.size() != 3)   return "ERR wrong number of arguments";

    const string& key = cmd.arguments[1];
    int decrement;

    try {
        decrement = stoi(cmd.arguments[2]);
    }
    catch(...){
        return "ERR decrement is not an integer";
    }

    try {
        int ans = db.decrby(key, decrement);
        persistence.save(db);
        return to_string(ans);
    }
    catch(...){
        return "ERR value is not an integer";
    }
}

else if(command =="DBSIZE"){
   if(cmd.arguments.size() != 1)  return "ERR wrong number of arguments";
  return to_string(db.size());
}

else if(command == "TYPE"){
    if(cmd.arguments.size() != 2)  return "ERR wrong number of arguments";

    if(db.exists(cmd.arguments[1])) return "string";

    return "none";
}

else if(command == "INFO"){
    if(cmd.arguments.size() != 1)   return "ERR wrong number of arguments";

    return db.info();
}

else if(command == "RENAME"){
    if(cmd.arguments.size() != 3) return "ERR wrong number of arguments";

    if(db.rename(cmd.arguments[1], cmd.arguments[2])) {
        persistence.save(db);
        return "OK";
    }
    return "ERR no such key";
}

else if(command == "ECHO"){
    if(cmd.arguments.size() != 2) return "ERR wrong number of arguments";

    return cmd.arguments[1];
}




  return "UNKNOWN COMMMAND!";

  }
