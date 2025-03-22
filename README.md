# BLINK DB - README

## Project Overview
BLINK DB is a high-performance key-value database that implements a TCP server with RESP-2 protocol support (Redis wire protocol). The system consists of two main components:
1. **Storage Engine (Part A)**: An in-memory key-value store with hash table implementation
2. **Network Layer (Part B)**: A TCP server with epoll() for I/O multiplexing and RESP-2 protocol support

## Prerequisites

- Linux-based operating system
- C++17 compatible compiler (g++ 8.0 or higher)
- Make build system
- Redis tools package (for benchmarking)
___________________________________________________________________________________________
## TLDR:
1. To run benchmark on Part A run: 
   ```
   cd part-a
   make all && make gen_benchmarks && make run_benchmarks
   ```
2. To run client and server
   ```
   cd part-b
   make
   make run
   ```
3. Run redis-benchmark (redis-tools installation required)
   ```
   make benchmark
   ```
___________________________________________________________________________________________

## Run benchmark on Part A
1. Build files and run this command
   ```bash
   cd part-a
   make all && make gen_benchmarks && make run_benchmarks
   ```
## Building the Project

### 1. Download blink-db

### 2. Build both server and client & start running them in seperate terminals :
   ```bash
   cd part-b
   make
   make run
   ```
OR
This will compile all necessary files and create the server and client executables in the `build` directory.

## Running the Server

```bash
make run-server
```

### Server Options:
- `-p, --port PORT`: Specify the port to listen on (default: 9001)
- `-c, --connections N`: Maximum number of concurrent connections (default: 1024)
- `-h, --help`: Display help message

## Running the Client

```bash
make run-client
```

### Client Options:
- `-h, --host HOST`: Server hostname or IP (default: localhost)
- `-p, --port PORT`: Server port (default: 9001)
- `--help`: Display help message

### 3. Using the Client

The client provides an interactive command-line interface. After starting the client, you can enter commands at the `blink>` prompt:

```
blink> SET mykey myvalue
OK
blink> GET mykey
myvalue
blink> DEL mykey
1
blink> GET mykey
NULL
blink> exit
```

### Supported Commands:
- `SET key value [EX seconds]`: Store a key-value pair with optional expiration time
- `GET key`: Retrieve a value by key
- `DEL key`: Delete a key-value pair
- `exit` or `quit`: Exit the client

## 4. Running Benchmarks

To evaluate the performance of BLINK DB, you can run benchmarks using the provided make target:

```bash
make benchmark
```

This command runs Redis benchmark tool with 9 different configurations:
- Request counts: 10,000, 100,000, and 1,000,000
- Connection counts: 10, 100, and 1,000
- Commands tested: SET and GET

**Note**: You need to install Redis tools to run benchmarks:
```bash
sudo apt-get install redis-tools
```

Benchmark results will be saved in the `result` directory.

## Project Structure

The project is organized into the following files:

- `server.h/cpp`: TCP server with epoll() for I/O multiplexing
- `connection.h/cpp`: Connection management for client connections
- `resp.h/cpp`: RESP-2 protocol encoder/decoder
- `client.h/cpp`: Client implementation for connecting to the server
- `client_main.cpp`: Entry point for the client application
- `main.cpp`: Entry point for the server application
- `StorageEngine.h/cpp`: Storage engine from Part A (key-value store)

## Clean Build

To clean the build files:
```bash
make clean
```

## Performance

The system is designed for high performance with sub-millisecond latency for most operations. The benchmarks demonstrate the system's capability to handle thousands of concurrent connections while maintaining high throughput.

## Additional Notes

- The server uses edge-triggered epoll for optimal performance with non-blocking I/O
- The system implements the complete RESP-2 protocol, allowing compatibility with Redis clients
- Connection timeouts and buffer size limits are implemented to prevent resource exhaustion attacks
