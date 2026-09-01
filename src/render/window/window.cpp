#include <graphicsh.hpp>

#include <iostream>

#include <render/window/input.hpp>
#include <render/window/window.hpp>

#include <include/core.hpp>

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
    w->viewport_size.x = width;
    w->viewport_size.y = height;

    glViewport(0, 0, w->viewport_size.x, w->viewport_size.y);

    w->on_resize();
}

void iconify_callback(GLFWwindow* window, int flag) {
    axiom::window* w = (axiom::window*)glfwGetWindowUserPointer(window);

    if(flag) {
        w->restore();
        glfwMaximizeWindow(window);
    }
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

window::window(ivec2 position, ivec2 size, float border, std::string name, bool decorated) {
    if(glfwInit() == GLFW_FALSE) {
        std::cout << "ERROR: GLFW failed to load.\n";
        exit(-1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if(!decorated) glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    this->decorated = decorated;
    
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    //

    window_handle = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);

    glfwMakeContextCurrent(window_handle);
    glfwShowWindow(window_handle);

    if(!decorated) remove_header(window_handle);

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

void window::clear_events() {
    pressed_buttons.clear();
    repeat_buttons.clear();
    released_buttons.clear();
    key_events.clear();
    mouse_button_events.clear();
    scroll_events.clear();
    cursor_events.clear();
    text_events.clear();
    //for(auto& [k, b] : input_map) b = false;
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

    static int operation;
    static ivec2 global_cursor_pos;
    static ivec2 prev_global_cursor_pos;
    static ivec2 global_cursor_delta;

    if(!decorated) {
        prev_global_cursor_pos = global_cursor_pos;
        global_cursor_pos = get_cursor_pos(window_handle);
        global_cursor_delta = global_cursor_pos - prev_global_cursor_pos;

        ivec4 s = axiom::get_window_range(window_handle);
        int border = 10;

        if(is_windowed()) {
            if(click_capture) {
                if(operation == 0) {
                    s.x += global_cursor_delta.x;
                    s.z -= global_cursor_delta.x;
                    
                    s.y += global_cursor_delta.y;
                    s.w -= global_cursor_delta.y;

                    std::cout << s.x << " " << s.y << "\n";

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 3);
                } else if(operation == 1) { 
                    s.z += global_cursor_delta.x;
                    
                    s.y += global_cursor_delta.y;
                    s.w -= global_cursor_delta.y;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 4);
                } else if(operation == 2) {
                    s.x += global_cursor_delta.x;
                    s.z -= global_cursor_delta.x;
                    
                    s.w += global_cursor_delta.y;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 4);
                } else if(operation == 3) {
                    s.z += global_cursor_delta.x;
                    
                    s.w += global_cursor_delta.y;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 3);
                } else if(operation == 4) {
                    s.x += global_cursor_delta.x;
                    s.z -= global_cursor_delta.x;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 1);
                } else if(operation == 5) {
                    s.z += global_cursor_delta.x;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 1);
                } else if(operation == 6) {
                    s.y += global_cursor_delta.y;
                    s.w -= global_cursor_delta.y;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 2);
                } else if(operation == 7) {
                    s.w += global_cursor_delta.y;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 2);
                } else if(operation == 8) {
                    s.x += global_cursor_delta.x;

                    s.y += global_cursor_delta.y;

                    glfwSetWindowSize(window_handle, s.z, s.w);
                    glfwSetWindowPos(window_handle, s.x, s.y);
                    
                    show_cursor();
                    set_cursor(window_handle, 0);
                }
            } else {
                if(click_capture == true) {
                    hide_cursor();
                }

                click_capture = false;
            }
            
            if(global_cursor_pos.x < s.x + border && global_cursor_pos.y < s.y + border) {
                show_cursor();
                set_cursor(window_handle, 3);

                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 0;
                }
            } else if(global_cursor_pos.x > s.x + s.z - border && global_cursor_pos.y < s.y + border) {
                show_cursor();
                set_cursor(window_handle, 4);
                
                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 1;
                }
            } else if(global_cursor_pos.x < s.x + border && global_cursor_pos.y > s.y + s.w - border) {
                show_cursor();
                set_cursor(window_handle, 4);
                
                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 2;
                }
            } else if(global_cursor_pos.x > s.x + s.z - border && global_cursor_pos.y > s.y + s.w - border) {
                show_cursor();
                set_cursor(window_handle, 3);
                
                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 3;
                }
            } else if(global_cursor_pos.x < s.x + border) {
                show_cursor();
                set_cursor(window_handle, 1);
                
                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 4;
                }
            } else if(global_cursor_pos.x > s.x + s.z - border) {
                show_cursor();
                set_cursor(window_handle, 1);
                
                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 5;
                }
            } else if(global_cursor_pos.y < s.y + border) {
                show_cursor();
                set_cursor(window_handle, 2);
                
                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 6;
                }
            } else if(global_cursor_pos.y > s.y + s.w - border) {
                show_cursor();
                set_cursor(window_handle, 2);
                
                hover_capture = true;

                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 7;
                }
            } else if(global_cursor_pos.y < s.y + border + 8 && is_windowed()) {
                show_cursor();
                set_cursor(window_handle, 0);
                
                hover_capture = true;
                
                if(pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    click_capture = true;
                    operation = 8;
                }
            } else {
                if(hover_capture == true) {
                    hide_cursor();
                }
                
                hover_capture = false;
            }
            
            if(!input_map[axiom::input_code::MOUSE_LEFT]) {
                if(click_capture == true) {
                    hide_cursor();
                }
                
                click_capture = false;
            }
        }

        screen_size = s.zw();
        viewport_size = s.zw();
    }
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
    rs.push_back(range);

    glfwSetWindowMonitor(window_handle, monitor, 0, 0, mode->width, mode->height, 60);
}

void window::make_windowed() {
    glfwRestoreWindow(window_handle);
    restore();
}

void window::make_minimized() {
    ivec4 range = get_window_range(window_handle);
    rs.push_back(range);

    glfwIconifyWindow(window_handle);
}

void window::make_maximized() {
    ivec4 range = get_window_range(window_handle);
    rs.push_back(range);

    glfwMaximizeWindow(window_handle);
}

void window::restore() {
    if(rs.size()) {
        glfwSetWindowMonitor(window_handle, nullptr, rs.back().x, rs.back().y, rs.back().z, rs.back().w, 60);
        rs.pop_back();
    }
}

void window::hide_cursor() {
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    if(cursor_disabled) cursor_pos = prev_cursor_pos;

    cursor_hidden = true;
    cursor_disabled = false;

    set_cursor(window_handle, -1);
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