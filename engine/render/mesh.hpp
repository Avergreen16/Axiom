#pragma once

#include <render/wrapper.hpp>

namespace axiom {

struct color_vertex {
    vec2 position;
    vec4 color;
};

struct color_mesh {
    std::vector<color_vertex> border;
    std::vector<color_vertex> area;

    std::shared_ptr<axiom::vertices> v_lines;
    std::shared_ptr<axiom::vertices> v_tris;
};

}