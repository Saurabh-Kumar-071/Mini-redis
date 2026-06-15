
#pragma once

#include <string>

class Database; //Forward Declaration  Trust me.A class named Database exists. pointer has fixed size memory allocation

class PersistenceManager{
public:
    void save(const Database& db);

    void load(Database& db);
};
