#include <graphicsh.hpp>

#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <iostream>

#include "platform.hpp"
#include "include/ui.hpp"

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
    
void copy_to_clipboard(std::string str) {
    size_t len = str.size() + 1;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), str.c_str(), len);
    GlobalUnlock(hMem);

    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

std::string paste_from_clipboard() {
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

bool IsFullscreen(HWND hwnd) {
    MONITORINFO monitorInfo = { 0 };
    monitorInfo.cbSize = sizeof(MONITORINFO);
    
    // Get the monitor that the window is primarily on
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    if (!GetMonitorInfoW(hMonitor, &monitorInfo)) {
        return false;
    }

    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        return false;
    }

    bool is_fullscreen = (windowRect.left <= monitorInfo.rcMonitor.left &&
            windowRect.right >= monitorInfo.rcMonitor.right &&
            windowRect.top <= monitorInfo.rcMonitor.top &&
            windowRect.bottom >= monitorInfo.rcMonitor.bottom);

    // Compare window bounds with monitor bounds
    return is_fullscreen;
}

bool IsFullscreen(HWND hwnd, NCCALCSIZE_PARAMS* params) {
    MONITORINFO monitorInfo = { 0 };
    monitorInfo.cbSize = sizeof(MONITORINFO);
    
    // Get the monitor that the window is primarily on
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    if (!GetMonitorInfoW(hMonitor, &monitorInfo)) {
        return false;
    }

    bool is_fullscreen = (params->rgrc[0].left <= monitorInfo.rcMonitor.left &&
            params->rgrc[0].right >= monitorInfo.rcMonitor.right &&
            params->rgrc[0].top <= monitorInfo.rcMonitor.top &&
            params->rgrc[0].bottom >= monitorInfo.rcMonitor.bottom);

    // Compare window bounds with monitor bounds
    return is_fullscreen;
}

WNDPROC oldWndProc = NULL;
GLFWwindow* wd;

bool dragging = false;
POINT dragStartMouse;
RECT dragStartWindow;

ivec4 get_window_range(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);

    RECT windowRect;
    RECT clientRect;

    GetWindowRect(hwnd, &windowRect);
    GetClientRect(hwnd, &clientRect);

    /*
    int windowWidth  = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int clientWidth  = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    int borderWidth  = windowWidth - clientWidth;
    int borderHeight = windowHeight - clientHeight;
    */

    int border = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    int title_height = GetSystemMetrics(SM_CYCAPTION);

    if(oldWndProc != NULL) {
        windowRect.top += title_height + border;
        windowRect.bottom -= border;
        windowRect.left += border;
        windowRect.right -= border;
    }
    
    int windowWidth  = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    return {windowRect.left, windowRect.top, windowWidth, windowHeight};
}

bool b = false;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool fullscreen = IsFullscreen(hwnd);
    bool maximized = IsZoomed(hwnd) && !fullscreen;
    bool minimized = IsIconic(hwnd);
    bool windowed = !fullscreen && !maximized && !minimized;

    if(dragging) b = true;

    if(minimized) {
        dragging = false;
        ReleaseCapture();
    }

    switch (msg)
    {   
        case WM_NCCALCSIZE:
            if(wParam) {
                auto* p = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                //p->rgrc[0].left = p->rgrc[1].left;
                //p->rgrc[0].top = p->rgrc[1].top;
                //p->rgrc[0].right = p->rgrc[0].left + (p->rgrc[1].right - p->rgrc[1].left);
                //p->rgrc[0].bottom = p->rgrc[0].top + (p->rgrc[1].bottom - p->rgrc[1].top);
                
                /*
                SetWindowPos(
                    hwnd,
                    nullptr,
                    p->rgrc[0].left, p->rgrc[0].left, p->rgrc[0].left, p->rgrc[0].left
                    SWP_NOMOVE |
                    SWP_NOSIZE |
                    SWP_NOZORDER |
                    SWP_FRAMECHANGED
                );
                */

                return 0;
            }
            break;
        case WM_NCHITTEST: {
            LRESULT result;
            if (DwmDefWindowProc(hwnd, msg, wParam, lParam, &result))
                return result;


            POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &p);

            RECT rect;
            GetClientRect(hwnd, &rect);

            const int BORDER = 6;

            if(windowed) {
                bool left   = p.x < BORDER;
                bool right  = p.x > rect.right - BORDER;
                bool top    = p.y < BORDER;
                bool bottom = p.y > rect.bottom - BORDER;

                if (top && left) return HTTOPLEFT;
                if (top && right) return HTTOPRIGHT;
                if (bottom && left) return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;

                if (left) return HTLEFT;
                if (right) return HTRIGHT;
                if (top) return HTTOP;
                if (bottom) return HTBOTTOM;
            }

            return HTCLIENT;
        }
        case WM_LBUTTONDOWN:
        {
            POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ClientToScreen(hwnd, (POINT*)&p);
            
            RECT rect;
            GetWindowRect(hwnd, &rect);

            const int bar_height = 24;

            vec4 range = vec4(rect.left, rect.top, rect.right - bar_height * 3, rect.top + bar_height);

            bool in_title_bar = p.x > range.x && p.y > range.y && p.x < range.z && p.y < range.w;

            //
            if(in_title_bar && windowed) {
                dragging = true;

                GetCursorPos(&dragStartMouse);
                GetWindowRect(hwnd, &dragStartWindow);

                SetCapture(hwnd);
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            if (dragging)
            {
                POINT p;
                GetCursorPos(&p);

                int dx = p.x - dragStartMouse.x;
                int dy = p.y - dragStartMouse.y;

                SetWindowPos(
                    hwnd,
                    nullptr,
                    dragStartWindow.left + dx,
                    dragStartWindow.top + dy,
                    0, 0,
                    SWP_NOSIZE | SWP_NOZORDER
                );
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            dragging = false;
            ReleaseCapture();
            break;
        }
    }

    return CallWindowProc(oldWndProc, hwnd, msg, wParam, lParam);
}

void remove_header(GLFWwindow* window) {
    glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);

    HWND hwnd = glfwGetWin32Window(window);

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style |= WS_THICKFRAME;
    style |= WS_POPUP;
    style |= WS_MINIMIZEBOX;
    style |= WS_MAXIMIZEBOX;
    style |= WS_SYSMENU;

    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    
    oldWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

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

bool is_fullscreen(GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    return width >= mode->width && height >= mode->height;
}   

bool is_maximized(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);

    return is_fullscreen(window) || IsZoomed(hwnd);
}

bool is_minimized(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);

    return IsMinimized(hwnd);
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

}