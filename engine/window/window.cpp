#include <glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <window/window.hpp>
#include <platform/platform.hpp>

namespace axiom {

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    axiom::window* w = (axiom::window*)glfwGetWindowUserPointer(window);

    axiom::action axiom_action;

    if(action == GLFW_PRESS) {
        axiom_action = axiom::action::PRESS;
    } else if(action == GLFW_RELEASE) {
        axiom_action = axiom::action::RELEASE;
    } else if(action == GLFW_REPEAT) {
        axiom_action = axiom::action::REPEAT;
    }

    w->key_events.push_back(key_event{glfw_input_map_key[key], scancode, axiom_action});
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    axiom::window* w = (axiom::window*)glfwGetWindowUserPointer(window);
    w->cursor_events.push_back(cursor_event{xpos, ypos});
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    axiom::window* w = (axiom::window*)glfwGetWindowUserPointer(window);
    
    axiom::action axiom_action;

    if(action == GLFW_PRESS) {
        axiom_action = axiom::action::PRESS;
    } else if(action == GLFW_RELEASE) {
        axiom_action = axiom::action::RELEASE;
    } else if(action == GLFW_REPEAT) {
        axiom_action = axiom::action::REPEAT;
    }

    w->mouse_button_events.push_back(mouse_button_event{glfw_input_map_mouse_button[button], axiom_action});
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    axiom::window* w = (axiom::window*)glfwGetWindowUserPointer(window);
    w->scroll_events.push_back(scroll_event{xoffset, yoffset});
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
    axiom::window* w = (axiom::window*)glfwGetWindowUserPointer(window);
    w->text_events.push_back(text_event{codepoint});
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    axiom::window* w = (axiom::window*)glfwGetWindowUserPointer(window);

    w->screen_size.x = width;
    w->screen_size.y = height;
    w->viewport_size.x = width + 1 * (width & 1);
    w->viewport_size.y = height + 1 * (height & 1);
}

void window::init_callbacks() {
    glfwSetWindowUserPointer(window_handle, this);

    glfwSetFramebufferSizeCallback(window_handle, framebuffer_size_callback);
    glfwSetKeyCallback(window_handle, key_callback);
    glfwSetCursorPosCallback(window_handle, cursor_pos_callback);
    glfwSetScrollCallback(window_handle, scroll_callback);
    glfwSetMouseButtonCallback(window_handle, mouse_button_callback);
    glfwSetCharCallback(window_handle, character_callback);
}

window::window(ivec2 position, ivec2 size, float border, std::string name, bool title_bar) {
    if(glfwInit() == GLFW_FALSE) {
        std::cout << "ERROR: GLFW failed to load.\n";
        exit(-1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    //

    window_handle = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);

    glfwMakeContextCurrent(window_handle);
    glfwShowWindow(window_handle);

    //if(!title_bar) remove_header(window_handle);

    int width, height;
    glfwGetWindowSize(window_handle, &width, &height);
    screen_size = {width, height};
    viewport_size = {screen_size.x + 1 * (screen_size.x & 1), screen_size.y + 1 * (screen_size.y & 1)};

    init_callbacks();

    if(!gladLoadGL()) {
        std::cout << "ERROR: GLAD failed to load.\n";
        glfwTerminate();
    }
    
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // icon 

    /*
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

    /////
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    */
}

void window::poll_events() {
    glfwPollEvents();
    
    if(glfwWindowShouldClose(window_handle)) {
        should_close = true;
    }

    //

    cursor_delta = glm::vec2(0.0f);
    scroll_delta = 0.0f;
    char_delta = "";

    pressed_buttons.clear();
    repeat_buttons.clear();
    released_buttons.clear();

    for(axiom::key_event event : key_events) {
        if(event.action == axiom::action::PRESS) {
            pressed_buttons.emplace(event.key);
            input_map[event.key] = true;
        }
        if(event.action == axiom::action::RELEASE) {
            released_buttons.emplace(event.key);
            input_map[event.key] = false;
        }
        if(event.action == axiom::action::REPEAT) {
            repeat_buttons.emplace(event.key);
        }
    }

    for(axiom::mouse_button_event event : mouse_button_events) {
        if(event.action == axiom::action::PRESS) {
            pressed_buttons.emplace(event.button);
            input_map[event.button] = true;
        }
        if(event.action == axiom::action::RELEASE) {
            released_buttons.emplace(event.button);
            input_map[event.button] = false;
        }
        if(event.action == axiom::action::REPEAT) {
            repeat_buttons.emplace(event.button);
        }
    }

    for(axiom::scroll_event event : scroll_events) {
        scroll_delta += event.y;
    }

    for(axiom::cursor_event event : cursor_events) {
        glm::vec2 new_cursor_pos = {event.xpos, screen_size.y - event.ypos - 1};
        cursor_delta += new_cursor_pos - cursor_pos;
        cursor_pos = new_cursor_pos;
    }

    for(axiom::text_event event : text_events) {
        char c = event.codepoint;
        char_delta += c;
    }

    key_events.clear();
    mouse_button_events.clear();
    scroll_events.clear();
    cursor_events.clear();
    text_events.clear();
}

bool window::is_fullscreen() {
    return axiom::is_fullscreen(window_handle);
}

bool window::is_maximized() {
    return axiom::is_maximized(window_handle);
}

bool window::is_windowed() {
    return !is_maximized() && !is_minimized();
}

bool window::is_minimized() {
    return axiom::is_minimized(window_handle);
}

/*
if(fullscreen) {
    glfwSetWindowMonitor(core.window.window, nullptr,  core.window.prev_pos.x, core.window.prev_pos.y, core.window.prev_pos.z, core.window.prev_pos.w, 0 );
    fullscreen = false;
    core.window.fullscreen = false;
    glfwRestoreWindow(core.window.window);
} else if(glfwGetWindowAttrib(core.window.window, GLFW_MAXIMIZED)) {
    glfwRestoreWindow(core.window.window);
} else { 
}
*/

void window::make_fullscreen() {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    ivec4 range = get_window_range(window_handle);

    prev_pos = range.xy();
    prev_size = range.zw();

    //

    glfwSetWindowMonitor(window_handle, monitor, 0, 0, mode->width, mode->height, 0);
}

void window::make_windowed() {
    glfwRestoreWindow(window_handle);
    glfwSetWindowMonitor(window_handle, nullptr, prev_pos.x, prev_pos.y, prev_size.x, prev_size.y, 0);
}

void window::make_minimized() {
    glfwIconifyWindow(window_handle);
}

void window::make_maximized() {
    ivec4 range = get_window_range(window_handle);

    prev_pos = range.xy();
    prev_size = range.zw();

    glfwMaximizeWindow(window_handle);
}

void window::hide_cursor() {
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    if(cursor_disabled) cursor_pos = prev_cursor_pos;

    cursor_hidden = true;
    cursor_disabled = false;
}

void window::disable_cursor() {
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if(!cursor_disabled) prev_cursor_pos = cursor_pos;
    
    cursor_disabled = true;
}

void window::show_cursor() {
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if(cursor_disabled) cursor_pos = prev_cursor_pos;
    
    cursor_hidden = false;
    cursor_disabled = false;
}

}