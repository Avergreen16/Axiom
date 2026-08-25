#include <scene/camera2d.hpp>
#include <scene/transform2d.hpp>
#include <scene/camera3d.hpp>
#include <scene/transform3d.hpp>
#include <include/math.hpp>

namespace axiom {

// 2d

mat4 get_model(transform2d& object) {
    return glm::translate(vec3(object.position, 0.0f)) * mat4(object.orientation); //  - camera.position
}

mat4 get_view(camera2d& camera, transform2d& transform) {
    return mat4(glm::transpose(transform.orientation)) * glm::translate(vec3(-transform.position, 0.0f));
}

mat4 get_proj(camera2d& camera) {
    return glm::scale(vec3(1.0f / camera.aspect * camera.zoom, 1.0f));
}

// 3d

mat4 get_infinite_proj_matrix(vec2 window_size, float fov, float near_plane, float npz, float fpz) {
    vec2 aspect = window_size / glm::min(window_size.x, window_size.y);

    float focal_length = 1.0f / tan(fov * (axiom::pi / 180) * 0.5f);
    float A = -fpz;
    float B = (npz - fpz) * near_plane;

    mat4 matrix = glm::identity<mat4>();

    matrix[3][3] = 0;
    matrix[0][0] = focal_length;
    matrix[1][1] = focal_length;
    matrix[2][2] = A;
    matrix[3][2] = B;
    matrix[2][3] = -1;

    matrix[0][0] /= aspect.x;
    matrix[1][1] /= aspect.y;

    return matrix;
}

mat4 get_ortho_proj_matrix(float left, float right, float bottom, float top, float near, float far) {
    float x = (right - left) * 0.5f;
    float y = (top - bottom) * 0.5f;
    float z = (far - near) * 0.5f;

    vec3 pos = vec3(right + left, top + bottom, far + near) * 0.5f;
    vec3 scale = vec3(right - left, top - bottom, far - near) * 0.5f;

    return glm::translate(vec3(0.0f, 0.0f, 0.5f)) * glm::scale(vec3(1.0f, 1.0f, 0.5f)) * glm::scale(1.0f / scale) * glm::translate(-pos);
}

mat4 get_model(transform3d& object, transform3d& camera) {
    return glm::translate(object.position - camera.position) * mat4(object.orientation);
}

mat4 get_view(camera3d& camera, transform3d& transform) {
    return mat4(glm::transpose(transform.orientation));
}

mat4 get_proj(camera3d& camera) {
    return get_infinite_proj_matrix(camera.aspect, camera.fov, camera.near, 1.0f, 0.0f);
}

}