#pragma once

#include <ui/widget_base.hpp>
#include <ui/ui_system.hpp>
#include <include/window.hpp>

namespace axiom {

struct screen_widget : widget {
    uint header = 24;
    uint shadow_width = 10;
    uint border_width = 1;

    std::string label;
    vec2 text_size;
    vec3 color;

    axiom::window* win;

    bool fullscreen = false;
    bool hover_minimize = false;
    bool hover_maximize = false;
    bool hover_close = false;

    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();

    static ulong insert(std::string name, vec3 color, axiom::window* win);
};

}