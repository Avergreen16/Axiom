#include <physics-2d/collider.hpp>

#include <iostream>

namespace axiom {

void collider2d::create_bounding_box() {
    bounding_box.minimum = vec2(FLT_MAX, FLT_MAX);
    bounding_box.maximum = vec2(-FLT_MAX, -FLT_MAX);

    for(collision_shape2d& cs : shapes) {
        cs.bounding_box.minimum = vec2(FLT_MAX, FLT_MAX);
        cs.bounding_box.maximum = vec2(-FLT_MAX, -FLT_MAX);
        
        vec2 min_x = cs.orientation * support(transpose(cs.orientation) * vec2(-1.0f, 0.0f), cs.vertices) + cs.position;
        vec2 max_x = cs.orientation * support(transpose(cs.orientation) * vec2(1.0f, 0.0f), cs.vertices) + cs.position;
        vec2 min_y = cs.orientation * support(transpose(cs.orientation) * vec2(0.0f, -1.0f), cs.vertices) + cs.position;
        vec2 max_y = cs.orientation * support(transpose(cs.orientation) * vec2(0.0f, 1.0f), cs.vertices) + cs.position;

        cs.bounding_box.minimum = vec2(min_x.x, min_y.y);
        cs.bounding_box.maximum = vec2(max_x.x, max_y.y);

        bounding_box.minimum = glm::min(bounding_box.minimum, cs.bounding_box.minimum);
        bounding_box.maximum = glm::max(bounding_box.maximum, cs.bounding_box.maximum);
    }
}

void collider2d::create_BVH() {
    create_bounding_box();

    BVH_node2d root;
    for(int i = 0; i < shapes.size(); ++i) root.children.push_back(i);

    BVH.push_back(root);

    uint32_t ca;
    uint32_t cb;

    auto split = [&](BVH_node2d& node) {
        uint32_t index = 0;

        axiom::bounding_box2d centers;

        for(int i : node.children) {
            collision_shape2d& shape = shapes[i];
            vec2 center = (shape.bounding_box.minimum + shape.bounding_box.maximum) * 0.5f;

            node.bounding_box.minimum = glm::min(node.bounding_box.minimum, shape.bounding_box.minimum);
            node.bounding_box.maximum = glm::max(node.bounding_box.maximum, shape.bounding_box.maximum);

            centers.minimum = glm::min(centers.minimum, center);
            centers.maximum = glm::max(centers.maximum, center);
        }

        if(node.children.size() > 1) {
            vec2 size = centers.maximum - centers.minimum;
            vec2 center = (centers.minimum + centers.maximum) * 0.5f;

            BVH_node2d child_a;
            BVH_node2d child_b;

            int ii = 0;
            if(size.y > size.x) ii = 1;

            for(int i : node.children) {
                collision_shape2d& shape = shapes[i];

                float c = (shape.bounding_box.minimum[ii] + shape.bounding_box.maximum[ii]) * 0.5f;
                if(c < center[ii]) child_a.children.push_back(i);
                else child_b.children.push_back(i);
            }

            ca = BVH.size();
            cb = BVH.size() + 1;

            node.children = {ca, cb};

            BVH.push_back(child_a);
            BVH.push_back(child_b);

            return true;
        } else return false;
    };

    std::vector<uint32_t> open_nodes = {0};
    std::vector<uint32_t> new_open_nodes = {};

    while(true) {
        if(open_nodes.size() == 0) break;

        for(uint32_t n : open_nodes) {
            if(split(BVH[n])) {
                new_open_nodes.push_back(ca);
                new_open_nodes.push_back(cb);
            }
        }

        open_nodes = std::move(new_open_nodes);
        new_open_nodes.clear();
    }
}


vec2 support(vec2 direction, vec2 center, mat2 orientation, vec2 radii) {
    vec2 local = transpose(orientation) * direction;

    vec2 q = {
        radii.x * radii.x * local.x,
        radii.y * radii.y * local.y
    };

    float denom = sqrt(q.x * local.x + q.y * local.y);
    if(denom == 0) denom = 1.0f;

    vec2 point = orientation * (q / denom);

    return center + point;
}

vec2 support(vec2 direction, vec2 center, mat2 orientation, vec2 radii, std::vector<clipping_plane2d>& planes) {
    vec2 local = transpose(orientation) * direction;

    vec2 q = {
        radii.x * radii.x * local.x,
        radii.y * radii.y * local.y
    };

    float denom = sqrt(q.x * local.x + q.y * local.y);
    if(denom == 0) denom = 1.0f;

    vec2 point = (q / denom);

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

vec2 support(vec2 direction, std::vector<vertex_element2d> ellipsoids) {
    float max_dot = -FLT_MAX;
    vec2 point = vec2(0.0f);

    for(vertex_element2d& e : ellipsoids) {
        vec2 new_point;
        if(e.planes.size()) new_point = support(direction, e.center, e.orientation, e.radii, e.planes);
        else new_point = support(direction, e.center, e.orientation, e.radii);

        float new_dot = dot(new_point, direction);

        if(new_dot > max_dot) {
            max_dot = new_dot;
            point = new_point;
        }
    }

    return point;
}

}