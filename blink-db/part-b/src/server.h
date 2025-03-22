/**
 * @file server.h
 * @brief TCP server with epoll() for I/O multiplexing
 * 
 * @details This header defines the Server class, which is the core component of the BLINK DB
 * networking layer. It implements a high-performance TCP server using the epoll() mechanism
 * for efficient I/O multiplexing, allowing it to handle thousands of concurrent connections
 * with minimal resource usage. The server integrates with the storage engine from Part A
 * and communicates with clients using the RESP-2 protocol.
 */

 #pragma once

 #include <string>
 #include <unordered_map>
 #include <functional>
 #include <sys/epoll.h>
 #include <atomic>
 #include <vector>
 #include "StorageEngine.h"
 
 // Forward declaration
 class Connection;
 class RespProtocol;
 
 /**
  * @class Server
  * @brief Implements a TCP server using epoll for efficient I/O multiplexing
  * 
  * @details The Server class is responsible for:
  * - Setting up and managing a TCP socket listening on a configured port
  * - Using epoll() to efficiently handle multiple client connections
  * - Accepting new connections and creating Connection objects to manage them
  * - Dispatching received commands to appropriate handlers
  * - Maintaining the server lifecycle (initialization, running, shutdown)
  * - Integrating with the Storage Engine to execute commands
  * 
  * This implementation follows an event-driven architecture using edge-triggered epoll
  * for optimal performance with non-blocking I/O operations.
  */
 class Server {
 public:
     /**
      * @typedef CommandHandler
      * @brief Function type for command handlers
      * 
      * @details Defines the signature for command handler functions that process
      * client commands and return RESP-formatted responses.
      */
     using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;
 
     /**
      * @brief Construct a TCP server
      * 
      * @details Initializes a new server instance with the specified configuration.
      * The server won't start listening until init() is called.
      * 
      * @param port Port to listen on (default: 9001)
      * @param max_connections Maximum number of concurrent connections (default: 1024)
      */
     Server(int port = 9001, int max_connections = 1024);
     
     /**
      * @brief Destroy the server and release resources
      * 
      * @details Closes all connections, releases socket and epoll resources,
      * and ensures the server is properly shut down.
      */
     ~Server();
 
     /**
      * @brief Initialize the server
      * 
      * @details Creates the listening socket, binds to the configured port,
      * initializes epoll, and registers command handlers. This must be called
      * before run().
      * 
      * @return true on successful initialization, false on failure
      */
     bool init();
     
     /**
      * @brief Start the server and run the event loop
      * 
      * @details Enters the main server loop, listening for events using epoll and
      * processing them accordingly. This method blocks until stop() is called from
      * another thread or a signal handler.
      */
     void run();
     
     /**
      * @brief Stop the server gracefully
      * 
      * @details Terminates the main event loop, closes all client connections,
      * and releases server resources. Can be called from a signal handler or
      * another thread to shut down the server.
      */
     void stop();
     
     /**
      * @brief Register a command handler
      * 
      * @details Associates a command name (e.g., "SET") with a handler function
      * that will be called when clients send that command. The handler receives
      * the command arguments and returns a RESP-formatted response.
      * 
      * @param command Command name (e.g., "SET", "GET", "DEL") - case-sensitive
      * @param handler Function to handle the command
      */
     void register_command(const std::string& command, CommandHandler handler);
 
     /**
      * @brief Execute a command and return the response
      * 
      * @details Looks up the appropriate handler for the command and executes it,
      * returning the RESP-formatted response. If the command is not recognized or
      * an error occurs, returns an appropriate error response.
      * 
      * @param command Command name (e.g., "SET", "GET", "DEL")
      * @param args Vector of command arguments
      * @return Response string in RESP format
      */
     std::string execute_command(const std::string& command, const std::vector<std::string>& args);
 
 private:
     int port_;                             ///< Server port number to listen on
     int listen_fd_;                        ///< Listening socket file descriptor
     int epoll_fd_;                         ///< epoll instance file descriptor
     int max_connections_;                  ///< Maximum number of concurrent connections allowed
     std::atomic<bool> running_;            ///< Flag to control the server loop execution
     
     StorageEngine storage_engine_;         ///< Storage engine from Part A for data operations
     std::unordered_map<std::string, CommandHandler> command_handlers_; ///< Map of command names to handler functions
     std::unordered_map<int, Connection*> connections_; ///< Map of file descriptors to Connection objects
 
     /**
      * @brief Set a socket to non-blocking mode
      * 
      * @details Modifies socket flags using fcntl() to make operations non-blocking,
      * which is essential for epoll edge-triggered mode.
      * 
      * @param fd Socket file descriptor to modify
      * @return true on success, false on failure
      */
     bool set_nonblocking(int fd);
     
     /**
      * @brief Accept a new connection
      * 
      * @details Accepts a new client connection from the listening socket,
      * creates a Connection object to manage it, and registers it with epoll
      * for event monitoring.
      * 
      * @return true on successful connection acceptance, false on failure
      */
     bool accept_connection();
     
     /**
      * @brief Handle an epoll event
      * 
      * @details Processes events for client connections, including reading data,
      * writing responses, and handling errors or disconnections. Updates epoll
      * registration based on whether the connection has pending writes.
      * 
      * @param fd File descriptor that triggered the event
      * @param events Event flags from epoll_wait
      */
     void handle_event(int fd, uint32_t events);
     
     /**
      * @brief Close a connection and clean up resources
      * 
      * @details Removes a connection from epoll monitoring, deletes the
      * associated Connection object, and closes the socket.
      * 
      * @param fd File descriptor of the connection to close
      */
     void close_connection(int fd);
 };
 