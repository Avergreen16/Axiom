#include <scene/camera2d.hpp>
#include <scene/transform2d.hpp>

namespace axiom {

mat4 get_model(transform2d& object) {
    return mat4(object.orientation) * glm::translate(vec3(object.position, 0.0f)); //  - camera.position
}

mat4 get_view(camera2d& camera, transform2d& transform) {
    return mat4(glm::transpose(transform.orientation)) * glm::translate(vec3(-transform.position, 0.0f));
}

mat4 get_proj(camera2d& camera) {
    return glm::scale(vec3(1.0f / camera.aspect * camera.zoom, 1.0f));
}

}