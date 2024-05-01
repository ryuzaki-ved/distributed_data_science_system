#include "utils/types.h"
#include <sstream>
#include <cstring>
#include <algorithm>

// Add these includes for string vector serialization
#include <vector>
#include <string>

namespace dds {

// ... [existing code unchanged] ...

// --- New Feature: Serialize and Deserialize std::vector<std::string> ---

std::vector<char> serialize_string_vector(const std::vector<std::string>& vec) {
    std::vector<char> data;

    // Serialize vector size
    Index size = static_cast<Index>(vec.size());
    data.resize(sizeof(Index));
    std::memcpy(data.data(), &size, sizeof(Index));

    // Serialize each string: first its length, then its data
    for (const auto& str : vec) {
        Index len = static_cast<Index>(str.size());
        size_t old_size = data.size();
        data.resize(old_size + sizeof(Index) + len);
        std::memcpy(data.data() + old_size, &len, sizeof(Index));
        std::memcpy(data.data() + old_size + sizeof(Index), str.data(), len);
    }
    return data;
}

std::vector<std::string> deserialize_string_vector(const std::vector<char>& data) {
    std::vector<std::string> vec;
    if (data.size() < sizeof(Index)) {
        throw std::runtime_error("Invalid string vector data: insufficient header size");
    }
    const char* ptr = data.data();
    Index size;
    std::memcpy(&size, ptr, sizeof(Index));
    ptr += sizeof(Index);

    for (Index i = 0; i < size; ++i) {
        if (ptr + sizeof(Index) > data.data() + data.size()) {
            throw std::runtime_error("Invalid string vector data: insufficient string length header");
        }
        Index len;
        std::memcpy(&len, ptr, sizeof(Index));
        ptr += sizeof(Index);

        if (ptr + len > data.data() + data.size()) {
            throw std::runtime_error("Invalid string vector data: insufficient string data");
        }
        vec.emplace_back(ptr, len);
        ptr += len;
    }
    return vec;
}

// ... [existing code unchanged] ...

} // namespace dds
