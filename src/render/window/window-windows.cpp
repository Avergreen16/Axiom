#include <graphicsh.hpp>

#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <iostream>

#include <render/window/window.hpp>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

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
    size_t len = str.size() + 1;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), str.c_str(), len);
    GlobalUnlock(hMem);

    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

std::string paste_from_clipboard(GLFWwindow* window) {
    if (!OpenClipboard(nullptr)) return "";

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr)
    {
        CloseClipboard();
        return "";
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText == nullptr)
    {
        CloseClipboard();
        return "";
    }

    std::string text(pszText);

    GlobalUnlock(hData);
    CloseClipboard();

    return text;
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

void print_wsize(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);

    RECT wr, cr;
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);

    printf(
        "RENDER    window=(%ld, %ld) %ldx%ld  client=(%ld, %ld) %ldx%ld\n",
        wr.left,
        wr.top,

        wr.right - wr.left,
        wr.bottom - wr.top,

        cr.left,
        cr.top,

        cr.right - cr.left,
        cr.bottom - cr.top
    );
}

void set_cursor(GLFWwindow* window, int id) {
    if(id == 0) SetCursor(LoadCursor(nullptr, IDC_ARROW)); // normal arrow
    else if(id == 1) SetCursor(LoadCursor(nullptr, IDC_SIZEWE)); // horizontal resize
    else if(id == 2) SetCursor(LoadCursor(nullptr, IDC_SIZENS)); // vertical resize
    else if(id == 3) SetCursor(LoadCursor(nullptr, IDC_SIZENWSE)); // diagonal
    else if(id == 4) SetCursor(LoadCursor(nullptr, IDC_SIZENESW)); // diagonal
}

ivec2 get_cursor_pos(GLFWwindow* window) {
    POINT pos;
    GetCursorPos(&pos);

    return {pos.x, pos.y};
}

}