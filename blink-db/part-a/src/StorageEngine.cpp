/**
 * @file StorageEngine.cpp
 * @brief Core implementation of BLINK DB storage engine
 */

 #include "StorageEngine.h"
 #include <algorithm>
 
 /**
  * @brief Construct a new Storage Engine object
  * @param max_memory Maximum allowed memory in bytes
  * 
  * @details Initializes the storage engine with specified memory limit
  * and starts background eviction daemon thread
  */
 StorageEngine::StorageEngine(size_t max_memory) 
     : max_memory(max_memory) {
     start_eviction_daemon();
 }
 
 /**
  * @brief Destroy the Storage Engine object
  * 
  * @details Stops eviction thread and cleans up resources
  */
 StorageEngine::~StorageEngine() {
     running = false;
     if (eviction_thread.joinable()) eviction_thread.join();
 }
 
 /**
  * @brief Store or update a key-value pair
  * @param key Key to store/update
  * @param value Value to associate with key
  * @param ttl Time-to-live in seconds (default: no expiration)
  * 
  * @details Implements SET operation with thread safety and memory management:
  * - Updates existing entries' memory usage
  * - Applies LRU tracking
  * - Enforces memory limits
  * 
  * @note Locks mutex during operation
  */
 void StorageEngine::set(const std::string& key, const std::string& value,
                        std::chrono::seconds ttl) {
     std::lock_guard<std::mutex> lock(mtx);
 
     Entry new_entry{
         .value = value,
         .ttl = ttl,
         .last_accessed = std::chrono::system_clock::now()
     };
 
     Entry old_entry;
     if (store.get(key, old_entry)) {
         current_memory -= key.size() + old_entry.value.size();
     }
 
     store.insert(key, new_entry);
     current_memory += key.size() + value.size();
     mem_manager.update_lru(key);
     enforce_memory_limits();
 }
 
 /**
  * @brief Retrieve value for a key
  * @param key Key to look up
  * @return std::string Retrieved value or empty string
  * 
  * @details Implements GET operation with:
  * - Access time updating
  * - TTL expiration checks
  * - LRU tracking updates
  * 
  * @note Locks mutex during operation
  */
 std::string StorageEngine::get(const std::string& key) {
     std::lock_guard<std::mutex> lock(mtx);
 
     Entry entry;
     if (!store.get(key, entry)) return "";
 
     entry.last_accessed = std::chrono::system_clock::now();
     store.insert(key, entry); // Update access time
 
     if (entry.ttl != std::chrono::seconds::max() && 
         (std::chrono::system_clock::now() - entry.last_accessed) > entry.ttl) {
         store.remove(key);
         return "";
     }
 
     mem_manager.update_lru(key);
     return entry.value;
 }
 
 /**
  * @brief Delete a key-value pair
  * @param key Key to delete
  * @return true If key existed and was deleted
  * @return false If key didn't exist
  * 
  * @details Implements DEL operation with:
  * - Memory usage adjustment
  * - LRU tracking cleanup
  * 
  * @note Locks mutex during operation
  */
 bool StorageEngine::del(const std::string& key) {
     std::lock_guard<std::mutex> lock(mtx);
 
     Entry entry;
     if (!store.get(key, entry)) return false;
 
     current_memory -= key.size() + entry.value.size();
     store.remove(key);
     mem_manager.evict_lru(key);
     return true;
 }
 
 /**
  * @brief Enforce memory limits using LRU policy
  * 
  * @details Iteratively removes least recently used entries until:
  * - Memory usage is below max_memory
  * - LRU queue is empty
  * 
  * @note Called automatically during SET operations
  */
 void StorageEngine::enforce_memory_limits() {
     while (current_memory > max_memory) {
         std::string key = mem_manager.evict_lru();
         if (key.empty()) break;
 
         Entry entry;
         if (store.get(key, entry)) {
             current_memory -= key.size() + entry.value.size();
             store.remove(key);
         }
     }
 }
 
 /**
  * @brief Evict expired entries based on TTL
  * 
  * @details Background process that:
  * 1. Scans all entries for expiration
  * 2. Removes expired entries
  * 3. Updates LRU tracking
  * 
  * @note Runs in dedicated thread started by constructor
  * Complexity: O(n) where n = number of entries
  */
 void StorageEngine::evict_expired() {
     auto now = std::chrono::system_clock::now();
     std::vector<std::string> keys_to_remove;
 
     // Scan all buckets in hash table
     for (size_t i = 0; i < store.get_capacity(); ++i) {
         auto* current = store.get_table()[i];
         while (current) {
             if (current->value.ttl != std::chrono::seconds::max() &&
                 (now - current->value.last_accessed) > current->value.ttl) {
                 keys_to_remove.push_back(current->key);
             }
             current = current->next;
         }
     }
 
     // Batch remove expired entries
     for (const auto& key : keys_to_remove) {
         store.remove(key);
         mem_manager.evict_lru(key);
     }
 }
 