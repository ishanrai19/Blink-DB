/**
 * @file client.cpp
 * @brief Implementation of BLINK DB client
 * 
 * @details This file contains the implementation of the Client class which
 * establishes a connection to the BLINK DB server, encodes and sends commands 
 * using the RESP-2 protocol, and decodes responses from the server.
 */

 #include "client.h"
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <iostream>
 #include <sstream>
 #include <cstring>
 #include <algorithm>
 #include <netdb.h>
 
 /** Maximum receive buffer size */
 const size_t MAX_BUFFER_SIZE = 65536; // 64KB
 
 /**
  * @brief Construct a new Client object
  * 
  * @param host Server hostname or IP address
  * @param port Server port number
  */
 Client::Client(const std::string& host, int port)
     : host_(host), port_(port), socket_fd_(-1) {
 }
 
 /**
  * @brief Destroy the Client object and close any open connection
  */
 Client::~Client() {
     disconnect();
 }
 
 /**
  * @brief Establish a connection to the BLINK DB server
  * 
  * @details Creates a TCP socket, resolves the server hostname, and connects to the server.
  * Sets a 5-second timeout for socket operations to prevent blocking indefinitely.
  * 
  * @return true if connection was successful, false otherwise
  */
 bool Client::connect() {
     // Create socket
     socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
     if (socket_fd_ < 0) {
         std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
         return false;
     }
     
     // Resolve hostname
     struct hostent* server = gethostbyname(host_.c_str());
     if (server == nullptr) {
         std::cerr << "Error resolving hostname: " << host_ << std::endl;
         close(socket_fd_);
         socket_fd_ = -1;
         return false;
     }
     
     // Prepare server address structure
     struct sockaddr_in server_addr;
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
     server_addr.sin_port = htons(port_);
     
     // Connect to server
     if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
         std::cerr << "Error connecting to server: " << strerror(errno) << std::endl;
         close(socket_fd_);
         socket_fd_ = -1;
         return false;
     }
 
     // Set socket timeout to prevent blocking indefinitely
     struct timeval tv;
     tv.tv_sec = 5;  // 5 second timeout
     tv.tv_usec = 0;
     if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
         std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
     }
     
     std::cout << "Connected to BLINK DB server at " << host_ << ":" << port_ << std::endl;
     return true;
 }
 
 /**
  * @brief Disconnect from the server
  * 
  * @details Closes the socket connection if it's open
  */
 void Client::disconnect() {
     if (socket_fd_ >= 0) {
         close(socket_fd_);
         socket_fd_ = -1;
     }
 }
 
 /**
  * @brief Check if client is connected to server
  * 
  * @return true if connected, false otherwise
  */
 bool Client::is_connected() const {
     return socket_fd_ >= 0;
 }
 
 /**
  * @brief Execute a command on the server
  * 
  * @details Encodes the command and arguments using RESP-2 protocol, sends it to
  * the server, receives the response, and decodes it to a human-readable format.
  * 
  * @param command Command to execute (e.g., "SET", "GET", "DEL")
  * @param args Vector of command arguments
  * @return Human-readable response string or error message
  */
 std::string Client::execute(const std::string& command, const std::vector<std::string>& args) {
     if (!is_connected()) {
         return "Error: Not connected to server";
     }
     
     std::string resp_command = encode_command(command, args);
     
     if (!send_data(resp_command)) {
         return "Error: Failed to send command to server";
     }
     
     std::string resp_response = receive_data();
     if (resp_response.empty()) {
         return "Error: No response from server";
     }
     
     return decode_response(resp_response);
 }
 
 /**
  * @brief Run an interactive client session
  * 
  * @details Prompts the user for commands, parses them, executes them on the server,
  * and displays the results. Continues until the user enters "exit" or "quit".
  * 
  * @param on_response Optional callback function to handle response display
  */
 void Client::run_interactive(std::function<void(const std::string&)> on_response) {
     if (!is_connected()) {
         std::cerr << "Error: Not connected to server. Call connect() first." << std::endl;
         return;
     }
     
     std::cout << "BLINK DB client (Type 'exit' or 'quit' to exit)" << std::endl;
     std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
     
     std::string line;
     while (true) {
         std::cout << "blink> ";
         std::getline(std::cin, line);
         
         // Trim whitespace
         line.erase(0, line.find_first_not_of(" \t"));
         line.erase(line.find_last_not_of(" \t") + 1);
         
         if (line.empty()) {
             continue;
         }
         
         if (line == "exit" || line == "quit" || 
            line == "EXIT" || line == "QUIT") {
            std::cout << "Exiting client..." << std::endl;
            break;
        }
        
         std::string command;
         std::vector<std::string> args;
         
         if (!parse_command_line(line, command, args)) {
             std::cout << "Error: Invalid command format" << std::endl;
             continue;
         }
         
         std::string response = execute(command, args);
         
         if (on_response) {
             on_response(response);
         } else {
             std::cout << response << std::endl;
         }
     }
 }
 
 /**
  * @brief Encode a command and arguments in RESP-2 protocol format
  * 
  * @details Formats the command as a RESP array with command and arguments
  * as bulk strings.
  * 
  * @param command Command name (e.g., "SET", "GET", "DEL")
  * @param args Command arguments
  * @return RESP-2 encoded command string
  */
 std::string Client::encode_command(const std::string& command, const std::vector<std::string>& args) {
     // Format as RESP array: *<number of elements>\r\n
     std::string resp = "*" + std::to_string(args.size() + 1) + "\r\n";
     
     // Add command as bulk string
     resp += "$" + std::to_string(command.length()) + "\r\n" + command + "\r\n";
     
     // Add each argument as bulk string
     for (const auto& arg : args) {
         resp += "$" + std::to_string(arg.length()) + "\r\n" + arg + "\r\n";
     }
     
     return resp;
 }
 
 /**
  * @brief Decode a RESP-2 protocol response to human-readable format
  * 
  * @details Parses the RESP response based on its data type and converts
  * it to a human-readable string. Handles all five RESP data types.
  * 
  * @param resp_data RESP-encoded response from server
  * @return Human-readable response string
  */
 std::string Client::decode_response(const std::string& resp_data) {
     if (resp_data.empty()) {
         return "Error: Empty response";
     }
     
     char type = resp_data[0];
     std::string data = resp_data.substr(1);
     size_t line_end = data.find("\r\n");
     
     if (line_end == std::string::npos) {
         return "Error: Invalid RESP format";
     }
     
     std::string first_line = data.substr(0, line_end);
     
     switch (type) {
         case '+': // Simple String
             return first_line;
             
         case '-': // Error
             return "Error: " + first_line;
             
         case ':': // Integer
             return first_line;
             
         case '$': { // Bulk String
             int len = std::stoi(first_line);
             if (len == -1) {
                 return "NULL"; // Null bulk string
             }
             
             // Extract the actual string
             return data.substr(line_end + 2, len);
         }
             
         case '*': { // Array
             int count = std::stoi(first_line);
             if (count == -1) {
                 return "NULL"; // Null array
             }
             
             // This is a simplified array decoder for basic use cases
             // In a real implementation, you would recursively decode each element
             return "(Array with " + std::to_string(count) + " elements)";
         }
             
         default:
             return "Error: Unknown RESP type: " + std::string(1, type);
     }
 }
 
 /**
  * @brief Send data to the server
  * 
  * @details Handles partial sends to ensure all data is transmitted.
  * 
  * @param data Data to send
  * @return true if all data was sent successfully, false otherwise
  */
 bool Client::send_data(const std::string& data) {
     if (!is_connected()) {
         return false;
     }
     
     ssize_t total_sent = 0;
     ssize_t data_size = data.size();
     
     while (total_sent < data_size) {
         ssize_t sent = send(socket_fd_, data.c_str() + total_sent, data_size - total_sent, 0);
         
         if (sent < 0) {
             std::cerr << "Error sending data: " << strerror(errno) << std::endl;
             return false;
         }
         
         total_sent += sent;
     }
     
     return true;
 }
 
 /**
  * @brief Receive data from the server
  * 
  * @details Reads data from the socket with timeout protection.
  * 
  * @return Received data or empty string on error
  */
 std::string Client::receive_data() {
     if (!is_connected()) {
         return "";
     }
     
     char buffer[MAX_BUFFER_SIZE];
     ssize_t received = recv(socket_fd_, buffer, MAX_BUFFER_SIZE - 1, 0);
     
     if (received < 0) {
         std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
         return "";
     }
     
     if (received == 0) {
         // Connection closed by server
         disconnect();
         return "";
     }
     
     buffer[received] = '\0';
     return std::string(buffer, received);
 }
 
 /**
  * @brief Parse a command line into command and arguments
  * 
  * @details Extracts the command name and arguments from user input.
  * Handles quoted arguments and converts the command to uppercase.
  * 
  * @param command_line Full command line from user
  * @param[out] command Command name (will be converted to uppercase)
  * @param[out] args Vector to store command arguments
  * @return true if parsing was successful, false otherwise
  */
 bool Client::parse_command_line(const std::string& command_line, std::string& command, std::vector<std::string>& args) {
     std::istringstream iss(command_line);
     
     // Get the command (first word)
     iss >> command;
     if (command.empty()) {
         return false;
     }
     
     // Convert command to uppercase
     std::transform(command.begin(), command.end(), command.begin(), ::toupper);
     
     args.clear();
     std::string arg;
     
     // Simple approach - treat everything after command as individual arguments
     while (iss >> arg) {
         // Remove quotes if present but don't require them
         if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"') {
             arg = arg.substr(1, arg.size() - 2);
         }
         args.push_back(arg);
     }
     
     return true;
 }
 