#pragma once

#include "../database/Database.h"
#include "../command/CommandParser.h"
#include "../command/CommandExecutor.h"
#include "../client/ClientManager.h"
#include "../persistence/PersistenceManager.h"
#include "../exception/SocketException.h"
#include "../logger/ConsoleLogger.h"
#include "../logger/FileLogger.h"

class ILogger;

class Server
{
private:
    Database db;
    CommandParser parser;
    CommandExecutor executor;
    ConnectionManager connectionManager;
    PersistenceManager persistence;

    ILogger& logger;
public:
    Server(ILogger& logger);

    void start();
};
