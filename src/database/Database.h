#pragma once

#include <string>
#include <unordered_map>
#include<chrono> // for timestamps
#include "../exception/DatabaseException.h"
using namespace std;

class Database
{
private:
    unordered_map<string,string> data;
    unordered_map<string,chrono::system_clock::time_point> expiry;

public:

    void set(const string& key,const string& value);

    string get(const string& key);

    void del(const string& key);

    void expire(const string& key,int seconds);

    bool isExpired(const string& key);

    int ttl(const string& key);

    void setExpiryTime(const string& key,const chrono::system_clock::time_point& tp);

    const unordered_map<string,string>& getAllData() const; //First const: Returned map cannot be modified.  Second const: Function cannot modify the Database object.

    const unordered_map<string,chrono::system_clock::time_point>& getAllExpiry() const;
};
