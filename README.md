# Redish - A Redis Clone in C++

This project is a simplified Redis clone written in C++. It features asynchronous I/O handling using epoll, a reactor-based architecture, and support for a subset of core Redis commands. The server is built from the ground up with scalability and testability in mind.

## Features

- Asynchronous TCP server using epoll and reactor pattern
- Incremental RESP protocol parser integrated into the event loop
- Support for essential Redis commands, including:
  - `PING`, `SET`, `GET`, `DEL`, `EXISTS`, `FLUSHDB`, `INCR`, `DECR`
  - List operations: `LPUSH`, `RPUSH`, `LRANGE`
  - Persistence: `SAVE`
- Scales to handle 50+ concurrent clients
- Thorough test coverage with Google Test (GTest) and redis-py
- CMake-based build system

## Build Instructions

```bash
git clone git@github.com:km5762/redish.git
cd redish
mkdir build && cd build
cmake ..
cmake --build .
```
