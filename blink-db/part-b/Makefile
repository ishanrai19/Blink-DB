# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -pthread
LDFLAGS := -lpthread

# Directories
SRC_DIR := src
BUILD_DIR := build
PARTA_DIR := ../part-a

# Source files
SERVER_SRCS := $(SRC_DIR)/server.cpp $(SRC_DIR)/connection.cpp $(SRC_DIR)/resp.cpp $(SRC_DIR)/main.cpp $(PARTA_DIR)/src/StorageEngine.cpp
CLIENT_SRCS := $(SRC_DIR)/client_main.cpp $(SRC_DIR)/client.cpp $(SRC_DIR)/resp.cpp

# Object files
SERVER_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(SERVER_SRCS)))
CLIENT_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(CLIENT_SRCS)))

# Include directories
INCLUDES := -I$(SRC_DIR) -I$(PARTA_DIR)/src

# Executables
SERVER_EXEC := $(BUILD_DIR)/blink_server
CLIENT_EXEC := $(BUILD_DIR)/blink_client

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Server target
$(SERVER_EXEC): $(SERVER_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Client target
$(CLIENT_EXEC): $(CLIENT_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Rule for server object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Rule for storage engine object file
$(BUILD_DIR)/StorageEngine.o: $(PARTA_DIR)/src/StorageEngine.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean target
clean:
	rm -rf $(BUILD_DIR)

# PHONY targets
.PHONY: all clean

# # Run targets
# run-server: $(SERVER_EXEC)
# 	./$(SERVER_EXEC)

# run-client: $(CLIENT_EXEC)
# 	./$(CLIENT_EXEC)
# Run targets
run: $(SERVER_EXEC) $(CLIENT_EXEC)
	@echo "Running server and client in separate terminals..."
	gnome-terminal -- ./$(SERVER_EXEC) &
	gnome-terminal -- ./$(CLIENT_EXEC) &

run-server: $(SERVER_EXEC)
	./$(SERVER_EXEC)

run-client: $(CLIENT_EXEC)
	./$(CLIENT_EXEC)

.PHONY: run run-server run-client

# Benchmark target
benchmark: $(SERVER_EXEC)
	@mkdir -p result
	@echo "Running benchmarks (this will take some time)..."
	
	@echo "Benchmark 1: 10,000 requests, 10 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 10000 -c 10 -t set,get > result/result_10000_10.txt
	
	@echo "Benchmark 2: 10,000 requests, 100 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 10000 -c 100 -t set,get > result/result_10000_100.txt
	
	@echo "Benchmark 3: 10,000 requests, 1000 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 10000 -c 1000 -t set,get > result/result_10000_1000.txt
	
	@echo "Benchmark 4: 100,000 requests, 10 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 100000 -c 10 -t set,get > result/result_100000_10.txt
	
	@echo "Benchmark 5: 100,000 requests, 100 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 100000 -c 100 -t set,get > result/result_100000_100.txt
	
	@echo "Benchmark 6: 100,000 requests, 1000 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 100000 -c 1000 -t set,get > result/result_100000_1000.txt
	
	@echo "Benchmark 7: 1,000,000 requests, 10 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 1000000 -c 10 -t set,get > result/result_1000000_10.txt
	
	@echo "Benchmark 8: 1,000,000 requests, 100 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 1000000 -c 100 -t set,get > result/result_1000000_100.txt
	
	@echo "Benchmark 9: 1,000,000 requests, 1000 connections"
	redis-benchmark -h 127.0.0.1 -p 9001 -n 1000000 -c 1000 -t set,get > result/result_1000000_1000.txt
	
	@echo "All benchmarks completed. Results are in the 'result' directory."
