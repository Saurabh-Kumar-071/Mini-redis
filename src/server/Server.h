#pragma once
#include "../scheduler/Scheduler.h"
#include "../database/Database.h"
#include "../command/CommandParser.h"
#include "../command/CommandExecutor.h"
#include "../client/ClientManager.h"
#include "../persistence/PersistenceManager.h"
#include "../exception/SocketException.h"
#include "../network/FileDescriptor.h"

class ILogger;

class Server
{
private:

    void handleClientEvent(epoll_event& event);

    EpollManager epollManager;        //Construct epollManager
    Database db;                      //Construct db
    CommandParser parser;             //Construct parser
    PersistenceManager persistence;   //construct peristance
    CommandExecutor executor;         // construct executor(db , persistance)
    ConnectionManager connectionManager;; // construct connectionmanager
    FileDescriptor server_fd;            // construct server_fd(-1)
    Scheduler scheduler;                //construct schedular(epollManager)
    ILogger& logger;                    //Store logger reference
                                             // after above doing then Run persistence.load(db) then start server ready
public:
    Server(ILogger& logger); //A constructor exists. It takes ILogger& as parameter.But it does not tell how it works.

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();
};
