#pragma once //Include this header file only one time.

#include <string>
#include "../database/Database.h"
#include "CommandParser.h"
#include "../persistence/PersistenceManager.h"


using namespace std;


class CommandExecutor{
   private:
     Database& db;
     PersistenceManager& persistence;
   public:
     CommandExecutor(Database& database , PersistenceManager& persistence);

     string execute(const ParsedCommand& cmd);

};
