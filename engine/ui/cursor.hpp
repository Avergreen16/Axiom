#pragma once

#include <ui/ui.hpp>

namespace axiom {

enum class cursor_mode{
    DEFAULT,
    CLICK,
    TEXT,
    DRAG_T,
    DRAG_TR,
    DRAG_R,
    DRAG_BR,
    DRAG_B,
    DRAG_BL,
    DRAG_L,
    DRAG_TL,
};

struct cursor {
    cursor_mode cursor_mode = axiom::cursor_mode::DEFAULT;
    ivec2 cursor_pos;
    ivec2 cursor_delta;
};

std::vector<ui_vertex> mesh_cursor(cursor_mode mode, ivec2 position);

}