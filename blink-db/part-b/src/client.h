/**
 * @file client.h
 * @brief Simple client for BLINK DB that communicates using RESP-2 protocol
 */

 #pragma once

 #include <string>
 #include <vector>
 #include <functional>
 
 /**
  * @class Client
  * @brief Client for BLINK DB server using RESP-2 protocol
  */
 class Client {
 public:
     /**
      * @brief Construct a new Client object
      * @param host Server hostname or IP
      * @param port Server port
      */
     Client(const std::string& host = "localhost", int port = 9001);
     
     /**
      * @brief Destroy the Client object
      */
     ~Client();
     
     /**
      * @brief Connect to the BLINK DB server
      * @return true if connection successful, false otherwise
      */
     bool connect();
     
     /**
      * @brief Disconnect from the server
      */
     void disconnect();
     
     /**
      * @brief Check if client is connected
      * @return true if connected, false otherwise
      */
     bool is_connected() const;
     
     /**
      * @brief Execute a command
      * @param command Command to execute (e.g., "SET", "GET", "DEL")
      * @param args Command arguments
      * @return Human-readable response from the server
      */
     std::string execute(const std::string& command, const std::vector<std::string>& args = {});
     
     /**
      * @brief Run interactive mode
      * @param on_response Callback for displaying responses
      */
     void run_interactive(std::function<void(const std::string&)> on_response = nullptr);
 
 private:
     std::string host_;
     int port_;
     int socket_fd_;
     
     /**
      * @brief Encode a command and arguments into RESP format
      * @param command Command name
      * @param args Command arguments
      * @return RESP encoded command
      */
     std::string encode_command(const std::string& command, const std::vector<std::string>& args);
     
     /**
      * @brief Decode a RESP response into human-readable format
      * @param resp_data RESP encoded response
      * @return Human-readable response
      */
     std::string decode_response(const std::string& resp_data);
     
     /**
      * @brief Send data to the server
      * @param data Data to send
      * @return true if successful, false otherwise
      */
     bool send_data(const std::string& data);
     
     /**
      * @brief Receive data from the server
      * @return Received data
      */
     std::string receive_data();
     
     /**
      * @brief Parse a command line into command and arguments
      * @param command_line Full command line
      * @param[out] command Command name
      * @param[out] args Command arguments
      * @return true if parsing successful, false otherwise
      */
     bool parse_command_line(const std::string& command_line, std::string& command, std::vector<std::string>& args);
 };
 
