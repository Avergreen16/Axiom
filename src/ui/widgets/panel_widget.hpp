#pragma once

#include <ui/widget_base.hpp>
#include <ui/system.hpp>

namespace axiom {

struct panel_widget : container_widget {
    void handle_inputs();
    void mesh();
    void init();

    capture_data handle_capture();

    static ulong insert();
};

}