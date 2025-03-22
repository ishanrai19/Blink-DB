/**
 * @file main.cpp
 * @brief Entry point for BLINK DB Storage Engine (Part 1)
 * 
 * @details Implements the REPL interface for interacting with the key-value store.
 * Demonstrates the core functionality of SET, GET, and DEL operations.
 */

 #include "StorageEngine.h"
 #include "REPL.h"
 #include <iostream>
 
 /**
  * @brief Main function for BLINK DB Storage Engine
  * 
  * @details Initializes the storage engine and REPL interface, then enters the
  * command processing loop. Handles user input for database operations until
  * explicit exit command.
  * 
  * @return int Exit status (0 for normal termination)
  */
 int main() {
     // Initialize storage engine with default 1GB memory limit
     StorageEngine engine;
     
     // Create REPL interface connected to the engine
     REPL repl(engine);
     
     // Display welcome message and command help
     std::cout << "BLINK DB Storage Engine v1.0\n";
     std::cout << "Supported commands:\n";
     std::cout << "  SET <key> \"<value>\" [EX <seconds>]\n";
     std::cout << "  GET <key>\n";
     std::cout << "  DEL <key>\n";
     std::cout << "  QUIT|EXIT\n\n";
     
     // Main REPL loop
     while(true) {
         std::cout << "User> ";
         std::string input;
         std::getline(std::cin, input);
         
         // Handle empty input
         if(input.empty()) continue;
         
         // Exit condition
         if(input == "quit" || input == "exit") break;
         
         // Process valid commands
         repl.process_command(input);
     }
     
     return 0;
 }
 