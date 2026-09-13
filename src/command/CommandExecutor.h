#pragma once //Include this header file only one time.

#include <string>
#include "../database/Database.h"
#include "CommandParser.h"
#include "../persistence/PersistenceManager.h"
#include "../protocol/CommandResponse.h"
#include "../client/ClientConnection.h"

using namespace std;


class CommandExecutor{
   private:
     Database& db;
     PersistenceManager& persistence;
     string serverPassword;   // empty = no auth required
   public:
     CommandExecutor(Database& database, PersistenceManager& persistence);

     // Load password from env var MINIREDIS_PASSWORD
     void loadPasswordFromEnv();

     // Returns true if a password is configured
     bool hasPassword() const { return !serverPassword.empty(); }

     // Full execute — used by the server (passes real client for auth state)
     CommandResponse execute(const ParsedCommand& cmd, ClientConnection& client);

     // Convenience overload for tests — creates a pre-authenticated dummy client
     CommandResponse execute(const ParsedCommand& cmd) {
         ClientConnection dummy(-1, true);  // fd=-1, authenticated=true
         return execute(cmd, dummy);
     }

};
