#pragma once

#include "../database/Database.h"
#include "../command/CommandParser.h"
#include "../command/CommandExecutor.h"
#include "../client/ClientManager.h"
#include "../persistence/PersistenceManager.h"

class Server
{
private:
    Database db;
    CommandParser parser;
    CommandExecutor executor;
    ConnectionManager connectionManager;
    PersistenceManager persistence;

public:
    Server();

    void start();
};
