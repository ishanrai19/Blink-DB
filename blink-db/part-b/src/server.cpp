/**
 * @file server.cpp
 * @brief Implementation of TCP server with epoll() for efficient I/O multiplexing
 * 
 * @details This file implements a high-performance TCP server using the epoll() mechanism
 * to handle multiple concurrent client connections efficiently. It integrates with the 
 * Storage Engine from Part A and uses the RESP-2 protocol for client communication.
 */

 #include "server.h"
 #include "connection.h"
 #include "resp.h"
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <iostream>
 #include <cstring>
 #include <errno.h>
 
 /**
  * @brief Constructs a new Server instance
  * 
  * @details Initializes server parameters but does not start listening until init() is called
  * 
  * @param port Port number to listen on (default: 9001)
  * @param max_connections Maximum number of concurrent connections allowed
  */
 Server::Server(int port, int max_connections)
     : port_(port),
       listen_fd_(-1),
       epoll_fd_(-1),
       max_connections_(max_connections),
       running_(false)
 {
 }
 
 /**
  * @brief Destroys the Server instance
  * 
  * @details Ensures proper cleanup by calling stop()
  */
 Server::~Server()
 {
     stop();
 }
 
 /**
  * @brief Initializes the server
  * 
  * @details Sets up the listening socket, binds to the specified port, initializes epoll,
  * and registers command handlers for SET, GET, and DEL operations.
  * 
  * @return true if initialization was successful, false otherwise
  */
 bool Server::init()
 {
     // Create listening socket
     listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
     if (listen_fd_ < 0)
     {
         std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
         return false;
     }
 
     // Set socket options
     int opt = 1;
     if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
     {
         std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
         close(listen_fd_);
         return false;
     }
 
     // Set non-blocking mode
     if (!set_nonblocking(listen_fd_))
     {
         close(listen_fd_);
         return false;
     }
 
     // Bind socket to port
     struct sockaddr_in server_addr;
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = INADDR_ANY;
     server_addr.sin_port = htons(port_);
 
     if (bind(listen_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
     {
         std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
         close(listen_fd_);
         return false;
     }
 
     // Start listening
     if (listen(listen_fd_, SOMAXCONN) < 0)
     {
         std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
         close(listen_fd_);
         return false;
     }
 
     // Create epoll instance
     epoll_fd_ = epoll_create1(0);
     if (epoll_fd_ < 0)
     {
         std::cerr << "Failed to create epoll instance: " << strerror(errno) << std::endl;
         close(listen_fd_);
         return false;
     }
 
     // Add listening socket to epoll
     struct epoll_event ev;
     memset(&ev, 0, sizeof(ev));
     ev.events = EPOLLIN;
     ev.data.fd = listen_fd_;
     if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev) < 0)
     {
         std::cerr << "Failed to add listening socket to epoll: " << strerror(errno) << std::endl;
         close(epoll_fd_);
         close(listen_fd_);
         return false;
     }
 
     // Register SET command handler
     register_command("SET", [this](const std::vector<std::string> &args) -> std::string
                      {
         if (args.size() < 2) return "-ERR wrong number of arguments for 'set' command\r\n";
         
         // Check for TTL (EX) parameter
         std::chrono::seconds ttl = std::chrono::seconds::max();
         if (args.size() >= 4 && args[2] == "EX") {
             try {
                 ttl = std::chrono::seconds(std::stoi(args[3]));
             } catch (const std::exception& e) {
                 return "-ERR invalid expire time in 'set' command\r\n";
             }
         }
         
         storage_engine_.set(args[0], args[1], ttl);
         return "+OK\r\n"; });
 
     // Register GET command handler
     register_command("GET", [this](const std::vector<std::string> &args) -> std::string
                      {
         if (args.size() != 1) return "-ERR wrong number of arguments for 'get' command\r\n";
         
         std::string value = storage_engine_.get(args[0]);
         if (value.empty()) {
             return "$-1\r\n"; // NULL bulk string
         } else {
             return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
         } });
 
     // Register DEL command handler
     register_command("DEL", [this](const std::vector<std::string> &args) -> std::string
                      {
         if (args.size() != 1) return "-ERR wrong number of arguments for 'del' command\r\n";
         
         bool success = storage_engine_.del(args[0]);
         return ":" + std::to_string(success ? 1 : 0) + "\r\n"; });
 
     std::cout << "Server initialized on port " << port_ << std::endl;
     return true;
 }
 
 /**
  * @brief Runs the server main event loop
  * 
  * @details Continuously monitors for events using epoll, accepting new connections
  * and processing I/O for existing connections. This method blocks until stop() is called.
  */
 void Server::run()
 {
     if (listen_fd_ < 0 || epoll_fd_ < 0)
     {
         std::cerr << "Server not initialized" << std::endl;
         return;
     }
 
     running_ = true;
     const int MAX_EVENTS = 64;
     struct epoll_event events[MAX_EVENTS];
 
     std::cout << "Server running, waiting for connections..." << std::endl;
 
     while (running_)
     {
         int num_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
 
         if (num_events < 0)
         {
             if (errno == EINTR)
                 continue; // Interrupted by signal
             std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
             break;
         }
 
         for (int i = 0; i < num_events; i++)
         {
             int fd = events[i].data.fd;
             uint32_t event_flags = events[i].events;
 
             if (fd == listen_fd_)
             {
                 // New connection
                 accept_connection();
             }
             else
             {
                 // Existing connection event
                 handle_event(fd, event_flags);
             }
         }
     }
 
     std::cout << "Server stopped" << std::endl;
 }
 
 /**
  * @brief Stops the server gracefully
  * 
  * @details Closes all client connections, cleans up resources, and terminates the event loop
  */
 void Server::stop()
 {
     running_ = false;
 
     // Close all connections
     for (auto &pair : connections_)
     {
         close_connection(pair.first);
     }
     connections_.clear();
 
     // Close server sockets
     if (epoll_fd_ >= 0)
     {
         close(epoll_fd_);
         epoll_fd_ = -1;
     }
 
     if (listen_fd_ >= 0)
     {
         close(listen_fd_);
         listen_fd_ = -1;
     }
 }
 
 /**
  * @brief Registers a command handler function
  * 
  * @details Associates a command name with a function that will process it
  * 
  * @param command Command name (e.g., "SET", "GET", "DEL")
  * @param handler Function to handle the command
  */
 void Server::register_command(const std::string &command, CommandHandler handler)
 {
     command_handlers_[command] = std::move(handler);
 }
 
 /**
  * @brief Sets a socket to non-blocking mode
  * 
  * @details Uses fcntl to modify the file descriptor flags
  * 
  * @param fd Socket file descriptor to modify
  * @return true if successful, false on error
  */
 bool Server::set_nonblocking(int fd)
 {
     int flags = fcntl(fd, F_GETFL, 0);
     if (flags < 0)
     {
         std::cerr << "fcntl(F_GETFL) failed: " << strerror(errno) << std::endl;
         return false;
     }
 
     if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
     {
         std::cerr << "fcntl(F_SETFL) failed: " << strerror(errno) << std::endl;
         return false;
     }
 
     return true;
 }
 
 /**
  * @brief Accepts a new client connection
  * 
  * @details Accepts a connection from the listening socket, creates a Connection
  * object, and registers it with epoll for event monitoring
  * 
  * @return true if connection was accepted successfully, false otherwise
  */
 bool Server::accept_connection()
 {
     struct sockaddr_in client_addr;
     socklen_t client_len = sizeof(client_addr);
 
     int client_fd = accept(listen_fd_, (struct sockaddr *)&client_addr, &client_len);
     if (client_fd < 0)
     {
         if (errno == EAGAIN || errno == EWOULDBLOCK)
         {
             // No more connections to accept
             return true;
         }
         std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
         return false;
     }
 
     // Check if we've reached max connections
     if (connections_.size() >= static_cast<size_t>(max_connections_))
     {
         std::cerr << "Maximum connections reached, rejecting connection" << std::endl;
         close(client_fd);
         return false;
     }
 
     // Set non-blocking mode
     if (!set_nonblocking(client_fd))
     {
         close(client_fd);
         return false;
     }
 
     // Create a new connection object
     auto conn = new Connection(client_fd, this);
     connections_[client_fd] = conn;
 
     // Add to epoll
     struct epoll_event ev;
     memset(&ev, 0, sizeof(ev));
     ev.events = EPOLLIN | EPOLLET; // Edge-triggered mode
     ev.data.fd = client_fd;
     if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0)
     {
         std::cerr << "Failed to add client socket to epoll: " << strerror(errno) << std::endl;
         close_connection(client_fd);
         return false;
     }
 
     std::cout << "New connection accepted, fd: " << client_fd << std::endl;
     return true;
 }
 
 /**
  * @brief Handles events for an existing connection
  * 
  * @details Processes read/write events and error conditions for a client connection.
  * Updates epoll registration based on whether data is pending to be written.
  * 
  * @param fd File descriptor that triggered the event
  * @param events Event flags from epoll_wait
  */
 void Server::handle_event(int fd, uint32_t events)
 {
     auto it = connections_.find(fd);
     if (it == connections_.end())
     {
         // Unknown connection, close it
         epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
         close(fd);
         return;
     }
 
     Connection *conn = it->second;
 
     if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
     {
         // Error or connection closed by client
         std::cout << "Connection closed or error, fd: " << fd << std::endl;
         close_connection(fd);
         return;
     }
 
     if (events & EPOLLIN)
     {
         // Data available to read
         if (!conn->handle_read())
         {
             close_connection(fd);
             return;
         }
 
         // Check if the connection now has data to write
         if (conn->has_pending_writes())
         {
             // Update epoll to watch for write events too
             struct epoll_event ev;
             memset(&ev, 0, sizeof(ev));
             ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
             ev.data.fd = fd;
             if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0)
             {
                 std::cerr << "Failed to modify epoll events: " << strerror(errno) << std::endl;
             }
         }
     }
 
     if (events & EPOLLOUT)
     {
         // Socket ready for writing
         if (!conn->handle_write())
         {
             close_connection(fd);
             return;
         }
 
         // If no more data to write, update epoll to stop watching for write events
         if (!conn->has_pending_writes())
         {
             struct epoll_event ev;
             memset(&ev, 0, sizeof(ev));
             ev.events = EPOLLIN | EPOLLET;
             ev.data.fd = fd;
             if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0)
             {
                 std::cerr << "Failed to modify epoll events: " << strerror(errno) << std::endl;
             }
         }
     }
 }
 
 /**
  * @brief Closes a client connection
  * 
  * @details Removes the connection from epoll monitoring, deletes the Connection object,
  * and closes the socket.
  * 
  * @param fd File descriptor of the connection to close
  */
 void Server::close_connection(int fd)
 {
     auto it = connections_.find(fd);
     if (it != connections_.end())
     {
         epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
         delete it->second;
         connections_.erase(it);
     }
     close(fd);
 }
 
 /**
  * @brief Executes a command and returns the response
  * 
  * @details Looks up the appropriate handler for the command and executes it,
  * returning the RESP-formatted response or an error message.
  * 
  * @param command Command name (e.g., "SET", "GET", "DEL")
  * @param args Vector of command arguments
  * @return RESP-formatted response string
  */
 std::string Server::execute_command(const std::string &command, const std::vector<std::string> &args)
 {
     auto it = command_handlers_.find(command);
     if (it == command_handlers_.end())
     {
         return "-ERR unknown command '" + command + "'\r\n";
     }
 
     try
     {
         return it->second(args);
     }
     catch (const std::exception &e)
     {
         return "-ERR internal error: " + std::string(e.what()) + "\r\n";
     }
 }
 