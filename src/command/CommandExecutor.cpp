
#include "CommandExecutor.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <set>

using namespace std;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

CommandExecutor::CommandExecutor(Database& database, PersistenceManager& persistence)
    : db(database), persistence(persistence) {
}

// ---------------------------------------------------------------------------
// Load default user from environment (backward-compatible with old single password)
// ---------------------------------------------------------------------------

void CommandExecutor::loadPasswordFromEnv() {
    aclManager.loadDefaultFromEnv();
}

// ---------------------------------------------------------------------------
// Helper: extract the primary key from a key-based command
// ---------------------------------------------------------------------------

string CommandExecutor::getPrimaryKey(const string& command, const ParsedCommand& cmd) const {
    static const set<string> keyCommands = {
        "GET", "SET", "DEL", "EXPIRE", "TTL", "PERSIST", "APPEND",
        "STRLEN", "GETSET", "INCRBY", "DECRBY", "INCR", "DECR",
        "TYPE", "RENAME", "EXISTS", "MGET", "MSET", "MDEL"
    };
    if (keyCommands.count(command) == 0) return "";
    if (cmd.arguments.size() < 2)       return "";
    return cmd.arguments[1];
}

// ---------------------------------------------------------------------------
// ACL subcommand handler
// ---------------------------------------------------------------------------

CommandResponse CommandExecutor::handleAclCommand(const ParsedCommand& cmd, ClientConnection& client) {
    if (cmd.arguments.size() < 2) {
        return {ResponseType::Error, "ERR wrong number of arguments for 'acl' command"};
    }

    string subcmd = cmd.arguments[1];
    for (char& c : subcmd) c = toupper(static_cast<unsigned char>(c));

    // ── ACL WHOAMI ────────────────────────────────────────────────────────
    if (subcmd == "WHOAMI") {
        string username = client.getCurrentUsername();
        if (username.empty()) username = "default";
        return {ResponseType::BulkString, username};
    }

    // ── ACL LIST ──────────────────────────────────────────────────────────
    if (subcmd == "LIST") {
        vector<string> usernames = aclManager.listUsers();
        vector<string> result;
        for (const string& uname : usernames) {
            const AclUser* u = aclManager.getUser(uname);
            if (!u) continue;
            string line = "user " + u->username;
            line += u->enabled ? " on" : " off";
            line += " >";
            line += u->password.empty() ? "(nopass)" : "***";
            if (u->allCommands) {
                line += " +@all";
            } else {
                for (const string& c : u->allowedCommands) {
                    line += " +" + c;
                }
            }
            line += " ~" + u->keyPattern;
            result.push_back(line);
        }
        return {ResponseType::Array, "", result};
    }

    // ── ACL SETUSER <username> [flags...] ─────────────────────────────────
    if (subcmd == "SETUSER") {
        if (cmd.arguments.size() < 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'acl|setuser' command"};
        }

        // Only users with full access (allCommands) can manage other users
        const string& caller = client.getCurrentUsername();
        if (!caller.empty()) {
            const AclUser* callerUser = aclManager.getUser(caller);
            if (callerUser && !callerUser->allCommands) {
                return {ResponseType::Error,
                        "NOPERM this user has no permissions to run the 'acl|setuser' command"};
            }
        }

        const string& username = cmd.arguments[2];

        // Start from existing user values (or defaults for a new user)
        AclUser* existing  = aclManager.getUser(username);
        string password    = existing ? existing->password       : "";
        bool allCmds       = existing ? existing->allCommands    : false;
        set<string> allowed = existing ? existing->allowedCommands : set<string>{};
        string keyPattern  = existing ? existing->keyPattern     : "*";
        bool enabled       = existing ? existing->enabled        : true;

        // Parse rule tokens
        for (size_t i = 3; i < cmd.arguments.size(); i++) {
            const string& token = cmd.arguments[i];

            if (token[0] == '>') {
                // >password — set the password
                password = token.substr(1);

            } else if (token == "on") {
                enabled = true;

            } else if (token == "off") {
                enabled = false;

            } else if (token == "allcommands") {
                allCmds = true;

            } else if (token == "nocommands") {
                allCmds = false;
                allowed.clear();

            } else if (token == "allkeys" || token == "~*") {
                keyPattern = "*";

            } else if (token == "resetkeys") {
                keyPattern = "";

            } else if (token == "reset") {
                // Reset to safe defaults
                password   = "";
                allCmds    = false;
                allowed.clear();
                keyPattern = "*";
                enabled    = true;

            } else if (token[0] == '+') {
                // +COMMAND — whitelist a specific command
                string cmdName = token.substr(1);
                for (char& c : cmdName) c = toupper(static_cast<unsigned char>(c));
                allowed.insert(cmdName);

            } else if (token[0] == '-') {
                // -COMMAND — remove a command from whitelist
                string cmdName = token.substr(1);
                for (char& c : cmdName) c = toupper(static_cast<unsigned char>(c));
                allowed.erase(cmdName);

            } else if (token[0] == '~') {
                // ~pattern — set key pattern
                keyPattern = token.substr(1);
            }
        }

        aclManager.addUser(username, password, allCmds, allowed, keyPattern);
        AclUser* u = aclManager.getUser(username);
        if (u) u->enabled = enabled;

        return {ResponseType::SimpleString, "OK"};
    }

    // ── ACL DELUSER <username> ────────────────────────────────────────────
    if (subcmd == "DELUSER") {
        if (cmd.arguments.size() < 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'acl|deluser' command"};
        }

        const string& caller = client.getCurrentUsername();
        if (!caller.empty()) {
            const AclUser* callerUser = aclManager.getUser(caller);
            if (callerUser && !callerUser->allCommands) {
                return {ResponseType::Error,
                        "NOPERM this user has no permissions to run the 'acl|deluser' command"};
            }
        }

        const string& username = cmd.arguments[2];
        if (username == "default") {
            return {ResponseType::Error, "ERR The 'default' user cannot be removed"};
        }
        bool deleted = aclManager.deleteUser(username);
        return {ResponseType::Integer, deleted ? "1" : "0"};
    }

    // ── ACL GETUSER <username> ────────────────────────────────────────────
    if (subcmd == "GETUSER") {
        if (cmd.arguments.size() < 3) {
            return {ResponseType::Error, "ERR wrong number of arguments for 'acl|getuser' command"};
        }
        const string& username = cmd.arguments[2];
        const AclUser* u = aclManager.getUser(username);
        if (!u) {
            return {ResponseType::Null, ""};
        }
        string info = "username:" + u->username;
        info += " enabled:" + string(u->enabled ? "yes" : "no");
        info += " allcommands:" + string(u->allCommands ? "yes" : "no");
        if (!u->allCommands) {
            info += " commands:";
            for (const auto& c : u->allowedCommands) {
                info += "+" + c + " ";
            }
        }
        info += " keys:~" + u->keyPattern;
        return {ResponseType::BulkString, info};
    }

    return {ResponseType::Error,
            "ERR unknown subcommand '" + cmd.arguments[1] + "' for 'acl' command"};
}

// ---------------------------------------------------------------------------
// Main execute
// ---------------------------------------------------------------------------

CommandResponse CommandExecutor::execute(const ParsedCommand& cmd, ClientConnection& client){
    if(cmd.arguments.empty()) {
        return {ResponseType::Error, "ERR unknown command"};
    }

    if(cmd.arguments[0] == "__MALFORMED_PROTOCOL_ERROR__") {
        return {ResponseType::Error, "ERR Protocol error: unbalanced or corrupted format"};
    }

    string command = cmd.arguments[0];
    // Convert command name to uppercase for case insensitivity
    for (char& c : command) {
        c = toupper(static_cast<unsigned char>(c));
    }

    // ── AUTH — always allowed (client must authenticate before anything else) ──
    if (command == "AUTH") {
        if (!aclManager.hasUsers()) {
            return {ResponseType::Error,
                    "ERR Client sent AUTH, but no password is set. Did you mean ACL SETUSER with >password?"};
        }
        if (cmd.arguments.size() == 2) {
            // AUTH <password>  →  authenticate as "default" user (backward compat)
            AclUser* user = aclManager.authenticate("default", cmd.arguments[1]);
            if (!user) {
                return {ResponseType::Error,
                        "WRONGPASS invalid username-password pair or user is disabled."};
            }
            client.setAuthenticated(true);
            client.setCurrentUsername("default");
            return {ResponseType::SimpleString, "OK"};

        } else if (cmd.arguments.size() == 3) {
            // AUTH <username> <password>
            AclUser* user = aclManager.authenticate(cmd.arguments[1], cmd.arguments[2]);
            if (!user) {
                return {ResponseType::Error,
                        "WRONGPASS invalid username-password pair or user is disabled."};
            }
            client.setAuthenticated(true);
            client.setCurrentUsername(cmd.arguments[1]);
            return {ResponseType::SimpleString, "OK"};

        } else {
            return {ResponseType::Error, "ERR wrong number of arguments for 'auth' command"};
        }
    }

    // ── NOAUTH — block unauthenticated clients when users are registered ──
    if (aclManager.hasUsers() && !client.isAuthenticated()) {
        return {ResponseType::Error,
                "NOAUTH Authentication required. Please run AUTH <username> <password>"};
    }

    // ── ACL — handle ACL management commands ──────────────────────────────
    if (command == "ACL") {
        return handleAclCommand(cmd, client);
    }

    // ── Permission checks for authenticated named users ───────────────────
    const string& currentUser = client.getCurrentUsername();
    if (!currentUser.empty()) {
        // 1. Command permission
        if (!aclManager.canRunCommand(currentUser, command)) {
            return {ResponseType::Error,
                    "NOPERM this user has no permissions to run the '" +
                    cmd.arguments[0] + "' command"};
        }
        // 2. Key permission (only for commands that operate on keys)
        string primaryKey = getPrimaryKey(command, cmd);
        if (!primaryKey.empty() && !aclManager.canAccessKey(currentUser, primaryKey)) {
            return {ResponseType::Error,
                    "NOPERM No permissions to access key '" + primaryKey + "'"};
        }
    }

    // ── Command handlers ──────────────────────────────────────────────────

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
