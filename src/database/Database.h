#pragma once

#include <string>
#include <unordered_map>
#include "../exception/DatabaseException.h"
using namespace std;

class Database
{
private:
    unordered_map<string,string> data;

public:

    void set(const string& key,const string& value);

    string get(const string& key);

    void del(const string& key);

const unordered_map<string,string>& getAllData() const;
};
