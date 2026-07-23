#pragma once

#include <ui/widget_base.hpp>
#include <ui/ui.hpp>

namespace axiom {

struct menu_node {
    std::string label;
    std::vector<menu_node> children;
    std::function<void()> callback = []() {};
};

struct menu_widget : widget {
    vec3 color;
    float drop_height;
    float drop_unit_height;
    float button_width;

    uint index = 0xFFFFFFFF;
    double timer = 0.0;

    std::shared_ptr<menu_node> root;
    std::vector<uint> path;

    float scroll = 0.0f;
    float scroll_height = 0.0f;
    uint hovered = 0xFFFFFFFF;
    uint clicked = 0xFFFFFFFF;
    
    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();
    
    static ulong insert(vec2 position, float z, vec3 color, float w, float h, float h2, std::shared_ptr<menu_node> root, std::vector<uint> path);
};

}