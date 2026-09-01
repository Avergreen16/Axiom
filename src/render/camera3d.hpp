#pragma once

#include <include/core.hpp>

namespace axiom {

struct camera3d {
    float near = 1.0f / 64.0f;
    float fov;
    vec2 aspect;
};

}