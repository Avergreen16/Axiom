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

}