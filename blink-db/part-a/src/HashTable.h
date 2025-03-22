#pragma once
#include <vector>
#include <list>
#include <string>
#include <functional>
#include <stdexcept>

/**
 * @class HashTable
 * @brief Custom hash table implementation with separate chaining collision resolution
 * @tparam K Key type
 * @tparam V Value type
 * 
 * @details Implements a hash table with dynamic resizing and LRU-friendly structure.
 *          Uses separate chaining for collision resolution. Automatically resizes
 *          when load factor exceeds 0.7 or falls below 0.2 (for capacities > 8).
 */
template <typename K, typename V>
class HashTable
{
private:
    /**
     * @struct Node
     * @brief Node structure for linked list in separate chaining
     */
    struct Node
    {
        K key;        ///< The key stored in this node
        V value;      ///< The value associated with the key
        Node *next;   ///< Pointer to next node in the chain

        /**
         * @brief Construct a new Node object
         * @param k Key for the node
         * @param v Value to store
         * @param n Pointer to next node in chain
         */
        Node(const K &k, const V &v, Node *n = nullptr)
            : key(k), value(v), next(n) {}
    };

    std::vector<Node *> table;    ///< The hash table buckets
    size_t capacity;              ///< Current number of buckets
    size_t size;                  ///< Number of elements in the table
    const double LOAD_FACTOR = 0.7; ///< Load factor threshold for resizing

    /**
     * @brief Hash function for key distribution
     * @param key The key to hash
     * @return size_t Bucket index
     */
    size_t hash(const K &key) const
    {
        return std::hash<K>{}(key) % capacity;
    }

    /**
     * @brief Resize the hash table to new capacity
     * @param new_capacity New number of buckets
     * 
     * @details Rehashes all existing entries into new table. Complexity O(n)
     */
    void resize(size_t new_capacity)
    {
        std::vector<Node *> new_table(new_capacity, nullptr);

        // Rehash all entries
        for (size_t i = 0; i < capacity; ++i)
        {
            Node *current = table[i];
            while (current)
            {
                Node *next = current->next;
                size_t new_index = std::hash<K>{}(current->key) % new_capacity;

                current->next = new_table[new_index];
                new_table[new_index] = current;

                current = next;
            }
        }

        table = std::move(new_table);
        capacity = new_capacity;
    }

public:
    /**
     * @brief Get the internal table structure
     * @return std::vector<Node*>& Reference to the bucket array
     */
    std::vector<Node *> &get_table() { return table; }

    /**
     * @brief Get current table capacity
     * @return size_t Number of buckets
     */
    size_t get_capacity() const { return capacity; }

    /**
     * @brief Construct a new Hash Table object
     * @param initial_capacity Starting number of buckets (default: 8)
     * @throws std::invalid_argument If initial_capacity < 1
     */
    explicit HashTable(size_t initial_capacity = 8)
        : capacity(initial_capacity), size(0)
    {
        if (initial_capacity < 1)
            throw std::invalid_argument("Invalid capacity");
        table.resize(capacity, nullptr);
    }

    /**
     * @brief Destroy the Hash Table object
     * @details Clears all nodes and deallocates memory
     */
    ~HashTable()
    {
        clear();
    }

    /**
     * @brief Insert or update a key-value pair
     * @param key The key to insert/update
     * @param value The value to associate with the key
     * 
     * @details Automatically resizes table if load factor exceeds threshold.
     *          Time complexity: O(1) average case, O(n) worst case
     */
    void insert(const K &key, const V &value)
    {
        if (size >= LOAD_FACTOR * capacity)
        {
            resize(2 * capacity);
        }

        size_t index = hash(key);
        Node *current = table[index];

        // Update existing key if found
        while (current)
        {
            if (current->key == key)
            {
                current->value = value;
                return;
            }
            current = current->next;
        }

        // Insert new node at head of chain
        table[index] = new Node(key, value, table[index]);
        size++;
    }

    /**
     * @brief Retrieve value for a key
     * @param key The key to search for
     * @param value[out] Reference to store retrieved value
     * @return true If key was found
     * @return false If key was not found
     */
    bool get(const K &key, V &value) const
    {
        size_t index = hash(key);
        Node *current = table[index];

        while (current)
        {
            if (current->key == key)
            {
                value = current->value;
                return true;
            }
            current = current->next;
        }
        return false;
    }

    /**
     * @brief Remove a key-value pair
     * @param key The key to remove
     * @return true If key was found and removed
     * @return false If key was not found
     * 
     * @details Automatically shrinks table if load factor falls below 0.2
     *          (for capacities > 8)
     */
    bool remove(const K &key)
    {
        size_t index = hash(key);
        Node *prev = nullptr;
        Node *current = table[index];

        while (current)
        {
            if (current->key == key)
            {
                if (prev)
                {
                    prev->next = current->next;
                }
                else
                {
                    table[index] = current->next;
                }

                delete current;
                size--;

                if (capacity > 8 && size < 0.2 * capacity)
                {
                    resize(capacity / 2);
                }
                return true;
            }

            prev = current;
            current = current->next;
        }
        return false;
    }

    /**
     * @brief Clear all entries from the hash table
     * @details Deallocates all nodes and resets to initial state
     */
    void clear()
    {
        for (size_t i = 0; i < capacity; ++i)
        {
            Node *current = table[i];
            while (current)
            {
                Node *next = current->next;
                delete current;
                current = next;
            }
            table[i] = nullptr;
        }
        size = 0;
    }

    // Disable copy operations
    HashTable(const HashTable &) = delete;            ///< Disabled copy constructor
    HashTable &operator=(const HashTable &) = delete; ///< Disabled assignment operator
};
