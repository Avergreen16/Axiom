#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct grid_widget : widget {
    uint num_columns;
    
    std::vector<float> rows;
    std::vector<float> columns;
    std::vector<float> row_buffers;
    std::vector<float> column_buffers;

    static ulong insert(uint num_columns);

    void init();
};

}