#include <physics-2d/mesh.hpp>

#include <iostream>

namespace axiom {

struct mesh_face {
    int i0;
    int i1;

    bool finished = false;
};

struct ellipsoid {
    vec2 center;
    vec2 radii;
    mat2 orientation;
};


vec2 support(vec2 direction, vec2 center, mat2 orientation, vec2 radii) {
    vec2 local = transpose(orientation) * direction;

    vec2 q = {
        radii.x * radii.x * local.x,
        radii.y * radii.y * local.y
    };

    float denom = sqrt(q.x * local.x + q.y * local.y);
    if(denom == 0) denom = 1.0f;

    return center + orientation * (q / denom);
}

vec2 support(vec2 direction, std::vector<ellipsoid> ellipsoids) {
    float max_dot = -FLT_MAX;
    vec2 point = vec2(0.0f);

    for(ellipsoid& e : ellipsoids) {
        vec2 new_point = support(direction, e.center, e.orientation, e.radii);

        float new_dot = dot(new_point, direction);

        if(new_dot > max_dot) {
            max_dot = new_dot;
            point = new_point;
        }
    }

    return point;
}

std::vector<vec2> create_mesh(std::vector<vec2> v, vec2 radius, bool create_interior) {
    std::vector<ellipsoid> ellipsoids = { // glm::rotate(glm::identity<mat3>(), axiom::pi * 0.125f)
        ellipsoid{vec2(0.0f, 0.0f), vec2(1.0f, 2.0f), glm::identity<mat2>()},
        ellipsoid{vec2(0.0f, -3.0f), vec2(1.0f, 0.0f), glm::rotate(glm::identity<mat3>(), axiom::pi * 0.125f)},
    };

    std::vector<vec2> points = {support(vec2(1.0f, 0.0f), ellipsoids), support(vec2(-1.0f, 0.0f), ellipsoids)};

    std::vector<mesh_face> faces = {mesh_face(0, 1, false), mesh_face(1, 0, false)};
    
    while(true) {
        std::vector<mesh_face> new_faces;

        bool finish = true;

        for(mesh_face& face : faces) {
            if(!face.finished) {
                vec2 normal = vec2(points[face.i0] - points[face.i1]);
                normal = vec2(-normal.y, normal.x);

                vec2 s = support(normal, ellipsoids);

                bool overwrite = false;

                if(dot(s, normal) < dot(points[face.i0], normal) + 0.00001f) {
                    face.finished = true;
                } else overwrite = true;

                if(overwrite) {
                    points.push_back(s);
                    new_faces.push_back(mesh_face(face.i0, points.size() - 1, false));
                    new_faces.push_back(mesh_face(points.size() - 1, face.i1, false));

                    finish = false;
                }
            }
        }

        faces = new_faces;

        if(finish) break;
    }

    vec2 c = vec2(0.0f);
    for(vec2 v : points) c += v;
    c /= points.size();

    std::sort(points.begin(), points.end(), 
        [c](const vec2& a, const vec2& b) {
            float theta_a = std::atan2(a.y - c.y, a.x - c.x);
            float theta_b = std::atan2(b.y - c.y, b.x - c.x);

            return theta_a < theta_b;
        }
    );

    std::cout << points.size() << "\n";
    return points;
}

}