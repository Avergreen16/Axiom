#include <array>
#include <vector>
#include <experimental/simd>

namespace stdx = std::experimental;

#include <xsimd/xsimd.hpp>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

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

constexpr int PRIME_X = 73856093;
constexpr int PRIME_Y = 19349663;
constexpr int PRIME_Z = 83492791;

namespace axiom {

uint hash(ivec3 v);

uint hash(uint x);

uint hash(uvec2 v);

uint hash(uvec3 v);

uint hash(uvec4 v);

ulong hash(ulong x);

ulong hash(glm::vec<2, ulong> v);

ulong hash(glm::vec<3, ulong> v);

ulong hash(glm::vec<4, ulong> v);

float to_float(uint m);

int hash(ivec3 v, uint seed);

struct hash_coord {
    std::size_t operator()(const ivec2& v) const;
    std::size_t operator()(const ivec3& v) const;
    std::size_t operator()(const ivec4& v) const;

    std::size_t operator()(const vec2& v) const;
    std::size_t operator()(const vec3& v) const;
    std::size_t operator()(const vec4& v) const;
};

}