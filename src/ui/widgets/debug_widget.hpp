#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct debug_widget : widget {
    vec3 color;
    
    void mesh();

    static ulong insert(vec2 size, float max_width, vec3 color);
};

}