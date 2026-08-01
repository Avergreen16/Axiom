#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct match_widget : widget {
    vec4 color;
    bool border;
    vec4 buf = vec4(0.0f);

    void mesh();
    void init();

    static ulong insert(vec4 color, bool border, vec4 buf = vec4(0.0f));
};

}