#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct tab {
    std::string label;
    float width;
    vec3 color;

    std::function<void()> swap = []() {};

    bool text_dirty = true;
    std::vector<ui_vertex> text_vertices;
    vec2 text_size;
};

struct tab_widget : widget {
    float tab_height;
    float tab_sep;
    std::vector<tab> tabs;
    uint selected = 0;

    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();

    static ulong insert(float tab_height, float tab_sep, std::vector<tab> tabs);
};

}