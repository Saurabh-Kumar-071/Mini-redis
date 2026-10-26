#!/bin/bash
set -e

# Terminate background processes cleanly on exit
trap 'kill $(jobs -p) 2>/dev/null || true' EXIT SIGTERM SIGINT

echo "[+] Starting MiniRedis C++ Backend Server on port 8080..."
if [ -f "/app/build/MiniRedis" ]; then
    /app/build/MiniRedis &
elif [ -f "/app/MiniRedis" ]; then
    /app/MiniRedis &
else
    ./build/MiniRedis &
fi

# Wait for MiniRedis to accept connections
echo "[*] Waiting for MiniRedis to accept connections..."
sleep 1

echo "[+] Starting Node.js Web Gateway on port ${PORT:-3000}..."
if [ -f "/app/web/server.js" ]; then
    exec node /app/web/server.js
else
    exec node web/server.js
fi