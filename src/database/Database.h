#pragma once

#include <string>
#include <unordered_map>
#include<unordered_set>
#include<chrono> // for timestamps
#include "../exception/DatabaseException.h"
#include<vector>
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

    bool exists(const string& key);

    vector<string> keys() const; // const because you only read the key no CRUD operaton

    void clear();

    bool persist(const string& key);

    int append(const string& key, const string& value);

    int strlen(const string& key) const;

    string getset(const string& key,const string& value);

    int incrby(const string& key, int increment);

    int decrby(const string& key, int decrement);

    size_t size() const;

    string info() const;

    bool rename(const string& oldKey, const string& newKey);

    const unordered_map<string,string>& getAllData() const; //First const: Returned map cannot be modified.  Second const: Function cannot modify the Database object.

    const unordered_map<string,chrono::system_clock::time_point>& getAllExpiry() const;
};
