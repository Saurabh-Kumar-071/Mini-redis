#include "Database.h"
#include <iostream>
#include<vector>
using namespace std;

void Database::set(const string &key, const string& value){
    data[key] = value;
    expiry.erase(key); // Setting a key removes any previous TTL (Redis SET behavior)
}

string Database::get(const string& key) {
    if(isExpired(key)){
        data.erase(key);
        expiry.erase(key);
        return "";
    }

    auto it = data.find(key);
    if(it == data.end()){
        return "";
    }

    return it->second;
}

bool Database::del(const string &key){
    auto it = data.find(key);
    if(it != data.end()){
        data.erase(it);
        expiry.erase(key);
        return true;
    }
    expiry.erase(key);
    return false;
}

const unordered_map<string,string>& Database::getAllData() const{
    return data;
}

bool Database::expire(const string& key, int seconds){
    if (!exists(key)) return false;

    expiry[key] = chrono::system_clock::now() + chrono::seconds(seconds);
    return true;
}

bool Database::isExpired(const string& key){
    auto it = expiry.find(key);
    if(it == expiry.end()) return false;

    auto now = chrono::system_clock::now();
    if(now >= it->second) return true;
    return false;
}

int Database::ttl(const string& key){
    auto it = data.find(key);
    if(it == data.end()){
        return -2; // key doesn't exist
    }

    if(isExpired(key)){
        data.erase(key);
        expiry.erase(key);
        return -2;
    }

    auto expIt = expiry.find(key);
    if(expIt == expiry.end()){
        return -1; // key exists but no TTL
    }

    auto remaining_time = expIt->second - chrono::system_clock::now();
    auto remaining_ms = chrono::duration_cast<chrono::milliseconds>(remaining_time).count();
    if (remaining_ms <= 0) {
        data.erase(key);
        expiry.erase(key);
        return -2;
    }

    return static_cast<int>((remaining_ms + 999) / 1000);
}

bool Database::exists(const string& key){
    auto it = data.find(key);
    if(it == data.end()) return false;

    if(isExpired(key)){
        data.erase(key);
        expiry.erase(key);
        return false;
    }
    return true;
}

const unordered_map<string,chrono::system_clock::time_point>& Database::getAllExpiry() const{
    return expiry;
}

void Database::setExpiryTime(const string& key, const chrono::system_clock::time_point& tp){
    expiry[key] = tp;
}

vector<string> Database::keys(){
    cleanupExpiredKeys();
    vector<string> result;
    result.reserve(data.size());

    for (const auto& pair : data){
        result.push_back(pair.first);
    }
    return result;
}

void Database::clear(){
    data.clear();
    expiry.clear();
}

bool Database::persist(const string& key) {
    if (!exists(key)) return false;
    auto it = expiry.find(key);
    if(it != expiry.end()){
        expiry.erase(it);
        return true;
    }
    return false;
}

int Database::append(const string& key, const string& value){
    if (isExpired(key)) {
        data.erase(key);
        expiry.erase(key);
    }
    auto& str = data[key];
    str += value;
    return str.size();
}

int Database::strlen(const string& key){
    if (!exists(key)) return 0;
    return data[key].size();
}

string Database::getset(const string& key, const string& value){
    string old_val = "";
    if (exists(key)) {
        old_val = data[key];
    }
    set(key, value);
    return old_val;
}

int Database::incrby(const string& key, int increment){
    if (!exists(key)) {
        set(key, to_string(increment));
        return increment;
    }

    int value = stoi(get(key));
    value += increment;
    data[key] = to_string(value);
    return value;
}

int Database::decrby(const string& key, int decrement){
    return incrby(key, -decrement);
}

size_t Database::size(){
    cleanupExpiredKeys();
    return data.size();
}

string Database::info(){
    cleanupExpiredKeys();
    return "MiniRedis\nKeys: " + to_string(data.size()) + "\nTTL Keys: " + to_string(expiry.size());
}

bool Database::rename(const string& oldKey, const string& newKey){
    if (!exists(oldKey))
        return false;

    data[newKey] = std::move(data[oldKey]);
    data.erase(oldKey);

    auto exp = expiry.find(oldKey);
    if(exp != expiry.end())
    {
        expiry[newKey] = exp->second;
        expiry.erase(exp);
    }
    else
    {
        expiry.erase(newKey);
    }

    return true;
}

bool Database::cleanupExpiredKeys(){
    bool changed = false;
    auto it = expiry.begin();
    while (it != expiry.end()){
        if (chrono::system_clock::now() >= it->second) {
            data.erase(it->first);
            it = expiry.erase(it);
            changed = true;
        }
        else{
            ++it;
        }
    }
    return changed;
}


