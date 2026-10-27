const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const net = require('net');
const path = require('path');
const userManager = require('./userManager');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const fs = require('fs');

// Automatically load .env if present
function loadEnv() {
    const envPaths = [
        path.join(__dirname, '../.env'),
        path.join(__dirname, '.env')
    ];
    for (const p of envPaths) {
        if (fs.existsSync(p)) {
            try {
                const content = fs.readFileSync(p, 'utf-8');
                for (const line of content.split('\n')) {
                    const trimmed = line.trim();
                    if (!trimmed || trimmed.startsWith('#')) continue;
                    const eqIdx = trimmed.indexOf('=');
                    if (eqIdx !== -1) {
                        const key = trimmed.slice(0, eqIdx).trim();
                        const val = trimmed.slice(eqIdx + 1).trim();
                        if (!process.env[key]) {
                            process.env[key] = val;
                        }
                    }
                }
            } catch (e) {}
            break;
        }
    }
}
loadEnv();

const REDIS_HOST = process.env.REDIS_HOST || '127.0.0.1';
const REDIS_PORT = parseInt(process.env.REDIS_PORT || '8080', 10);
const PORT = parseInt(process.argv[2] || process.env.PORT || '3000', 10);
const DEFAULT_PASSWORD = process.env.MINIREDIS_PASSWORD || '';

app.use(express.static(path.join(__dirname, 'public')));

// Health check endpoint for cloud platforms (Render, Railway, etc.)
app.get('/healthz', (req, res) => {
    res.json({
        status: 'healthy',
        service: 'MiniRedis Cloud Gateway',
        adminConfigured: !!DEFAULT_PASSWORD,
        registeredUsers: userManager.getUserCount()
    });
});

// Single-key commands where args[1] is the key
const SINGLE_KEY_COMMANDS = new Set([
    'GET', 'SET', 'EXPIRE', 'TTL', 'PERSIST', 'APPEND',
    'STRLEN', 'GETSET', 'INCRBY', 'DECRBY', 'INCR', 'DECR',
    'TYPE'
]);

// Multi-key commands where all arguments starting at index 1 are keys
const MULTI_KEY_COMMANDS = new Set([
    'DEL', 'MDEL', 'MGET', 'EXISTS'
]);

// Restricted commands for regular users
const RESTRICTED_FOR_USERS = new Set([
    'FLUSHALL', 'CONFIG', 'SHUTDOWN', 'ACL', 'SAVE', 'BGSAVE'
]);

function parseCommandArgs(input) {
    const args = [];
    const regex = /[^\s"']+|"([^"]*)"|'([^']*)'/g;
    let match;
    while ((match = regex.exec(input)) !== null) {
        if (match[1] !== undefined) {
            args.push(match[1]);
        } else if (match[2] !== undefined) {
            args.push(match[2]);
        } else {
            args.push(match[0]);
        }
    }
    return args;
}

function encodeRESP(args) {
    let out = `*${args.length}\r\n`;
    for (const arg of args) {
        const str = String(arg);
        out += `$${Buffer.byteLength(str, 'utf-8')}\r\n${str}\r\n`;
    }
    return out;
}

function tryParseOneResp(buf) {
    if (!buf || buf.length === 0) return null;
    const type = buf[0];
    const crlf = buf.indexOf('\r\n');
    if (crlf === -1) return null;

    if (type === '+' || type === '-' || type === ':') {
        return {
            raw: buf.slice(0, crlf + 2),
            consumed: crlf + 2
        };
    }

    if (type === '$') {
        const len = parseInt(buf.slice(1, crlf), 10);
        if (isNaN(len)) return null;
        if (len === -1) {
            return { raw: buf.slice(0, crlf + 2), consumed: crlf + 2 };
        }
        const dataEnd = crlf + 2 + len;
        if (buf.length < dataEnd + 2) return null;
        return {
            raw: buf.slice(0, dataEnd + 2),
            consumed: dataEnd + 2
        };
    }

    if (type === '*') {
        const count = parseInt(buf.slice(1, crlf), 10);
        if (isNaN(count)) return null;
        if (count <= 0) {
            return { raw: buf.slice(0, crlf + 2), consumed: crlf + 2 };
        }
        let pos = crlf + 2;
        for (let i = 0; i < count; i++) {
            if (pos >= buf.length) return null;
            const elem = tryParseOneResp(buf.slice(pos));
            if (!elem) return null;
            pos += elem.consumed;
        }
        return {
            raw: buf.slice(0, pos),
            consumed: pos
        };
    }

    return {
        raw: buf.slice(0, crlf + 2),
        consumed: crlf + 2
    };
}

function extractKeysFromResp(rawResp) {
    if (!rawResp || !rawResp.startsWith('*')) return [];
    const lines = rawResp.split('\r\n');
    const count = parseInt(lines[0].slice(1), 10);
    if (isNaN(count) || count <= 0) return [];

    const keys = [];
    let lineIdx = 1;
    while (lineIdx < lines.length && keys.length < count) {
        const header = lines[lineIdx++];
        if (!header) break;
        if (header.startsWith('$-1')) {
            keys.push(null);
        } else if (header.startsWith('$')) {
            keys.push(lines[lineIdx++] || '');
        }
    }
    return keys.filter(k => k !== null);
}

function formatResp(respStr, stripPrefix = '') {
    if (!respStr) return '';
    const firstChar = respStr[0];

    if (firstChar === '+') {
        return respStr.slice(1).replace(/\r\n$/, '') + '\r\n';
    }
    if (firstChar === '-') {
        return '(error) ' + respStr.slice(1).replace(/\r\n$/, '') + '\r\n';
    }
    if (firstChar === ':') {
        return '(integer) ' + respStr.slice(1).replace(/\r\n$/, '') + '\r\n';
    }
    if (firstChar === '$') {
        if (respStr.startsWith('$-1')) {
            return '(nil)\r\n';
        }
        const newlineIdx = respStr.indexOf('\r\n');
        if (newlineIdx !== -1) {
            let val = respStr.slice(newlineIdx + 2).replace(/\r\n$/, '');
            if (stripPrefix && val.startsWith(stripPrefix)) {
                val = val.slice(stripPrefix.length);
            }
            return `"${val}"\r\n`;
        }
    }
    if (firstChar === '*') {
        if (respStr.startsWith('*-1')) {
            return '(nil)\r\n';
        }
        if (respStr.startsWith('*0')) {
            return '(empty array)\r\n';
        }
        const lines = respStr.split('\r\n');
        const count = parseInt(lines[0].slice(1), 10);
        if (isNaN(count)) return respStr;
        if (count === 0) return '(empty array)\r\n';

        let lineIdx = 1;
        let out = '';
        let itemNum = 1;
        while (lineIdx < lines.length && itemNum <= count) {
            const header = lines[lineIdx++];
            if (!header) break;
            if (header.startsWith('$-1')) {
                out += `${itemNum}) (nil)\r\n`;
                itemNum++;
            } else if (header.startsWith('$')) {
                let val = lines[lineIdx++] || '';
                if (stripPrefix && val.startsWith(stripPrefix)) {
                    val = val.slice(stripPrefix.length);
                }
                out += `${itemNum}) "${val}"\r\n`;
                itemNum++;
            } else if (header.startsWith(':')) {
                out += `${itemNum}) (integer) ${header.slice(1)}\r\n`;
                itemNum++;
            } else if (header.startsWith('+')) {
                out += `${itemNum}) ${header.slice(1)}\r\n`;
                itemNum++;
            }
        }
        return out || respStr;
    }
    return respStr;
}

wss.on('connection', (ws) => {
    let currentUser = null;
    const client = new net.Socket();
    let responseBuffer = '';
    const pendingCallbacks = [];
    let backendAuthenticated = !DEFAULT_PASSWORD;

    function sendToBackend(args) {
        return new Promise((resolve, reject) => {
            if (!client || client.destroyed) {
                return reject(new Error('MiniRedis backend not connected'));
            }
            pendingCallbacks.push((err, rawResp) => {
                if (err) return reject(err);
                resolve(rawResp);
            });
            client.write(encodeRESP(args));
        });
    }

    client.connect(REDIS_PORT, REDIS_HOST, () => {
        // If server has a default password configured in env, authenticate the backend socket immediately
        if (DEFAULT_PASSWORD) {
            client.write(encodeRESP(['AUTH', DEFAULT_PASSWORD]));
        }

        if (ws.readyState === WebSocket.OPEN) {
            const welcomeBanner = 
`MiniRedis Cloud Terminal
Please authenticate to continue:
  • Admin Login:         AUTH <password>
  • Create New Account:  REGISTER <username> <password>
  • User Login:          AUTH <username> <password>
`;
            ws.send(JSON.stringify({
                type: 'system',
                data: welcomeBanner
            }));
        }
    });

    client.on('data', (chunk) => {
        responseBuffer += chunk.toString('utf-8');

        // Consume the handshake AUTH response from the C++ server if needed
        if (!backendAuthenticated) {
            const parsed = tryParseOneResp(responseBuffer);
            if (parsed) {
                responseBuffer = responseBuffer.slice(parsed.consumed);
                backendAuthenticated = true;
            } else {
                return; // Wait for handshake response to finish
            }
        }

        while (pendingCallbacks.length > 0) {
            const parsed = tryParseOneResp(responseBuffer);
            if (!parsed) break; // Incomplete response, wait for more chunks
            responseBuffer = responseBuffer.slice(parsed.consumed);
            const cb = pendingCallbacks.shift();
            cb(null, parsed.raw);
        }
    });

    client.on('close', () => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({
                type: 'system',
                data: '\r\n[MiniRedis connection closed]\r\n'
            }));
            ws.close();
        }
    });

    client.on('error', (err) => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({
                type: 'error',
                data: `\r\nConnection error: ${err.message}\r\n`
            }));
        }
        while (pendingCallbacks.length > 0) {
            const cb = pendingCallbacks.shift();
            cb(err);
        }
    });

    ws.on('message', async (message) => {
        try {
            const raw = message.toString().trim();
            if (!raw) return;

            const args = parseCommandArgs(raw);
            if (args.length === 0) return;

            const commandName = args[0].toUpperCase();

            // 1. CLEAR command (UI helper)
            if (commandName === 'CLEAR') {
                return;
            }

            // 2. HELP command
            if (commandName === 'HELP') {
                const helpText = 
`MiniRedis Cloud Commands:
  • Authentication:
      AUTH <password>         Log in as default administrator
      REGISTER <user> <pass>  Create a new private user database
      AUTH <user> <pass>      Log in to your private database
      WHOAMI                  Show current logged-in identity
      LOGOUT                  Log out of current session
  • Data Operations (keys are isolated for regular users):
      SET <key> <val>         Store a string value
      GET <key>               Retrieve value
      DEL <key1> [key2...]    Delete one or more keys
      EXISTS <key1>...        Check if keys exist
      INCR / DECR <key>       Increment or decrement counter
      INCRBY / DECRBY <key>   Increment/decrement by amount
      MSET <k1> <v1>...       Set multiple key-values
      MGET <k1> <k2>...       Get multiple values
      EXPIRE <key> <sec>      Set key expiration
      TTL <key>               Check time to live
      KEYS [pattern]          List your keys (e.g. KEYS *)
      DBSIZE                  Count total keys in your database
      FLUSHDB                 Delete keys in your database
      PING [msg]              Server health check
      TYPE <key>              Get key data type
`;
                return ws.send(JSON.stringify({ type: 'output', data: helpText }));
            }

            // 3. REGISTER command
            if (commandName === 'REGISTER') {
                if (args.length !== 3) {
                    return ws.send(JSON.stringify({
                        type: 'error',
                        data: "(error) ERR Usage: REGISTER <username> <password>\r\n"
                    }));
                }
                const username = args[1].trim().toLowerCase();
                const password = args[2];

                if (username === 'default' || username === 'admin') {
                    return ws.send(JSON.stringify({
                        type: 'error',
                        data: "(error) ERR 'default' and 'admin' are reserved usernames.\r\n"
                    }));
                }

                if (!/^[a-zA-Z0-9_]{3,20}$/.test(username)) {
                    return ws.send(JSON.stringify({
                        type: 'error',
                        data: '(error) ERR Username must be 3-20 characters (alphanumeric and underscores only).\r\n'
                    }));
                }
                if (password.length < 3) {
                    return ws.send(JSON.stringify({
                        type: 'error',
                        data: '(error) ERR Password must be at least 3 characters long.\r\n'
                    }));
                }

                if (userManager.userExists(username)) {
                    return ws.send(JSON.stringify({
                        type: 'error',
                        data: `(error) ERR Username '${username}' already exists. Use AUTH ${username} <password>\r\n`
                    }));
                }

                userManager.createUser(username, password);
                currentUser = username;

                return ws.send(JSON.stringify({
                    type: 'auth_success',
                    user: username,
                    data: `OK Registered and logged in as '${username}'. Your database is private and isolated.\r\n`
                }));
            }

            // 4. AUTH command (Supports both: AUTH <password> and AUTH <username> <password>)
            if (commandName === 'AUTH') {
                // Case A: AUTH <password>  (Default Admin Login)
                if (args.length === 2) {
                    const password = args[1];
                    if (!DEFAULT_PASSWORD) {
                        return ws.send(JSON.stringify({
                            type: 'error',
                            data: '(error) ERR No admin password configured. Set MINIREDIS_PASSWORD environment variable to enable admin login.\r\n'
                        }));
                    }
                    if (password === DEFAULT_PASSWORD) {
                        currentUser = 'default';
                        return ws.send(JSON.stringify({
                            type: 'auth_success',
                            user: 'admin',
                            data: 'OK Logged in as administrator (default). Full database access granted.\r\n'
                        }));
                    } else {
                        return ws.send(JSON.stringify({
                            type: 'error',
                            data: '(error) WRONGPASS Invalid password for default account.\r\n'
                        }));
                    }
                }
                // Case B: AUTH <username> <password>
                else if (args.length === 3) {
                    const username = args[1].trim().toLowerCase();
                    const password = args[2];

                    if (username === 'default' || username === 'admin') {
                        if (!DEFAULT_PASSWORD) {
                            return ws.send(JSON.stringify({
                                type: 'error',
                                data: '(error) ERR No admin password configured. Set MINIREDIS_PASSWORD environment variable to enable admin login.\r\n'
                            }));
                        }
                        if (password === DEFAULT_PASSWORD) {
                            currentUser = 'default';
                            return ws.send(JSON.stringify({
                                type: 'auth_success',
                                user: 'admin',
                                data: 'OK Logged in as administrator (default). Full database access granted.\r\n'
                            }));
                        } else {
                            return ws.send(JSON.stringify({
                                type: 'error',
                                data: '(error) WRONGPASS Invalid password for admin account.\r\n'
                            }));
                        }
                    }

                    if (userManager.verifyUser(username, password)) {
                        currentUser = username;
                        return ws.send(JSON.stringify({
                            type: 'auth_success',
                            user: username,
                            data: `OK Logged in as '${username}'. Welcome back! Your database is private and isolated.\r\n`
                        }));
                    } else {
                        return ws.send(JSON.stringify({
                            type: 'error',
                            data: '(error) WRONGPASS Invalid username or password.\r\n'
                        }));
                    }
                } else {
                    return ws.send(JSON.stringify({
                        type: 'error',
                        data: "(error) ERR Usage: AUTH <password> (for admin) or AUTH <username> <password>\r\n"
                    }));
                }
            }

            // 5. Unauthenticated guard
            if (!currentUser) {
                return ws.send(JSON.stringify({
                    type: 'error',
                    data: '(error) NOAUTH Authentication required. Please log in with AUTH or create an account with REGISTER.\r\n'
                }));
            }

            // ── Authenticated User Operations ──────────────────────────────
            const isAdmin = (currentUser === 'default');
            const userPrefix = isAdmin ? '' : `u:${currentUser}:`;

            // 6. WHOAMI
            if (commandName === 'WHOAMI') {
                return ws.send(JSON.stringify({
                    type: 'output',
                    data: `"${isAdmin ? 'admin (default)' : currentUser}"\r\n`
                }));
            }

            // 7. LOGOUT
            if (commandName === 'LOGOUT') {
                currentUser = null;
                return ws.send(JSON.stringify({
                    type: 'logout',
                    data: 'OK Logged out. Please authenticate to continue.\r\n'
                }));
            }

            // 8. Restricted commands (Only for non-admin users)
            if (!isAdmin && RESTRICTED_FOR_USERS.has(commandName)) {
                return ws.send(JSON.stringify({
                    type: 'error',
                    data: `(error) ERR Command '${commandName}' is restricted for non-admin users.\r\n`
                }));
            }

            // 9. DBSIZE
            if (commandName === 'DBSIZE') {
                if (isAdmin) {
                    const rawResp = await sendToBackend(['DBSIZE']);
                    return ws.send(JSON.stringify({ type: 'output', data: formatResp(rawResp) }));
                } else {
                    const rawResp = await sendToBackend(['KEYS']);
                    const allKeys = extractKeysFromResp(rawResp);
                    const userCount = allKeys.filter(k => k.startsWith(userPrefix)).length;
                    return ws.send(JSON.stringify({
                        type: 'output',
                        data: `(integer) ${userCount}\r\n`
                    }));
                }
            }

            // 10. FLUSHDB
            if (commandName === 'FLUSHDB') {
                if (isAdmin) {
                    const rawResp = await sendToBackend(['FLUSHDB']);
                    return ws.send(JSON.stringify({ type: 'output', data: formatResp(rawResp) }));
                } else {
                    const rawResp = await sendToBackend(['KEYS']);
                    const allKeys = extractKeysFromResp(rawResp);
                    const userKeys = allKeys.filter(k => k.startsWith(userPrefix));
                    if (userKeys.length > 0) {
                        await sendToBackend(['DEL', ...userKeys]);
                    }
                    return ws.send(JSON.stringify({
                        type: 'output',
                        data: 'OK\r\n'
                    }));
                }
            }

            // 11. KEYS
            if (commandName === 'KEYS') {
                if (isAdmin) {
                    const rawResp = await sendToBackend(['KEYS']);
                    return ws.send(JSON.stringify({ type: 'output', data: formatResp(rawResp) }));
                } else {
                    const rawResp = await sendToBackend(['KEYS']);
                    const allKeys = extractKeysFromResp(rawResp);
                    // Strictly isolate keys: only keys with userPrefix
                    const userKeys = allKeys.filter(k => k.startsWith(userPrefix));
                    const stripped = userKeys.map(k => k.slice(userPrefix.length));
                    if (stripped.length === 0) {
                        return ws.send(JSON.stringify({ type: 'output', data: '(empty array)\r\n' }));
                    }
                    let out = '';
                    stripped.forEach((k, idx) => {
                        out += `${idx + 1}) "${k}"\r\n`;
                    });
                    return ws.send(JSON.stringify({
                        type: 'output',
                        data: out
                    }));
                }
            }

            // 12. Transparent key prefixing (only applied to non-admin users)
            const backendArgs = [...args];

            if (!isAdmin) {
                if (SINGLE_KEY_COMMANDS.has(commandName)) {
                    if (backendArgs[1]) {
                        backendArgs[1] = `${userPrefix}${backendArgs[1]}`;
                    }
                } else if (MULTI_KEY_COMMANDS.has(commandName)) {
                    for (let i = 1; i < backendArgs.length; i++) {
                        backendArgs[i] = `${userPrefix}${backendArgs[i]}`;
                    }
                } else if (commandName === 'MSET') {
                    for (let i = 1; i < backendArgs.length; i += 2) {
                        backendArgs[i] = `${userPrefix}${backendArgs[i]}`;
                    }
                } else if (commandName === 'RENAME') {
                    if (backendArgs[1]) backendArgs[1] = `${userPrefix}${backendArgs[1]}`;
                    if (backendArgs[2]) backendArgs[2] = `${userPrefix}${backendArgs[2]}`;
                }
            }

            // Send command to MiniRedis backend
            const rawResp = await sendToBackend(backendArgs);
            const formatted = formatResp(rawResp);

            return ws.send(JSON.stringify({
                type: 'output',
                data: formatted
            }));

        } catch (err) {
            console.error('Command processing error:', err.message);
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({
                    type: 'error',
                    data: `(error) ERR ${err.message}\r\n`
                }));
            }
        }
    });

    ws.on('error', (err) => {
        console.error('WebSocket client error:', err.message);
        client.destroy();
    });

    ws.on('close', () => {
        client.destroy();
    });
});

// Graceful termination for Render & Docker
function gracefulShutdown(signal) {
    console.log(`\n[${signal}] Shutting down gateway gracefully...`);
    wss.close(() => {
        server.close(() => {
            process.exit(0);
        });
    });
}
process.on('SIGTERM', () => gracefulShutdown('SIGTERM'));
process.on('SIGINT', () => gracefulShutdown('SIGINT'));

server.listen(PORT, '0.0.0.0', () => {
    console.log(`MiniRedis Cloud Terminal running at http://0.0.0.0:${PORT}`);
});