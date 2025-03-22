/**
 * @file MemoryManager.h
 * @brief LRU eviction policy manager for BLINK DB storage engine
 */

 #pragma once
 #include <list>
 #include <string>
 
 /**
  * @class MemoryManager
  * @brief Manages Least Recently Used (LRU) eviction policy
  * 
  * @details Tracks key access patterns using a queue structure. Integrates with
  *          StorageEngine to enforce memory limits as per project requirements.
  */
 class MemoryManager {
 private:
     std::list<std::string> lru_queue; ///< LRU tracking queue (front=MRU, back=LRU)
 
 public:
     /**
      * @brief Update LRU queue on key access
      * @param key The accessed key
      * 
      * @details Moves the key to the front of the LRU queue. Complexity O(n) due
      *          to list search. Called on GET/SET operations.
      */
     void update_lru(const std::string& key) {
         lru_queue.remove(key);
         lru_queue.push_front(key);
     }
 
     /**
      * @brief Evict least recently used key
      * @return std::string Evicted key (empty if queue is empty)
      * 
      * @details Removes and returns the LRU key from queue back. Complexity O(1).
      *          Called when enforcing memory limits.
      */
     std::string evict_lru() {
         if (!lru_queue.empty()) {
             std::string key = lru_queue.back();
             lru_queue.pop_back();
             return key;
         }
         return "";
     }
 
     /**
      * @brief Remove specific key from LRU tracking
      * @param key The key to remove
      * 
      * @details Used when keys are explicitly deleted. Complexity O(n).
      *          Maintains queue consistency after manual deletions.
      */
     void evict_lru(const std::string& key) {
         lru_queue.remove(key);
     }
 };
 