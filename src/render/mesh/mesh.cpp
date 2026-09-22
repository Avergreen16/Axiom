#include <graphicsh.hpp>

#include <render/mesh/mesh.hpp>

namespace axiom {
    
void color_mesh2d::load() {
    if(!v_lines) {
        v_lines = std::shared_ptr<axiom::vertices>(new axiom::vertices);
        v_lines->init();
    }

    v_lines->vertex_buffer_data(border.data(), border.size(), sizeof(color_vertex2d), GL_STATIC_DRAW);
    v_lines->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex2d), 0);
    v_lines->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(color_vertex2d), sizeof(float) * 3);

    if(!v_tris) {
        v_tris = std::shared_ptr<axiom::vertices>(new axiom::vertices);
        v_tris->init();
    }

    v_tris->vertex_buffer_data(area.data(), area.size(), sizeof(color_vertex2d), GL_STATIC_DRAW);
    v_tris->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex2d), 0);
    v_tris->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(color_vertex2d), sizeof(float) * 3);
}

void texture_mesh2d::load() {
    if(!vertices) {
        vertices = std::shared_ptr<axiom::vertices>(new axiom::vertices);
        vertices->init();
    }

    vertices->vertex_buffer_data(vs.data(), vs.size(), sizeof(texture_vertex2d), GL_STATIC_DRAW);
    vertices->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(texture_vertex2d), 0);
    vertices->add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(texture_vertex2d), sizeof(float) * 3);
    vertices->add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(texture_vertex2d), sizeof(float) * 5);
}

void color_mesh3d::load() {
    if(!vertices) {
        vertices = std::shared_ptr<axiom::vertices>(new axiom::vertices);
        vertices->init();
    }

    vertices->vertex_buffer_data(vs.data(), vs.size(), sizeof(color_vertex3d), GL_STATIC_DRAW);
    vertices->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex3d), 0);
    vertices->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(color_vertex3d), sizeof(float) * 3);
    vertices->add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(color_vertex3d), sizeof(float) * 7);
}

void texture_mesh3d::load() {
    if(!vertices) {
        vertices = std::shared_ptr<axiom::vertices>(new axiom::vertices);
        vertices->init();
    }

    vertices->vertex_buffer_data(vs.data(), vs.size(), sizeof(texture_vertex3d), GL_STATIC_DRAW);
    vertices->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(texture_vertex3d), 0);
    vertices->add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(texture_vertex3d), sizeof(float) * 3);
    vertices->add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(texture_vertex3d), sizeof(float) * 5);
    vertices->add_vertex_attribute(3, 3, GL_FLOAT, false, sizeof(texture_vertex3d), sizeof(float) * 9);
}

void texture_range_mesh3d::load() {
    if(!vertices) {
        vertices = std::shared_ptr<axiom::vertices>(new axiom::vertices);
        vertices->init();
    }

    vertices->vertex_buffer_data(vs.data(), vs.size(), sizeof(texture_range_vertex3d), GL_STATIC_DRAW);
    vertices->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(texture_range_vertex3d), 0);
    vertices->add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(texture_range_vertex3d), sizeof(float) * 3);
    vertices->add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(texture_range_vertex3d), sizeof(float) * 5);
    vertices->add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(texture_range_vertex3d), sizeof(float) * 9);
    vertices->add_vertex_attribute(4, 3, GL_FLOAT, false, sizeof(texture_range_vertex3d), sizeof(float) * 13);
}

}