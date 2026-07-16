#include "core.hpp"
#include "ecs.hpp"
#include "input.hpp"
#include "render.hpp"
#include "stb_image.h"
#include "stb_image_write.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <dwmapi.h>
#include <windowsx.h>

glm::mat3 create_rot_mat(glm::vec3 forward, glm::vec3 up) {
    glm::vec3 y = glm::normalize(forward - up * glm::dot(forward, up));
    glm::vec3 x = glm::normalize(glm::cross(y, up));

    return glm::mat3(x, y, up);
}

glm::mat3 z_rot(float angle) {
    glm::vec3 x = glm::vec3(cos(angle), sin(angle), 0);
    glm::vec3 y = glm::vec3(-sin(angle), cos(angle), 0);
    glm::vec3 z = glm::vec3(0, 0, 1);
    return glm::mat3(x, y, z);
}

glm::mat3 x_rot(float angle) {
    glm::vec3 x = glm::vec3(1, 0, 0);
    glm::vec3 y = glm::vec3(0, cos(angle), sin(angle));
    glm::vec3 z = glm::vec3(0, -sin(angle), cos(angle));
    return glm::mat3(x, y, z);
}

glm::mat3 y_rot(float angle) {
    glm::vec3 x = glm::vec3(cos(angle), 0, sin(angle));
    glm::vec3 y = glm::vec3(0, 0, 1);
    glm::vec3 z = glm::vec3(-sin(angle), 0, cos(angle));
    return glm::mat3(x, y, z);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    core.events.push_back(Key_event{key, scancode, action});
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    core.events.push_back(Cursor_event{xpos, ypos});
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    core.events.push_back(Mouse_button_event{button, action});
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    core.events.push_back(Scroll_event{xoffset, yoffset});
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
    core.events.push_back(Text_event{codepoint});
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    core.window.screen_size.x = width;
    core.window.screen_size.y = height ;
    core.window.viewport_size.x = width + 1 * (width & 1);
    core.window.viewport_size.y = height + 1 * (height & 1);

    glViewport(0, 0, core.window.viewport_size.x, core.window.viewport_size.y);

    Render_system& render_system = ecs.get_system<Render_system>();
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    
    if(glfwGetWindowAttrib(core.window.window, GLFW_ICONIFIED)) core.window.minimized = true;
    else core.window.minimized = false;

    gui_system.call();
    render_system.call();

    //core.render();
}

WNDPROC oldWndProc;
GLFWwindow* wd;

bool dragging = false;
POINT dragStartMouse;
RECT dragStartWindow;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool fullscreen = core.window.fullscreen;
    bool maximized = IsZoomed(hwnd) && !fullscreen;
    bool minimized = IsIconic(hwnd);
    bool windowed = !fullscreen && !maximized && !minimized;

    if(minimized) {
        dragging = false;
        ReleaseCapture();
    }

    switch (msg)
    {   
        case WM_NCCALCSIZE:
        {
            if (wParam)
            {   
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
            break;
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

void handle(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

    //style &= ~WS_CAPTION;
    //style &= ~WS_SYSMENU;
    //style &= ~WS_MINIMIZEBOX;
    //style &= ~WS_MAXIMIZEBOX;
    //style &= ~WS_BORDER;
    //style &= ~WS_DLGFRAME;
    //style |= WS_THICKFRAME;

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

Window::Window(glm::ivec2 size) {
    window = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);

    glfwMakeContextCurrent(window);

    handle(window);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    screen_size = {width, height};
    viewport_size = {screen_size.x + 1 * (screen_size.x & 1), screen_size.y + 1 * (screen_size.y & 1)};
    

    if(!gladLoadGL()) {
        std::cout << "ERROR: GLAD failed to load.\n";
        glfwTerminate();
    }

    // icon 

    ivec3 img_data;
    stbi_set_flip_vertically_on_load(false);
    uint8_t* pixels = stbi_load("resources/textures/axiom_icon.png", &img_data.x, &img_data.y, &img_data.z, 4);
    stbi_set_flip_vertically_on_load(true);

    GLFWimage images[2];
    images[0].width = img_data.x;
    images[0].height = img_data.y;
    images[0].pixels = pixels;
    glfwSetWindowIcon(window, 1, images); // Set the window icon
    stbi_image_free(pixels); // Free image data after setting

    // init glad and set viewport
    
    glViewport(0, 0, viewport_size.x, viewport_size.y);
}

void Window::init_callbacks() {
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCharCallback(window, character_callback);
}

double Core::get_delta_time() {
    prev_time = current_time;
    current_time = get_time();
    double delta_time = current_time - prev_time;

    return delta_time;
}

Core::Core(int num_threads) : thread_pool(num_threads) {
    lua_wrapper.init();
}

glm::mat4 Core::get_infinite_proj_matrix(glm::ivec2 window_size, float fov, float near_plane, float np, float fp) {
    float aspect = float(window_size.x) / window_size.y;
    float focal_length = 1.0f / tan(fov * (M_PI / 180) * 0.5f);
    float A = -fp;
    float B = (np - fp) * near_plane;

    glm::mat4 matrix = glm::identity<glm::mat4>();

    matrix[3][3] = 0;
    matrix[0][0] = focal_length;
    matrix[1][1] = focal_length;
    matrix[2][2] = A;
    matrix[3][2] = B;
    matrix[2][3] = -1;

    if(aspect > 1.0) matrix[1][1] *= aspect;
    else matrix[0][0] /= aspect;

    return matrix;
}

glm::mat4 Core::get_proj_matrix_ortho(glm::ivec2 window_size, float near_plane, float far_plane, float width, float height) {
    float right = width * 0.5;
    float left = -width * 0.5;
    float top = height * 0.5;
    float bottom = -height * 0.5;

    mat4 matrix = glm::identity<mat4>();

    matrix[0][0] = 2.0f / (right - left);
    matrix[1][1] = 2.0f / (top - bottom);
    matrix[2][2] = -1.0f / (near_plane - far_plane);
    matrix[3][2] = - (far_plane + near_plane) / (far_plane - near_plane);

    mat4 translate_forward = translate(vec3{0, 0, 0.5f});

    matrix = translate_forward * matrix;

    return matrix;
}

glm::mat4 Core::get_infinite_proj_matrix_ortho(glm::ivec2 window_size, float near_plane, float far_plane, float np, float fp, float width, float height) {
    float right = width * 0.5;
    float left = -width * 0.5;
    float top = height * 0.5;
    float bottom = -height * 0.5;

    mat4 matrix = glm::identity<mat4>();

    matrix[0][0] = 2.0f / (right - left);
    matrix[1][1] = 2.0f / (top - bottom);
    matrix[2][2] = -1.0f / (near_plane - far_plane);
    matrix[3][2] = - (far_plane + near_plane) / (far_plane - near_plane);

    mat4 translate_forward = translate(vec3{0, 0, 0.5f});

    matrix = translate_forward * matrix;

    return matrix;
}

glm::mat4 Core::get_view_matrix(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
    return glm::lookAt({0, 0, 0}, direction, up) * glm::inverse(glm::translate(position));
}

glm::mat4 Core::get_model_matrix(pvec3 position, pvec3 origin) {
    vec3 t = position - origin;
    return glm::translate(t);
}

glm::mat4 Core::get_model_matrix_inv(pvec3 position, pvec3 origin) {
    vec3 t = position - origin;
    return glm::translate(-t);
}

bool Core::time_step(double step) {
    double d_prev = prev_time / step;
    double d_current = current_time / step;

    return floor(d_prev) != floor(d_current);
}

void Core::handle_events() {
    cursor_delta = glm::vec2(0.0f);
    scroll_delta = 0.0f;
    char_delta = "";

    pressed_buttons.clear();
    repeat_buttons.clear();
    released_buttons.clear();

    for(Event& e : core.events) {
        switch(e.index()) {
            case 0: {
                Key_event& k = std::get<Key_event>(e);

                if(k.action == GLFW_PRESS) {
                    pressed_buttons.emplace(k.key);
                    key_map[k.key] = true;
                }
                if(k.action == GLFW_RELEASE) {
                    released_buttons.emplace(k.key);
                    key_map[k.key] = false;
                }
                if(k.action == GLFW_REPEAT) {
                    repeat_buttons.emplace(k.key);
                }

                break;
            }
            case 1: {
                Mouse_button_event& m = std::get<Mouse_button_event>(e);
                
                if(m.action == GLFW_PRESS) {
                    pressed_buttons.emplace(m.button);
                    key_map[m.button] = true;
                }
                if(m.action == GLFW_RELEASE) {
                    released_buttons.emplace(m.button);
                    key_map[m.button] = false;
                }
                if(m.action == GLFW_REPEAT) {
                    repeat_buttons.emplace(m.button);
                }

                break;
            }
            case 2: {
                Scroll_event& s = std::get<Scroll_event>(e);
                scroll_delta += s.y;

                break;
            }
            case 3: {
                Cursor_event& c = std::get<Cursor_event>(e);
                glm::vec2 new_cursor_pos = {c.xpos, core.window.screen_size.y - c.ypos - 1};
                cursor_delta += new_cursor_pos - cursor_pos;
                cursor_pos = new_cursor_pos;

                break;
            } case 4: {
                Text_event& t = std::get<Text_event>(e);

                char c = t.codepoint;
                char_delta += c;

                break;
            }
        }
    }
}

Core core(0);

void Lua_Wrapper::init() {
    // start lua
    state = luaL_newstate();
    luaL_openlibs(state);
}

void Lua_Wrapper::run_file(std::string path) {
    int r = luaL_dofile(state, path.c_str());
    
    if(r != LUA_OK) {
        std::string errormsg = lua_tostring(state, -1);
        std::cout << errormsg << std::endl;
    }
}

static int dispatch(lua_State* state)
{
    auto* func = static_cast<std::function<int(lua_State*)>*>(
        lua_touserdata(state, lua_upvalueindex(1))
    );

    return (*func)(state);
}

void Lua_Wrapper::expose(lua_function func, std::string name) {
    std::unique_ptr<lua_function> pfunc = std::make_unique<lua_function>(std::move(func));
    lua_function* ptr = pfunc.get();
    functions.push_back(std::move(pfunc));

    lua_pushlightuserdata(state, ptr);
    lua_pushcclosure(state, dispatch, 1);
    lua_setglobal(state, name.c_str());
}