#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct drop_option {
    std::string label;
    bool dirty = true;
    std::vector<ui_vertex> vertices;
    vec2 size;
};

struct drop_widget : container_widget {
    uint selected = 0;
    uint hovered = 0xFFFFFFFF;
    
    vec3 color;
    bool drop_down = false;
    bool drop_direction = false;
    float drop_height;
    float drop_unit_height;
    float button_width;

    std::vector<drop_option> options;
    float scroll = 0.0f;
    float scroll_height = 0.0f;

    std::vector<axiom::text> text;

    std::function<void(drop_widget&)> callback;

    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();
    void toggle_drop();
    
    std::vector<axiom::text*> get_text();
    
    static ulong insert(vec2 size, vec3 color, float w, float h, float h2, std::vector<std::string> options, uint selected, std::function<void(drop_widget&)> callback = [](drop_widget& self) {});
};

}