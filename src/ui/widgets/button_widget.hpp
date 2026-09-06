#pragma once

#include <ui/widget_base.hpp>

namespace axiom {
    
struct button_widget : widget {
    bool hovered = false;
    bool pressed = false;
    bool held = false;

    std::function<void(button_widget&)> callback;

    vec3 color;

    vec4 icon = vec4(0.0f);
    vec2 icon_size = vec2(20.0f);

    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();

    static ulong insert(vec2 size, vec3 color, std::string str, std::function<void(button_widget&)> callback = [](button_widget& w) {});
    static ulong insert(vec2 size, vec3 color, vec4 icon, std::function<void(button_widget&)> callback = [](button_widget& w) {});
};

}