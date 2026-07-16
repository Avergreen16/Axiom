#pragma once

#include <vector>
#include <cstring>

struct Packet {
    uint32_t type;
    std::vector<uint8_t> body;
};

template<typename Type>
Type read_buffer(uint32_t& location, std::vector<uint8_t>& buffer) {
    Type value;
    std::memcpy(&value, &buffer[location], sizeof(Type));
    location += sizeof(Type);

    return value;
}

template<typename Type>
void read_buffer(Type* t, uint32_t size, uint32_t& location, std::vector<uint8_t>& buffer) {
    std::memcpy(t, &buffer[location], sizeof(Type) * size);
    location += sizeof(Type) * size;
}

template<typename Type>
void write_buffer(Type t, std::vector<uint8_t>& buffer) {
    uint32_t start = buffer.size();

    buffer.resize(buffer.size() + sizeof(Type));

    std::memcpy(&buffer[start], &t, sizeof(Type));
}

template<typename Type>
void write_buffer(Type* t, uint32_t size, std::vector<uint8_t>& buffer) {
    uint32_t start = buffer.size();

    buffer.resize(buffer.size() + sizeof(Type) * size);

    std::memcpy(&buffer[start], t, sizeof(Type) * size);
} 