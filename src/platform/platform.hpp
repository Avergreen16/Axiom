#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <deque>

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

struct GLFWwindow;

namespace axiom {

void copy_to_clipboard(GLFWwindow* window, std::string str);
std::string paste_from_clipboard(GLFWwindow* window);

void remove_header(GLFWwindow* window);

bool is_fullscreen(GLFWwindow* window);
bool is_maximized(GLFWwindow* window);
bool is_minimized(GLFWwindow* window);

ivec4 get_window_range(GLFWwindow* window);

void print_wsize(GLFWwindow* window);


void set_cursor(GLFWwindow* window, int id);

ivec2 get_cursor_pos(GLFWwindow* window);

}