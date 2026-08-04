#pragma once

#include <include/math.hpp>
#include <scene/transform2d.hpp>
#include <scene/camera2d.hpp>

namespace axiom {

mat4 get_model(transform2d& object);
mat4 get_view(camera2d& camera, transform2d& transform);
mat4 get_proj(camera2d& camera);

}