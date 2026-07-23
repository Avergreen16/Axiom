#pragma once;
#include "hash.hpp"

#include <array>
#include <vector>
#include <experimental/simd>

namespace stdx = std::experimental;

#include <xsimd/xsimd.hpp>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\orthonormalize.hpp"

#include <variant>
#include <unordered_set>
#include <string>

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;
using ulong = uint64_t;

namespace axiom {

//pnum to_pnum(uint m);

struct random {
    ulong seed;
    ulong value;
    
    random(ulong init_seed);

    random(const random& r) = default;
    random& operator=(const random& r) = default;
    random(random&& r) = default;
    random& operator=(random&& r) = default;

    float operator()();
    ulong next();
    vec3 unit_vector();

    float operator()(glm::vec<3, ulong> i);
    ulong hash_i(glm::vec<3, ulong> i);
    vec3 unit_vector(glm::vec<3, ulong> i);
    vec3 cube_vector(glm::vec<3, ulong> i);
};

struct random32 {
    uint seed;
    uint value;

    random32(uint init_seed);

    random32(const random32& r) = default;
    random32& operator=(const random32& r) = default;
    random32(random32&& r) = default;
    random32& operator=(random32&& r) = default;

    float operator()();
    uint next();
    vec3 unit_vector();

    float operator()(uvec3 i);
    uint hash_i(uvec3 i);
    vec3 unit_vector(uvec3 i);
    vec3 cube_vector(uvec3 i);
};

}