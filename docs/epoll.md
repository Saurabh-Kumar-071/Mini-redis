## Phase 3: Event Driven Server Using epoll

### Why epoll?

Problem:

Thread-per-client architecture does not scale.

Example:

10000 Clients
→ 10000 Threads

This causes:

* High memory usage
* Context switching overhead
* Poor scalability

---

### epoll_create1()

Purpose:

Create an epoll instance.

Example:

```cpp
int epoll_fd = epoll_create1(0);
```

Learned:

epoll acts as an event manager.

---

### epoll_ctl()

Purpose:

Register sockets with epoll.

Example:

```cpp
epoll_ctl(
    epoll_fd,
    EPOLL_CTL_ADD,
    server_fd,
    &ev
);
```

Learned:

EPOLL_CTL_ADD → start watching a socket.

EPOLL_CTL_DEL → stop watching a socket.

---

### EPOLLIN

Meaning:

Socket is ready for reading.

Example:

```cpp
ev.events = EPOLLIN;
```

---

### epoll_wait()

Purpose:

Sleep until an event occurs.

Example:

```cpp
epoll_wait(
    epoll_fd,
    events,
    10,
    -1
);
```

Learned:

* Does not poll continuously.
* Wakes only when something becomes ready.
* Efficient for large numbers of clients.

---

### server_fd Event

When:

```cpp
events[i].data.fd == server_fd
```

Meaning:

A new client wants to connect.

Action:

```cpp
accept(...)
```

---

### client_fd Event

When:

```cpp
events[i].data.fd != server_fd
```

Meaning:

An existing client sent data.

Action:

```cpp
recv(...)
```

---

### Client Registration

After accept():

```cpp
client_event.data.fd = client_fd;

epoll_ctl(
    epoll_fd,
    EPOLL_CTL_ADD,
    client_fd,
    &client_event
);
```

Learned:

epoll must watch both:

* server_fd
* client_fd

---

### Disconnect Handling

When:

```cpp
recv(...) == 0
```

Meaning:

Client disconnected.

Correct cleanup:

```cpp
epoll_ctl(
    epoll_fd,
    EPOLL_CTL_DEL,
    fd,
    nullptr
);

close(fd);
```

Learned:

Remove from epoll before closing.

---

### Echo Server Using epoll

Flow:

Client
↓
recv()
↓
send()
↓
Client receives response

---

### First Redis Command

Implemented:

PING

Response:

PONG

Learned:

Server can parse commands and generate custom responses instead of simply echoing data.
