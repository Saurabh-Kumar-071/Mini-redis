#pragma once //Include this header file only one time.

#include <string>
#include "../database/Database.h"
#include "CommandParser.h"
using namespace std;


class CommandExecutor{
   private:
     Database& db;

   public:
     CommandExecutor(Database& database);

     string execute(const ParsedCommand& cmd);
};
