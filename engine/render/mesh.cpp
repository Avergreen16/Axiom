#include <graphicsh.hpp>

#include <render/mesh.hpp>

namespace axiom {

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