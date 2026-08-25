#include <array>
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

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;

namespace axiom {

extern std::array<vec3, 16> perlin_vectors;

vec3 get_vec(int i);

struct noise_gen {
    static std::array<int, 256> hash_table;
    static std::array<vec3, 16> perlin_vectors;
    static std::array<vec3, 256> voronoi_vectors;

    static uint8_t hash_with_table(uvec3 i);

    static float voronoi_noise(glm::vec3 position, float period, uint32_t seed);

    static float perlin_noise(glm::vec3 position, float period, uint32_t octaves, uint32_t seed, float persistance = 0.5f);

    static float ridged_perlin_noise(glm::vec3 position, float period, uint32_t octaves, uint32_t seed, float persistance = 0.5f);

    static std::vector<float> perlin_noise(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    static std::vector<float> ridged_perlin_noise(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    static std::vector<float> perlin_noise_normalized(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    static std::vector<float> ridged_perlin_noise_normalized(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    void generate_noise(glm::ivec4 index, float* ptr);
};

}