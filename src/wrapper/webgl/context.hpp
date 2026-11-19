
#pragma once

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
#include <vector>

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

enum class input_code {
    // numbers
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    // letters
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

    // F keys
    KEY_ESCAPE,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24, KEY_F25,
    
    // navigation
    KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW, KEY_RIGHT_ARROW, KEY_DELETE, KEY_END, KEY_INSERT, KEY_HOME, KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_SCROLL_LOCK, KEY_PAUSE, KEY_ENTER, KEY_BACKSPACE,

    // symbols
    KEY_GRAVE, KEY_TAB, KEY_SPACE, KEY_MINUS, KEY_EQUALS, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_SEMICOLON, KEY_QUOTE, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_BACKSLASH,
    KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_LEFT_CTRL, KEY_RIGHT_CTRL, KEY_LEFT_SUPER, KEY_RIGHT_SUPER, KEY_LEFT_ALT, KEY_RIGHT_ALT,
    KEY_CAPS_LOCK, KEY_PRINT_SCREEN,

    // numpad
    KEY_PAD_0, KEY_PAD_1, KEY_PAD_2, KEY_PAD_3, KEY_PAD_4, KEY_PAD_5, KEY_PAD_6, KEY_PAD_7, KEY_PAD_8, KEY_PAD_9,
    KEY_PAD_POINT, KEY_NUM_LOCK, KEY_PAD_ENTER, KEY_PAD_ADD, KEY_PAD_SUBTRACT, KEY_PAD_MULTIPLY, KEY_PAD_DIVIDE,
    
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_CENTER,
    MOUSE_1,
    MOUSE_2,
    MOUSE_3,
    MOUSE_4,
    MOUSE_5,
    MOUSE_6,
    MOUSE_7,
    MOUSE_8,

    // unknown
    UNKNOWN = 0
};

enum class action {
    PRESS,
    RELEASE,
    REPEAT
};

enum class cursor_mode{
    DEFAULT,
    CLICK,
    TEXT,
    DRAG_T,
    DRAG_TR,
    DRAG_R,
    DRAG_BR,
    DRAG_B,
    DRAG_BL,
    DRAG_L,
    DRAG_TL,
};

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

struct window_state {
    bool iconified;
    ivec2 size;
    ivec2 position;
};

struct iconify_event {
    bool flag;
};

struct resize_event {
    ivec2 prev_size;
    ivec2 new_size;
};

struct touch {
    uint identifier;
    vec2 position;
    vec2 delta = vec2(0.0f);
};

struct context {
    struct implementation;
    std::unique_ptr<implementation> imp;

    public:
    
    float resize_border;
    ivec2 size;
    
    bool should_close = false;
    
    bool click_capture = false;
    bool hover_capture = false;
    bool dragging = false;
    bool move = false;
    
    std::vector<ivec4> rs;
    vec2 prev_cursor_pos = vec2(0.0f);
    
    std::vector<axiom::key_event> key_events;
    std::vector<axiom::mouse_button_event> mouse_button_events;
    std::vector<axiom::scroll_event> scroll_events;
    std::vector<axiom::cursor_event> cursor_events;
    std::vector<axiom::text_event> text_events;
    std::vector<axiom::iconify_event> iconify_events;
    std::vector<axiom::resize_event> resize_events;

    //
    
    vec2 cursor_delta;
    float scroll_delta;
    std::string char_delta;

    vec2 cursor_pos = vec2(0.0f);
    bool cursor_hidden = false;
    bool cursor_disabled = false;

    std::unordered_map<uint, touch> touches;

    std::unordered_map<axiom::input_code, bool> input_map;
    std::unordered_set<axiom::input_code> pressed_buttons;
    std::unordered_set<axiom::input_code> released_buttons;
    std::unordered_set<axiom::input_code> repeat_buttons;

    //

    //context() = default;

    context();
    ~context();

    context(context& ctx) = delete;
    context& operator=(context& ctx) = delete;
    
    bool cursor_in_window();

    void hide_cursor();
    void disable_cursor();
    void show_cursor();
    void set_loop(void(*loop)());
    void create_shaders();
    void set_cursor(cursor_mode mode);

    bool is_fullscreen();
    bool is_maximized();
    bool is_windowed();
    bool is_minimized();

    /*
    void poll_events();



    void make_fullscreen();
    void make_maximized();
    void make_windowed();
    void make_minimized();
    void restore();

    */

    void clear_events();

    //typedef bool (*em_key_callback_func)(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
    /*
    friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    friend void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
    friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    friend void character_callback(GLFWwindow* window, unsigned int codepoint);
    friend void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    friend void iconify_callback(GLFWwindow* window, int flag);
    */
};

//void copy_to_clipboard(GLFWwindow* window, std::string str);
//std::string paste_from_clipboard(GLFWwindow* window);

//bool is_fullscreen(GLFWwindow* window);
//bool is_maximized(GLFWwindow* window);
//bool is_minimized(GLFWwindow* window);

//ivec4 get_window_range(GLFWwindow* window);
//int get_caption_height(GLFWwindow* window);

//void print_wsize(GLFWwindow* window);

//void set_cursor(GLFWwindow* window, int id);

//ivec2 get_cursor_pos(GLFWwindow* window);

}