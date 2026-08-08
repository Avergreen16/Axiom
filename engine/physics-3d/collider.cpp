#include <physics-3d/collider.hpp>

#include <iostream>

namespace axiom {

vec3 support(vec3 direction, vec3 center, mat3 orientation, vec3 radii) {
    vec3 local = transpose(orientation) * direction;

    vec3 q = {
        radii.x * radii.x * local.x,
        radii.y * radii.y * local.y,
        radii.z * radii.z * local.z
    };

    float denom = sqrt(q.x * local.x + q.y * local.y + q.z * local.z);
    if(denom == 0) denom = 1.0f;

    vec3 point = orientation * (q / denom);

    return center + point;
}

/*
vec3 support(vec3 direction, vec3 center, mat3 orientation, vec3 radii, std::vector<clipping_plane3d>& planes) {
    vec3 local = transpose(orientation) * direction;

    vec3 q = {
        radii.x * radii.x * local.x,
        radii.y * radii.y * local.y,
        radii.z * radii.z * local.z
    };

    float denom = sqrt(q.x * local.x + q.y * local.y + q.z * local.z);
    if(denom == 0) denom = 1.0f;

    vec3 point = (q / denom);

    for(clipping_plane2d& p : planes) {
        if(dot(p.normal, point - p.origin) > 0.0f) {
            vec2 pp = p.origin / radii;
            vec2 n = normalize(vec2(p.normal.y, -p.normal.x) / radii);

            float l = dot(n, -pp);
            float d = length(pp + n * l);
            float ll = sqrt(1.0f - d * d);
            float l0 = l - ll;
            float l1 = l + ll;

            vec2 p0 = radii * (pp + n * l0);
            vec2 p1 = radii * (pp + n * l1);

            float dot0 = dot(p0, local);
            float dot1 = dot(p1, local);

            if(dot0 > dot1) point = p0;
            else point = p1;
        }
    }

    return center + orientation * point;
}
*/

vec3 support(vec3 direction, std::vector<vertex_element3d> ellipsoids) {
    float max_dot = -FLT_MAX;
    vec3 point = vec3(0.0f);

    for(vertex_element3d& e : ellipsoids) {
        vec3 new_point;
        //if(e.planes.size()) new_point = support(direction, e.center, e.orientation, e.radii, e.planes);
        new_point = support(direction, e.center, e.orientation, e.radii);

        float new_dot = dot(new_point, direction);

        if(new_dot > max_dot) {
            max_dot = new_dot;
            point = new_point;
        }
    }

    return point;
}

}