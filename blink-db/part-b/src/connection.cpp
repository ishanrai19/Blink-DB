/**
 * @file connection.cpp
 * @brief Implementation of Connection management layer
 * 
 * @details This file implements the Connection class which manages individual client
 * connections to the BLINK DB server. It handles buffer management for partial reads/writes,
 * connection state tracking, command processing, and timeout detection.
 */

 #include "connection.h"
 #include "server.h"
 #include "resp.h"
 #include <unistd.h>
 #include <errno.h>
 #include <cstring>
 #include <iostream>
 #include <sys/socket.h>
 #include <algorithm>
 
 /** @brief Maximum size for a single read operation */
 const size_t MAX_READ_SIZE = 65536; // 64KB
 
 /** @brief Maximum allowed input buffer size to prevent memory exhaustion attacks */
 const size_t MAX_INPUT_BUFFER_SIZE = 1024 * 1024 * 10; // 10MB
 
 /**
  * @brief Constructs a new Connection object
  * 
  * @details Initializes a connection with the provided socket file descriptor and server pointer.
  * Sets the connection state to CONNECTED and records the current time as the last activity.
  * 
  * @param fd Socket file descriptor for this connection
  * @param server Pointer to the server instance that owns this connection
  */
 Connection::Connection(int fd, Server* server)
     : fd_(fd),
       server_(server),
       state_(State::CONNECTED) {
     update_last_activity();
 }
 
 /**
  * @brief Destroys the Connection object
  * 
  * @details Closes the socket file descriptor if it's still open
  */
 Connection::~Connection() {
     if (fd_ >= 0) {
         close(fd_);
         fd_ = -1;
     }
 }
 
 /**
  * @brief Handles data available for reading from the socket
  * 
  * @details Performs a non-blocking read from the socket, appends data to the input buffer,
  * and processes any complete commands found in the buffer. Implements protection against
  * buffer overflow attacks by limiting the maximum buffer size.
  * 
  * @return true if read was successful or would block, false on error or connection closed
  */
 bool Connection::handle_read() {
     if (state_ != State::CONNECTED) {
         return false;
     }
     
     // Allocate read buffer
     char read_buffer[MAX_READ_SIZE];
     
     // Non-blocking read
     ssize_t bytes_read = recv(fd_, read_buffer, sizeof(read_buffer), 0);
     
     if (bytes_read > 0) {
         // Update last activity timestamp
         update_last_activity();
         
         // Check input buffer size to prevent OOM attacks
         if (input_buffer_.size() + bytes_read > MAX_INPUT_BUFFER_SIZE) {
             std::cerr << "Input buffer overflow from client: " << fd_ << std::endl;
             return false;
         }
         
         // Append to input buffer
         input_buffer_.append(read_buffer, bytes_read);
         
         // Process any complete commands
         return process_commands();
     } else if (bytes_read == 0) {
         // Connection closed by client
         state_ = State::CLOSING;
         std::cout << "Connection closed by client: " << fd_ << std::endl;
         return false;
     } else {
         // Error or would block
         if (errno == EAGAIN || errno == EWOULDBLOCK) {
             // No data available right now, not an error
             return true;
         }
         
         // Actual error
         std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
         return false;
     }
 }
 
 /**
  * @brief Handles socket ready for writing
  * 
  * @details Sends data from the output queue to the client. Handles partial writes
  * by keeping track of what's been sent and what remains to be sent.
  * 
  * @return true if write was successful or would block, false on error
  */
 bool Connection::handle_write() {
     if (state_ != State::CONNECTED || output_queue_.empty()) {
         return true; // Nothing to write
     }
     
     // Get the front message
     const std::string& message = output_queue_.front();
     
     // Try to send it
     ssize_t bytes_sent = send(fd_, message.data(), message.size(), 0);
     
     if (bytes_sent > 0) {
         // Update last activity timestamp
         update_last_activity();
         
         if (static_cast<size_t>(bytes_sent) == message.size()) {
             // Full message sent, remove from queue
             output_queue_.pop_front();
         } else {
             // Partial send, keep remaining data in queue
             output_queue_.front() = message.substr(bytes_sent);
         }
         
         return true;
     } else if (bytes_sent == 0) {
         // Connection closed
         std::cerr << "Connection closed during write: " << fd_ << std::endl;
         return false;
     } else {
         // Error
         if (errno == EAGAIN || errno == EWOULDBLOCK) {
             // Would block, try again later
             return true;
         }
         
         std::cerr << "Error writing to socket: " << strerror(errno) << std::endl;
         return false;
     }
 }
 
 /**
  * @brief Adds a response to the output queue
  * 
  * @details Enqueues a response to be sent to the client when the socket is ready for writing.
  * The server needs to update the epoll registration to include EPOLLOUT when there are
  * pending responses.
  * 
  * @param response The response string to send to the client
  */
 void Connection::add_response(const std::string& response) {
     if (state_ == State::CONNECTED) {
         output_queue_.push_back(response);
         
         // Make sure the socket is registered for writing
         // Note: This would typically involve modifying epoll, will be handled by server
     }
 }
 
 /**
  * @brief Checks if the connection has timed out
  * 
  * @details Compares the elapsed time since the last activity with the provided timeout
  * value to determine if the connection should be considered timed out.
  * 
  * @param timeout_ms Timeout duration in milliseconds
  * @return true if the connection has timed out, false otherwise
  */
 bool Connection::check_timeout(std::chrono::milliseconds timeout_ms) {
     auto now = std::chrono::steady_clock::now();
     auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_activity_);
     
     return elapsed > timeout_ms;
 }
 
 /**
  * @brief Processes complete commands from the input buffer
  * 
  * @details Parses RESP-formatted commands from the input buffer, extracts arguments,
  * executes the commands via the server, and queues responses for sending. Implements
  * incremental parsing to handle partial commands and protocol errors.
  * 
  * @return true if command processing was successful, false on protocol error
  */
 bool Connection::process_commands() {
     // This is a simplified implementation. In a real implementation, you'd use
     // a RESP parser to extract commands from the input buffer.
     
     // For now, just check if we have a complete RESP array
     size_t pos = 0;
     while (pos < input_buffer_.size()) {
         // Check if this is the start of a RESP array
         if (input_buffer_[pos] == '*') {
             // Find the end of the command (CRLF sequence)
             size_t end_pos = input_buffer_.find("\r\n", pos);
             if (end_pos == std::string::npos) {
                 // Incomplete command, wait for more data
                 break;
             }
             
             // Parse array size
             int array_size = 0;
             try {
                 array_size = std::stoi(input_buffer_.substr(pos + 1, end_pos - pos - 1));
             } catch (const std::exception& e) {
                 std::cerr << "Error parsing RESP array size: " << e.what() << std::endl;
                 return false;
             }
             
             if (array_size <= 0) {
                 // Invalid array size
                 std::cerr << "Invalid RESP array size: " << array_size << std::endl;
                 return false;
             }
             
             // Parse array elements (bulk strings)
             std::vector<std::string> args;
             size_t current_pos = end_pos + 2; // Skip CRLF
             
             bool complete = true;
             for (int i = 0; i < array_size; i++) {
                 // Check if we have enough data
                 if (current_pos >= input_buffer_.size() || input_buffer_[current_pos] != '$') {
                     complete = false;
                     break;
                 }
                 
                 // Find bulk string length
                 size_t len_end = input_buffer_.find("\r\n", current_pos);
                 if (len_end == std::string::npos) {
                     complete = false;
                     break;
                 }
                 
                 // Parse bulk string length
                 int bulk_len = 0;
                 try {
                     bulk_len = std::stoi(input_buffer_.substr(current_pos + 1, len_end - current_pos - 1));
                 } catch (const std::exception& e) {
                     std::cerr << "Error parsing RESP bulk string length: " << e.what() << std::endl;
                     return false;
                 }
                 
                 if (bulk_len < 0) {
                     // Null bulk string
                     args.push_back("");
                     current_pos = len_end + 2; // Skip CRLF
                     continue;
                 }
                 
                 // Check if we have the complete bulk string
                 size_t data_start = len_end + 2; // Skip CRLF
                 size_t data_end = data_start + bulk_len;
                 
                 if (data_end + 2 > input_buffer_.size()) { // +2 for CRLF
                     complete = false;
                     break;
                 }
                 
                 // Extract bulk string data
                 args.push_back(input_buffer_.substr(data_start, bulk_len));
                 current_pos = data_end + 2; // Skip CRLF
             }
             
             if (!complete) {
                 // Incomplete command, wait for more data
                 break;
             }
             
             // Command is complete, process it
             if (!args.empty()) {
                 // Convert command to uppercase
                 std::string command = args[0];
                 std::transform(command.begin(), command.end(), command.begin(), ::toupper);
                 
                 // Execute command
                 std::vector<std::string> command_args(args.begin() + 1, args.end());
                 std::string response = server_->execute_command(command, command_args);
                 add_response(response);
             }
             
             // Remove processed command from input buffer
             input_buffer_.erase(0, current_pos);
             pos = 0; // Reset position for next command
         } else {
             // Not a RESP array, scan for next command
             pos++;
         }
     }
     
     return true;
 }
 
 /**
  * @brief Updates the last activity timestamp
  * 
  * @details Records the current time as the last activity time for this connection.
  * Used for tracking connection freshness and implementing timeouts.
  */
 void Connection::update_last_activity() {
     last_activity_ = std::chrono::steady_clock::now();
 }
 
 /**
  * @brief Resets the connection state
  * 
  * @details Clears the input buffer and output queue, and updates the last activity timestamp.
  * Used when reusing a connection or recovering from error states.
  */
 void Connection::reset() {
     input_buffer_.clear();
     output_queue_.clear();
     update_last_activity();
 }
 