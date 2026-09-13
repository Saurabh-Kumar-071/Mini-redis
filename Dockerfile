# Multi-stage Dockerfile for MiniRedis

# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code and build files
COPY . .

# Build the project in Release mode
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make

# Stage 2: Minimal runtime environment
FROM ubuntu:22.04

# Create a non-root user for security — never run as root!
RUN useradd -r -s /bin/false -d /app miniredis

WORKDIR /app

# Copy compiled binary from builder stage
COPY --from=builder /app/build/MiniRedis /app/MiniRedis

# Create data directory and set correct ownership
RUN mkdir -p /app/data && chown -R miniredis:miniredis /app

# Create directory for dump.rdb persistence
VOLUME ["/app/data"]

# Expose Redis port
EXPOSE 8080

# Drop root privileges — run as non-root user
USER miniredis

# Run MiniRedis server
CMD ["/app/MiniRedis"]
