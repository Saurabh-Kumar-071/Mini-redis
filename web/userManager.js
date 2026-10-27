const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

class UserManager {
    constructor() {
        // Prefer persistent data directory, fallback to local directory
        const primaryDir = path.join(__dirname, '../data');
        const fallbackDir = path.join(__dirname, 'data');
        
        try {
            if (!fs.existsSync(primaryDir)) {
                fs.mkdirSync(primaryDir, { recursive: true });
            }
            this.filePath = path.join(primaryDir, 'users.json');
        } catch (e) {
            if (!fs.existsSync(fallbackDir)) {
                fs.mkdirSync(fallbackDir, { recursive: true });
            }
            this.filePath = path.join(fallbackDir, 'users.json');
        }

        this.users = {};
        this.loadUsers();
    }

    loadUsers() {
        try {
            if (fs.existsSync(this.filePath)) {
                const data = fs.readFileSync(this.filePath, 'utf-8');
                this.users = JSON.parse(data);
            } else {
                this.users = {};
                this.saveUsers();
            }
        } catch (err) {
            console.error('[UserManager] Failed to load users, starting with empty store:', err.message);
            this.users = {};
        }
    }

    saveUsers() {
        try {
            fs.writeFileSync(this.filePath, JSON.stringify(this.users, null, 2), 'utf-8');
        } catch (err) {
            console.error('[UserManager] Failed to save users:', err.message);
        }
    }

    hashPassword(password, salt) {
        return crypto.pbkdf2Sync(password, salt, 1000, 32, 'sha256').toString('hex');
    }

    userExists(username) {
        if (!username) return false;
        const normalized = username.trim().toLowerCase();
        return Object.prototype.hasOwnProperty.call(this.users, normalized);
    }

    createUser(username, password) {
        const normalized = username.trim().toLowerCase();
        if (this.userExists(normalized)) {
            return { success: false, error: 'Username already exists' };
        }

        const salt = crypto.randomBytes(16).toString('hex');
        const hash = this.hashPassword(password, salt);

        this.users[normalized] = {
            username: normalized,
            salt,
            hash,
            createdAt: new Date().toISOString()
        };

        this.saveUsers();
        return { success: true, username: normalized };
    }

    verifyUser(username, password) {
        const normalized = username.trim().toLowerCase();
        if (!this.userExists(normalized)) {
            return false;
        }

        const user = this.users[normalized];
        const computedHash = this.hashPassword(password, user.salt);
        return computedHash === user.hash;
    }

    getUserCount() {
        return Object.keys(this.users).length;
    }
}

module.exports = new UserManager();
