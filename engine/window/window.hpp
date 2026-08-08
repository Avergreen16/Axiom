
#pragma once

#include <input/input.hpp>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\orthonormalize.hpp"

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

struct GLFWwindow;

namespace axiom {

struct key_event {
    axiom::input_code key;
    int scancode;
    axiom::action action;
};

struct mouse_button_event {
    axiom::input_code button;
    axiom::action action;
};

struct scroll_event {
    double x;
    double y;
};

struct cursor_event {
    double xpos;
    double ypos;
};

struct text_event {
    uint32_t codepoint;
};

class window {
    public:
    GLFWwindow* window_handle = nullptr;
    
    float resize_border;
    ivec2 screen_size;
    ivec2 viewport_size;

    private: 
    
    ivec2 prev_pos;
    ivec2 prev_size;
    ivec2 prev_cursor_pos = ivec2(0);

    bool dragging = false;
    
    std::vector<axiom::key_event> key_events;
    std::vector<axiom::mouse_button_event> mouse_button_events;
    std::vector<axiom::scroll_event> scroll_events;
    std::vector<axiom::cursor_event> cursor_events;
    std::vector<axiom::text_event> text_events;
    
    void init_callbacks();

    public:

    bool should_close = false;
    
    std::unordered_map<axiom::input_code, bool> input_map;
    std::unordered_set<axiom::input_code> pressed_buttons;
    std::unordered_set<axiom::input_code> released_buttons;
    std::unordered_set<axiom::input_code> repeat_buttons;
    
    vec2 cursor_pos = vec2(0.0f);
    bool cursor_hidden = false;
    bool cursor_disabled = false;

    vec2 cursor_delta;
    float scroll_delta;
    std::string char_delta;

    window() = default;

    window(ivec2 position, ivec2 size, float border, std::string name = "Axiom", bool title_bar = true);

    void poll_events();

    bool is_fullscreen();
    bool is_maximized();
    bool is_windowed();
    bool is_minimized();

    void make_fullscreen();
    void make_maximized();
    void make_windowed();
    void make_minimized();

    void hide_cursor();
    void disable_cursor();
    void show_cursor();

    friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    friend void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
    friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    friend void character_callback(GLFWwindow* window, unsigned int codepoint);
    friend void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};

}