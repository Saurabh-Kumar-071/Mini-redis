#pragma once

#include "AclUser.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <set>

class AclManager {
private:
    std::unordered_map<std::string, AclUser> users;

    // Simple wildcard match: supports only prefix* patterns (e.g. "cache:*")
    bool matchKeyPattern(const std::string& pattern, const std::string& key) const;

public:
    // Add or update a user
    void addUser(const std::string& username,
                 const std::string& password,
                 bool allCommands = true,
                 const std::set<std::string>& allowedCommands = {},
                 const std::string& keyPattern = "*");

    // Authenticate: returns pointer to user if credentials match and user is enabled, else nullptr
    AclUser* authenticate(const std::string& username, const std::string& password);

    // Check if user can run the given command (uppercase)
    bool canRunCommand(const std::string& username, const std::string& command) const;

    // Check if user can access the given key
    bool canAccessKey(const std::string& username, const std::string& key) const;

    // Delete a user (cannot delete "default")
    bool deleteUser(const std::string& username);

    // Get a mutable user by name (nullptr if not found)
    AclUser* getUser(const std::string& username);

    // Get an immutable user by name (nullptr if not found)
    const AclUser* getUser(const std::string& username) const;

    // List all usernames
    std::vector<std::string> listUsers() const;

    // Returns true if any users are registered (auth enforcement is ON)
    bool hasUsers() const;

    // Load default user from MINIREDIS_PASSWORD environment variable
    void loadDefaultFromEnv();
};
