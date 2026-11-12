#include <iostream>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <emscripten/key_codes.h>

#include <wrapper/context.hpp>
#include <include/core.hpp>

namespace axiom {

struct input_cache {
    std::vector<axiom::key_event> key_events;
    std::vector<axiom::mouse_button_event> mouse_button_events;
    std::vector<axiom::scroll_event> scroll_events;
    std::vector<axiom::cursor_event> cursor_events;
    std::vector<axiom::text_event> text_events;
    std::vector<axiom::iconify_event> iconify_events;
    std::vector<axiom::resize_event> resize_events;
    
    std::unordered_set<axiom::input_code> pressed_buttons;
    std::unordered_set<axiom::input_code> released_buttons;
    std::unordered_set<axiom::input_code> repeat_buttons;
    
    vec2 cursor_delta = vec2(0.0f);
    float scroll_delta = 0.0f;
    std::string char_delta = "";
};
    
struct context::implementation {
    context* parent;

    std::string canvas_name;
    bool cursor_inside = false;

    input_cache cache;

    //

    void init(std::string canvas_name);
};

bool key_callback(int event_type, const EmscriptenKeyboardEvent* key_event, void* userdata) {
    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);

    return EM_TRUE;
}

bool ui_callback(int event_type, const EmscriptenUiEvent *ui_event, void *userdata) {
    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);

    if(event_type == EMSCRIPTEN_EVENT_RESIZE) {
        ivec2 viewport_size;
        emscripten_get_canvas_element_size("#canvas", &viewport_size.x, &viewport_size.y);

        std::cout << viewport_size << "\n";

        ctx->imp->cache.resize_events.push_back(axiom::resize_event(ctx->size, viewport_size));
    }

    return EM_TRUE;
}

bool wheel_callback(int event_type, const EmscriptenWheelEvent* wheel_event, void* userdata) {
    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);

    if(event_type == EMSCRIPTEN_EVENT_WHEEL) {
        ctx->imp->cache.scroll_delta += wheel_event->deltaY * 0.01f;
    }

    return EM_TRUE;
}

bool mouse_callback(int event_type, const EmscriptenMouseEvent* mouse_event, void *userdata) {
    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);
    
    if(event_type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        if(mouse_event->button == 0) {
            ctx->imp->cache.mouse_button_events.push_back({axiom::input_code::MOUSE_LEFT, axiom::action::PRESS});
        }

        if(mouse_event->button == 1) {
            ctx->imp->cache.mouse_button_events.push_back({axiom::input_code::MOUSE_CENTER, axiom::action::PRESS});
        }

        if(mouse_event->button == 2) {
            ctx->imp->cache.mouse_button_events.push_back({axiom::input_code::MOUSE_RIGHT, axiom::action::PRESS});
        }
    } else if(event_type == EMSCRIPTEN_EVENT_MOUSEUP) {
        if(mouse_event->button == 0) {
            ctx->imp->cache.mouse_button_events.push_back({axiom::input_code::MOUSE_LEFT, axiom::action::RELEASE});
        }

        if(mouse_event->button == 1) {
            ctx->imp->cache.mouse_button_events.push_back({axiom::input_code::MOUSE_CENTER, axiom::action::RELEASE});
        }

        if(mouse_event->button == 2) {
            ctx->imp->cache.mouse_button_events.push_back({axiom::input_code::MOUSE_RIGHT, axiom::action::RELEASE});
        }
    } else if(event_type == EMSCRIPTEN_EVENT_MOUSEENTER) {
        ctx->imp->cursor_inside = true;
    } else if(event_type == EMSCRIPTEN_EVENT_MOUSELEAVE) {
        ctx->imp->cursor_inside = false;
    } else if(event_type == EMSCRIPTEN_EVENT_MOUSEMOVE) {
        auto ratio = emscripten_get_device_pixel_ratio();

        vec2 offset = {mouse_event->movementX * ratio, mouse_event->movementY * ratio};
        
        ctx->imp->cache.cursor_delta += offset;
        ctx->cursor_pos = {mouse_event->targetX * ratio, mouse_event->targetY * ratio};
        ctx->imp->cursor_inside = true;
    }

    return true;
}

void context::implementation::init(std::string canvas_name) {
    this->canvas_name = canvas_name;

    emscripten_set_keydown_callback(canvas_name.c_str(), parent, true, key_callback);
    emscripten_set_keyup_callback(canvas_name.c_str(), parent, true, key_callback);
    emscripten_set_keypress_callback(canvas_name.c_str(), parent, true, key_callback);

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, parent, true, ui_callback);

    emscripten_set_mousemove_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mouseenter_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mouseleave_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mousedown_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mouseup_callback(canvas_name.c_str(), parent, true, mouse_callback);

    emscripten_set_wheel_callback(canvas_name.c_str(), parent, true, wheel_callback);
}

/*
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

    w->key_events.push_back(key_event{glfw_input_map_key[key], scancode, axiom_action});
}
*/

context::context() : imp(new implementation(this)) {
    imp->init("#canvas");
    
    ivec2 viewport_size;
    emscripten_get_canvas_element_size("#canvas", &viewport_size.x, &viewport_size.y);

    size = viewport_size;
}

void context::clear_events() {
    input_cache front;
    std::swap(front, imp->cache);

    pressed_buttons.clear();
    repeat_buttons.clear();
    released_buttons.clear();

    key_events = front.key_events;
    mouse_button_events = front.mouse_button_events;
    scroll_events = front.scroll_events;
    cursor_events = front.cursor_events;
    text_events = front.text_events;
    resize_events = front.resize_events;

    cursor_delta = front.cursor_delta;
    scroll_delta = front.scroll_delta;

    for(auto& mb : mouse_button_events) {
        if(mb.action == axiom::action::PRESS) {
            pressed_buttons.emplace(mb.button);
            input_map[mb.button] = true;
        } else if(mb.action == axiom::action::REPEAT) {
            repeat_buttons.emplace(mb.button);
        } else if(mb.action == axiom::action::RELEASE) {
            released_buttons.emplace(mb.button);
            input_map[mb.button] = false;
        } 
    }

    for(auto& rs : resize_events) {
        size = rs.new_size;
    }
}

context::~context() = default;

EM_JS(void, set_mouse_visible, (bool visible), {
    document.querySelector("#canvas").style.cursor =
        visible ? "default" : "none";
});

bool context::cursor_in_window() {
    return cursor_pos.x >= 0 && cursor_pos.y >= 0 && cursor_pos.x < size.x && cursor_pos.y < size.y;
}

void context::hide_cursor() {
    set_mouse_visible(false);
    if(cursor_disabled) cursor_pos = prev_cursor_pos;
    
    cursor_hidden = true;
    cursor_disabled = false;
}

void context::disable_cursor() {
    set_mouse_visible(false);
    emscripten_request_pointerlock(imp->canvas_name.c_str(), 1);

    if(!cursor_disabled) prev_cursor_pos = cursor_pos;
    
    cursor_hidden = true;
    cursor_disabled = true;
}

void context::show_cursor() {
    set_mouse_visible(true);
    emscripten_exit_pointerlock();
    if(cursor_disabled) cursor_pos = prev_cursor_pos;
    
    cursor_hidden = false;
    cursor_disabled = false;
}

}