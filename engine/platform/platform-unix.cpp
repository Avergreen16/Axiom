#include <iostream>

#include "platform.hpp"

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
    
void copy_to_clipboard(std::string str) {

}

std::string paste_from_clipboard() {
    return "UNIX PLACEHOLDER";
}

ivec4 get_window_range(GLFWwindow* window) {
    return ivec4(0, 0, 100, 100);
}

void remove_header(GLFWwindow* window) {

}

bool is_fullscreen(GLFWwindow* window) {
    return false;
}

bool is_maximized(GLFWwindow* window) {
    return true;
}

bool is_minimized(GLFWwindow* window) {
    return false;
}

}