# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -pthread
LDFLAGS := -lpthread

# Directories
SRC_DIR := src
TEST_DIR := test
DOCS_DIR := docs
BUILD_DIR := build

# Source files
MAIN_SRCS := $(SRC_DIR)/main.cpp $(SRC_DIR)/StorageEngine.cpp
BENCHMARK_SRCS := $(TEST_DIR)/benchmark.cpp $(SRC_DIR)/StorageEngine.cpp
GEN_BENCH_SRCS := $(TEST_DIR)/generate_benchmark.cpp

# Executables
MAIN_EXEC := $(BUILD_DIR)/blink_db
BENCHMARK_EXEC := $(BUILD_DIR)/benchmark
GEN_BENCH_EXEC := $(BUILD_DIR)/generate_benchmark

# Include directories
INCLUDES := -I$(SRC_DIR)

# Default target
all: directories $(MAIN_EXEC) $(BENCHMARK_EXEC) $(GEN_BENCH_EXEC)

# Create build directory
directories:
	@mkdir -p $(BUILD_DIR)

# Main program
$(MAIN_EXEC): $(MAIN_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Benchmark program
$(BENCHMARK_EXEC): $(BENCHMARK_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Benchmark generator
$(GEN_BENCH_EXEC): $(GEN_BENCH_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Run the main program
run: $(MAIN_EXEC)
	./$(MAIN_EXEC)

# Generate benchmark files
gen_benchmarks: $(GEN_BENCH_EXEC)
	./$(GEN_BENCH_EXEC)

# Run benchmarks
run_benchmarks: all
	@mkdir -p results
	./build/generate_benchmark
	./build/benchmark read_heavy_large.txt > results/results_read_heavy.txt
	./build/benchmark balanced_large.txt > results/results_balanced.txt
	./build/benchmark write_heavy_large.txt > results/results_write_heavy.txt
	@echo "Benchmark results written to results/ directory"


# Generate documentation with Doxygen
# docs:
# 	doxygen Doxyfile

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
	rm -f results/*.txt
	rm -f *_large.txt

# PHONY targets
.PHONY: all run gen_benchmarks run_benchmarks clean directories
