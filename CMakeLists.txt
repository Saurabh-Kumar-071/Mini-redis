cmake_minimum_required(VERSION 3.10)

project(MiniRedis)

set(CMAKE_CXX_STANDARD 17)

add_executable(
    MiniRedis
    src/main.cpp
    src/server/Server.cpp
    src/database/Database.cpp
    src/command/CommandParser.cpp
    src/command/CommandExecutor.cpp
    src/client/ClientConnection.cpp
    src/client/ClientManager.cpp
    src/persistence/PersistenceManager.cpp
    src/logger/ConsoleLogger.cpp
    src/logger/FileLogger.cpp
    src/logger/CompositeLogger.cpp
    src/utils/Split.cpp
)

target_link_libraries(
    MiniRedis
    pthread
)
