#include <bit>

#include "random.hpp"

namespace axiom {

random32::random32(uint seed) {
    this->seed = hash(seed);
    state = seed;
}

float random32::operator()() {
    state += 0x5DE017D1u;
    uint value = hash(state);

    return to_float(value);
}

uint random32::next() {
    state += 0x5DE017D1u;
    uint value = hash(state);

    return value;
}

vec3 random32::unit_vector() {
    vec3 r;

    bool c = true;
    while(c) {
        r = {operator()(), operator()(), operator()()};
        r = r * 2.0f - 1.0f;
        
        float len = length(r);
        if(!(len > 1 || len == 0)) c = false; 
    }
    
    r = normalize(r);

    return r;
}

float random32::operator()(uvec3 i) {
    uint hash_value = hash(i) ^ seed;

    return to_float(hash_value);
}

uint random32::hash_i(uvec3 i) {
    uint hash_value = hash(i) ^ seed;

    return hash_value;
}

vec3 random32::unit_vector(uvec3 i) {
    glm::vec3 unit_vector;
    float u_length = 10;
    
    while(u_length > 1.0 || u_length == 0.0) {
        float x = to_float(seed ^ hash_i(uvec3(i.x, i.y, i.z)));
        float z = to_float(seed ^ hash_i(uvec3(i.y, 0xFFFFFFFFu - i.z, i.x)));
        float y = to_float(seed ^ hash_i(uvec3(i.z, 0xFFFFFFFFu - i.x, 0xFFFFFFFFu - i.y)));

        unit_vector = glm::vec3(x, y, z);
        u_length = glm::length(unit_vector);
    }

    return normalize(unit_vector);
}

vec3 random32::cube_vector(uvec3 i) {
    float x = to_float(next() ^ hash_i(uvec3(i.x, i.y, i.z)));
    float z = to_float(next() ^ hash_i(uvec3(i.y, 0xFFFFFFFFu - i.z, i.x)));
    float y = to_float(next() ^ hash_i(uvec3(i.z, 0xFFFFFFFFu - i.x, 0xFFFFFFFFu - i.y)));

    return vec3(x, y, z);
}

//

random::random(uint64_t seed) {
    this->seed = hash(seed);
    state = seed;
}

float random::operator()() {
    state += 0x5DE017D10019FE17ul;
    uint64_t value = hash(state);

    return to_float(value);
}

uint64_t random::next() {
    state += 0x5DE017D10019FE17ul;
    uint64_t value = hash(state);

    return value;
}


vec3 random::unit_vector() {
    vec3 r;

    bool c = true;
    while(c) {
        r = {operator()(), operator()(), operator()()};
        
        float len = length(r);
        if(!(len > 1 || len == 0)) c = false; 
    }
    
    r = normalize(r);

    return r;
}

float random::operator()(glm::vec<3, uint64_t> i) {
    uint hash_value = hash(i) ^ seed;

    return to_float(hash_value);
}

uint64_t random::hash_i(glm::vec<3, uint64_t> i) {
    uint64_t hash_value = hash(i) ^ seed;

    return hash_value;
}

vec3 random::unit_vector(glm::vec<3, uint64_t> i) {
    uint64_t current_value = seed;

    glm::vec3 unit_vector;
    float u_length = 10;
    while(u_length > 1.0 || u_length == 0.0) {
        float x = to_float(seed ^ hash_i(glm::vec<3, uint64_t>(i.x, i.y, i.z)));
        float z = to_float(seed ^ hash_i(glm::vec<3, uint64_t>(i.y, 0xFFFFFFFFFFFFFFFFu - i.z, i.x)));
        float y = to_float(seed ^ hash_i(glm::vec<3, uint64_t>(i.z, 0xFFFFFFFFFFFFFFFFu - i.x, 0xFFFFFFFFFFFFFFFFu - i.y)));

        unit_vector = glm::vec3(x, y, z);
        u_length = glm::length(unit_vector);
    }

    return normalize(unit_vector);
}

vec3 random::cube_vector(glm::vec<3, uint64_t> i) {
    float x = to_float(seed ^ hash_i(glm::vec<3, uint64_t>(i.x, i.y, i.z)));
    float z = to_float(seed ^ hash_i(glm::vec<3, uint64_t>(i.y, 0xFFFFFFFFFFFFFFFFu - i.z, i.x)));
    float y = to_float(seed ^ hash_i(glm::vec<3, uint64_t>(i.z, 0xFFFFFFFFFFFFFFFFu - i.x, 0xFFFFFFFFFFFFFFFFu - i.y)));

    return glm::vec<3, uint64_t>(x, y, z);
}

}