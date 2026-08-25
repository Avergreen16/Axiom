#pragma once

#include <include/math.hpp>
#include <scene/transform2d.hpp>
#include <scene/transform3d.hpp>
#include <scene/camera2d.hpp>
#include <scene/camera3d.hpp>

namespace axiom {

mat4 get_model(transform2d& object);
mat4 get_view(camera2d& camera, transform2d& transform);
mat4 get_proj(camera2d& camera);

mat4 get_model(transform3d& object, transform3d& camera);
mat4 get_view(camera3d& camera, transform3d& transform);
mat4 get_proj(camera3d& camera);

mat4 get_infinite_proj_matrix(vec2 window_size, float fov, float near_plane, float npz, float fpz);
mat4 get_ortho_proj_matrix(float left, float right, float bottom, float top, float near, float far);

}