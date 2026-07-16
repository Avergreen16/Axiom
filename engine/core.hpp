#pragma once
#include "wrapper.hpp"
#include "pnum.hpp"
#include "random.hpp"
#include "utility.hpp"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

constexpr int start_x = 768;
constexpr int start_y = 768;
constexpr float border = 6;
const std::string name = "Axiom";

template<typename type>
type& get(std::unique_ptr<type>& a) {
    return (*a.get());
}

template<typename type_before, typename type_after>
type_after& convert(type_before& b) {
    return *(type_after*)&b;
}

float smoothstep(float a);

glm::mat3 create_rot_mat(glm::vec3 forward, glm::vec3 up);

glm::mat3 z_rot(float angle);

glm::mat3 x_rot(float angle);

glm::mat3 y_rot(float angle);

struct Window {
    GLFWwindow* window = nullptr;
    float resize_border = border;
    glm::ivec2 screen_size = {start_x, start_y};
    glm::ivec2 viewport_size = {start_x, start_y};

    bool fullscreen = false;
    bool minimized = false;
    glm::ivec4 prev_pos = {0, 0, start_x, start_y};

    bool dragging = false;

    Window() = default;

    Window(glm::ivec2 size);

    void init_callbacks();
};

using lua_function = std::function<int(lua_State*)>;

struct Lua_Wrapper {
    lua_State* state;
    std::vector<std::unique_ptr<lua_function>> functions;

    void init();
    void run_file(std::string path);
    void expose(lua_function func, std::string name);
};

enum event_types:uint16_t{KEY, MOUSE_BUTTON, SCROLL, CURSOR, TEXT};

struct Key_event {
    int key;
    int scancode;
    int action;
};

struct Mouse_button_event {
    int button;
    int action;
};

struct Scroll_event {
    double x;
    double y;
};

struct Cursor_event {
    double xpos;
    double ypos;
};

struct Text_event {
    uint32_t codepoint;
};

using Event = std::variant<Key_event, Mouse_button_event, Scroll_event, Cursor_event, Text_event>;

struct Model;
struct Mesh;

struct Core {
    bool game_running = true;
    Window window;
    Lua_Wrapper lua_wrapper;
    
    double start_time = 0;
    double current_time = 0;
    double prev_time = 0;

    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, std::shared_ptr<Model>> models;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;

    std::vector<Event> events;

    // input handling
    std::unordered_map<int, bool> key_map;
    std::unordered_set<GLenum> pressed_buttons;
    std::unordered_set<GLenum> released_buttons;
    std::unordered_set<GLenum> repeat_buttons;
    
    vec2 cursor_pos = vec2(0.0f);
    vec2 cursor_delta;
    float scroll_delta;
    bool cursor_disabled = false;

    std::string char_delta;

    // matrices
    glm::mat4 proj = glm::identity<mat4>();
    glm::mat4 view = glm::identity<mat4>();

    double delta_time = 1;

    // components
    Random random = Random(uint32_t(get_absolute_time() * 100.0));
    Thread_pool thread_pool;
    //Thread_pool thread_pool_b;

    int number_base = 10;
    
    Profiler profiler;

    Core(int num_threads);

    void init();

    double get_delta_time();

    glm::mat4 get_infinite_proj_matrix(glm::ivec2 window_size, float fov, float near, float np, float fp);

    glm::mat4 get_proj_matrix_ortho(glm::ivec2 window_size, float near, float far, float width, float height);

    glm::mat4 get_infinite_proj_matrix_ortho(glm::ivec2 window_size, float near, float far, float np, float fp, float width, float height);

    glm::mat4 get_view_matrix(glm::vec3 position, glm::vec3 direction, glm::vec3 up);

    glm::mat4 get_model_matrix(pvec3 position, pvec3 origin);
    glm::mat4 get_model_matrix_inv(pvec3 position, pvec3 origin);

    bool time_step(double step);

    void handle_events();
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void character_callback(GLFWwindow* window, unsigned int codepoint);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

struct aiMesh;
struct aiNode;
struct aiScene;
struct aiAnimation;

extern Core core;