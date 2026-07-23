#include <ui/cursor.hpp>

namespace axiom {

std::vector<ui_vertex> mesh_cursor(cursor_mode mode, ivec2 position) {
    ivec2 size;
    ivec4 tex_range;
    ivec2 rel_pos;

    switch(mode) {
        case axiom::cursor_mode::DEFAULT: 
            size = {10, 16};
            tex_range = {80, 0, 10, 16};
            rel_pos = {0, 0};
            break;
        case axiom::cursor_mode::CLICK: 
            size = {16, 16};
            tex_range = {96, 0, 16, 16};
            rel_pos = {-5, 0};
            break;
        case axiom::cursor_mode::DRAG_T: 
            size = {9, 17};
            tex_range = {112, 0, 9, 17};
            rel_pos = {-4, 8};
            break;
        case axiom::cursor_mode::DRAG_TR: 
            size = {13, 13};
            tex_range = {80, 32, 13, 13};
            rel_pos = {-6, 6};
            break;
        case axiom::cursor_mode::DRAG_R: 
            size = {17, 9};
            tex_range = {80, 16, 17, 9};
            rel_pos = {-8, 4};
            break;
        case axiom::cursor_mode::DRAG_BR: 
            size = {13, 13};
            tex_range = {96, 32, 13, 13};
            rel_pos = {-6, 6};
            break;
        case axiom::cursor_mode::DRAG_B: 
            size = {9, 17};
            tex_range = {112, 0, 9, 17};
            rel_pos = {-4, 8};
            break;
        case axiom::cursor_mode::DRAG_BL: 
            size = {13, 13};
            tex_range = {80, 32, 13, 13};
            rel_pos = {-6, 6};
            break;
        case axiom::cursor_mode::DRAG_L: 
            size = {17, 9};
            tex_range = {80, 16, 17, 9};
            rel_pos = {-8, 4};
            break;
        case axiom::cursor_mode::DRAG_TL:
            size = {13, 13};
            tex_range = {96, 32, 13, 13};
            rel_pos = {-6, 6};
            break;
        case axiom::cursor_mode::TEXT:
            size = {7, 14};
            tex_range = {112, 32, 7, 14};
            rel_pos = {-3, 7}; 
            break;
    }
    
    std::vector<ui_vertex> v = {
        ui_vertex({0, -size.y, 1.0f}, tex_range.xy()),
        ui_vertex({size.x, -size.y, 1.0f}, tex_range.xy() + ivec2(tex_range.z, 0)),
        ui_vertex({0, 0, 1.0f}, tex_range.xy() + ivec2(0, tex_range.w)),
        ui_vertex({size.x, 0, 1.0f}, tex_range.xy() + ivec2(tex_range.z, tex_range.w)),
    };

    for(ui_vertex& vv : v) {
        vv.pos += vec3(position + rel_pos, 0.0f);
        vv.data = 0xf0000001;
    }

    v = {v[0], v[1], v[3], v[0], v[3], v[2]};

    return v;
}

}