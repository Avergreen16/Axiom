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

struct win32_window {
    LONG_PTR oldWndProc = NULL;
};
    
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

int get_caption_height(GLFWwindow* window) {
    return GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER) + GetSystemMetrics(SM_CYCAPTION);
}

ivec4 get_window_range(GLFWwindow* window) {
    int x, y;
    int width, height;

    glfwGetWindowPos(window, &x, &y);
    glfwGetWindowSize(window, &width, &height);

    int caption_y = get_caption_height(window);
    y += caption_y;
    height -= caption_y;

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
    else if(id == 5) SetCursor(LoadCursor(nullptr, IDC_HAND)); // normal arrow
}

ivec2 get_cursor_pos(GLFWwindow* window) {
    POINT pos;
    GetCursorPos(&pos);

    return {pos.x, pos.y};
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    window* win = reinterpret_cast<window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    win32_window* w = reinterpret_cast<win32_window*>(win->platform_data);

    bool fullscreen = is_fullscreen(win->window_handle);
    bool maximized = is_maximized(win->window_handle);
    bool minimized = is_minimized(win->window_handle);
    bool windowed = !fullscreen && !maximized && !minimized;

    static bool dragging = false;
    static POINT dragStartMouse;
    static RECT dragStartWindow;
    
    switch (msg) {   
        case WM_NCCALCSIZE: {
            if(!win->decorated) {
                int border = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                    
                NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
                //auto prev = params->rgrc[0];

                //float border = core.window.resize_border;

                //DefWindowProc(hwnd, msg, wParam, lParam);
                
                if(fullscreen) {
                    
                } else if(maximized) {
                    params->rgrc[0].top += border;
                    params->rgrc[0].bottom -= border;
                    params->rgrc[0].left += border;
                    params->rgrc[0].right -= border;
                } else {
                    params->rgrc[0].bottom -= border;
                    params->rgrc[0].left += border;
                    params->rgrc[0].right -= border;
                }

                return 0;
            }
        }
        case WM_NCHITTEST: {
            LRESULT result;
            if (DwmDefWindowProc(hwnd, msg, wParam, lParam, &result))
                return result;


            POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &p);

            RECT rect;
            GetClientRect(hwnd, &rect);

            const int BORDER = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

            if(windowed) {
                bool left   = p.x < 0.0f;
                bool right  = p.x > rect.right;
                bool top    = p.y < BORDER;
                bool bottom = p.y > rect.bottom;

                if (top && left) {
                    set_cursor(win->window_handle, 3);
                    return HTTOPLEFT;
                }
                if (top && right) {
                    set_cursor(win->window_handle, 4);
                    return HTTOPRIGHT;
                }
                if (bottom && left) return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;

                if (left) return HTLEFT;
                if (right) return HTRIGHT;
                if (top) {
                    set_cursor(win->window_handle, 2);
                    return HTTOP;
                }
                if (bottom) return HTBOTTOM;
            }

            return HTCLIENT;
        }

        case WM_SETCURSOR: {
            if(win->cursor_in_window()) return 0;
            break;
        }
    }

    return CallWindowProcW((WNDPROC)w->oldWndProc, hwnd, msg, wParam, lParam);
};

void handle_window_platform(window& win) {
    win32_window* ww = new win32_window;

    HWND hwnd = glfwGetWin32Window(win.window_handle);

    //
    
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style |= WS_THICKFRAME;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    //
    
    LONG_PTR before = GetWindowLongPtrW(
        hwnd,
        GWLP_WNDPROC
    );

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)&win);
    ww->oldWndProc = SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

    win.platform_data = (void*)ww;
    
    SetWindowPos(
        hwnd,
        nullptr,
        0, 0, 0, 0,
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_NOZORDER |
        SWP_FRAMECHANGED
    );
}

void window_platform_destruct(window& window) {
    delete reinterpret_cast<win32_window*>(window.platform_data);
}

bool window::cursor_in_window() {
    double x, y;
    int wx, wy;
    int width, height;
    glfwGetCursorPos(window_handle, &x, &y);
    glfwGetWindowSize(window_handle, &width, &height);

    wx = 0;
    wy = 0;

    if(!decorated && is_windowed()) {
        int border = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        wy += border;
    }

    return x >= wx && x < width && y >= wy && y < height;
}

}