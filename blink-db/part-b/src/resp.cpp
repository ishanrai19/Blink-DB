/**
 * @file resp.cpp
 * @brief Implementation of RESP-2 protocol encoder/decoder
 * 
 * @details This file implements the Redis Serialization Protocol (RESP-2) for BLINK DB.
 * It provides classes and methods for encoding and decoding data in the RESP-2 format,
 * supporting all five data types: Simple Strings, Errors, Integers, Bulk Strings, and Arrays.
 * The implementation handles incremental parsing, allowing for processing of partial messages.
 */

 #include "resp.h"
 #include <sstream>
 #include <stdexcept>
 
 /** @brief CRLF sequence used in RESP-2 protocol */
 const std::string CRLF = "\r\n";
 
 /**
  * @brief Create a Simple String RESP value
  * 
  * @param value String content
  * @return RespValue object representing a Simple String
  */
 RespProtocol::RespValue RespProtocol::RespValue::createSimpleString(const std::string& value) {
     RespValue resp(Type::SIMPLE_STRING, false);
     resp.string_value_ = value;
     return resp;
 }
 
 /**
  * @brief Create an Error RESP value
  * 
  * @param value Error message
  * @return RespValue object representing an Error
  */
 RespProtocol::RespValue RespProtocol::RespValue::createError(const std::string& value) {
     RespValue resp(Type::ERROR, false);
     resp.string_value_ = value;
     return resp;
 }
 
 /**
  * @brief Create an Integer RESP value
  * 
  * @param value Integer value
  * @return RespValue object representing an Integer
  */
 RespProtocol::RespValue RespProtocol::RespValue::createInteger(int64_t value) {
     RespValue resp(Type::INTEGER, false);
     resp.int_value_ = value;
     return resp;
 }
 
 /**
  * @brief Create a Bulk String RESP value
  * 
  * @param value String content (can be binary)
  * @return RespValue object representing a Bulk String
  */
 RespProtocol::RespValue RespProtocol::RespValue::createBulkString(const std::string& value) {
     RespValue resp(Type::BULK_STRING, false);
     resp.string_value_ = value;
     return resp;
 }
 
 /**
  * @brief Create a Null Bulk String RESP value
  * 
  * @return RespValue object representing a Null Bulk String ($-1\r\n)
  */
 RespProtocol::RespValue RespProtocol::RespValue::createNullBulkString() {
     return RespValue(Type::BULK_STRING, true);
 }
 
 /**
  * @brief Create an Array RESP value
  * 
  * @param values Vector of RESP values to include in the array
  * @return RespValue object representing an Array
  */
 RespProtocol::RespValue RespProtocol::RespValue::createArray(const std::vector<RespValue>& values) {
     RespValue resp(Type::ARRAY, false);
     resp.array_values_ = values;
     return resp;
 }
 
 /**
  * @brief Create a Null Array RESP value
  * 
  * @return RespValue object representing a Null Array (*-1\r\n)
  */
 RespProtocol::RespValue RespProtocol::RespValue::createNullArray() {
     return RespValue(Type::ARRAY, true);
 }
 
 /**
  * @brief Get the string value stored in this RESP value
  * 
  * @details Can only be called on SIMPLE_STRING, ERROR, or BULK_STRING types
  * 
  * @return The stored string value
  * @throws std::runtime_error if type is not string-compatible or value is null
  */
 std::string RespProtocol::RespValue::getString() const {
     if (type_ != Type::SIMPLE_STRING && type_ != Type::ERROR && type_ != Type::BULK_STRING) {
         throw std::runtime_error("Not a string type");
     }
     if (is_null_) {
         throw std::runtime_error("Null value has no string");
     }
     return string_value_;
 }
 
 /**
  * @brief Get the integer value stored in this RESP value
  * 
  * @details Can only be called on INTEGER type
  * 
  * @return The stored integer value
  * @throws std::runtime_error if type is not INTEGER or value is null
  */
 int64_t RespProtocol::RespValue::getInteger() const {
     if (type_ != Type::INTEGER) {
         throw std::runtime_error("Not an integer type");
     }
     if (is_null_) {
         throw std::runtime_error("Null value has no integer");
     }
     return int_value_;
 }
 
 /**
  * @brief Get the array values stored in this RESP value
  * 
  * @details Can only be called on ARRAY type
  * 
  * @return Reference to the vector of RESP values
  * @throws std::runtime_error if type is not ARRAY or array is null
  */
 const std::vector<RespProtocol::RespValue>& RespProtocol::RespValue::getArray() const {
     if (type_ != Type::ARRAY) {
         throw std::runtime_error("Not an array type");
     }
     if (is_null_) {
         throw std::runtime_error("Null array has no elements");
     }
     return array_values_;
 }
 
 /**
  * @brief Encode a RESP value to wire format
  * 
  * @details Converts the given RESP value object to its string representation
  * according to the RESP-2 protocol specification
  * 
  * @param value RESP value to encode
  * @return String containing the RESP-2 encoded data
  * @throws std::runtime_error if the value has an unknown type
  */
 std::string RespProtocol::encode(const RespValue& value) {
     switch (value.getType()) {
         case Type::SIMPLE_STRING:
             return encodeSimpleString(value.getString());
         case Type::ERROR:
             return encodeError(value.getString());
         case Type::INTEGER:
             return encodeInteger(value.getInteger());
         case Type::BULK_STRING:
             if (value.isNull()) {
                 return encodeNullBulkString();
             }
             return encodeBulkString(value.getString());
         case Type::ARRAY:
             if (value.isNull()) {
                 return encodeNullArray();
             }
             return encodeArray(value.getArray());
         default:
             throw std::runtime_error("Unknown RESP type");
     }
 }
 
 /**
  * @brief Encode a command with arguments as a RESP array
  * 
  * @details Formats a command and its arguments as a RESP array of bulk strings
  * 
  * @param command Command name
  * @param args Command arguments
  * @return RESP-encoded command string
  */
 std::string RespProtocol::encodeCommand(const std::string& command, const std::vector<std::string>& args) {
     std::vector<RespValue> array_elements;
     array_elements.push_back(RespValue::createBulkString(command));
     
     for (const auto& arg : args) {
         array_elements.push_back(RespValue::createBulkString(arg));
     }
     
     return encodeArray(array_elements);
 }
 
 /**
  * @brief Encode a Simple String in RESP format
  * 
  * @param value String content
  * @return RESP-encoded Simple String (+value\r\n)
  */
 std::string RespProtocol::encodeSimpleString(const std::string& value) {
     return "+" + value + CRLF;
 }
 
 /**
  * @brief Encode an Error in RESP format
  * 
  * @param value Error message
  * @return RESP-encoded Error (-value\r\n)
  */
 std::string RespProtocol::encodeError(const std::string& value) {
     return "-" + value + CRLF;
 }
 
 /**
  * @brief Encode an Integer in RESP format
  * 
  * @param value Integer value
  * @return RESP-encoded Integer (:value\r\n)
  */
 std::string RespProtocol::encodeInteger(int64_t value) {
     return ":" + std::to_string(value) + CRLF;
 }
 
 /**
  * @brief Encode a Bulk String in RESP format
  * 
  * @param value String content
  * @return RESP-encoded Bulk String ($length\r\nvalue\r\n)
  */
 std::string RespProtocol::encodeBulkString(const std::string& value) {
     return "$" + std::to_string(value.size()) + CRLF + value + CRLF;
 }
 
 /**
  * @brief Encode a Null Bulk String in RESP format
  * 
  * @return RESP-encoded Null Bulk String ($-1\r\n)
  */
 std::string RespProtocol::encodeNullBulkString() {
     return "$-1" + CRLF;
 }
 
 /**
  * @brief Encode an Array in RESP format
  * 
  * @param values Vector of RESP values
  * @return RESP-encoded Array
  */
 std::string RespProtocol::encodeArray(const std::vector<RespValue>& values) {
     std::string result = "*" + std::to_string(values.size()) + CRLF;
     for (const auto& value : values) {
         result += encode(value);
     }
     return result;
 }
 
 /**
  * @brief Encode a Null Array in RESP format
  * 
  * @return RESP-encoded Null Array (*-1\r\n)
  */
 std::string RespProtocol::encodeNullArray() {
     return "*-1" + CRLF;
 }
 
 /**
  * @brief Parse RESP-2 data from a string
  * 
  * @details Incrementally parses RESP-2 formatted data from the input string.
  * Returns std::nullopt if more data is needed to complete parsing.
  * 
  * @param data String containing RESP-2 formatted data
  * @param[out] bytes_consumed Output parameter that will contain the number of bytes processed
  * @return std::optional<RespValue> Parsed value if complete, std::nullopt if more data needed
  * @throws std::runtime_error if the data contains invalid RESP format
  */
 std::optional<RespProtocol::RespValue> RespProtocol::parse(const std::string& data, size_t& bytes_consumed) {
     if (data.empty()) {
         return std::nullopt;
     }
     
     bytes_consumed = 0;
     
     switch (data[0]) {
         case '+':
             return parseSimpleString(data, bytes_consumed);
         case '-':
             return parseError(data, bytes_consumed);
         case ':':
             return parseInteger(data, bytes_consumed);
         case '$':
             return parseBulkString(data, bytes_consumed);
         case '*':
             return parseArray(data, bytes_consumed);
         default:
             throw std::runtime_error("Invalid RESP data type");
     }
 }
 std::optional<RespProtocol::RespValue> RespProtocol::parseSimpleString(const std::string& data, size_t& pos) {
    size_t crlf_pos = findCRLF(data, 1);
    if (crlf_pos == std::string::npos) {
        return std::nullopt;  // Incomplete, need more data
    }
    
    std::string value = data.substr(1, crlf_pos - 1);
    pos = crlf_pos + 2;  // +2 for CRLF
    
    return RespValue::createSimpleString(value);
}

std::optional<RespProtocol::RespValue> RespProtocol::parseError(const std::string& data, size_t& pos) {
    size_t crlf_pos = findCRLF(data, 1);
    if (crlf_pos == std::string::npos) {
        return std::nullopt;  // Incomplete, need more data
    }
    
    std::string value = data.substr(1, crlf_pos - 1);
    pos = crlf_pos + 2;  // +2 for CRLF
    
    return RespValue::createError(value);
}

std::optional<RespProtocol::RespValue> RespProtocol::parseInteger(const std::string& data, size_t& pos) {
    size_t crlf_pos = findCRLF(data, 1);
    if (crlf_pos == std::string::npos) {
        return std::nullopt;  // Incomplete, need more data
    }
    
    std::string int_str = data.substr(1, crlf_pos - 1);
    pos = crlf_pos + 2;  // +2 for CRLF
    
    try {
        int64_t value = std::stoll(int_str);
        return RespValue::createInteger(value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid integer format in RESP data");
    }
}

std::optional<RespProtocol::RespValue> RespProtocol::parseBulkString(const std::string& data, size_t& pos) {
    size_t crlf_pos = findCRLF(data, 1);
    if (crlf_pos == std::string::npos) {
        return std::nullopt;  // Incomplete, need more data
    }
    
    std::string len_str = data.substr(1, crlf_pos - 1);
    
    try {
        int len = std::stoi(len_str);
        
        if (len == -1) {
            // Null bulk string
            pos = crlf_pos + 2;  // +2 for CRLF
            return RespValue::createNullBulkString();
        }
        
        if (len < 0) {
            throw std::runtime_error("Invalid bulk string length in RESP data");
        }
        
        // Check if we have the full string + the trailing CRLF
        if (data.size() < crlf_pos + 2 + len + 2) {
            return std::nullopt;  // Incomplete, need more data
        }
        
        std::string value = data.substr(crlf_pos + 2, len);
        pos = crlf_pos + 2 + len + 2;  // +2 for header CRLF, +2 for trailing CRLF
        
        return RespValue::createBulkString(value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid bulk string format in RESP data");
    }
}

std::optional<RespProtocol::RespValue> RespProtocol::parseArray(const std::string& data, size_t& pos) {
    size_t crlf_pos = findCRLF(data, 1);
    if (crlf_pos == std::string::npos) {
        return std::nullopt;  // Incomplete, need more data
    }
    
    std::string len_str = data.substr(1, crlf_pos - 1);
    
    try {
        int len = std::stoi(len_str);
        
        if (len == -1) {
            // Null array
            pos = crlf_pos + 2;  // +2 for CRLF
            return RespValue::createNullArray();
        }
        
        if (len < 0) {
            throw std::runtime_error("Invalid array length in RESP data");
        }
        
        std::vector<RespValue> elements;
        size_t current_pos = crlf_pos + 2;  // Start after array header CRLF
        
        for (int i = 0; i < len; i++) {
            if (current_pos >= data.size()) {
                return std::nullopt;  // Incomplete, need more data
            }
            
            size_t element_bytes = 0;
            auto element = parse(data.substr(current_pos), element_bytes);
            
            if (!element.has_value()) {
                return std::nullopt;  // Incomplete, need more data
            }
            
            elements.push_back(element.value());
            current_pos += element_bytes;
        }
        
        pos = current_pos;
        return RespValue::createArray(elements);
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid array format in RESP data");
    }
}

size_t RespProtocol::findCRLF(const std::string& data, size_t start) {
    if (start >= data.size()) {
        return std::string::npos;
    }
    
    for (size_t i = start; i < data.size() - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }
    
    return std::string::npos;
}
