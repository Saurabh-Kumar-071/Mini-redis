#pragma once //Include this header file only one time.

#include <string>
#include <set>
#include "../database/Database.h"
#include "CommandParser.h"
#include "../persistence/PersistenceManager.h"
#include "../protocol/CommandResponse.h"
#include "../client/ClientConnection.h"
#include "../acl/AclManager.h"

using namespace std;


class CommandExecutor{
   private:
     Database& db;
     PersistenceManager& persistence;
     AclManager aclManager;  // replaces single serverPassword

     // Returns the primary key from a key-based command (used for key permission checks)
     string getPrimaryKey(const string& command, const ParsedCommand& cmd) const;

     // Handles all ACL subcommands (SETUSER, DELUSER, LIST, WHOAMI, GETUSER)
     CommandResponse handleAclCommand(const ParsedCommand& cmd, ClientConnection& client);

   public:
     CommandExecutor(Database& database, PersistenceManager& persistence);

     // Load default user from MINIREDIS_PASSWORD env var
     void loadPasswordFromEnv();

     // Returns true if any users are registered (auth is required)
     bool hasPassword() const { return aclManager.hasUsers(); }

     // Full execute — used by the server (passes real client for auth state)
     CommandResponse execute(const ParsedCommand& cmd, ClientConnection& client);

     // Convenience overload for tests — creates a pre-authenticated dummy client
     CommandResponse execute(const ParsedCommand& cmd) {
         ClientConnection dummy(-1, true);  // fd=-1, authenticated=true
         return execute(cmd, dummy);
     }

};
