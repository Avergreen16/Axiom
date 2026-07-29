#pragma once

#include <ui/widget_base.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

struct scroll_widget : widget {
    float total_scrollable = 0.0f;
    float scroll_width;
    bool reserve = false;
    float scroll_anchor = 0.0f;

    float scroll_pos = 0.0f;

    ulong anchor_widget = 0xFFFFFFFFFFFFFFFD;//NULL_WIDGET;
    float anchor_frac = 0.0f;

    bool capture_scroll = false;

    std::vector<ulong> prev_order;

    void handle_inputs();
    void mesh();
    void init();

    capture_data handle_capture();

    static ulong insert(float scroll_width, bool reserve);
};

}