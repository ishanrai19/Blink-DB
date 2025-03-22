/**
 * @file main.cpp
 * @brief Entry point for BLINK DB server (Part B)
 * 
 * @details Initializes and runs the BLINK DB server with the TCP layer
 * and RESP-2 protocol implementation. This file handles:
 * - Command-line argument parsing
 * - Signal handling for graceful shutdown
 * - Server initialization and execution
 * - Error reporting and usage information
 * 
 * The server implements a Redis-compatible key-value store that communicates
 * using the RESP-2 protocol and listens on port 9001 by default.
 */

 #include "server.h"
 #include <iostream>
 #include <csignal>
 #include <cstring>
 
 /** @brief Global server instance for signal handling */
 Server* g_server = nullptr;
 
 /**
  * @brief Signal handler for graceful shutdown
  * 
  * @details Handles SIGINT (Ctrl+C) and SIGTERM signals to ensure the
  * server shuts down gracefully, closing all connections and releasing
  * resources properly.
  * 
  * @param sig Signal number received from the operating system
  */
 void signal_handler(int sig) {
     std::cout << "Received signal " << sig << " (" << strsignal(sig) << "), shutting down..." << std::endl;
     if (g_server) {
         g_server->stop();
     }
 }
 
 /**
  * @brief Print usage information
  * 
  * @details Displays the command-line options and their descriptions
  * to help users understand how to configure the server.
  * 
  * @param prog_name Name of the program executable
  */
 void print_usage(const char* prog_name) {
     std::cout << "Usage: " << prog_name << " [options]" << std::endl;
     std::cout << "Options:" << std::endl;
     std::cout << "  -p, --port PORT     Server port (default: 9001)" << std::endl;
     std::cout << "  -c, --connections N Max connections (default: 1024)" << std::endl;
     std::cout << "  -h, --help          Show this help message" << std::endl;
 }
 
 /**
  * @brief Main function
  * 
  * @details Entry point for the BLINK DB server. Processes command-line arguments,
  * sets up signal handlers, initializes the server, and starts the main event loop.
  * The server continues running until stopped by a signal or fatal error.
  * 
  * Server configuration can be customized through command-line options including:
  * - Port number (-p, --port)
  * - Maximum concurrent connections (-c, --connections)
  * 
  * @param argc Number of command-line arguments
  * @param argv Array of command-line argument strings
  * @return 0 on successful execution and graceful shutdown, 1 on error
  */
 int main(int argc, char* argv[]) {
     // Default settings
     int port = 9001;
     int max_connections = 1024;
 
     // Parse command line arguments
     for (int i = 1; i < argc; i++) {
         std::string arg = argv[i];
         
         if (arg == "-p" || arg == "--port") {
             if (i + 1 < argc) {
                 try {
                     port = std::stoi(argv[++i]);
                 } catch (const std::exception& e) {
                     std::cerr << "Invalid port number" << std::endl;
                     return 1;
                 }
             } else {
                 std::cerr << "Port number required" << std::endl;
                 return 1;
             }
         } else if (arg == "-c" || arg == "--connections") {
             if (i + 1 < argc) {
                 try {
                     max_connections = std::stoi(argv[++i]);
                 } catch (const std::exception& e) {
                     std::cerr << "Invalid connection count" << std::endl;
                     return 1;
                 }
             } else {
                 std::cerr << "Connection count required" << std::endl;
                 return 1;
             }
         } else if (arg == "-h" || arg == "--help") {
             print_usage(argv[0]);
             return 0;
         } else {
             std::cerr << "Unknown option: " << arg << std::endl;
             print_usage(argv[0]);
             return 1;
         }
     }
 
     // Set up signal handlers for graceful shutdown
     std::signal(SIGINT, signal_handler);   // Ctrl+C
     std::signal(SIGTERM, signal_handler);  // kill
     
     std::cout << "BLINK DB Server v1.0" << std::endl;
     std::cout << "Starting server on port " << port << std::endl;
     std::cout << "Maximum connections: " << max_connections << std::endl;
     
     // Create and initialize server
     Server server(port, max_connections);
     g_server = &server;
     
     if (!server.init()) {
         std::cerr << "Failed to initialize server" << std::endl;
         return 1;
     }
     
     // Run the server (blocking call)
     server.run();
     
     std::cout << "Server shutdown complete" << std::endl;
     return 0;
 }
 