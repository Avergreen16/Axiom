#include <iostream>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <emscripten/key_codes.h>

#include <wrapper/webgl/context.hpp>
#include <wrapper/webgl/input.hpp>
#include <wrapper/webgl/wrapper.hpp>

#include <include/render.hpp>

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

    std::unordered_map<uint, touch> touches;
    
    vec2 cursor_delta = vec2(0.0f);
    float scroll_delta = 0.0f;
    std::string char_delta = "";
};
    
struct context::implementation {
    context* parent;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE emscripten_webgl_context;
    std::string canvas_name;
    bool cursor_inside = false;

    input_cache cache;

    //

    void init(std::string canvas_name);
};

bool key_callback(int event_type, const EmscriptenKeyboardEvent* k_event, void* userdata) {
    std::cout << "KEY\n";

    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);

    axiom::action axiom_action;

    if(event_type == EMSCRIPTEN_EVENT_KEYDOWN) {
        axiom_action = axiom::action::PRESS;
    } else if(event_type == EMSCRIPTEN_EVENT_KEYUP) {
        axiom_action = axiom::action::RELEASE;
    }
    /* else if(aevent_type == EMSCRIPTEN_EVENT_KEYDOWN) {
        axiom_action = axiom::action::REPEAT;
    }*/

    std::string key = k_event->code;
    ctx->imp->cache.key_events.push_back(key_event{input_map_key[key], int(k_event->charCode), axiom_action});

    return EM_TRUE;
}

bool ui_callback(int event_type, const EmscriptenUiEvent *ui_event, void *userdata) {
    std::cout << "UI\n";

    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);

    if(event_type == EMSCRIPTEN_EVENT_RESIZE) {
        ivec2 viewport_size;
        emscripten_get_canvas_element_size("#canvas", &viewport_size.x, &viewport_size.y);

        ctx->imp->cache.resize_events.push_back(axiom::resize_event(ctx->size, viewport_size));
    }

    return EM_TRUE;
}

bool wheel_callback(int event_type, const EmscriptenWheelEvent* wheel_event, void* userdata) {
    std::cout << "WHEEL\n";

    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);

    if(event_type == EMSCRIPTEN_EVENT_WHEEL) {
        ctx->imp->cache.scroll_delta += wheel_event->deltaY * 0.01f;
    }

    return EM_TRUE;
}

bool mouse_callback(int event_type, const EmscriptenMouseEvent* mouse_event, void *userdata) {
    std::cout << "MOUSE\n";

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

        vec2 offset = {mouse_event->movementX * ratio, -mouse_event->movementY * ratio};
        
        ctx->imp->cache.cursor_delta += offset;
        ctx->cursor_pos = {mouse_event->targetX * ratio, ctx->size.y - (mouse_event->targetY + 0.5f) * ratio};
        ctx->imp->cursor_inside = true;
    }

    return true;
}

bool touch_callback(int event_type, const EmscriptenTouchEvent* touch_event, void *userdata) {
    std::cout << "TOUCH\n";

    axiom::context* ctx = reinterpret_cast<axiom::context*>(userdata);

    if(event_type == EMSCRIPTEN_EVENT_TOUCHSTART) {
        for(uint i = 0; i < touch_event->numTouches; ++i) {
            auto& t = touch_event->touches[i];
            auto ratio = emscripten_get_device_pixel_ratio();

            if(t.isChanged) {
                ctx->imp->cache.touches.emplace(
                    t.identifier,
                    touch{
                        uint(t.identifier),
                        vec2(t.targetX * ratio, ctx->size.y - (t.targetY + 0.5f) * ratio),
                        vec2(0.0f)
                    }
                );
            }
        }
    } else if(event_type == EMSCRIPTEN_EVENT_TOUCHEND) {
        for(uint i = 0; i < touch_event->numTouches; ++i) {
            auto& t = touch_event->touches[i];

            if(t.isChanged) {
                ctx->imp->cache.touches.erase(t.identifier);
            }
        }
    } else if(event_type == EMSCRIPTEN_EVENT_TOUCHMOVE) {
        for(uint i = 0; i < touch_event->numTouches; ++i) {
            auto ratio = emscripten_get_device_pixel_ratio();

            auto& t = touch_event->touches[i];

            if(t.isChanged) {
                auto& touch = ctx->imp->cache.touches[t.identifier];

                vec2 new_pos = {t.targetX * ratio, ctx->size.y - (t.targetY + 0.5) * ratio};

                touch.delta += new_pos - touch.position;
                touch.position = new_pos;
            }
        }
    } else if(event_type == EMSCRIPTEN_EVENT_TOUCHCANCEL) {
        for(uint i = 0; i < touch_event->numTouches; ++i) {
            auto& t = touch_event->touches[i];

            if(t.isChanged) {
                ctx->imp->cache.touches.erase(t.identifier);
            }
        }
    }

    return true;
};

void context::implementation::init(std::string canvas_name) {
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);

    attributes.majorVersion = 2;
    attributes.minorVersion = 0;
    attributes.antialias = EM_FALSE;

    emscripten_webgl_context = emscripten_webgl_create_context("#canvas", &attributes);

    if (emscripten_webgl_context <= 0) {
        printf("Failed to create WebGL context!\n");
    }
    
    emscripten_webgl_make_context_current(emscripten_webgl_context);

    //

    this->canvas_name = canvas_name;

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, parent, true, key_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, parent, true, key_callback);
    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, parent, true, key_callback);

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, parent, true, ui_callback);

    emscripten_set_mousemove_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mouseenter_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mouseleave_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mousedown_callback(canvas_name.c_str(), parent, true, mouse_callback);
    emscripten_set_mouseup_callback(canvas_name.c_str(), parent, true, mouse_callback);

    emscripten_set_wheel_callback(canvas_name.c_str(), parent, true, wheel_callback);

    emscripten_set_touchstart_callback(canvas_name.c_str(), parent, true, touch_callback);
    emscripten_set_touchend_callback(canvas_name.c_str(), parent, true, touch_callback);
    emscripten_set_touchmove_callback(canvas_name.c_str(), parent, true, touch_callback);
    emscripten_set_touchcancel_callback(canvas_name.c_str(), parent, true, touch_callback);
}

void context::set_loop(void(*loop)()) {
    emscripten_set_main_loop(loop, 0, 1);
}

void context::create_shaders() {
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
    input_cache front = [this]() {
        input_cache c;
        c.touches = imp->cache.touches;
        for(auto& [i, touch] : c.touches) {
            touch.delta = vec2(0.0f); 
        }

        return c;
    }();
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
    
    for(auto& mb : key_events) {
        if(mb.action == axiom::action::PRESS) {
            pressed_buttons.emplace(mb.key);
            input_map[mb.key] = true;
        } else if(mb.action == axiom::action::REPEAT) {
            repeat_buttons.emplace(mb.key);
        } else if(mb.action == axiom::action::RELEASE) {
            released_buttons.emplace(mb.key);
            input_map[mb.key] = false;
        } 
    }

    for(auto& rs : resize_events) {
        size = rs.new_size;
    }

    touches = front.touches;
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

void context::set_cursor(cursor_mode mode) {
    switch(mode) {
        case axiom::cursor_mode::DEFAULT: {
            EM_ASM({
                document.querySelector('#canvas').style.cursor = 'default';
            });
            
            break;
        }
        case axiom::cursor_mode::CLICK: {
            EM_ASM({
                document.querySelector('#canvas').style.cursor = 'pointer';
            });
            
            break;
        }
        case axiom::cursor_mode::DRAG_B:
        case axiom::cursor_mode::DRAG_T: {
            EM_ASM({
                document.querySelector('#canvas').style.cursor = 'ns-resize';
            });
            
            break;
        }
        case axiom::cursor_mode::DRAG_L:
        case axiom::cursor_mode::DRAG_R: {
            EM_ASM({
                document.querySelector('#canvas').style.cursor = 'ew_resize';
            });
            
            break;
        }
        case axiom::cursor_mode::DRAG_BL:
        case axiom::cursor_mode::DRAG_TR: {
            EM_ASM({
                document.querySelector('#canvas').style.cursor = 'nesw-resize';
            });
            
            break;
        }
        case axiom::cursor_mode::DRAG_BR:
        case axiom::cursor_mode::DRAG_TL: {
            EM_ASM({
                document.querySelector('#canvas').style.cursor = 'nwse-resize';
            });
            
            break;
        }
        case axiom::cursor_mode::TEXT: {
            EM_ASM({
                document.querySelector('#canvas').style.cursor = 'text';
            });
            
            break;
        }
    }
}

bool context::is_fullscreen() {
    return false;
}

bool context::is_maximized() {
    return false;
}

bool context::is_windowed() {
    return true;
}

bool context::is_minimized() {
    return false;
}

}