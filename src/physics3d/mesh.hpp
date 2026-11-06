#pragma once
#include <include/core.hpp>
#include <physics3d/collider.hpp>

namespace axiom {

struct output_vertex {
    vec3 position;
    uint element;
};
    
void create_mesh(std::vector<vec3> elements, std::vector<output_vertex>* vertices, std::vector<uint>* indices);
void triangulate(std::vector<vec3> vertices, std::vector<uint>* indices);

}