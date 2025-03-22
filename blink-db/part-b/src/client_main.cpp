/**
 * @file client_main.cpp
 * @brief Entry point for BLINK DB client
 * 
 * @details This file implements the main entry point for the BLINK DB client application,
 * handling command-line argument parsing, server connection, and interactive command processing.
 */

 #include "client.h"
 #include <iostream>
 #include <string>
 
 /**
  * @brief Main entry point for the BLINK DB client application
  * 
  * Initializes the client, connects to the specified server, and runs
  * the interactive client interface for sending commands.
  * 
  * @param argc Number of command-line arguments
  * @param argv Array of command-line arguments
  * @return 0 on successful execution, 1 on error
  */
 int main(int argc, char* argv[]) {
     // Default server settings
     std::string host = "localhost";
     int port = 9001;
     
     // Parse command-line arguments
     for (int i = 1; i < argc; i++) {
         std::string arg = argv[i];
         if (arg == "-h" || arg == "--host") {
             if (i + 1 < argc) {
                 host = argv[++i];
             }
         } else if (arg == "-p" || arg == "--port") {
             if (i + 1 < argc) {
                 try {
                     port = std::stoi(argv[++i]);
                 } catch (const std::exception& e) {
                     std::cerr << "Invalid port number" << std::endl;
                     return 1;
                 }
             }
         } else if (arg == "--help") {
             std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
             std::cout << "Options:" << std::endl;
             std::cout << "  -h, --host HOST   Server hostname or IP (default: localhost)" << std::endl;
             std::cout << "  -p, --port PORT   Server port (default: 9001)" << std::endl;
             std::cout << "  --help            Display this help message" << std::endl;
             return 0;
         }
     }
     
     // Create client and connect to server
     Client client(host, port);
     if (!client.connect()) {
         std::cerr << "Failed to connect to " << host << ":" << port << std::endl;
         return 1;
     }
     
     // Run in interactive mode
     client.run_interactive();
     
     return 0;
 }
 