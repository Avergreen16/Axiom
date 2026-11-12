#include <render/mesh/render_mesh.hpp>
#include <include/core.hpp>

namespace axiom {

void render_meshes(uint camera) {
    axiom::transform3d cam_transform = ecs.get_component<axiom::transform3d>(camera);
    axiom::transform3d cam_cam = ecs.get_component<axiom::transform3d>(camera);
}

}