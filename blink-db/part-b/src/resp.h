/**
 * @file resp.h
 * @brief RESP-2 protocol encoder/decoder for BLINK DB
 * 
 * @details Implements the Redis Serialization Protocol (RESP-2) for communication
 * between clients and the BLINK DB server. Handles all five RESP data types:
 * Simple Strings, Errors, Integers, Bulk Strings, and Arrays.
 * 
 * This implementation provides both encoding and incremental parsing capabilities
 * for complete RESP-2 protocol support, enabling efficient client-server communication
 * with the same wire format used by Redis.
 */

 #pragma once

 #include <string>
 #include <vector>
 #include <optional>
 
 /**
  * @class RespProtocol
  * @brief Encoder/decoder for RESP-2 protocol
  * 
  * @details Provides static methods for encoding and parsing data in the RESP-2 format.
  * The implementation is focused on efficiency and correctness, supporting all five
  * RESP data types and handling edge cases like null values and incremental parsing.
  */
 class RespProtocol {
 public:
     /**
      * @enum Type
      * @brief RESP data types
      * 
      * @details Enumerates the five data types defined in the RESP-2 protocol specification.
      * Each type has a specific wire format and usage scenarios.
      */
     enum class Type {
         SIMPLE_STRING,  ///< Simple string prefixed with "+" (e.g., "+OK\r\n")
         ERROR,          ///< Error message prefixed with "-" (e.g., "-ERR message\r\n")
         INTEGER,        ///< Integer prefixed with ":" (e.g., ":1000\r\n")
         BULK_STRING,    ///< Bulk string prefixed with "$" (e.g., "$6\r\nfoobar\r\n")
         ARRAY           ///< Array prefixed with "*" (e.g., "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n")
     };
 
     /**
      * @class RespValue
      * @brief Represents a RESP value of any supported type
      * 
      * @details This class encapsulates a value in the RESP protocol, handling all five data types.
      * It provides type-safe access to the value contents through getter methods that perform
      * runtime type checking. The class supports null values for both Bulk Strings and Arrays.
      */
     class RespValue {
     public:
         /**
          * @brief Construct a null RESP value
          * 
          * @details Default constructor creates a null Bulk String value.
          * Equivalent to $-1\r\n in RESP protocol.
          */
         RespValue() : type_(Type::BULK_STRING), is_null_(true) {}
 
         /**
          * @brief Construct a Simple String RESP value
          * 
          * @details Creates a Simple String value prefixed with "+" in RESP format.
          * Simple strings cannot contain CR or LF characters.
          * 
          * @param value String content
          * @return RespValue object representing a Simple String
          */
         static RespValue createSimpleString(const std::string& value);
 
         /**
          * @brief Construct an Error RESP value
          * 
          * @details Creates an Error value prefixed with "-" in RESP format.
          * Typically used to represent error conditions.
          * 
          * @param value Error message
          * @return RespValue object representing an Error
          */
         static RespValue createError(const std::string& value);
 
         /**
          * @brief Construct an Integer RESP value
          * 
          * @details Creates an Integer value prefixed with ":" in RESP format.
          * Used to represent numeric values such as counters or response codes.
          * 
          * @param value Integer value
          * @return RespValue object representing an Integer
          */
         static RespValue createInteger(int64_t value);
 
         /**
          * @brief Construct a Bulk String RESP value
          * 
          * @details Creates a Bulk String value in RESP format. Bulk strings
          * are binary-safe and can contain any byte sequence including null bytes.
          * 
          * @param value String content (can be binary)
          * @return RespValue object representing a Bulk String
          */
         static RespValue createBulkString(const std::string& value);
 
         /**
          * @brief Construct a Null Bulk String RESP value
          * 
          * @details Creates a special Bulk String value representing NULL.
          * Encoded as "$-1\r\n" in RESP protocol.
          * 
          * @return RespValue object representing a Null Bulk String
          */
         static RespValue createNullBulkString();
 
         /**
          * @brief Construct an Array RESP value
          * 
          * @details Creates an Array value containing other RESP values.
          * Arrays can contain mixed types and are used for commands and complex responses.
          * 
          * @param values Vector of RESP values to include in the array
          * @return RespValue object representing an Array
          */
         static RespValue createArray(const std::vector<RespValue>& values);
 
         /**
          * @brief Construct a Null Array RESP value
          * 
          * @details Creates a special Array value representing NULL.
          * Encoded as "*-1\r\n" in RESP protocol.
          * 
          * @return RespValue object representing a Null Array
          */
         static RespValue createNullArray();
 
         /**
          * @brief Get the type of this RESP value
          * 
          * @details Returns the type enum indicating which of the five RESP types
          * this value represents.
          * 
          * @return Type enum value
          */
         Type getType() const { return type_; }
 
         /**
          * @brief Check if this RESP value is null
          * 
          * @details Returns whether this value is a null representation.
          * Only relevant for Bulk String and Array types.
          * 
          * @return true if null, false otherwise
          */
         bool isNull() const { return is_null_; }
 
         /**
          * @brief Get string value (for SIMPLE_STRING, ERROR, BULK_STRING)
          * 
          * @details Retrieves the string content of this value. Only valid for
          * string-compatible types (Simple String, Error, Bulk String).
          * 
          * @return String value
          * @throws std::runtime_error if type is not string-compatible or if value is null
          */
         std::string getString() const;
 
         /**
          * @brief Get integer value (for INTEGER)
          * 
          * @details Retrieves the integer content of this value. Only valid for
          * the Integer type.
          * 
          * @return Integer value
          * @throws std::runtime_error if type is not INTEGER or if value is null
          */
         int64_t getInteger() const;
 
         /**
          * @brief Get array values (for ARRAY)
          * 
          * @details Retrieves the vector of elements in this array. Only valid for
          * the Array type.
          * 
          * @return Vector of RESP values
          * @throws std::runtime_error if type is not ARRAY or if array is null
          */
         const std::vector<RespValue>& getArray() const;
 
     private:
         Type type_;                   ///< Type of this RESP value
         bool is_null_ = false;        ///< Whether this value is null
         std::string string_value_;    ///< String data for SIMPLE_STRING, ERROR, BULK_STRING
         int64_t int_value_ = 0;       ///< Integer data for INTEGER
         std::vector<RespValue> array_values_; ///< Array elements for ARRAY
 
         /**
          * @brief Construct a RESP value with type and null flag
          * 
          * @details Private constructor used by factory methods to create
          * typed RESP values with specified null state.
          * 
          * @param type RESP type
          * @param is_null Whether this value is null
          */
         RespValue(Type type, bool is_null) : type_(type), is_null_(is_null) {}
     };
 
     /**
      * @brief Encode a RESP value to string for transmission
      * 
      * @details Converts a RespValue object to its wire format representation
      * according to the RESP-2 protocol specification. The resulting string
      * can be sent over a network connection.
      * 
      * @param value RESP value to encode
      * @return String encoded in RESP format
      * @throws std::runtime_error if encoding fails or if value has an unknown type
      */
     static std::string encode(const RespValue& value);
 
     /**
      * @brief Encode a command with arguments to RESP array format
      * 
      * @details Convenience method for creating a command in RESP format.
      * Commands are represented as arrays where the first element is the
      * command name and subsequent elements are arguments.
      * 
      * @param command Command name (e.g., "SET", "GET", "DEL")
      * @param args Command arguments
      * @return String encoded in RESP format
      */
     static std::string encodeCommand(const std::string& command, 
                                   const std::vector<std::string>& args = {});
 
     /**
      * @brief Parse RESP data from input buffer
      * 
      * @details Attempts to parse a complete RESP value from the provided data buffer.
      * If a complete value cannot be parsed (e.g., due to incomplete data), returns
      * std::nullopt. This enables incremental parsing of RESP protocol data.
      * 
      * @param data Input buffer containing RESP data
      * @param[out] bytes_consumed Number of bytes consumed from input
      * @return Parsed RESP value if complete, or std::nullopt if more data needed
      * @throws std::runtime_error if the data contains invalid RESP format
      */
     static std::optional<RespValue> parse(const std::string& data, size_t& bytes_consumed);
 
 private:
     /**
      * @brief Encode a Simple String in RESP format
      * @param value String content
      * @return RESP-encoded Simple String
      */
     static std::string encodeSimpleString(const std::string& value);
     
     /**
      * @brief Encode an Error in RESP format
      * @param value Error message
      * @return RESP-encoded Error
      */
     static std::string encodeError(const std::string& value);
     
     /**
      * @brief Encode an Integer in RESP format
      * @param value Integer value
      * @return RESP-encoded Integer
      */
     static std::string encodeInteger(int64_t value);
     
     /**
      * @brief Encode a Bulk String in RESP format
      * @param value String content
      * @return RESP-encoded Bulk String
      */
     static std::string encodeBulkString(const std::string& value);
     
     /**
      * @brief Encode a Null Bulk String in RESP format
      * @return RESP-encoded Null Bulk String
      */
     static std::string encodeNullBulkString();
     
     /**
      * @brief Encode an Array in RESP format
      * @param values Vector of RESP values
      * @return RESP-encoded Array
      */
     static std::string encodeArray(const std::vector<RespValue>& values);
     
     /**
      * @brief Encode a Null Array in RESP format
      * @return RESP-encoded Null Array
      */
     static std::string encodeNullArray();
 
     /**
      * @brief Parse a Simple String from RESP data
      * @param data Input buffer containing RESP data
      * @param[out] pos Position after successful parse
      * @return Parsed Simple String value, or std::nullopt if incomplete
      */
     static std::optional<RespValue> parseSimpleString(const std::string& data, size_t& pos);
     
     /**
      * @brief Parse an Error from RESP data
      * @param data Input buffer containing RESP data
      * @param[out] pos Position after successful parse
      * @return Parsed Error value, or std::nullopt if incomplete
      */
     static std::optional<RespValue> parseError(const std::string& data, size_t& pos);
     
     /**
      * @brief Parse an Integer from RESP data
      * @param data Input buffer containing RESP data
      * @param[out] pos Position after successful parse
      * @return Parsed Integer value, or std::nullopt if incomplete
      */
     static std::optional<RespValue> parseInteger(const std::string& data, size_t& pos);
     
     /**
      * @brief Parse a Bulk String from RESP data
      * @param data Input buffer containing RESP data
      * @param[out] pos Position after successful parse
      * @return Parsed Bulk String value, or std::nullopt if incomplete
      */
     static std::optional<RespValue> parseBulkString(const std::string& data, size_t& pos);
     
     /**
      * @brief Parse an Array from RESP data
      * @param data Input buffer containing RESP data
      * @param[out] pos Position after successful parse
      * @return Parsed Array value, or std::nullopt if incomplete
      */
     static std::optional<RespValue> parseArray(const std::string& data, size_t& pos);
 
     /**
      * @brief Find CRLF sequence in data
      * @param data String to search in
      * @param start Position to start searching from
      * @return Position of CRLF if found, std::string::npos otherwise
      */
     static size_t findCRLF(const std::string& data, size_t start);
 };
 