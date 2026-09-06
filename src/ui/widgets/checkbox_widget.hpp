#pragma once

#include <ui/widget_base.hpp>

namespace axiom {
    
struct checkbox_widget : widget {
    bool hovered = false;
    bool checked = false;

    std::function<void(checkbox_widget&)> callback;

    vec3 color;

    vec4 icon = vec4(0.0f);
    vec2 icon_size = vec2(20.0f);

    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();

    static ulong insert(vec2 size, vec3 color, bool checked, std::function<void(checkbox_widget&)> callback = [](checkbox_widget& w) {});
};

}