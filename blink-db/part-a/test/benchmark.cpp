/**
 * @file benchmark.cpp
 * @brief Benchmark runner for BLINK DB Storage Engine (Part 1)
 */

 #include "StorageEngine.h"
 #include <iostream>
 #include <fstream>
 #include <string>
 #include <vector>
 #include <chrono>
 #include <algorithm>
 #include <numeric>
 #include <iomanip>
 
 // Track statistics for each operation type
 struct OpStats {
     std::vector<double> latencies;
     size_t count = 0;
     
     void add_latency(double latency_ms) {
         latencies.push_back(latency_ms);
         count++;
     }
     
     double avg_latency() const {
         if (latencies.empty()) return 0.0;
         return std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
     }
     
     double p95_latency() const {
         if (latencies.empty()) return 0.0;
         std::vector<double> sorted_latencies = latencies;
         std::sort(sorted_latencies.begin(), sorted_latencies.end());
         return sorted_latencies[static_cast<size_t>(latencies.size() * 0.95)];
     }
 };
 
 // Parse a benchmark file line into command and args
 std::vector<std::string> parse_line(const std::string& line) {
     std::vector<std::string> tokens;
     std::string token;
     bool in_quotes = false;
     char quote_char = 0;
     
     for (char c : line) {
         if ((c == '"' || c == '\'') && (!in_quotes || quote_char == c)) {
             in_quotes = !in_quotes;
             quote_char = c;
             continue;
         }
         
         if (c == ' ' && !in_quotes) {
             if (!token.empty()) {
                 tokens.push_back(token);
                 token.clear();
             }
         } else {
             token += c;
         }
     }
     
     if (!token.empty()) {
         tokens.push_back(token);
     }
     
     return tokens;
 }
 
 int main(int argc, char* argv[]) {
     if (argc != 2) {
         std::cerr << "Usage: " << argv[0] << " <benchmark_file>\n";
         return 1;
     }
     
     std::string benchmark_file = argv[1];
     std::ifstream file(benchmark_file);
     
     if (!file.is_open()) {
         std::cerr << "Error: Could not open file " << benchmark_file << std::endl;
         return 1;
     }
     
     StorageEngine engine;
     std::string line;
     
     // Track statistics by operation type
     OpStats set_stats, get_stats, del_stats;
     
     // Start timing the entire benchmark
     auto bench_start = std::chrono::high_resolution_clock::now();
     
     // Process each command
     while (std::getline(file, line)) {
         if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
         
         auto tokens = parse_line(line);
         if (tokens.empty()) continue;
         
         std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(), ::toupper);
         
         if (tokens[0] == "SET" && tokens.size() >= 3) {
             auto start = std::chrono::high_resolution_clock::now();
             
             // Handle TTL if specified
             std::chrono::seconds ttl = std::chrono::seconds::max();
             if (tokens.size() >= 5 && tokens[3] == "EX") {
                 ttl = std::chrono::seconds(std::stoi(tokens[4]));
             }
             
             engine.set(tokens[1], tokens[2], ttl);
             
             auto end = std::chrono::high_resolution_clock::now();
             double latency = std::chrono::duration<double, std::milli>(end - start).count();
             set_stats.add_latency(latency);
             
         } else if (tokens[0] == "GET" && tokens.size() >= 2) {
             auto start = std::chrono::high_resolution_clock::now();
             
             engine.get(tokens[1]); // Ignore the result for benchmarking
             
             auto end = std::chrono::high_resolution_clock::now();
             double latency = std::chrono::duration<double, std::milli>(end - start).count();
             get_stats.add_latency(latency);
             
         } else if (tokens[0] == "DEL" && tokens.size() >= 2) {
             auto start = std::chrono::high_resolution_clock::now();
             
             engine.del(tokens[1]); // Ignore the result for benchmarking
             
             auto end = std::chrono::high_resolution_clock::now();
             double latency = std::chrono::duration<double, std::milli>(end - start).count();
             del_stats.add_latency(latency);
         }
     }
     
     // Calculate total benchmark time
     auto bench_end = std::chrono::high_resolution_clock::now();
     double total_time_ms = std::chrono::duration<double, std::milli>(bench_end - bench_start).count();
     
     // Calculate total operations
     size_t total_ops = set_stats.count + get_stats.count + del_stats.count;
     
     // Print results
     std::cout << "======== BLINK DB BENCHMARK RESULTS ========\n";
     std::cout << "Benchmark file: " << benchmark_file << "\n";
     std::cout << "Total operations: " << total_ops << "\n";
     std::cout << "Total time: " << std::fixed << std::setprecision(2) << total_time_ms << " ms\n";
     std::cout << "Operations per second: " << std::fixed << std::setprecision(2) 
               << (total_ops * 1000.0 / total_time_ms) << " ops/sec\n\n";
     
     std::cout << "Operation breakdown:\n";
     std::cout << "SET: " << set_stats.count << " operations (" 
               << std::fixed << std::setprecision(1) << (100.0 * set_stats.count / total_ops) << "%)\n";
     std::cout << "GET: " << get_stats.count << " operations (" 
               << std::fixed << std::setprecision(1) << (100.0 * get_stats.count / total_ops) << "%)\n";
     std::cout << "DEL: " << del_stats.count << " operations (" 
               << std::fixed << std::setprecision(1) << (100.0 * del_stats.count / total_ops) << "%)\n\n";
     
     std::cout << "Latency statistics (ms):\n";
     std::cout << "                    Avg     P95\n";
     std::cout << "SET:          " << std::setw(8) << std::fixed << std::setprecision(3) << set_stats.avg_latency() 
               << " " << std::setw(8) << std::fixed << std::setprecision(3) << set_stats.p95_latency() << "\n";
     std::cout << "GET:          " << std::setw(8) << std::fixed << std::setprecision(3) << get_stats.avg_latency() 
               << " " << std::setw(8) << std::fixed << std::setprecision(3) << get_stats.p95_latency() << "\n";
     std::cout << "DEL:          " << std::setw(8) << std::fixed << std::setprecision(3) << del_stats.avg_latency() 
               << " " << std::setw(8) << std::fixed << std::setprecision(3) << del_stats.p95_latency() << "\n";
     
     return 0;
 }
 