# http-server-cpp

A **HTTP/1.1 server built from scratch in C++** supporting concurrent clients via multithreading, gzip compression, and file serving. Built as part of the [CodeCrafters HTTP Server challenge](https://app.codecrafters.io/courses/http-server/overview).

## Features

- **Multithreaded** — each client handled in a dedicated `std::thread`
- **Gzip compression** — detects `Accept-Encoding: gzip` and compresses responses using zlib
- **File serving** — GET and POST to `/files/<name>` reads/writes files from a given directory
- **Echo route** — `/echo/<text>` returns the text back
- **User-Agent route** — `/user-agent` returns the client's User-Agent header
- **Video streaming** — `/stream` endpoint for AVI video streaming
- **Runs on Linux / WSL**

## Routes

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Root — returns an HTML page |
| GET | `/echo/<text>` | Echoes text back (gzip if supported) |
| GET | `/user-agent` | Returns the request's User-Agent |
| GET | `/files/<name>` | Serves a file from the directory |
| POST | `/files/<name>` | Writes request body to a file |
| GET | `/stream` | Streams a video file |

## Build & Run

### Prerequisites
- g++ with C++20 support
- zlib (`sudo apt install zlib1g-dev`)
- cmake (optional)

### Compile directly
```bash
g++ server.cpp request_handler.cpp -o server -lz -lpthread
./server
```

### Or with CMake
```bash
mkdir build && cd build
cmake ..
make
./server
```

### With file directory support
```bash
./server --directory /path/to/files
```

Server listens on port **4221**.

## WSL / Windows Setup

If running inside WSL, forward the port from Windows:

```powershell
# In PowerShell (Admin) — replace <WSL_IP> with output of: ip addr show eth0
netsh interface portproxy add v4tov4 listenport=4221 listenaddress=0.0.0.0 connectport=4221 connectaddress=<WSL_IP>
netsh advfirewall firewall add rule name="WSL Port 4221" dir=in action=allow protocol=TCP localport=4221
```

## Project Structure

```
http-server-cpp/
├── server.cpp           # Entry point — socket setup, accept loop, thread spawn
├── request_handler.h    # Handler declarations
├── request_handler.cpp  # Route handlers (echo, files, user-agent, stream, gzip)
├── CMakeLists.txt       # CMake build config
└── .gitignore
```

## License

MIT License