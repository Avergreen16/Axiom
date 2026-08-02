#pragma once

#include <include/math.hpp>

namespace axiom {

struct camera2d {
    float zoom = 1.0f;
    vec2 aspect = vec2(1.0f);
};

}