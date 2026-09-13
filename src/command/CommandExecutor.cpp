
#include "CommandExecutor.h"
#include <algorithm>
#include <cctype>

using namespace std;

CommandExecutor::CommandExecutor(Database& database, PersistenceManager& persistence)
    : db(database), persistence(persistence) {
}

CommandResponse CommandExecutor::execute(const ParsedCommand& cmd){
    if(cmd.arguments.empty()) {
        return {ResponseType::Error, "ERR unknown command"};
    }

    if(cmd.arguments[0] == "__MALFORMED_PROTOCOL_ERROR__") {
        return {ResponseType::Error, "ERR Protocol error: unbalanced or corrupted format"};
    }

    string command = cmd.arguments[0];
    // Convert command name to uppercase for case insensitivity
    for (char &c : command) {
        c = toupper(static_cast<unsigned char>(c));
    }

    // 1. PING
    if(command == "PING"){
        if (cmd.arguments.size() == 1) {
            return {ResponseType::SimpleString, "PONG"};
        } else if (cmd.arguments.size() == 2) {
            return {ResponseType::BulkString, cmd.arguments[1]};
        } else {
            return {ResponseType::Error, "ERR wrong number of arguments for 'ping' command"};
        }
    }

    // 2. SET
    else if(command == "SET"){
        if (cmd.arguments.size() != 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'set' command"};
        }
        const string& key = cmd.arguments[1];
        const string& value = cmd.arguments[2];
        db.set(key, value);
        persistence.markDirty();
        return {ResponseType::SimpleString, "OK"};
    }

    // 3. GET
    else if(command == "GET"){
        if (cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'get' command"};
        }
        const string& key = cmd.arguments[1];
        if (!db.exists(key)) {
            return {ResponseType::Null, ""};
        }
        return {ResponseType::BulkString, db.get(key)};
    }

    // 4. DEL
    else if(command == "DEL"){
        if (cmd.arguments.size() < 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'del' command"};
        }
        int count = 0;
        for (size_t i = 1; i < cmd.arguments.size(); ++i) {
            if (db.del(cmd.arguments[i])) {
                count++;
            }
        }
        if (count > 0) {
            persistence.markDirty();
        }
        return {ResponseType::Integer, to_string(count)};
    }

    // 5. EXPIRE
    else if(command == "EXPIRE"){
        if (cmd.arguments.size() != 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'expire' command"};
        }
        const string& key = cmd.arguments[1];
        int seconds = 0;
        try {
            seconds = stoi(cmd.arguments[2]);
        } catch (...) {
            return {ResponseType::Error, "ERR value is not an integer or out of range"};
        }

        if (db.expire(key, seconds)) {
            persistence.markDirty();
            return {ResponseType::Integer, "1"};
        }
        return {ResponseType::Integer, "0"};
    }

    // 6. TTL
    else if(command == "TTL"){
        if (cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'ttl' command"};
        }
        const string& key = cmd.arguments[1];
        return {ResponseType::Integer, to_string(db.ttl(key))};
    }

    // 7. BIG (test command)
    else if(command == "BIG"){
        return {ResponseType::BulkString, string(10000000, 'A')};
    }

    // 8. MSET
    else if(command == "MSET"){
        if(cmd.arguments.size() < 3 || cmd.arguments.size() % 2 == 0) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'mset' command"};
        }
        for(size_t i = 1; i < cmd.arguments.size(); i += 2){
            db.set(cmd.arguments[i], cmd.arguments[i + 1]);
        }
        persistence.markDirty();
        return {ResponseType::SimpleString, "OK"};
    }

    // 9. EXISTS
    else if(command == "EXISTS"){
        if(cmd.arguments.size() < 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'exists' command"};
        }
        int count = 0;
        for (size_t i = 1; i < cmd.arguments.size(); ++i) {
            if (db.exists(cmd.arguments[i])) {
                count++;
            }
        }
        return {ResponseType::Integer, to_string(count)};
    }

    // 10. MGET
    else if(command == "MGET"){
        if(cmd.arguments.size() < 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'mget' command"};
        }
        vector<string> result;
        for(size_t i = 1; i < cmd.arguments.size(); i++){
            if (db.exists(cmd.arguments[i])) {
                result.push_back(db.get(cmd.arguments[i]));
            } else {
                result.push_back("$-1"); // RESP null bulk string indicator in array
            }
        }
        return {ResponseType::Array, "", result};
    }

    // 11. MDEL
    else if(command == "MDEL"){
        if(cmd.arguments.size() < 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'mdel' command"};
        }
        int deleted = 0;
        for(size_t i = 1; i < cmd.arguments.size(); i++){
            if(db.del(cmd.arguments[i])){
                deleted++;
            }
        }
        if (deleted > 0) {
            persistence.markDirty();
        }
        return {ResponseType::Integer, to_string(deleted)};
    }

    // 12. INCR
    else if(command == "INCR"){
        if (cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'incr' command"};
        }
        const string& key = cmd.arguments[1];
        try {
            if (!db.exists(key)){
                db.set(key, "1");
                persistence.markDirty();
                return {ResponseType::Integer, "1"};
            }

            long long value = stoll(db.get(key));
            value++;
            db.set(key, to_string(value));
            persistence.markDirty();
            return {ResponseType::Integer, to_string(value)};
        } catch (...) {
            return {ResponseType::Error, "ERR value is not an integer or out of range"};
        }
    }

    // 13. DECR
    else if(command == "DECR"){
        if (cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'decr' command"};
        }
        const string& key = cmd.arguments[1];
        try {
            if (!db.exists(key)){
                db.set(key, "-1");
                persistence.markDirty();
                return {ResponseType::Integer, "-1"};
            }

            long long value = stoll(db.get(key));
            value--;
            db.set(key, to_string(value));
            persistence.markDirty();
            return {ResponseType::Integer, to_string(value)};
        } catch (...) {
            return {ResponseType::Error, "ERR value is not an integer or out of range"};
        }
    }

    // 14. KEYS
    else if(command == "KEYS"){
        if (cmd.arguments.size() != 1 && cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'keys' command"};
        }
        vector<string> keys = db.keys();
        return {ResponseType::Array, "", keys};
    }

    // 15. FLUSHDB
    else if(command == "FLUSHDB"){
        if(cmd.arguments.size() != 1) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'flushdb' command"};
        }
        db.clear();
        persistence.markDirty();
        return {ResponseType::SimpleString, "OK"};
    }

    // 16. PERSIST
    else if(command == "PERSIST"){
        if(cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'persist' command"};
        }
        const string& key = cmd.arguments[1];
        if(db.persist(key)){
            persistence.markDirty();
            return {ResponseType::Integer, "1"};
        }
        return {ResponseType::Integer, "0"};
    }

    // 17. APPEND
    else if(command == "APPEND"){
        if(cmd.arguments.size() != 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'append' command"};
        }
        int len = db.append(cmd.arguments[1], cmd.arguments[2]);
        persistence.markDirty();
        return {ResponseType::Integer, to_string(len)};
    }

    // 18. STRLEN
    else if(command == "STRLEN"){
        if(cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'strlen' command"};
        }
        const string& key = cmd.arguments[1];
        return {ResponseType::Integer, to_string(db.strlen(key))};
    }

    // 19. GETSET
    else if(command == "GETSET"){
        if(cmd.arguments.size() != 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'getset' command"};
        }
        const string& key = cmd.arguments[1];
        const string& value = cmd.arguments[2];
        if (!db.exists(key)) {
            db.set(key, value);
            persistence.markDirty();
            return {ResponseType::Null, ""};
        }
        string old_val = db.getset(key, value);
        persistence.markDirty();
        return {ResponseType::BulkString, old_val};
    }

    // 20. INCRBY
    else if(command == "INCRBY"){
        if(cmd.arguments.size() != 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'incrby' command"};
        }
        const string& key = cmd.arguments[1];
        int increment = 0;
        try {
            increment = stoi(cmd.arguments[2]);
        } catch (...) {
            return {ResponseType::Error, "ERR value is not an integer or out of range"};
        }

        try {
            int ans = db.incrby(key, increment);
            persistence.markDirty();
            return {ResponseType::Integer, to_string(ans)};
        } catch (...) {
            return {ResponseType::Error, "ERR value is not an integer or out of range"};
        }
    }

    // 21. DECRBY
    else if(command == "DECRBY"){
        if(cmd.arguments.size() != 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'decrby' command"};
        }
        const string& key = cmd.arguments[1];
        int decrement = 0;
        try {
            decrement = stoi(cmd.arguments[2]);
        } catch (...) {
            return {ResponseType::Error, "ERR value is not an integer or out of range"};
        }

        try {
            int ans = db.decrby(key, decrement);
            persistence.markDirty();
            return {ResponseType::Integer, to_string(ans)};
        } catch (...) {
            return {ResponseType::Error, "ERR value is not an integer or out of range"};
        }
    }

    // 22. DBSIZE
    else if(command == "DBSIZE"){
        if(cmd.arguments.size() != 1) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'dbsize' command"};
        }
        return {ResponseType::Integer, to_string(db.size())};
    }

    // 23. TYPE
    else if(command == "TYPE"){
        if(cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'type' command"};
        }
        if(db.exists(cmd.arguments[1])) {
            return {ResponseType::SimpleString, "string"};
        }
        return {ResponseType::SimpleString, "none"};
    }

    // 24. INFO
    else if(command == "INFO"){
        return {ResponseType::BulkString, db.info()};
    }

    // 25. RENAME
    else if(command == "RENAME"){
        if(cmd.arguments.size() != 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'rename' command"};
        }
        if(db.rename(cmd.arguments[1], cmd.arguments[2])) {
            persistence.markDirty();
            return {ResponseType::SimpleString, "OK"};
        }
        return {ResponseType::Error, "ERR no such key"};
    }

    // 26. ECHO
    else if(command == "ECHO"){
        if(cmd.arguments.size() != 2) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'echo' command"};
        }
        return {ResponseType::BulkString, cmd.arguments[1]};
    }

    // 27. CONFIG
    else if(command == "CONFIG"){
        return {ResponseType::Array, "", {}};
    }

    return {ResponseType::Error, "ERR unknown command '" + cmd.arguments[0] + "'"};
}
