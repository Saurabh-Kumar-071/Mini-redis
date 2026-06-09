Why Thread Per Client Fails

1. Memory overhead
2. Context switching
3. Thousands of sleeping threads

Why Non-Blocking Alone Fails

1. Polling wastes CPU

Why epoll Exists

1. OS tells us only ready clients
2. One thread can manage thousands of connections


