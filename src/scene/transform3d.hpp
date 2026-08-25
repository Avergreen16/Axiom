#pragma once

#include <include/math.hpp>

namespace axiom {

struct transform3d {
    vec3 position;
    mat3 orientation;
};

}