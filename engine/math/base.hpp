#include <string>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

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

const std::vector<std::string> integers = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "\xC2\x80", "\xC2\x81", "\xC2\x82", "\xC2\x83", "\xC2\x84", "\xC2\x85"};
const std::string integers_letters = "0123456789ABCDEF";

const double hexond_ratio = 86400.0 / 65536.0;

namespace axiom {

const float pi = 3.14159265358979323846f;
const float max_float = FLT_MAX;
const float sqrt2 = sqrt(2.0f);
const float sqrt3 = sqrt(3.0f);
    
std::string to_base(int32_t num, int base, bool use_i2 = false);
std::string to_base(int64_t num, int base, bool use_i2 = false);
std::string to_base(float num, int base, int max_float, bool use_i2 = false);
int from_base(std::string num, int base);

}