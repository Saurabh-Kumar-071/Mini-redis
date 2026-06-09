# Mini Redis Learning Journal

## Project Goal

Build a Redis-like server in C++ from scratch to understand:

* Computer Networking
* TCP Sockets
* Concurrency
* Event Driven Systems
* Database Internals
* Redis Architecture

---

## Phase 1: Basic TCP Server

### socket()

Purpose:

Create a communication endpoint.

Example:

```cpp
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
```

Learned:

* AF_INET = IPv4
* SOCK_STREAM = TCP
* Returns a file descriptor

Analogy:

Buying a phone before making calls.

---

### bind()

Purpose:

Attach a socket to an IP address and port.

Example:

```cpp
bind(server_fd,
     (sockaddr*)&server_addr,
     sizeof(server_addr));
```

Learned:

* Port = room number inside a machine
* IP = machine address

Analogy:

Assigning a phone number to a phone.

---

### listen()

Purpose:

Start accepting connection requests.

Example:

```cpp
listen(server_fd, 5);
```

Learned:

* Backlog queue stores pending connections.

Analogy:

Turning the phone on and waiting for calls.

---

### accept()

Purpose:

Accept an incoming connection.

Example:

```cpp
int client_fd = accept(...);
```

Learned:

* server_fd listens
* client_fd communicates

Important:

Server socket and client socket are different.

---

### recv()

Purpose:

Receive data from a client.

Example:

```cpp
recv(client_fd,
     buffer,
     sizeof(buffer),
     0);
```

Learned:

* recv() is blocking by default.
* recv() > 0 => data received
* recv() == 0 => client disconnected
* recv() < 0 => error

---

### send()

Purpose:

Send data to a client.

Example:

```cpp
send(client_fd,
     buffer,
     bytes_received,
     0);
```

Learned:

* Used to reply to clients.
* Built an Echo Server.

---

## Phase 2: Multi-Client Server Using Threads

### Problem

Single-threaded server blocks on recv().

One slow client can block all others.

---

### std::thread

Purpose:

Handle multiple clients simultaneously.

Example:

```cpp
std::thread t(handleClient, client_fd);
```

Learned:

Each client can be handled independently.

---

### join()

Meaning:

Wait for the thread to finish.

Problem:

Main thread stops accepting new clients.

---

### detach()

Meaning:

Allow thread to run independently.

Example:

```cpp
t.detach();
```

Result:

Main thread immediately returns to accept().

---

### Current Architecture

Main Thread
|
accept()
|
+---+---+
|   |   |
T1  T2  T3

Each client gets a dedicated thread.

---

## Problems With Thread Per Client

1. High memory usage
2. Context switching overhead
3. Not scalable for thousands of clients

Example:

10000 Clients
→ 10000 Threads

Not practical.

---

## Next Topic

### Non-Blocking Sockets

Goal:

Avoid waiting forever inside recv().

Idea:

Instead of:

recv()
→ wait

Use:

recv()
→ return immediately if no data exists

This is the foundation required for epoll.
