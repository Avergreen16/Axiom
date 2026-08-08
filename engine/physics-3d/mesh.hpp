#pragma once
#include <include/math.hpp>
#include <physics-3d/collider.hpp>

namespace axiom {
    
void create_mesh(std::vector<vertex_element3d> elements, std::vector<vec3>* vertices, std::vector<uint>* indices);
//void create_mesh(axiom::collider3d& shape, std::vector<vec3>* surface);

}