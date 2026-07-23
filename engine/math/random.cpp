#include <bit>

#include "random.hpp"

namespace axiom {

random32::random32(uint init_seed) {
    seed = init_seed;
    value = seed;
}

float random32::operator()() {
    value = hash(value);

    return to_float(value);
}

uint random32::next() {
    value = hash(value);

    return value;
}

vec3 random32::unit_vector() {
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


float random32::operator()(uvec3 i) {
    uint hash_value = hash(uvec4(i, seed));

    return to_float(hash_value);
}

uint random32::hash_i(uvec3 i) {
    uint hash_value = hash(uvec4(i, seed));

    return hash_value;
}

vec3 random32::unit_vector(uvec3 i) {
    uint current_value = seed;

    glm::vec3 unit_vector;
    float u_length = 10;
    while(u_length > 1.0 || u_length == 0.0) {
        current_value = hash(current_value);
        float x = to_float(hash(i) ^ current_value);
        current_value = hash(current_value);
        float y = to_float(hash(i) ^ current_value);
        current_value = hash(current_value);
        float z = to_float(hash(i) ^ current_value);

        unit_vector = glm::vec3(x, y, z);
        u_length = glm::length(unit_vector);
    }

    return normalize(unit_vector);
}

vec3 random32::cube_vector(uvec3 i) {
    uint current_value = seed;

    glm::vec3 vector;
    current_value = hash(current_value);
    float x = to_float(hash(i) ^ current_value);
    current_value = hash(current_value);
    float y = to_float(hash(i) ^ current_value);
    current_value = hash(current_value);
    float z = to_float(hash(i) ^ current_value);

    vector = {x, y, z};

    return vector;
}

random::random(uint64_t init_seed) {
    seed = init_seed;
    value = seed;
}

float random::operator()() {
    value = hash(value);

    return to_float(value);
}

uint64_t random::next() {
    value = hash(value);

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
    uint hash_value = hash(uvec4(i, seed));

    return to_float(hash_value);
}

uint64_t random::hash_i(glm::vec<3, uint64_t> i) {
    uint64_t hash_value = hash(uvec4(i, seed));

    return hash_value;
}

vec3 random::unit_vector(glm::vec<3, uint64_t> i) {
    uint64_t current_value = seed;

    glm::vec3 unit_vector;
    float u_length = 10;
    while(u_length > 1.0 || u_length == 0.0) {
        current_value = hash(current_value);
        float x = to_float(hash(i) ^ current_value);
        current_value = hash(current_value);
        float y = to_float(hash(i) ^ current_value);
        current_value = hash(current_value);
        float z = to_float(hash(i) ^ current_value);

        unit_vector = glm::vec3(x, y, z);
        u_length = glm::length(unit_vector);
    }

    return normalize(unit_vector);
}

vec3 random::cube_vector(glm::vec<3, uint64_t> i) {
    uint64_t current_value = seed;

    glm::vec3 vector;
    current_value = hash(current_value);
    float x = to_float(hash(i) ^ current_value);
    current_value = hash(current_value);
    float y = to_float(hash(i) ^ current_value);
    current_value = hash(current_value);
    float z = to_float(hash(i) ^ current_value);

    vector = {x, y, z};

    return vector;
}

}