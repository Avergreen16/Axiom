#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct spacer_widget : widget {
    bool visual = false;
    vec4 color = vec4(0.0f);

    void mesh();
    void init();

    static ulong insert(vec2 min_size, vec2 max_size, bool visual = false, vec4 color = vec4(0.0f), bool step = false);
};

}