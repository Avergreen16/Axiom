#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct render_target;

struct render_widget : widget {
    uint target;

    void mesh();
    void init();
    void handle_inputs();
    bool handle_capture();

    static ulong insert();
};

}