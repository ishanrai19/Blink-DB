/**
 * @file REPL.h
 * @brief Read-Eval-Print Loop interface for BLINK DB (Part 1)
 * 
 * @details Implements the command-line interface for interacting with the storage engine.
 * Separates user input handling from core database logic as per project requirements.
 */

 #pragma once
 #include "StorageEngine.h"
 #include <iostream>
 #include <sstream>
 #include <vector>
 #include <algorithm>
 
 /**
  * @class REPL
  * @brief Command processor for BLINK DB storage engine
  * 
  * @details Handles parsing and execution of user commands according to the specification:
  * - SET <key> "<value>" [EX <seconds>]
  * - GET <key>
  * - DEL <key>
  * 
  * Maintains strict separation from StorageEngine implementation (Note 2 compliance)
  */
 class REPL {
     StorageEngine& engine; ///< Reference to the underlying storage engine
     
     /**
      * @brief Tokenize user input into command components
      * @param input Raw user input string
      * @return std::vector<std::string> Parsed tokens
      * 
      * @details Handles quoted values with spaces using stateful parsing:
      * - Supports both single (') and double (") quotes
      * - Preserves whitespace inside quotes
      * - Example: "value with spaces" becomes single token
      */
     std::vector<std::string> tokenize(const std::string& input) {
         std::vector<std::string> tokens;
         std::istringstream ss(input);
         std::string token;
         bool in_quotes = false;
         char quote_char = 0;
         
         while(ss >> std::ws) {
             if(ss.peek() == '"' || ss.peek() == '\'') {
                 in_quotes = !in_quotes;
                 quote_char = ss.get();
                 std::getline(ss, token, quote_char);
                 if(!token.empty()) {
                     tokens.push_back(token);
                 }
                 continue;
             }
             
             if(in_quotes) {
                 std::getline(ss, token, quote_char);
                 in_quotes = false;
                 if(!token.empty()) tokens.push_back(token);
             }
             else {
                 ss >> token;
                 if(!token.empty()) tokens.push_back(token);
             }
         }
         return tokens;
     }
 
 public:
     /**
      * @brief Construct a new REPL interface
      * @param engine Reference to StorageEngine instance
      */
     REPL(StorageEngine& engine) : engine(engine) {}
 
     /**
      * @brief Process a single user command
      * @param input Raw command string from user
      * 
      * @details Implements full command processing workflow:
      * 1. Tokenization of input
      * 2. Command validation
      * 3. Execution via StorageEngine
      * 4. Error handling and output formatting
      * 
      * @throws std::invalid_argument For invalid TTL values
      * @throws std::exception For general processing errors
      */
     void process_command(const std::string& input) {
         auto tokens = tokenize(input);
         if(tokens.empty()) return;
 
         try {
             std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(), ::toupper);
             
             if(tokens[0] == "SET" && tokens.size() >= 3) {
                 // Handle TTL if specified
                 std::chrono::seconds ttl = std::chrono::seconds::max();
                 if(tokens.size() >= 5 && tokens[3] == "EX") {
                     ttl = std::chrono::seconds(std::stoi(tokens[4]));
                 }
                 
                 engine.set(tokens[1], tokens[2], ttl);
                //  std::cout << "OK" << std::endl;
             }
             else if(tokens[0] == "GET" && tokens.size() >= 2) {
                 auto value = engine.get(tokens[1]);
                 std::cout << (value.empty() ? "NULL" : value) << std::endl;
             }
             else if(tokens[0] == "DEL" && tokens.size() >= 2) {
                 bool deleted = engine.del(tokens[1]);
                 if(!deleted) std::cout << "Does not exist.\n";
             }
             else {
                 std::cout << "ERROR: Invalid command format" << std::endl;
             }
         }
         catch(const std::invalid_argument& e) {
             std::cout << "ERROR: Invalid numeric argument" << std::endl;
         }
         catch(const std::exception& e) {
             std::cout << "ERROR: " << e.what() << std::endl;
         }
     }
 };
 