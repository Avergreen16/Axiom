#include <iostream>

#include <graphicsh.hpp>
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
    
void copy_to_clipboard(GLFWwindow* window, std::string str) {
    glfwSetClipboardString(window, str.data());
}

std::string paste_from_clipboard(GLFWwindow* window) {
    return glfwGetClipboardString(window);
}

ivec4 get_window_range(GLFWwindow* window) {
    int x, y;
    int width, height;

    glfwGetWindowPos(window, &x, &y);
    glfwGetWindowSize(window, &width, &height);

    return ivec4(x, y, width, height);
}

void remove_header(GLFWwindow* window) {
    glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
}

bool is_fullscreen(GLFWwindow* window) {
    return glfwGetWindowMonitor(window) != nullptr;
}

bool is_maximized(GLFWwindow* window) {
    return glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
}

bool is_minimized(GLFWwindow* window) {
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED);
}

void set_cursor(GLFWwindow* window, int id) {
    static GLFWcursor* arrow_cursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    static GLFWcursor* resize_ew_cursor = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    static GLFWcursor* resize_ns_cursor = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    static GLFWcursor* resize_nwse_cursor = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    static GLFWcursor* resize_nesw_cursor = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);

    if(id == 0) glfwSetCursor(window, arrow_cursor); // normal arrow
    else if(id == 1) glfwSetCursor(window, resize_ew_cursor); // horizontal resize
    else if(id == 2) glfwSetCursor(window, resize_ns_cursor); // vertical resize
    else if(id == 3) glfwSetCursor(window, resize_nwse_cursor); // diagonal
    else if(id == 4) glfwSetCursor(window, resize_nesw_cursor); // diagonal

    if(id == -1) glfwSetCursor(window, nullptr);
}

ivec2 get_cursor_pos(GLFWwindow* window) {
    ivec4 window_range = get_window_range(window);

    double mouse_x, mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    return {mouse_x + window_range.x, mouse_y + window_range.y};
}

}