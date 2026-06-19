#pragma once

#include "../database/Database.h"
#include "../command/CommandParser.h"
#include "../command/CommandExecutor.h"
#include "../client/ClientManager.h"
#include "../persistence/PersistenceManager.h"
#include "../exception/SocketException.h"

class ILogger;

class Server
{
private:
    int server_fd;

    Database db;
    CommandParser parser;
    CommandExecutor executor;
    ConnectionManager connectionManager;
    PersistenceManager persistence;

    ILogger& logger;
public:
    Server(ILogger& logger);

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    ~Server();

    void start();
};
