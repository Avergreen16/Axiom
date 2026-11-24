#include <wrapper/opengl/graphicsh.hpp>

#include <iostream>

#include <wrapper/opengl/input.hpp>
#include <wrapper/opengl/context.hpp>

#include <include/core.hpp>

namespace axiom {

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    axiom::context* w = (axiom::context*)glfwGetWindowUserPointer(window);

    axiom::action axiom_action;

    if(action == GLFW_PRESS) {
        axiom_action = axiom::action::PRESS;
    } else if(action == GLFW_RELEASE) {
        axiom_action = axiom::action::RELEASE;
    } else if(action == GLFW_REPEAT) {
        axiom_action = axiom::action::REPEAT;
    }

    w->key_events.push_back(key_event{input_map_key[key], scancode, axiom_action});
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    axiom::context* w = (axiom::context*)glfwGetWindowUserPointer(window);
    w->cursor_events.push_back(cursor_event{xpos, ypos});
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    axiom::context* w = (axiom::context*)glfwGetWindowUserPointer(window);
    
    axiom::action axiom_action;

    if(action == GLFW_PRESS) {
        axiom_action = axiom::action::PRESS;
    } else if(action == GLFW_RELEASE) {
        axiom_action = axiom::action::RELEASE;
    } else if(action == GLFW_REPEAT) {
        axiom_action = axiom::action::REPEAT;
    }

    w->mouse_button_events.push_back(mouse_button_event{input_map_mouse_button[button], axiom_action});
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    axiom::context* w = (axiom::context*)glfwGetWindowUserPointer(window);
    w->scroll_events.push_back(scroll_event{xoffset, yoffset});
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
    axiom::context* w = (axiom::context*)glfwGetWindowUserPointer(window);
    w->text_events.push_back(text_event{codepoint});
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    axiom::context* w = (axiom::context*)glfwGetWindowUserPointer(window);

    w->size.x = width;
    w->size.y = height;

    glViewport(0, 0, w->size.x, w->size.y);

    w->on_resize();
}

void iconify_callback(GLFWwindow* window, int flag) {
    axiom::context* w = (axiom::context*)glfwGetWindowUserPointer(window);

    if(flag) {
        w->restore();
        glfwMaximizeWindow(window);
    }
}

void context::init_callbacks() {
    glfwSetWindowUserPointer(window_handle, this);

    glfwSetFramebufferSizeCallback(window_handle, framebuffer_size_callback);
    glfwSetKeyCallback(window_handle, key_callback);
    glfwSetCursorPosCallback(window_handle, cursor_pos_callback);
    glfwSetScrollCallback(window_handle, scroll_callback);
    glfwSetMouseButtonCallback(window_handle, mouse_button_callback);
    glfwSetCharCallback(window_handle, character_callback);
}

context::context(ivec2 position, ivec2 size, float border, std::string name, bool decorated) {
    if(glfwInit() == GLFW_FALSE) {
        std::cout << "ERROR: GLFW failed to load.\n";
        exit(-1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //if(!decorated) glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    this->decorated = decorated;
    
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    //
    
    window_handle = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);

    glfwMakeContextCurrent(window_handle);
    glfwShowWindow(window_handle);

    handle_window_platform(*this);

    int width, height;
    glfwGetWindowSize(window_handle, &width, &height);
    this->size = {width, height};

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
    
    glViewport(0, 0, size.x, size.y);

    /////
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    */
}

void context::clear_events() {
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

void context::poll_events() {
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
        glm::vec2 new_cursor_pos = {event.xpos, size.y - event.ypos - 1};
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
            if(move) {
                s.x += global_cursor_delta.x;
                s.y += global_cursor_delta.y;

                glfwSetWindowPos(window_handle, s.x, s.y);
                
                show_cursor();
                set_cursor(axiom::cursor_mode::DEFAULT);
            }
        }
    }
}

bool context::is_fullscreen() {
    return axiom::is_fullscreen(window_handle);
}

bool context::is_maximized() {
    return axiom::is_maximized(window_handle);
}

bool context::is_windowed() {
    return !is_maximized() && !is_minimized();
}

bool context::is_minimized() {
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

void context::make_fullscreen() {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    ivec4 range = get_window_range(window_handle);
    rs.push_back(range);

    glfwSetWindowMonitor(window_handle, monitor, 0, 0, mode->width, mode->height, 60);
}

void context::make_windowed() {
    glfwRestoreWindow(window_handle);
    restore();
}

void context::make_minimized() {
    ivec4 range = get_window_range(window_handle);
    rs.push_back(range);

    glfwIconifyWindow(window_handle);
}

void context::make_maximized() {
    ivec4 range = get_window_range(window_handle);
    rs.push_back(range);

    glfwMaximizeWindow(window_handle);
}

void context::restore() {
    if(rs.size()) {
        glfwSetWindowMonitor(window_handle, nullptr, rs.back().x, rs.back().y, rs.back().z, rs.back().w, 60);
        rs.pop_back();
    }
}

void context::hide_cursor() {
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    if(cursor_disabled) cursor_pos = prev_cursor_pos;

    cursor_hidden = true;
    cursor_disabled = false;

    //set_cursor(window_handle, -1);
}

void context::disable_cursor() {
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if(!cursor_disabled) prev_cursor_pos = cursor_pos;
    
    cursor_disabled = true;
}

void context::show_cursor() {
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if(cursor_disabled) cursor_pos = prev_cursor_pos;
    
    cursor_hidden = false;
    cursor_disabled = false;
}

context::~context() {
    window_platform_destruct(*this);
}

void context::set_cursor(cursor_mode mode) {
    wset_cursor(window_handle, mode);
}

void context::create_shaders() {
    /*
    auto& render_system = axiom::ecs.get_system<axiom::render_system>();

    axiom::text_asset vert;
    axiom::text_asset frag;

    vert = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/texture2d.vert");
    frag = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/texture2d.frag");
    render_system.shaders.emplace("texture2d", std::move(
        axiom::shader(vert, frag)
    ));
    
    vert = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/grid2d.vert");
    frag = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/grid2d.frag");
    render_system.shaders.emplace("grid2d", std::move(
        axiom::shader(vert, frag)
    ));
    
    vert = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/grid3d.vert");
    frag = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/grid3d.frag");
    render_system.shaders.emplace("grid3d", std::move(
        axiom::shader(vert, frag)
    ));
    
    vert = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/ui.vert");
    frag = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/ui.frag");
    render_system.shaders.emplace("ui", std::move(
        axiom::shader(vert, frag)
    ));
    
    vert = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/ui_composite.vert");
    frag = axiom::text_asset::load(render_system.resource_root + "/shaders/webgl/ui_composite.frag");
    render_system.shaders.emplace("ui_composite", std::move(
        axiom::shader(vert, frag)
    ));
    
    axiom::texture_asset tex;
    tex = axiom::texture_asset::load(render_system.resource_root + "/textures/ui.png");
    render_system.textures.emplace("ui", std::move(
        axiom::texture(tex, axiom::texture_format::RGBA8, 0)
    ));
    */
}

void context::swap_buffers() {
    glfwSwapBuffers(window_handle);
}

}