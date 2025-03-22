/**
 * @file connection.h
 * @brief Connection management layer for BLINK DB
 * 
 * @details Manages client connections including buffering for partial reads/writes,
 * connection state tracking, and timeout handling. Acts as an intermediary between
 * the server's socket handling and the RESP protocol processing. Implements
 * non-blocking I/O patterns to efficiently handle multiple concurrent clients.
 */

 #pragma once

 #include <string>
 #include <vector>
 #include <deque>
 #include <chrono>
 
 // Forward declarations
 class Server;
 class RespProtocol;
 
 /**
  * @class Connection
  * @brief Manages a single client connection
  * 
  * @details Handles all aspects of a client connection including:
  * - Buffering for partial reads and writes
  * - Command parsing and execution
  * - Connection state management
  * - Activity tracking for timeout detection
  * 
  * Each Connection instance is associated with a specific client socket and
  * communicates with the server to execute commands.
  */
 class Connection {
 public:
     /**
      * @brief Check if connection has pending data to write
      * 
      * @details Used by the server to determine whether to register
      * the socket for write events in epoll.
      * 
      * @return true if there is data in the output queue, false otherwise
      */
     bool has_pending_writes() const { return !output_queue_.empty(); }
 
     /**
      * @brief Connection state enumeration
      * 
      * @details Represents the lifecycle states of a client connection
      */
     enum class State {
         CONNECTED,     ///< Connection established and active
         CLOSING,       ///< Connection is being closed
         CLOSED         ///< Connection is closed
     };
 
     /**
      * @brief Construct a new Connection object
      * 
      * @details Initializes a connection with the specified socket file descriptor
      * and server reference. Sets initial state to CONNECTED.
      * 
      * @param fd Socket file descriptor for this connection
      * @param server Pointer to server instance that owns this connection
      */
     Connection(int fd, Server* server);
     
     /**
      * @brief Destroy the Connection object
      * 
      * @details Closes the socket if still open and releases resources
      */
     ~Connection();
 
     /**
      * @brief Handle data available to read
      * 
      * @details Called by the server when the socket is ready for reading.
      * Reads data from the socket, appends to input buffer, and processes
      * any complete commands found in the buffer.
      * 
      * @return true on success or would block, false on error/connection close
      */
     bool handle_read();
     
     /**
      * @brief Handle socket ready for writing
      * 
      * @details Called by the server when the socket is ready for writing.
      * Writes pending data from the output queue to the socket, handling
      * partial writes appropriately.
      * 
      * @return true on success or would block, false on error
      */
     bool handle_write();
     
     /**
      * @brief Add response to output buffer
      * 
      * @details Queues a response to be sent to the client when the socket
      * is ready for writing. The server should register for write events
      * when this method adds items to an empty queue.
      * 
      * @param response Response data to send to the client
      */
     void add_response(const std::string& response);
     
     /**
      * @brief Check if connection has timed out
      * 
      * @details Compares the time since last activity against the provided
      * timeout value to determine if the connection should be considered inactive.
      * 
      * @param timeout_ms Maximum allowed idle time in milliseconds
      * @return true if connection has been idle longer than timeout_ms, false otherwise
      */
     bool check_timeout(std::chrono::milliseconds timeout_ms);
     
     /**
      * @brief Get socket file descriptor
      * 
      * @details Returns the file descriptor associated with this connection.
      * Used by the server for epoll management.
      * 
      * @return int Socket file descriptor
      */
     int get_fd() const { return fd_; }
     
     /**
      * @brief Get connection state
      * 
      * @details Returns the current state of the connection (CONNECTED, CLOSING, CLOSED).
      * Used by the server to determine how to handle the connection.
      * 
      * @return State Current connection state
      */
     State get_state() const { return state_; }
 
 private:
     int fd_;                                  ///< Socket file descriptor
     Server* server_;                          ///< Server reference for command execution
     State state_;                             ///< Current connection state
     std::string input_buffer_;                ///< Buffer for incoming data
     std::deque<std::string> output_queue_;    ///< Queue of pending responses
     std::chrono::steady_clock::time_point last_activity_; ///< Last activity timestamp
     
     /**
      * @brief Process any complete commands in input buffer
      * 
      * @details Parses RESP-2 protocol commands from the input buffer,
      * executes them via the server, and queues responses for sending.
      * Handles partial commands by waiting for more data.
      * 
      * @return true on success, false on protocol error
      */
     bool process_commands();
     
     /**
      * @brief Update last activity timestamp
      * 
      * @details Records the current time as the last activity time
      * for this connection. Called on read/write operations to track
      * connection freshness for timeout detection.
      */
     void update_last_activity();
     
     /**
      * @brief Reset the connection state
      * 
      * @details Clears the input buffer and output queue, and updates
      * the last activity timestamp. Used when reusing a connection or
      * recovering from error states.
      */
     void reset();
 };
 