#include <render/mesh.hpp>

namespace axiom {

void color_mesh3d::load() {
    if(!vertices) {
        vertices = std::shared_ptr<axiom::vertices>(new axiom::vertices);
        vertices->init();
    }

    vertices->vertex_buffer_data(vs.data(), vs.size(), sizeof(color_vertex3d), GL_STATIC_DRAW);
    vertices->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex3d), 0);
    vertices->add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(color_vertex3d), sizeof(float) * 3);
    vertices->add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(color_vertex3d), sizeof(float) * 6);
}

}