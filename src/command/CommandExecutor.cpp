
#include "CommandExecutor.h"
#include<stdbool.h>
using namespace std;

CommandExecutor::CommandExecutor( Database& database , PersistenceManager& persistence): db(database) ,persistence(persistence){ // constructor initializer list.

}

CommandResponse CommandExecutor::execute(const ParsedCommand& cmd){

  if(cmd.arguments.empty()) return {ResponseType::SimpleString, "unknown Command"};;

  const string& command = cmd.arguments[0];

  if(command =="PING"){
   return {ResponseType::SimpleString, "PONG"};
  }

  else if(command=="SET"){
       if (cmd.arguments.size() != 3) return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string& key = cmd.arguments[1];
    const string& value = cmd.arguments[2];
     db.set(key , value);
     persistence.save(db);
     return {ResponseType::SimpleString, "OK"};
  }

  else if(command=="GET"){
    if (cmd.arguments.size() != 2) return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string& key = cmd.arguments[1];
    return {ResponseType::BulkString, db.get(key)};
  }

  else if(command=="DEL"){
    if (cmd.arguments.size() != 2)  return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string& key = cmd.arguments[1];
    db.del(key);
    persistence.save(db);
   return {ResponseType::SimpleString, "delete from database!"};
  }

  else if(command=="EXPIRE"){
       if (cmd.arguments.size() != 3)  return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string& key = cmd.arguments[1];
    const string& value = cmd.arguments[2];
    int val = stoi(value);
    db.expire(key ,val);
    persistence.save(db);
    return {ResponseType::SimpleString, "ok"};
  }

  else if(command=="TTL"){
     if (cmd.arguments.size() != 2) return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string& key = cmd.arguments[1];
   return {ResponseType::Integer, to_string(db.ttl(key))};
  }

  else if(command == "BIG"){
    return {ResponseType::BulkString, string(10000000, 'A')}; // only for testing why EPOLLOUT needed
}

  else if(command == "MSET"){

    if(cmd.arguments.size() < 3  || cmd.arguments.size() %2 ==0) return {ResponseType::Error, "ERR wrong number of arguments"};;

    for(size_t i = 1 ; i < cmd.arguments.size() ;i+=2){
      const string& key = cmd.arguments[i];
      const string& val = cmd.arguments[i+1];

      db.set(key ,val);
    }
     persistence.save(db);
     return {ResponseType::SimpleString, "ok"};
  }

  else if(command == "EXISTS"){
    if(cmd.arguments.size() !=2) return {ResponseType::Error, "ERR wrong number of arguments"};
     const string& key = cmd.arguments[1];
      return {ResponseType::Integer, to_string(db.exists(key) ? 1 : 0)};
  }

  else if(command == "MGET"){
    if(cmd.arguments.size() < 2) return {ResponseType::Error, "ERR wrong number of arguments"};;

    vector<string> result;
    for(size_t i = 1; i < cmd.arguments.size(); i++){
        result.push_back(db.get(cmd.arguments[i]));
    }
   return {ResponseType::Array, "", result};
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
   return {ResponseType::Integer, to_string(deleted)};
 }

 else if(command=="INCR"){
    const string& key = cmd.arguments[1];

    try{
      if (!db.exists(key)){
       db.set(key, "1");
       persistence.save(db);
      return {ResponseType::Integer, to_string(1)};
      }

      int value = stoi(db.get(key));
      value++;

      db.set(key , to_string(value));
      persistence.save(db);

    return {ResponseType::Integer, to_string(value)};
    }
    catch(const invalid_argument&){
       return {ResponseType::Error, "ERR val is not an integer"};
    }
 }

 else if(command == "DECR"){
   const string& key = cmd.arguments[1];

   try{
      if (!db.exists(key)){
      db.set(key, "1");
      persistence.save(db);
     return {ResponseType::Integer, to_string(1)};
     }

      int value = stoi(db.get(key));
      value--;

      db.set(key ,to_string(value));
      persistence.save(db);
   return {ResponseType::Integer, to_string(value)};
   }
   catch(const invalid_argument&){
    return {ResponseType::Error, "ERR val is not an integer"};
   }
 }

 else if(command == "KEYS"){
   if (cmd.arguments.size() != 1) return {ResponseType::Error, "ERR wrong number of arguments"};;

   vector<string>keys  = db.keys();
   return {ResponseType::Array, "", keys};
 }

 else if(command == "FLUSHDB"){
  if(cmd.arguments.size()!=1) return {ResponseType::Error, "ERR wrong number of arguments"};;

    db.clear();
    persistence.save(db);
  return {ResponseType::SimpleString, "ok"};
 }

 else if(command == "PERSIST"){
     if(cmd.arguments.size()!=2) return {ResponseType::Error, "ERR wrong number of arguments"};;

      const string& key = cmd.arguments[1];
     if(db.persist(key)){
      persistence.save(db);
      return {ResponseType::Integer, to_string(1)};
     }
    return {ResponseType::Integer, to_string(0)};
 }

 else if(command == "APPEND"){
    if(cmd.arguments.size() != 3) return {ResponseType::Error, "ERR wrong number of arguments"};;

    int len = db.append(cmd.arguments[1], cmd.arguments[2]);
    persistence.save(db);
   return {ResponseType::Integer, to_string(len)};
}

else if(command == "STRLEN"){
    if(cmd.arguments.size() != 2) return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string&key = cmd.arguments[1];
    return {ResponseType::Integer, to_string(db.strlen(key))};
}

else if(command =="GETSET"){
   if(cmd.arguments.size() != 3) return {ResponseType::Error, "ERR wrong number of arguments"};;
   const string& key = cmd.arguments[1];
   const string& value = cmd.arguments[2];
    string old_val = db.getset(key,value);
   persistence.save(db);
  return {ResponseType::BulkString, old_val};
}

else if(command == "INCRBY"){
    if(cmd.arguments.size() != 3) return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string& key = cmd.arguments[1];

    int increment;

    try{
        increment = stoi(cmd.arguments[2]);
    }
    catch(...){
       return {ResponseType::Error, "ERR Increment is not an integer"};
    }

    try{
        int ans = db.incrby(key, increment);

        persistence.save(db);

        return {ResponseType::Integer, to_string(ans)};
    }
    catch(...){
       return {ResponseType::Error, "ERR value is not an integer"};
    }
}

else if(command == "DECRBY"){
    if(cmd.arguments.size() != 3)   return {ResponseType::Error, "ERR wrong number of arguments"};;

    const string& key = cmd.arguments[1];
    int decrement;

    try {
        decrement = stoi(cmd.arguments[2]);
    }
    catch(...){
        return {ResponseType::Error, "ERR Decrement is not an integer"};
    }

    try {
        int ans = db.decrby(key, decrement);
        persistence.save(db);
       return {ResponseType::Integer, to_string(ans)};
    }
    catch(...){
        return {ResponseType::Error, "ERR value is not an integer"};
    }
}

else if(command =="DBSIZE"){
   if(cmd.arguments.size() != 1)  return {ResponseType::Error, "ERR wrong number of arguments"};
   return {ResponseType::Integer, to_string(db.size())};
}

else if(command == "TYPE"){
    if(cmd.arguments.size() != 2)  return {ResponseType::Error, "ERR wrong number of arguments"};

    if(db.exists(cmd.arguments[1]))  return {ResponseType::BulkString, "string"};

    return {ResponseType::BulkString, "none"};
}

else if(command == "INFO"){
    if(cmd.arguments.size() != 1)   return {ResponseType::Error, "ERR wrong number of arguments"};;

    return {ResponseType::BulkString, db.info()};
}

else if(command == "RENAME"){
    if(cmd.arguments.size() != 3) return {ResponseType::Error, "ERR wrong number of arguments"};;

    if(db.rename(cmd.arguments[1], cmd.arguments[2])) {
        persistence.save(db);
        return {ResponseType::SimpleString, "ok"};
    }
    return {ResponseType::Error, "ERR no such key"};
}

else if(command == "ECHO"){
    if(cmd.arguments.size() != 2) return {ResponseType::Error, "ERR wrong number of arguments"};;

    return {ResponseType::SimpleString, cmd.arguments[1]};
}


  return  {ResponseType::SimpleString, "unknown command!"};

  }
