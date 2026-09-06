#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

enum panel_mode {
    SCALE, 
    SIZE
};

struct panel_constraint {
    float value = 1.0f;
    axiom::panel_mode panel_mode = axiom::panel_mode::SCALE;
};

struct split_widget : widget {
    std::vector<panel_constraint> constraints;
    float prev_size = -1.0f;
    vec2 sep = vec2(0.0f);
    
    uint operation = 0;

    void handle_inputs();
    void recalibrate();
    void process_size();
    capture_data handle_capture();

    void init();

    static ulong insert(axiom::layout_mode layout, std::vector<panel_constraint> constraints);
};

}