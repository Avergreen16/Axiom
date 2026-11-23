#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct row_widget : widget {
    float row = 0.0f;
    std::vector<float> columns;
    std::vector<float> column_buffers;

    bool fill = false;

    static ulong insert(bool fill = false);
    
    void init();
};

}