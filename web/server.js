const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const net = require('net');
const path = require('path');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const REDIS_HOST = process.env.REDIS_HOST || '127.0.0.1';
const REDIS_PORT = parseInt(process.env.REDIS_PORT || '8080', 10);
const PORT = process.env.PORT || 3000;

app.use(express.static(path.join(__dirname, 'public')));

// Health check endpoint for cloud platforms (Render, Railway, etc.)
app.get('/healthz', (req, res) => {
    res.json({ status: 'healthy', service: 'MiniRedis Web Gateway' });
});

wss.on('connection', (ws) => {
    const client = new net.Socket();

    client.connect(REDIS_PORT, REDIS_HOST, () => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({
                type: 'system',
                data: 'Connected to MiniRedis C++ Server (Port ' + REDIS_PORT + ')\r\nType commands below (e.g. PING, SET, GET, INCR, DBSIZE)...\r\n'
            }));
        }
    });

    client.on('data', (data) => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({
                type: 'output',
                data: data.toString('utf-8')
            }));
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
                data: '\r\nConnection error: ' + err.message + '\r\n'
            }));
        }
    });

    ws.on('message', (message) => {
        try {
            const raw = message.toString();
            if (client && !client.destroyed) {
                client.write(raw.trim() + '\r\n');
            } else if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ type: 'error', data: '[Error: MiniRedis server is not connected]\r\n' }));
            }
        } catch (err) {
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ type: 'error', data: err.message }));
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

server.listen(PORT, '0.0.0.0', () => {
    console.log(`MiniRedis Web Terminal running at http://0.0.0.0:${PORT}`);
});