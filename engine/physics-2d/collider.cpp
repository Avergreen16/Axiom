#include <physics-2d/collider.hpp>

namespace axiom {

void collider2d::create_bounding_box() {
    bounding_box.minimum = vec2(FLT_MAX, FLT_MAX);
    bounding_box.maximum = vec2(-FLT_MAX, -FLT_MAX);

    for(collision_shape& cs : shapes) {
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

    BVH_node root;
    for(int i = 0; i < shapes.size(); ++i) root.children.push_back(i);

    BVH.push_back(root);

    uint32_t ca;
    uint32_t cb;

    auto split = [&](BVH_node& node) {
        uint32_t index = 0;

        axiom::bounding_box centers;

        for(int i : node.children) {
            collision_shape& shape = shapes[i];
            vec2 center = (shape.bounding_box.minimum + shape.bounding_box.maximum) * 0.5f;

            node.bounding_box.minimum = glm::min(node.bounding_box.minimum, shape.bounding_box.minimum);
            node.bounding_box.maximum = glm::max(node.bounding_box.maximum, shape.bounding_box.maximum);

            centers.minimum = glm::min(centers.minimum, center);
            centers.maximum = glm::max(centers.maximum, center);
        }

        if(node.children.size() > 1) {
            vec2 size = centers.maximum - centers.minimum;
            vec2 center = (centers.minimum + centers.maximum) * 0.5f;

            BVH_node child_a;
            BVH_node child_b;

            int ii = 0;
            if(size.y > size.x) ii = 1;

            for(int i : node.children) {
                collision_shape& shape = shapes[i];

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

    return center + orientation * (q / denom);
}

vec2 support(vec2 direction, std::vector<vertex_element> ellipsoids) {
    float max_dot = -FLT_MAX;
    vec2 point = vec2(0.0f);

    for(vertex_element& e : ellipsoids) {
        vec2 new_point = support(direction, e.center, e.orientation, e.radii);

        float new_dot = dot(new_point, direction);

        if(new_dot > max_dot) {
            max_dot = new_dot;
            point = new_point;
        }
    }

    return point;
}

}