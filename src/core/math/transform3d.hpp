#pragma once

#include "math/base.hpp"

namespace axiom {

struct transform3d {
    vec3 position;
    mat3 orientation;
};

}