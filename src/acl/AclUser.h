#pragma once

#include <string>
#include <set>

struct AclUser {
    std::string username;
    std::string password;

    // If allCommands is true, all commands are allowed (ignore allowedCommands set)
    bool allCommands;

    // Specific commands this user is allowed to run (only used when allCommands=false)
    std::set<std::string> allowedCommands;

    // Key glob pattern — "*" means all keys, "cache:*" means keys starting with "cache:"
    std::string keyPattern;

    bool enabled;

    AclUser()
        : allCommands(true), keyPattern("*"), enabled(true) {}

    AclUser(const std::string& user, const std::string& pass,
            bool allCmds = true,
            const std::set<std::string>& cmds = {},
            const std::string& kp = "*")
        : username(user), password(pass), allCommands(allCmds),
          allowedCommands(cmds), keyPattern(kp), enabled(true) {}
};
