#pragma once
#include <include/math.hpp>
#include <physics-2d/collider.hpp>

namespace axiom {
    
void create_mesh(std::vector<vertex_element> vertices, std::vector<vec2>* perimeter, std::vector<vec2>* area = nullptr);
void create_mesh(axiom::collider2d& shape, std::vector<vec2>* perimeter, std::vector<vec2>* area = nullptr);

}