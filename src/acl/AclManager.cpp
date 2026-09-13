
#include "AclManager.h"
#include <cstdlib>
#include <algorithm>

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool AclManager::matchKeyPattern(const std::string& pattern, const std::string& key) const {
    // "*" matches everything
    if (pattern == "*") return true;

    size_t wildcard = pattern.find('*');
    if (wildcard == std::string::npos) {
        // No wildcard — exact match required
        return pattern == key;
    }

    // Prefix wildcard: "cache:*" → key must start with "cache:"
    std::string prefix = pattern.substr(0, wildcard);
    return key.size() >= prefix.size() &&
           key.compare(0, prefix.size(), prefix) == 0;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AclManager::addUser(const std::string& username,
                         const std::string& password,
                         bool allCommands,
                         const std::set<std::string>& allowedCommands,
                         const std::string& keyPattern) {
    AclUser user;
    user.username       = username;
    user.password       = password;
    user.allCommands    = allCommands;
    user.allowedCommands = allowedCommands;
    user.keyPattern     = keyPattern;
    user.enabled        = true;
    users[username]     = user;
}

AclUser* AclManager::authenticate(const std::string& username, const std::string& password) {
    auto it = users.find(username);
    if (it == users.end())    return nullptr;
    AclUser& user = it->second;
    if (!user.enabled)         return nullptr;
    if (user.password != password) return nullptr;
    return &user;
}

bool AclManager::canRunCommand(const std::string& username, const std::string& command) const {
    auto it = users.find(username);
    if (it == users.end()) return false;
    const AclUser& user = it->second;
    if (!user.enabled) return false;
    if (user.allCommands) return true;
    return user.allowedCommands.count(command) > 0;
}

bool AclManager::canAccessKey(const std::string& username, const std::string& key) const {
    auto it = users.find(username);
    if (it == users.end()) return false;
    const AclUser& user = it->second;
    if (!user.enabled) return false;
    return matchKeyPattern(user.keyPattern, key);
}

bool AclManager::deleteUser(const std::string& username) {
    if (username == "default") return false; // default user is protected
    return users.erase(username) > 0;
}

AclUser* AclManager::getUser(const std::string& username) {
    auto it = users.find(username);
    return (it != users.end()) ? &it->second : nullptr;
}

const AclUser* AclManager::getUser(const std::string& username) const {
    auto it = users.find(username);
    return (it != users.end()) ? &it->second : nullptr;
}

std::vector<std::string> AclManager::listUsers() const {
    std::vector<std::string> result;
    result.reserve(users.size());
    for (const auto& pair : users) {
        result.push_back(pair.first);
    }
    return result;
}

bool AclManager::hasUsers() const {
    return !users.empty();
}

void AclManager::loadDefaultFromEnv() {
    const char* pwd = getenv("MINIREDIS_PASSWORD");
    if (pwd && pwd[0] != '\0') {
        // Default user gets full access — mirrors the old single-password behaviour
        addUser("default", std::string(pwd), /*allCommands=*/true, {}, "*");
    }
}
