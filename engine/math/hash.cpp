#include "hash.hpp"

#include <bit>

namespace axiom {
    
uint hash(ivec3 v) {
    int h = v.x * PRIME_X;
    h ^= v.y * PRIME_Y;
    h ^= v.z * PRIME_Z;

    h ^= (h >> 13);
    h = h * 60493 + 19990303;
    return abs(h) % 16;
}
    
uint hash(glm::uvec2 v) { 
    return hash(v.x ^ hash(v.y)); 
}

uint hash(glm::uvec3 v) { 
    return hash(v.x ^ hash(v.y)) ^ hash(v.z); 
}

uint hash(glm::uvec4 v) {
    return hash(v.x ^ hash(v.y)) ^ hash(v.z) ^ hash(v.w); 
}

uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

ulong hash(ulong x) {
    x += 1;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53;
    x ^= x >> 33;

    return x;
}

ulong hash(glm::vec<2, ulong> v) {
    return hash(v.x ^ hash(v.y));
}

ulong hash(glm::vec<3, ulong> v) {
    return hash(v.x ^ hash(v.y)) ^ hash(v.z);
}

ulong hash(glm::vec<4, ulong> v) {
    return hash(v.x ^ hash(v.y)) ^ hash(v.z) ^ hash(v.w); 
}

float to_float(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu; // mantissa bitmask
    const uint ieeeOne = 0x3F800000u; // 1.0

    m &= ieeeMantissa; // keep mantissa bits
    m |= ieeeOne; // add fractional part to 1.0

    float  f = std::bit_cast<float, uint>(m); // range 1 -> 2
    return f - 1.0f; // range 0 -> 1
}

std::size_t hash_coord::operator()(const ivec2& v) const {
    int hash = v.x * PRIME_X;
    hash ^= v.y * PRIME_Y;

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t hash_coord::operator()(const ivec3& v) const {
    int hash = v.x * PRIME_X;
    hash ^= v.y * PRIME_Y;
    hash ^= v.z * PRIME_Z;

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t hash_coord::operator()(const ivec4& v) const {
    int hash = v.x * PRIME_X;
    hash ^= v.y * PRIME_Y;
    hash ^= v.z * PRIME_Z;
    hash ^= v.y * PRIME_X;

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}


std::size_t hash_coord::operator()(const vec2& v) const {
    int hash = v.x * PRIME_X;
    hash ^= int(v.y * PRIME_Y);

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t hash_coord::operator()(const vec3& v) const {
    int hash = v.x * PRIME_X;
    hash ^= int(v.y * PRIME_Y);
    hash ^= int(v.z * PRIME_Z);

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t hash_coord::operator()(const vec4& v) const {
    int hash = v.x * PRIME_X;
    hash ^= int(v.y * PRIME_Y);
    hash ^= int(v.z * PRIME_Z);
    hash ^= int(v.y * PRIME_X);

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

int hash(ivec3 v, uint seed) {
    int h = v.x * PRIME_X;
    h ^= v.y * PRIME_Y;
    h ^= v.z * PRIME_Z;

    h ^= (h >> 13);
    h = h * 60493 + 19990303;
    h ^= hash(seed);
    return abs(h) % 16;
}

}