#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

enum split_mode {
    SCALE, 
    SIZE
};

enum split_layout {
    HORIZONTAL,
    VERTICAL
};

struct panel_constraint {
    axiom::split_mode split_mode = axiom::split_mode::SCALE;
    float value = 1.0f;
};

struct split_widget : container_widget {
    std::vector<panel_constraint> constraints;
    axiom::split_layout layout;
    float prev_size = -1.0f;
    vec2 sep = vec2(0.0f);
    
    uint operation = 0;

    void handle_inputs();
    void recalibrate();
    void process_size();
    capture_data handle_capture();

    void init();

    static ulong insert(axiom::split_layout layout, std::vector<axiom::split_mode> modes);
};

}