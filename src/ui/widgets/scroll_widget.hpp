#pragma once

#include <ui/widget_base.hpp>
#include <ui/system.hpp>

namespace axiom {

struct scroll_widget : widget {
    float total_scrollable = 0.0f;
    float scroll_width = FLT_MAX;
    bool reserve = false;
    float scroll_anchor = 0.0f;

    float scroll_pos = 0.0f;

    //
    
    ulong anchor_mode = 1;
    // 0 = widget
    // 1 = top
    // 2 = bottom

    ulong anchor_widget = NULL_WIDGET;
    float anchor_frac = 0.0f;

    bool capture_scroll = false;

    //

    std::vector<ulong> prev_order;
    
    // state
    float state_child_height = -1.0f;
    float state_self_height = -1.0f;

    void handle_inputs();
    void mesh();
    void init();

    capture_data handle_capture();

    static ulong insert(float scroll_width, bool reserve);
};

}