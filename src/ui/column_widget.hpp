#pragma once

#include <ui/widget_base.hpp>

namespace axiom {
    
struct column_widget : widget {
    std::vector<float> rows;
    std::vector<float> row_buffers;
    float column = 0.0f;

    bool fill = false;

    static ulong insert(bool fill = false);
    
    void init();
};

}