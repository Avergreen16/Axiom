#pragma once

#include <ui/ui.hpp>

namespace axiom {

struct cursor {
    cursor_mode cursor_mode = axiom::cursor_mode::DEFAULT;
    ivec2 cursor_pos;
    ivec2 cursor_delta;
};

std::vector<ui_vertex> mesh_cursor(cursor_mode mode, ivec2 position);

}