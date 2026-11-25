#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct match_widget : container_widget {
    vec4 color;
    bool border;
    vec4 buf = vec4(0.0f);

    void mesh();
    void init();
    
    capture_data handle_capture();

    static ulong insert(vec4 color, bool border, vec4 buf = vec4(0.0f));
};

}