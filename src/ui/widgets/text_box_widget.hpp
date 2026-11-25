#pragma once

#include <ui/widget_base.hpp>
#include <ui/text.hpp>

#include <sstream>

namespace axiom {

struct text_box_widget : widget {
    bool update = false;
    vec2 boundary;

    axiom::text text;

    void handle_inputs();
    void mesh();
    void init();
    std::function<void(text_box_widget&)> callback;

    axiom::capture_data handle_capture();
    
    std::vector<axiom::text*> get_text();
    
    static ulong insert(float width, vec2 boundary, std::string start, std::function<void(text_box_widget&)> callback = [](text_box_widget& self) {});
};

}