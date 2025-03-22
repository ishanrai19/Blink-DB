/**
 * @file StorageEngine.h
 * @brief Core header for BLINK DB storage engine implementation
 */

 #pragma once
 #include <string>
 #include <mutex>
 #include <chrono>
 #include <thread>
 #include <atomic>
 #include <vector>
 #include "HashTable.h"
 #include "MemoryManager.h"
 
 /**
  * @class StorageEngine
  * @brief In-memory key-value store with LRU eviction and TTL expiration
  * 
  * @details Implements CRUD operations using custom HashTable and MemoryManager.
  * Designed for Part 1 of DESIGN_LAB_PROJECT.pdf specifications with:
  * - O(1) average case performance
  * - Thread-safe operations
  * - Background TTL eviction thread
  */
 class StorageEngine {
 private:
     /**
      * @struct Entry
      * @brief Metadata for stored values
      */
     struct Entry {
         std::string value; ///< User-supplied data
         std::chrono::seconds ttl; ///< Time-to-live (default: max seconds)
         std::chrono::system_clock::time_point last_accessed; ///< LRU tracking timestamp
     };
 
     HashTable<std::string, Entry> store; ///< Custom hash table for core storage
     MemoryManager mem_manager; ///< LRU eviction policy manager
     size_t current_memory = 0; ///< Current memory usage in bytes
     size_t max_memory; ///< Maximum allowed memory (1GB default)
     std::mutex mtx; ///< Mutex for thread safety
     std::thread eviction_thread; ///< Background TTL eviction thread
     std::atomic<bool> running{true}; ///< Control flag for eviction thread
 
     /**
      * @brief Start background eviction daemon
      * @details Runs evict_expired() every second in separate thread
      */
     void start_eviction_daemon() {
         eviction_thread = std::thread([this] {
             while (running) {
                 std::this_thread::sleep_for(std::chrono::seconds(1));
                 evict_expired();
             }
         });
     }
 
 public:
     /**
      * @brief Construct a new Storage Engine
      * @param max_memory Maximum allowed memory in bytes (default: 1GB)
      */
     explicit StorageEngine(size_t max_memory = 1024 * 1024 * 1024);
     
     /**
      * @brief Destroy the Storage Engine
      * @details Stops eviction thread and cleans up resources
      */
     ~StorageEngine();
 
     /**
      * @brief Store/update a key-value pair
      * @param key Unique identifier
      * @param value Data to store
      * @param ttl Time-to-live in seconds (default: no expiration)
      * 
      * @note Thread-safe through mutex locking
      */
     void set(const std::string& key, const std::string& value,
             std::chrono::seconds ttl = std::chrono::seconds::max());
 
     /**
      * @brief Retrieve value for key
      * @param key Key to lookup
      * @return std::string Value or empty string if not found/expired
      * 
      * @details Updates last_accessed timestamp for LRU tracking
      * @note Thread-safe through mutex locking
      */
     std::string get(const std::string& key);
 
     /**
      * @brief Delete a key-value pair
      * @param key Key to remove
      * @return true If key existed and was deleted
      * @return false If key didn't exist
      * @note Thread-safe through mutex locking
      */
     bool del(const std::string& key);
 
 private:
     /**
      * @brief Enforce memory limits via LRU eviction
      * @details Called automatically during SET operations
      */
     void enforce_memory_limits();
 
     /**
      * @brief Remove expired entries based on TTL
      * @details Background process running in eviction_thread
      * Complexity: O(n) where n = number of entries
      */
     void evict_expired();
 };
 