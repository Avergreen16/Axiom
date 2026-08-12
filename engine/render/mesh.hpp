#pragma once

#include <render/wrapper.hpp>

namespace axiom {

struct color_vertex2d {
    vec2 position;
    vec4 color;
};

struct color_vertex3d {
    vec3 position;
    vec4 color;
    vec3 normal;
};

struct texture_vertex3d {
    vec3 position;
    vec2 texture;
    vec4 color;
    vec3 normal;
};

struct color_mesh2d {
    std::vector<color_vertex2d> border;
    std::vector<color_vertex2d> area;

    std::shared_ptr<axiom::vertices> v_lines;
    std::shared_ptr<axiom::vertices> v_tris;
};

struct color_mesh3d {
    std::vector<color_vertex3d> vs;

    std::shared_ptr<axiom::vertices> vertices;

    void load();
};

struct texture_mesh3d {
    std::vector<texture_vertex3d> vs;

    std::shared_ptr<axiom::vertices> vertices;

    void load();
};

}