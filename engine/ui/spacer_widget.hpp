#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct spacer_widget : widget {
    bool visual = false;

    void mesh();
    void init();

    static ulong insert(vec2 min_size, vec2 max_size, bool visual = false, bool step = false);
};

}