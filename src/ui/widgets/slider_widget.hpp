#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct slider_widget : widget {
    bool text_dirty = true;
    bool hovered = false;
    bool pressed = false;

    float current_value;
    vec2 range;
    float step;

    float slider_width;
    std::string label;
    std::function<void(slider_widget&)> callback;

    axiom::text text;

    vec3 color;

    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();
    
    std::vector<axiom::text*> get_text();

    static ulong insert(vec2 size, float slider_width, vec3 color, vec2 range, float step, float start, std::string str, std::function<void(slider_widget&)> callback = [](slider_widget& w) {});
};

}