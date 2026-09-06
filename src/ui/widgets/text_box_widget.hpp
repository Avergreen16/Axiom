#pragma once

#include <ui/widget_base.hpp>
#include <ui/text.hpp>

#include <sstream>

namespace axiom {

struct text_box_widget : widget {
    bool update = false;
    vec2 boundary;

    void handle_inputs();
    void mesh();
    void init();
    std::function<void(text_box_widget&)> callback;

    axiom::capture_data handle_capture();
    
    static ulong insert(float width, vec2 boundary, std::string start, std::function<void(text_box_widget&)> callback = [](text_box_widget& self) {});
};

}