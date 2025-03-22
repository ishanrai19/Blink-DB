/**
 * @file generate_benchmarks.cpp
 * @brief Generate large benchmark files for BLINK DB
 */

 #include <iostream>
 #include <fstream>
 #include <random>
 #include <string>
 #include <vector>
 
 // Generate a benchmark file with specified operation percentages
 void generate_benchmark(const std::string& filename, 
                         int get_percent, int set_percent, int del_percent,
                         int num_operations) {
     std::ofstream file(filename);
     if (!file.is_open()) {
         std::cerr << "Failed to open file: " << filename << std::endl;
         return;
     }
     
     std::random_device rd;
     std::mt19937 gen(rd());
     std::uniform_int_distribution<> op_dist(1, 100);
     std::uniform_int_distribution<> key_dist(1, 1000);
     std::uniform_int_distribution<> value_len_dist(5, 50);
     
     // Character set for random value generation
     const std::string chars = 
         "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ";
     std::uniform_int_distribution<> char_dist(0, chars.size() - 1);
     
     // First create 100 keys to ensure we have data to get/delete
     for (int i = 1; i <= 100; i++) {
         int value_len = value_len_dist(gen);
         std::string value;
         for (int j = 0; j < value_len; j++) {
             value += chars[char_dist(gen)];
         }
         file << "SET key" << i << " \"" << value << "\"\n";
     }
     
     // Generate random operations
     for (int i = 0; i < num_operations; i++) {
         int op = op_dist(gen);
         int key = key_dist(gen);
         
         if (op <= get_percent) {
             // GET operation
             file << "GET key" << key << "\n";
         } else if (op <= get_percent + set_percent) {
             // SET operation
             int value_len = value_len_dist(gen);
             std::string value;
             for (int j = 0; j < value_len; j++) {
                 value += chars[char_dist(gen)];
             }
             file << "SET key" << key << " \"" << value << "\"\n";
         } else {
             // DEL operation
             file << "DEL key" << key << "\n";
         }
     }
     
     file.close();
     std::cout << "Generated " << filename << " with " << num_operations + 100 << " operations" << std::endl;
 }
 
 int main() {
     // Generate 10000 operations for each workload type
     // Read-heavy: 75% GET, 20% SET, 5% DEL
     generate_benchmark("read_heavy_large.txt", 75, 20, 5, 100000);
     
     // Balanced: 40% GET, 40% SET, 20% DEL
     generate_benchmark("balanced_large.txt", 40, 40, 20, 100000);
     
     // Write-heavy: 20% GET, 70% SET, 10% DEL
     generate_benchmark("write_heavy_large.txt", 20, 70, 10, 100000);
     
     return 0;
 }
 