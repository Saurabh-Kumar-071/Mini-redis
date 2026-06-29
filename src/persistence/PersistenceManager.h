
#pragma once

#include <string>
#include "../exception/PersistenceException.h"
class Database; //Forward Declaration  Trust me.A class named Database exists. pointer has fixed size memory allocation

class PersistenceManager{
private:
     bool dirty = false;

public:
    void markDirty(); // mark dirty when data is change

    void saveIfDirty(const Database& db); // set a timer in bacground that is save after some time

    void save(const Database& db);

    void load(Database& db);
};
