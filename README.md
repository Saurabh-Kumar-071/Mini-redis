# MiniRedis: Asynchronous In-Memory Key-Value Store

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Linux](https://img.shields.io/badge/Platform-Linux-orange.svg)](https://www.kernel.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An asynchronous, high-performance, in-memory key-value database built in **C++17** from scratch on Linux. Implements the **Redis Serialization Protocol (RESP)**, an **epoll Edge-Triggered Reactor Event Loop**, **hybrid key eviction**, and **automated snapshot persistence**.

---

## Key Features

- **27 Redis Commands with Full RESP Compliance**: Implements core Redis commands with full standard RESP data types (Simple Strings, Errors, Integers, Bulk Strings, Null Bulk Strings, Arrays) and case-insensitive command parsing.
- **Asynchronous Reactor Event Loop**: Engineered on Linux Edge-Triggered (`EPOLLET`) `epoll` with non-blocking POSIX sockets to handle high-throughput concurrent clients without multi-threading overhead.
- **Hybrid Key Eviction Strategy**:
  - **Passive Expired Eviction**: Instant on-access expiration checks on `GET`, `EXISTS`, `TTL`, `STRLEN`, `APPEND`, `KEYS`.
  - **Active `timerfd` Periodic Sweeps**: Background timerfd sweeps every 5 seconds to reclaim memory from expired keys.
- **Snapshot Persistence (`dump.rdb`)**: Automated write-through snapshots persisting dirty database state and key TTL timestamps across server restarts.
- **High Stability & Clean Shutdown**:
  - **RAII Socket Management**: `FileDescriptor` RAII abstractions preventing descriptor leaks.
  - **Crash-Resilient Input Validation**: Fuzz-tested against corrupted/malformed RESP frames and integer boundaries.
  - **Graceful Signal Handling**: Intercepts `SIGINT` / `SIGTERM` (Ctrl+C), flushes pending dirty database state to `dump.rdb`, and shuts down cleanly.

---

## Supported Redis Commands (27 Commands)

| Category | Commands | Description |
|---|---|---|
| **Connection & Server** | `PING`, `ECHO`, `INFO`, `CONFIG`, `DBSIZE`, `FLUSHDB` | Server health, diagnostics, and DB cleanup |
| **String Operations** | `SET`, `GET`, `MSET`, `MGET`, `GETSET`, `APPEND`, `STRLEN` | Key-value manipulation with binary-safe strings |
| **Numeric Operations** | `INCR`, `DECR`, `INCRBY`, `DECRBY` | Atomic 64-bit integer counters |
| **Key & Expiration** | `EXISTS`, `DEL`, `MDEL`, `TYPE`, `KEYS`, `RENAME`, `EXPIRE`, `TTL`, `PERSIST` | Key lifecycle, namespace queries, and TTL timers |
| **Testing** | `BIG` | Diagnostic command for streaming large payload frames over epoll |

---

## Architecture Overview

```
                        +---------------------------+
                        |       Client Socket       |
                        +-------------+-------------+
                                      |
                                      v
                        +---------------------------+
                        |  Linux epoll (EPOLLET)    | <--- Linux timerfd (5s sweep)
                        +-------------+-------------+
                                      |
                                      v
                        +---------------------------+
                        |     Reactor Scheduler     |
                        +-------------+-------------+
                                      |
                        +-------------v-------------+
                        |     RESP Command Parser   |
                        +-------------+-------------+
                                      |
                        +-------------v-------------+
                        |      Command Executor     |
                        +-------------+-------------+
                                      |
                       +--------------+--------------+
                       |                             |
                       v                             v
            +--------------------+        +--------------------+
            |  Database (Memory) |        | Persistence (RDB)  |
            +--------------------+        +--------------------+
```

---

## Getting Started

### Prerequisites
- Linux OS (Ubuntu 20.04+, Debian, Arch Linux, etc.)
- GCC / G++ (C++17 support)
- CMake (version 3.10+)
- Python 3 (for live integration test suite)

### Build Instructions

```bash
# Clone repository
git clone https://github.com/Saurabh-Kumar-071/Mini-redis.git
cd Mini-redis

# Configure and compile
mkdir -p build && cd build
cmake ..
make
```

### Running the Server

```bash
./build/MiniRedis
```

Output:
```
[INFO] Socket Creation Successfully
[INFO] Bind connection is successfully
[INFO] Server listening on port 8080. Waiting for clients...
```

### Connecting with `redis-cli` or `nc`

```bash
# Connect using standard redis-cli
redis-cli -p 8080

127.0.0.1:8080> PING
PONG
127.0.0.1:8080> SET username Saurabh
OK
127.0.0.1:8080> GET username
"Saurabh"
127.0.0.1:8080> EXPIRE username 60
(integer) 1
127.0.0.1:8080> TTL username
(integer) 58
127.0.0.1:8080> INCR visitors
(integer) 1
```

---

## Running the Automated Test Suites

### 1. Unit & Component Test Suite (C++)
Runs comprehensive automated assertions across all 27 commands, RESP encoders, parser fuzzing, passive/active eviction, and RDB persistence:

```bash
./build/test_suite
```

### 2. Live TCP Integration Test Suite (Python)
Spawns live server process, tests concurrent TCP connections, command pipelining, malformed packet fuzzing, and verifies clean `SIGINT` shutdown and `dump.rdb` persistence:

```bash
python3 tests/integration_test.py
```

---

## Project Structure

```
.
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # Project documentation
├── src
│   ├── main.cpp                # Server entry point
│   ├── client                  # ClientConnection & socket buffer management
│   ├── command                 # RESP CommandParser & CommandExecutor (27 commands)
│   ├── database                # In-memory storage & TTL expiration logic
│   ├── exception               # Custom exception hierarchy
│   ├── logger                  # Console, File, and Composite logging
│   ├── network                 # EpollManager (EPOLLET) & FileDescriptor RAII
│   ├── persistence             # PersistenceManager (dump.rdb snapshot engine)
│   ├── protocol                # RESPEncoder serialization
│   ├── scheduler               # Reactor event dispatcher
│   └── utils                   # Helper utilities
└── tests
    ├── test_suite.cpp          # Automated C++ test suite
    └── integration_test.py     # Live TCP integration & fuzzing test suite
```

---

## Deployment

### Option 1: Docker (Recommended)

Build and run using Docker:
```bash
# Build the Docker image
docker build -t miniredis .

# Run container exposing port 8080
docker run -d -p 8080:8080 -v $(pwd)/data:/app miniredis
```

Or using **Docker Compose**:
```bash
docker compose up -d
```

### Option 2: Linux Cloud VM (AWS EC2 / DigitalOcean / GCP) with Systemd

1. Copy the repository to `/opt/miniredis` and build the binary:
   ```bash
   sudo git clone https://github.com/Saurabh-Kumar-071/Mini-redis.git /opt/miniredis
   cd /opt/miniredis && mkdir -p build && cd build && cmake .. && make
   ```

2. Install and enable the systemd service:
   ```bash
   sudo cp /opt/miniredis/miniredis.service /etc/systemd/system/
   sudo systemctl daemon-reload
   sudo systemctl enable --now miniredis
   ```

3. Check service status:
   ```bash
   sudo systemctl status miniredis
   ```

---

## License

This project is open source and available under the [MIT License](LICENSE).

