#include <math/3d.hpp>

namespace axiom {

mat3 rotate_to(vec3 a, vec3 b) {
    vec3 cross_p = cross(a, b);
    float d = dot(a, b);
    float f = acos(d);
    vec3 n = normalize(cross_p);

    if(glm::isinf(n.x) || glm::isnan(n.x) || f == 0 || glm::isnan(f)) {
        mat3 rot_mat = glm::identity<mat3>();
        if(dot(a, b) < 0) rot_mat = mat3(rot_mat[0], -rot_mat[1], -rot_mat[2]);
        return rot_mat;
    }
    mat3 matrix = rotate(f, n);

    return matrix;
}

mat3 rotate_to(vec3 a, vec3 b, vec3 axis) {
    float angle = atan2(dot(axis, cross(a, b)), dot(a, b));
    mat3 matrix = rotate(angle, axis);

    return matrix;
}

}