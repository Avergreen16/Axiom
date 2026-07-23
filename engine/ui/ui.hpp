#pragma once;

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\orthonormalize.hpp"

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
using ulong = uint64_t;

#include <string>
#include <map>
#include <memory>

#include <ecs/ecs.hpp>
#include <assets/assets.hpp>

namespace axiom {

extern vec3 color_physics;
extern vec3 color_editor;
extern vec3 color_debug;
extern vec3 color_lua;

extern vec3 color_red;
extern vec3 color_orange;
extern vec3 color_yellow;
extern vec3 color_green;
extern vec3 color_blue;
extern vec3 color_purple;
extern vec3 color_magenta;
extern vec3 color_rose;

bool includes(ivec2 point, ivec4 range);
bool includes(ivec4 range_a, ivec4 range_b);

vec4 intersect_range(vec4 a, vec4 b);

const ulong NULL_WIDGET = 0xFFFFFFFFFFFFFFFF;
const uint NULL_OPERATION = 0xFFFFFFFF;

struct ui_vertex {
    vec3 pos;
    vec2 tex_pos;
    vec4 color = vec4(1.0f);
    vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
    uint data = 0;
};

}
