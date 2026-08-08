#include "physics.hpp"
#include "core.hpp"
#include "input.hpp"

#include <numeric>

// sparse matrix

std::atomic<int> num_bb_checks = 0;
std::atomic<int> num_gjk_checks = 0;

std::unordered_map<ivec3, mat3, Hash_coord> directional; 

void Collider::create_BVH(ivec3 v) {
    Physics_system::create_bounding_box(*this);

    BVH_node root;
    //std::cout << collision_shapes.size() << " ";
    for(int i = 0; i < collision_shapes.size(); ++i) root.children.push_back(i);

    uint32_t N = collision_shapes.size();

    BVH.push_back(root);

    uint32_t ca;
    uint32_t cb;

    bool f = true;

    auto split = [&](BVH_node& node) {
        uint32_t index = 0;

        Bounding_box centers;

        for(int i : node.children) {
            Convex_collider& shape = collision_shapes[i];
            vec3 center = (shape.bounding_box.minimum + shape.bounding_box.maximum) * 0.5f;

            node.bounding_box.minimum = min(node.bounding_box.minimum, shape.bounding_box.minimum);
            node.bounding_box.maximum = max(node.bounding_box.maximum, shape.bounding_box.maximum);

            centers.minimum = min(centers.minimum, center);
            centers.maximum = max(centers.maximum, center);
        }

        if(node.split) {
            vec3 size = centers.maximum - centers.minimum;
            vec3 center = (centers.minimum + centers.maximum) * 0.5f;

            BVH_node child_a;
            BVH_node child_b;

            int ii = 0;
            if(size.y > size.x && size.y > size.z) ii = 1;
            else if(size.z > size.x && size.z > size.y) ii = 2;
            
            for(int i : node.children) {
                Convex_collider& shape = collision_shapes[i];

                float c = (shape.bounding_box.minimum[ii] + shape.bounding_box.maximum[ii]) * 0.5f;
                if(c < center[ii]) child_a.children.push_back(i);
                else child_b.children.push_back(i);
            }

            if(child_a.children.size() == 0)  {
                uint32_t split = child_b.children.size() / 2;
                
                child_a.children = std::vector<uint32_t>(child_b.children.begin() + split, child_b.children.end());
                child_b.children.resize(split);
            } else if(child_b.children.size() == 0) {
                uint32_t split = child_b.children.size() / 2;

                child_b.children = std::vector<uint32_t>(child_a.children.begin() + split, child_a.children.end());
                child_a.children.resize(split);
            }
               
                
            if(child_a.children.size() == 1) child_a.split = false;
            if(child_b.children.size() == 1) child_b.split = false;
            
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

        //std::cout << open_nodes.size() << " ";

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

std::vector<float> operator/(std::vector<float, std::allocator<float>>& a, std::vector<float, std::allocator<float>>& b) {
    if(a.size() == b.size()) {
        std::vector<float> ret;
        for(int i = 0; i < a.size(); ++i) {
            ret.push_back(a[i] / b[i]);
        }

        return ret;
    } else return {};
}

std::vector<float> operator+(std::vector<float> a, std::vector<float> b) {
    if(a.size() == b.size()) {
        std::vector<float> ret;
        for(int i = 0; i < a.size(); ++i) {
            ret.push_back(a[i] + b[i]);
        }

        return ret;
    } else return {};
}

std::vector<float> operator-(std::vector<float> a, std::vector<float> b) {
    if(a.size() == b.size()) {
        std::vector<float> ret;
        for(int i = 0; i < a.size(); ++i) {
            ret.push_back(a[i] - b[i]);
        }

        return ret;
    } else return {};
}

float dot(std::vector<float>& a, std::vector<float>& b) {
    float sum = 0;
    if(a.size() == b.size()) {
        for(int i = 0; i < a.size(); ++i) {
            sum += (a[i] * b[i]);
        }
    }
    return sum;
}

std::vector<float> operator*(std::vector<float> a, float b) {
    std::vector<float> ret;
    for(int i = 0; i < a.size(); ++i) {
        ret.push_back(a[i] * b);
    }

    return ret;
}

Bounding_box Physics_system::create_bounding_box(std::vector<vec3>& vertices) {
    Bounding_box bb;

    bb.minimum = vec3(__FLT_MAX__);
    bb.maximum = vec3(-__FLT_MAX__);

    for(glm::vec3& vertex : vertices) {
        bb.minimum = min(bb.minimum, vertex);
        bb.maximum = max(bb.maximum, vertex);
    }

    return bb;
}

void Physics_system::create_bounding_box(Collider& collider) {
    Bounding_box total_bb;

    collider.init_bb = true;

    total_bb.minimum = vec3(__FLT_MAX__);
    total_bb.maximum = vec3(-__FLT_MAX__);

    for(auto& shape : collider.collision_shapes) {
        Bounding_box bb;

        bb.minimum = vec3(__FLT_MAX__);
        bb.maximum = vec3(-__FLT_MAX__);
        float radius = max(max(shape.collision_shape->radius, shape.collision_shape->split_radius.x), max(shape.collision_shape->split_radius.y, shape.collision_shape->split_radius.z));

        for(glm::vec3& vertex : shape.collision_shape->vertices) {
            glm::vec3 position = vertex;

            vec3 global_position = vec3((shape.orientation * position + (vec3)shape.position));

            bb.minimum = min(bb.minimum, global_position - radius);
            bb.maximum = max(bb.maximum, global_position + radius);
        }

        shape.bounding_box = bb;

        total_bb.maximum = max(total_bb.maximum, bb.maximum);
        total_bb.minimum = min(total_bb.minimum, bb.minimum);

        vec3 size = bb.maximum - bb.minimum;
        bb.volume = size.x * size.y * size.z;
    }
    
    vec3 size = total_bb.maximum - total_bb.minimum;
    total_bb.volume = size.x * size.y * size.z;

    collider.bounding_box = total_bb;
}

/*
Bounding_box Physics_system::create_bounding_box(Collision_shape& cs, Transform& transform) {
    Bounding_box bb;

    bb.minimum = vec3(__FLT_MAX__);
    bb.maximum = vec3(-__FLT_MAX__);
    float radius = max(max(cs.radius, cs.split_radius.x), max(cs.split_radius.y, cs.split_radius.z));
    
    vec<3, int64_t> sector = {transform.position.x.sector, transform.position.y.sector, transform.position.z.sector};
    vec3 fraction = {transform.position.x.fraction, transform.position.y.fraction, transform.position.z.fraction};

    for(glm::vec3 vertex : cs.vertices) {
        glm::vec3 position = vertex;

        vec3 global_position = vec3((transform.orientation * position));

        bb.minimum = min(bb.minimum, global_position - vec3(radius));
        bb.maximum = max(bb.maximum, global_position + vec3(radius));
    }

    bb.minimum += fraction;
    bb.maximum += fraction;

    bb.sector = sector;

    return bb;
}
*/

struct Prune_data {
    uint32_t c;
    pvec3 v;
};

bool sort_x(const Prune_data& a, const Prune_data& b) {
    return float(a.v.x - b.v.x) > 0;
}

bool sort_y(const Prune_data& a, const Prune_data& b) {
    return float(a.v.y - b.v.y) > 0;
}

bool sort_z(const Prune_data& a, const Prune_data& b) {
    return float(a.v.z - b.v.z) > 0;
}

glm::vec3 ww;

glm::vec3 triangle_project(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 point, glm::vec3& ret_p, bool neg_term = true) {
    glm::vec3 product = glm::cross(a - c, b - c);
    glm::vec3 normal = glm::normalize(product);

    glm::vec3 p = (point - a) - normal * glm::dot((point - a), normal) + a;

    glm::vec3 product_0 = glm::cross(b - p, c - p);

    glm::vec3 product_1 = glm::cross(c - p, a - p);

    glm::vec3 product_2 = glm::cross(a - p, b - p);

    float product_length = glm::length(product);

    glm::vec3 weights = glm::vec3(glm::dot(product_0, normal) / product_length, glm::dot(product_1, normal) / product_length, glm::dot(product_2, normal) / product_length);

    ww = weights;

    bool r = 0.0f;
    
    if(neg_term && glm::dot(product_2, normal) < 0) r = true;
    if(neg_term && glm::dot(product_1, normal) < 0) r = true;
    if(neg_term && glm::dot(product_0, normal) < 0) r = true;

    if(r) {
        return glm::vec3(-1);
    }

    ret_p = p;

    return weights;
}

glm::vec3 project_clamp(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 point) {
    glm::vec3 product = glm::cross(a - c, b - c);
    glm::vec3 normal = glm::normalize(product);

    glm::vec3 p = (point - a) - normal * glm::dot((point - a), normal) + a;

    glm::vec3 product_0 = glm::cross(p - c, b - c);

    glm::vec3 product_1 = glm::cross(a - c, p - c);

    glm::vec3 product_2 = glm::cross(a - p, b - p);

    float p0 = dot(product_0, normal);
    float p1 = dot(product_1, normal);
    float p2 = dot(product_2, normal);
    
    float product_length = glm::length(product);

    // p0 -> b and c
    // p1 -> a and c
    // p2 -> a and b

    if(p0 < 0.0f) {
        if(p1 < 0.0f) {
            return vec3(0.0f, 0.0f, 1.0f);
        } else if(p2 < 0.0f) {
            return vec3(0.0f, 1.0f, 0.0f);
        } else {
            vec3 v0 = b;
            vec3 v1 = c;

            vec3 diff = v1 - v0;
            float len = length(diff);
            diff /= len;

            vec3 v2 = p - v0;
            float f = clamp(dot(diff, v2), 0.0f, len) / len;

            return vec3(0.0f, 1.0f - f, f);
        }
    } else if(p1 < 0.0f) {
        if(p2 < 0.0f) {
            return vec3(1.0f, 0.0f, 0.0f);
        } else {
            vec3 v0 = a;
            vec3 v1 = c;

            vec3 diff = v1 - v0;
            float len = length(diff);
            diff /= len;

            vec3 v2 = p - v0;
            float f = clamp(dot(diff, v2), 0.0f, len) / len;

            return vec3(1.0f - f, 0.0f, f);
        }
    } else if(p2 < 0.0f) {
        vec3 v0 = a;
        vec3 v1 = b;

        vec3 diff = v1 - v0;
        float len = length(diff);
        diff /= len;

        vec3 v2 = p - v0;
        float f = clamp(dot(diff, v2), 0.0f, len) / len;

        return vec3(1.0f - f, f, 0.0f);
    } else {
        return vec3(p0, p1, p2) / product_length;
    }
}

struct Simplex_vertex {
    glm::vec3 m;
    glm::vec3 a;
    glm::vec3 b;
};

struct Simplex {
    std::vector<Simplex_vertex> vertices;

    uint32_t find_closest_face(glm::vec3& weights, glm::vec3& dir) {
        float dist = __FLT_MAX__;
        uint32_t f = -1;
        weights = glm::vec3(-1);

        for(int i = 0; i < 4; ++i) {
            glm::vec3 p;

            glm::vec3 w = triangle_project(vertices[(i <= 0) ? 1 : 0].m, vertices[(i <= 1) ? 2 : 1].m, vertices[(i <= 2) ? 3 : 2].m, glm::vec3(0.0f), p);

            if(w.x != -1) {
                float length_p = glm::length(p);

                if(length_p < dist) {
                    dir = glm::normalize(glm::cross(vertices[(i <= 0) ? 1 : 0].m - vertices[(i <= 2) ? 3 : 2].m, vertices[(i <= 1) ? 2 : 1].m - vertices[(i <= 2) ? 3 : 2].m));

                    dist = length_p;
                    f = i;
                    weights = w;
                }
            }
        }

        return f;
    }
};

std::vector<glm::vec3> sphere_points() {
    std::vector<glm::vec3> return_vector;
    for(int z = -1; z <= 1; ++z) {
        for(int y = -1; y <= 1; ++y) {
            for(int x = -1; x <= 1; ++x) {
                if(!(z == 0 && y == 0 && x == 0)) {
                    glm::vec3 v = {x, y, z};
                    return_vector.push_back(glm::normalize(v));
                }
            }
        }
    }

    return return_vector;
}

void get_normal(glm::vec3& a, glm::vec3& b, glm::vec3& c, glm::vec3& dir_vertex, glm::vec3& output_normal, glm::vec3& output_centroid) {
    output_centroid = (a + b + c) / 3.0f;

    output_normal = glm::normalize(glm::cross(a - c, b - c));

    if(glm::dot(dir_vertex - output_centroid, output_normal) > 0.0f) output_normal = -output_normal;
}

glm::vec3 triangle_ray(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 ray_dir) {
    glm::vec3 product = glm::cross(a - c, b - c);
    glm::vec3 normal = glm::normalize(product);

    float dot = glm::dot(ray_dir, normal);
    if(dot == 0) return glm::vec3(-1);

    float d = glm::dot(normal, a);

    float t = d / dot;

    if(t < 0) return glm::vec3(-1);

    glm::vec3 p = t * ray_dir;

    glm::vec3 product_0 = glm::cross(p - c, b - c);
    if(glm::dot(product_0, normal) < 0.0f) return glm::vec3(-1);

    glm::vec3 product_1 = glm::cross(a - c, p - c);
    if(glm::dot(product_1, normal) < 0.0f) return glm::vec3(-1);

    glm::vec3 product_2 = glm::cross(a - p, b - p);
    if(glm::dot(product_2, normal) < 0.0f) return glm::vec3(-1);

    float product_length = glm::length(product);

    glm::vec3 weights = glm::vec3(glm::length(product_0) / product_length, glm::length(product_1) / product_length, glm::length(product_2) / product_length);

    return weights;
}

float plane_ray_zero(vec3 pp, vec3 np) {
    return glm::dot(np, pp);
}

int simplex_contains(glm::vec3 p, std::vector<Simplex_vertex>& points, bool output = false) {
    glm::vec3 centroid;
    glm::vec3 normal;
    float dotp;

    bool v0 = false;
    bool v1 = false;
    bool v2 = false;

    float dotp0 = __FLT_MAX__;
    float dotp1 = __FLT_MAX__;
    float dotp2 = __FLT_MAX__;

    get_normal(points[1].m, points[2].m, points[3].m, points[0].m, normal, centroid);
    dotp = glm::dot(p - centroid, normal);
    
    if(dotp > 0.0f) {
        v0 = true;
        dotp0 = dotp;
    }

    get_normal(points[0].m, points[2].m, points[3].m, points[1].m, normal, centroid);
    dotp = glm::dot(p - centroid, normal);
    
    if(dotp > 0.0f) {
        v1 = true;
        dotp1 = dotp;
    }

    get_normal(points[0].m, points[1].m, points[3].m, points[2].m, normal, centroid);
    dotp = glm::dot(p - centroid, normal);
    
    if(dotp >= 0.0f) {
        v2 = true;
        dotp2 = dotp;
    }

    if(dotp0 != __FLT_MAX__ && dotp0 < dotp1 && dotp0 < dotp2) {
        return 0;
    }
    if(dotp1 != __FLT_MAX__ && dotp1 < dotp0 && dotp1 < dotp2) {
        return 1;
    }
    if(dotp2 != __FLT_MAX__ && dotp2 < dotp0 && dotp2 < dotp1) {
        return 2;
    }

    return -1;
}

struct Polytope_return {
    std::vector<Simplex_vertex> vertices;
    glm::vec3 normal;
    glm::vec3 weights = vec3(-1.0f);
    bool f = false;
};

struct Polytope_face {
    std::vector<uint32_t> vertices;
    glm::vec3 normal;
};

float ddd;
bool error_outside = false;

struct Polytope {
    std::vector<Simplex_vertex> vertices;
    std::vector<Polytope_face> faces;
    vec3 center = vec3(0.0f);

    Polytope_return find_closest_face() {
        float dist = __FLT_MAX__;
        Polytope_return ret;

        Polytope_face* ret_face = nullptr;
        vec3 p;

        bool flip = false;

        for(Polytope_face& f : faces) {
            glm::vec3 a = vertices[f.vertices[0]].m;
            glm::vec3 normal = f.normal;

            Polytope_face* fface = &f;
            
            float d = -dot(-a, normal);
            if(d < dist) {
                ret_face = fface;
                dist = d;
            }
        }

        ddd = dist;

        vec3 wv = triangle_project(vertices[ret_face->vertices[0]].m, vertices[ret_face->vertices[1]].m, vertices[ret_face->vertices[2]].m, glm::vec3(0.0f), p, false);
        ret.normal = ret_face->normal;
        ret.weights = wv;
        ret.vertices = {vertices[ret_face->vertices[0]], vertices[ret_face->vertices[1]], vertices[ret_face->vertices[2]]};

        return ret;
    }

    void insert_face(std::vector<uint32_t> v) {
        glm::vec3 centroid = (vertices[v[0]].m + vertices[v[1]].m + vertices[v[2]].m) / 3.0f;
        glm::vec3 normal = glm::normalize(glm::cross(vertices[v[0]].m - vertices[v[2]].m, vertices[v[1]].m - vertices[v[2]].m));

        // error happened here (glitchy box)
        if(glm::dot(normal, (center / float(vertices.size())) - centroid) >= 0.0f) normal = -normal;

        faces.push_back(Polytope_face(v, normal));
    }

    void from_simplex(Simplex s) {
        vertices = {s.vertices[0], s.vertices[1], s.vertices[2], s.vertices[3]};

        center += s.vertices[0].m;
        center += s.vertices[1].m;
        center += s.vertices[2].m;
        center += s.vertices[3].m;

        insert_face({0, 1, 2});
        insert_face({0, 1, 3});
        insert_face({0, 2, 3});
        insert_face({1, 2, 3});
    }

    void expand(Simplex_vertex vertex) {
        uint32_t v_n = vertices.size();
        vertices.push_back(vertex);
        center += vertex.m;

        float min_d = FLT_MAX;
        std::vector<uint64_t> edges;
        std::vector<uint32_t> faces_seen;
        for(int i = 0; i < faces.size(); ++i) {
            glm::vec3 diff = vertex.m - vertices[faces[i].vertices[0]].m;

            if(glm::dot(faces[i].normal, diff) > 0.0f) {
                min_d = min(glm::dot(faces[i].normal, diff), min_d);

                faces_seen.push_back(i);
            }
        }

        for(uint32_t f : faces_seen) {
            Polytope_face face = faces[f];
            uint64_t edge_a = (uint64_t(std::min(face.vertices[0], face.vertices[1])) | (uint64_t(std::max(face.vertices[0], face.vertices[1])) << 32));
            uint64_t edge_b = (uint64_t(std::min(face.vertices[1], face.vertices[2])) | (uint64_t(std::max(face.vertices[1], face.vertices[2])) << 32));
            uint64_t edge_c = (uint64_t(std::min(face.vertices[0], face.vertices[2])) | (uint64_t(std::max(face.vertices[0], face.vertices[2])) << 32));

            edges.push_back(edge_a);
            edges.push_back(edge_b);
            edges.push_back(edge_c);
        }

        //std::sort(faces_seen.begin(), faces_seen.end());
        
        int i = 0;
        for(uint32_t f : faces_seen) {
            faces.erase(faces.begin() + f - i);
            ++i;
        }

        for(uint64_t edge : edges) {
            if(std::count(edges.begin(), edges.end(), edge) == 1) {
                uint32_t a = (edge & 0xFFFFFFFFull);
                uint32_t b = (edge >> 32);
                
                insert_face({a, b, v_n});
            }
        }

        //if(ii == false) std::cout << "ERROR " << min_d << "\n";
    }
};

vec3 transform_vertices(Transform& transform, Convex_collider& collider_shape, std::vector<vec3>& output, pvec3 global_offset) {
    vec3 center = vec3(0.0);

    vec3 offset = vec3(transform.position - global_offset);
    vec3 csp = vec3(collider_shape.position);
    for(glm::vec3& vertex : collider_shape.collision_shape->vertices) {
        vec3 v = transform.orientation * (collider_shape.orientation * vertex + csp) + offset;
        center += v;

        output.push_back(v);
    }

    return center / (float)collider_shape.collision_shape->vertices.size();
}

mat3 identity_mat = identity<mat3>();

glm::vec3 support_func(std::vector<glm::vec3>& vertices, vec3 radius, glm::vec3 direction, mat3& orientation = identity_mat, uint32_t* c = nullptr) {
    glm::vec3 r;
    float dot_product = -__FLT_MAX__;

    uint32_t n = 0;
    for(glm::vec3& v : vertices) {
        float d = glm::dot(direction, v);

        if(d > dot_product) {
            dot_product = d;
            r = v;
            if(c != nullptr) *c = n;
        }
        ++n;
    }

    //r += direction * radius;

    if(radius.x == radius.y && radius.x == radius.z) {
        r += direction * radius;
    } else {
        vec3 dir = transpose(orientation) * direction;
        vec3 ellipsoid = vec3(radius.x * radius.x * dir.x, radius.y * radius.y * dir.y, radius.z * radius.z * dir.z) * (1.0f / sqrt(radius.x * radius.x * dir.x * dir.x + radius.y * radius.y * dir.y * dir.y + radius.z * radius.z * dir.z * dir.z));
        ellipsoid = orientation * ellipsoid;

        r += ellipsoid;
    }

    return r;
}

uint32_t support_func(std::vector<glm::vec3>& vertices, glm::vec3 direction) {
    uint32_t r;
    float dot_product = -__FLT_MAX__;

    for(int i = 0; i < vertices.size(); ++i) {
        vec3 v = vertices[i];
        float d = glm::dot(direction, v);

        if(d > dot_product) {
            dot_product = d;
            r = i;
        }
    }

    return r;
}

uint32_t support_func(std::vector<glm::vec3>& vertices, std::unordered_set<uint32_t>& set, glm::vec3 direction) {
    uint32_t r;
    float dot_product = -__FLT_MAX__;

    for(int i = 0; i < vertices.size(); ++i) {
        vec3 v = vertices[i];
        float d = glm::dot(direction, v);

        if(d > dot_product && set.contains(i)) {
            dot_product = d;
            r = i;
        }
    }

    return r;
}

uint32_t support_func(std::vector<glm::vec2>& vertices, std::unordered_set<uint32_t>& set, glm::vec2 direction) {
    uint32_t r;
    float dot_product = -__FLT_MAX__;

    for(int i = 0; i < vertices.size(); ++i) {
        vec2 v = vertices[i];
        float d = glm::dot(direction, v);

        if(d > dot_product && set.contains(i)) {
            dot_product = d;
            r = i;
        }
    }

    return r;
}

std::vector<Return_point> Physics_system::collision(Transform& ta, Convex_collider& ca, Transform& tb, Convex_collider& cb, Return_tag& tag) {
    ++num_gjk_checks;

    std::vector<glm::vec3> a_vertices;
    std::vector<glm::vec3> b_vertices;

    float limit = 0.0001f;
    uint32_t iter_limit = 64;

    vec3 a_rel_pos = transform_vertices(ta, ca, a_vertices, ta.position);
    vec3 b_rel_pos = transform_vertices(tb, cb, b_vertices, ta.position);

    bool set = false;

    Simplex simplex;
    
    vec3 rad_a = vec3(ca.collision_shape->radius);
    if(rad_a.x == 0.0f) rad_a = ca.collision_shape->split_radius;
    vec3 rad_b = vec3(cb.collision_shape->radius);
    if(rad_b.x == 0.0f) rad_b = cb.collision_shape->split_radius;

    glm::vec3 direction = glm::normalize(vec3(a_rel_pos - b_rel_pos));

    vec3 offset = vec3(direction.y, -direction.z, direction.x);

    if(abs(glm::dot(offset, direction)) > 0.95) {
        offset = vec3(direction.z, -direction.y, direction.x);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    auto get_normal = [&](Convex_collider& c, mat3 ori, vec3 dir) -> shape_face* {
        float dd = -FLT_MAX;
        vec3 vec = dir;
        int32_t id = -1;

        uint32_t i = 0;
        for(shape_face& t : c.collision_shape->faces) {
            vec3 normal = ori * c.orientation * t.normal;

            float d = dot(normal, dir);

            if(d > dd) {
                vec = normal;
                dd = d;
                id = i;
            }

            ++i;
        }

        if(id >= 0) return &c.collision_shape->faces[id];
        else return nullptr;
    };
    
    float d = 0.0f;

    std::vector<uint32_t> selected;

    while(loop) {
        ++iterations;
        
        int size = simplex.vertices.size();
        if(size < 4) {
            if(iterations > iter_limit) {
                return {};
            }

            if(isnan(direction.x)) {
                std::cout << "NAN DIRECTION" << size << "\n";
                direction = vec3(1, 0, 0);
            }

            uint32_t ia;
            uint32_t ib;

            glm::vec3 point_a = support_func(a_vertices, rad_a, direction, ta.orientation, &ia);
            glm::vec3 point_b = support_func(b_vertices, rad_b, -direction, tb.orientation, &ib);

            GJK_step normal_step;
            normal_step.type = 1;

            GJK_step update_step;
            update_step.is = {ia, ib};
            update_step.type = 0;
            //pv.steps.push_back(update_step);

            glm::vec3 point_m = point_a - point_b;

            for(Simplex_vertex& v : simplex.vertices) {
                glm::vec3 difference = point_m - v.m;

                float dist = length(difference);

                if(dist == 0.0f) return {};
            }

            if(glm::dot(point_m, direction) <= limit) return {};

            simplex.vertices.push_back(Simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);

                normal_step.vs = {point_m, direction};
                //pv.steps.push_back(normal_step);
            } else if(size == 1) {
                glm::vec3 line_direction = normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                glm::vec3 rel_origin_pos = -simplex.vertices[1].m;

                glm::vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
                
                normal_step.vs = {closest_point, direction};
                //pv.steps.push_back(normal_step);
            } else if(size == 2) {
                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;
                d = glm::dot(normal, -center);

                direction = normal;
                
                normal_step.vs = {center, direction};
                //pv.steps.push_back(normal_step);
            }
        } else {
            int n = simplex_contains(glm::vec3(0, 0, 0), simplex.vertices, iterations > iter_limit);
            if(n == -1) {
                bool loop_epa = true;

                glm::vec3 weights;

                glm::vec3 contact_point_a;
                glm::vec3 contact_point_b;
                glm::vec3 separation_vector;
                glm::vec3 collision_normal;

                Polytope p;
                p.from_simplex(simplex);

                iterations = 0;
                uint32_t pass = 0;

                while(true) {
                    ++iterations;
                    
                    Polytope_return r = p.find_closest_face();

                    if(r.f) set = true;
                    
                    if(r.vertices.size() == 0) {
                        std::cout << "ERROR: ZERO\n";
                        return {};
                    }

                    direction = r.normal;
                
                    uint32_t ia, ib;
                    glm::vec3 point_a = support_func(a_vertices, rad_a, direction, ta.orientation, &ia);
                    glm::vec3 point_b = support_func(b_vertices, rad_b, -direction, tb.orientation, &ib);

                    glm::vec3 point_m = point_a - point_b;

                    float dist = dot(point_m, r.normal);
                    
                    if(iterations > iter_limit) {
                        std::cout << "limit EPA";
                        return {};
                    }

                    // end epa

                    float limit_2 = 0.01f;

                    if(abs(dist - dot(r.vertices[0].m, r.normal)) < limit_2) {
                        ++pass;

                        contact_point_a = r.vertices[0].a * r.weights.x + r.vertices[1].a * r.weights.y + r.vertices[2].a * r.weights.z;
                        contact_point_b = r.vertices[0].b * r.weights.x + r.vertices[1].b * r.weights.y + r.vertices[2].b * r.weights.z;

                        vec3 main_dir = a_rel_pos - b_rel_pos;
                        vec3 collision_normal;
                        bool line = false;
                        float sep = length(contact_point_a - contact_point_b);
                        
                        if(r.vertices[2].a != r.vertices[0].a && r.vertices[2].a != r.vertices[1].a && r.vertices[0].a != r.vertices[1].a) { // triangle to vertex, triangle is a
                            collision_normal = normalize(cross(r.vertices[0].a - r.vertices[2].a, r.vertices[1].a - r.vertices[2].a));

                            tag.type = COLLISION_TYPE_FACE;
                            tag.vid_a[0] = ivec3(r.vertices[0].a * 64.0f);
                            tag.vid_a[1] = ivec3(r.vertices[1].a * 64.0f);
                            tag.vid_a[2] = ivec3(r.vertices[2].a * 64.0f);
                            tag.vid_b[0] = ivec3(r.vertices[0].b * 64.0f);
                        } else if(r.vertices[2].b != r.vertices[0].b && r.vertices[2].b != r.vertices[1].b && r.vertices[0].b != r.vertices[1].b) { // triangle to vertex, triangle is b
                            collision_normal = normalize(cross(r.vertices[0].b - r.vertices[2].b, r.vertices[1].b - r.vertices[2].b));

                            tag.type = COLLISION_TYPE_EDGE;
                            tag.vid_a[0] = ivec3(r.vertices[0].a * 64.0f);
                            tag.vid_b[0] = ivec3(r.vertices[0].b * 64.0f);
                            tag.vid_b[1] = ivec3(r.vertices[1].b * 64.0f);
                            tag.vid_b[2] = ivec3(r.vertices[2].b * 64.0f);
                        } else { // line to line
                            vec3 a0;
                            vec3 a1;
                            vec3 b0;
                            vec3 b1;

                            if(r.vertices[0].a != r.vertices[1].a) {
                                a0 = r.vertices[0].a;
                                a1 = r.vertices[1].a;
                            } else {
                                a0 = r.vertices[0].a;
                                a1 = r.vertices[2].a;
                            }

                            if(r.vertices[0].b != r.vertices[1].b) {
                                b0 = r.vertices[0].b;
                                b1 = r.vertices[1].b;
                            } else {
                                b0 = r.vertices[0].b;
                                b1 = r.vertices[2].b;
                            }

                            collision_normal = normalize(cross(a0 - a1, b0 - b1));

                            line = true;

                            tag.type = COLLISION_TYPE_VERTEX;
                            tag.vid_a[0] = ivec3(a0 * 64.0f);
                            tag.vid_a[1] = ivec3(a1 * 64.0f);
                            tag.vid_b[0] = ivec3(b0 * 64.0f);
                            tag.vid_b[1] = ivec3(b1 * 64.0f);
                        }
                        
                        std::vector<Return_point> return_points;
                        if(dot(collision_normal, main_dir) < 0.0f) collision_normal = -collision_normal;
                        
                        if(ca.collision_shape->faces.size() == 1) {
                            if(dot(collision_normal, ca.collision_shape->faces[0].normal) > 0.1f) collision_normal = -collision_normal;
                        } else if(cb.collision_shape->faces.size() == 1) {
                            if(dot(collision_normal, -cb.collision_shape->faces[0].normal) > 0.1f) collision_normal = -collision_normal;
                        }
                        
                        shape_face* af = get_normal(ca, ta.orientation, -collision_normal);
                        shape_face* bf = get_normal(cb, tb.orientation, collision_normal);

                        float threshold = cos(30.0f * M_PI / 180.0f);
                        float dot_a = 0.0f;// 
                        float dot_b = 0.0f;//
                        if(af) dot_a = dot(af->normal, -collision_normal);
                        if(bf) dot_b = dot(bf->normal, collision_normal);
                        
                        if(ca.collision_shape->faces.size() == 0 || cb.collision_shape->faces.size() == 0 || af == nullptr || bf == nullptr) return_points.push_back(Return_point(contact_point_a, contact_point_b, collision_normal));
                        else {
                            shape_face& a_face = *af;
                            shape_face& b_face = *bf;

                            vec3 a_normal = ta.orientation * ca.orientation * a_face.normal;
                            vec3 b_normal = tb.orientation * cb.orientation * b_face.normal;

                            std::vector<vec2> a_verts;
                            std::vector<vec2> b_verts;

                            mat3 rot_mat = rotate_to(collision_normal, vec3(0, 0, 1));
                            vec3 a_c = rot_mat * a_vertices[af->vertices[0]];
                            vec3 b_c = rot_mat * b_vertices[bf->vertices[0]];

                            for(uint32_t i : a_face.vertices) {
                                a_verts.push_back((rot_mat * a_vertices[i]).xy());
                            }
                            for(uint32_t i : b_face.vertices) {
                                b_verts.push_back((rot_mat * b_vertices[i]).xy());
                            }

                            a_normal = rot_mat * a_normal;
                            b_normal = rot_mat * b_normal;
                            
                            rot_mat = transpose(rot_mat);

                            if((a_verts.size() <= 2 && b_verts.size() <= 2) || (a_verts.size() <= 1 || b_verts.size() <= 1)) return_points.push_back(Return_point(contact_point_a, contact_point_b, collision_normal));
                            else {
                                std::vector<vec2> vertices_c;

                                vec2 a_center = vec2(0.0f);
                                for(vec2 v : a_verts) {
                                    a_center += v.xy();
                                }
                                a_center /= a_verts.size();

                                vec2 b_center = vec2(0.0f);
                                for(vec2 v : b_verts) {
                                    b_center += v.xy();
                                }
                                b_center /= b_verts.size();

                                vec2 center;
                                vec2 vv = vec2(0.0f, 1.0f);

                                bool flip = false;
                                
                                for(vec2 v : b_verts) {
                                    vertices_c.push_back(v);
                                }

                                std::function<void()> sutherland_hodgman = [&]() {
                                    vec2 center = vec2(0.0f);
                                    int num_verts = 0;
                                    for(int i = 0; i < a_verts.size(); ++i) {
                                        center += a_verts[i];
                                        ++num_verts;
                                    }
                                    center /= num_verts;

                                    for(int i = 0; i < a_verts.size(); ++i) {
                                        vec2 C = a_verts[i];
                                        vec2 D = a_verts[(i + 1) % a_verts.size()];
                                        std::vector<vec2> new_c;
                                        vec3 c = cross(vec3(D - C, 0), vec3(center - C, 0));

                                        for(int j = 0; j < vertices_c.size(); ++j) {  
                                            vec2 A = vertices_c[j];
                                            vec2 B = vertices_c[(j + 1) % vertices_c.size()];

                                            vec2 d = normalize(D - C);
                                            d = vec2(-d.y, d.x);

                                            vec2 b = vec2(B - A);
                                            vec2 a = vec2(C - A);

                                            float m = dot(d, b);
                                            float x = dot(d, a) / m;

                                            vec2 E;
                                            bool has_E = false;

                                            if(x != NAN) {
                                                if(x > 0 && x < 1) {
                                                    E = vec2(A.xy() + b * x);
                                                    has_E = true;
                                                }
                                            }

                                            if(dot(cross(vec3(D - C, 0), vec3(A - C, 0)), c) > 0) {
                                                if(has_E) {
                                                    new_c.push_back(A);
                                                    new_c.push_back(E);
                                                } else {
                                                    new_c.push_back(A);
                                                }
                                            } else {
                                                if(has_E) {
                                                    new_c.push_back(E);
                                                }
                                            }
                                        }

                                        vertices_c = new_c;
                                    }
                                };

                                sutherland_hodgman();

                                if(vertices_c.size() == 0) return_points.push_back(Return_point(a_c, b_c, collision_normal));
                                
                                Particle_system& ps = ecs.get_system<Particle_system>();

                                // FLAG

                                for(vec2 v : vertices_c) { 
                                    vec3 va = vec3(v, 0);
                                    float a_z = dot(a_c - va, a_normal) / a_normal.z;
                                    va.z = a_z;
                                    //va.z = a_c.z;

                                    vec3 vb = vec3(v, 0);
                                    float b_z = dot(b_c - vb, b_normal) / b_normal.z;
                                    vb.z = b_z;
                                    //vb.z = b_c.z;

                                    return_points.push_back(Return_point(rot_mat * va, rot_mat * vb, collision_normal));
                                }
                            }
                        }

                        for(Return_point& rp : return_points) {
                            rp.a = rp.a;
                            rp.b = rp.b + pvec3(ta.position - tb.position);
                            rp.normal = collision_normal;
                        }

                        /*
                        if(set) {
                            pv.pos = pvec3(pnum(0xC3500001E20BE2, 0xEC), pnum(-0x752FFFFEDEC2A3, 0xFE), pnum(0x9C400000909141, 0x8D));
                            Physics_system& ps = ecs.get_system<Physics_system>();
                            ps.visualizer = pv;
                        }
                        */
                        return return_points;
                    } else {
                        p.expand({point_m, point_a, point_b});
                    }
                }
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;
            }
        }
    }
}

bool Physics_system::GJK(Convex_collider& ca, Transform& ta, Convex_collider& cb, Transform& tb) {
    std::vector<glm::vec3> a_vertices;
    std::vector<glm::vec3> b_vertices;

    float limit = 0.001f;
    uint32_t iter_limit = 128;

    vec3 a_rel_pos = transform_vertices(ta, ca, a_vertices, ta.position);
    vec3 b_rel_pos = transform_vertices(tb, cb, b_vertices, ta.position);

    Simplex simplex;
    
    vec3 rad_a = vec3(ca.collision_shape->radius);
    if(rad_a.x == 0.0f) rad_a = ca.collision_shape->split_radius;
    vec3 rad_b = vec3(cb.collision_shape->radius);
    if(rad_b.x == 0.0f) rad_b = cb.collision_shape->split_radius;

    glm::vec3 direction = glm::normalize(vec3(ta.position - tb.position));

    vec3 offset = vec3(direction.y, -direction.z, direction.x);

    if(abs(glm::dot(offset, direction)) > 0.95) {
        offset = vec3(direction.z, -direction.y, direction.x);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    auto get_normal = [&](Convex_collider& c, mat3 ori, vec3 dir) -> shape_face* {
        float dd = -FLT_MAX;
        vec3 vec = dir;
        int32_t id = -1;

        uint32_t i = 0;
        for(shape_face& t : c.collision_shape->faces) {
            vec3 normal = ori * t.normal;

            float d = dot(normal, dir);

            if(d > dd) {
                vec = normal;
                dd = d;
                id = i;
            }

            ++i;
        }

        if(id >= 0) return &c.collision_shape->faces[id];
        else return nullptr;
    };

    while(loop) {
        ++iterations;
        
        int size = simplex.vertices.size();
        if(size < 4) {
            if(iterations > iter_limit) return false;

            if(isnan(direction.x)) {
                std::cout << "NAN DIRECTION\n";
                direction = vec3(1, 0, 0);
            }

            glm::vec3 point_a = support_func(a_vertices, rad_a, direction, ta.orientation);
            glm::vec3 point_b = support_func(b_vertices, rad_b, -direction, tb.orientation);

            glm::vec3 point_m = point_a - point_b;

            for(Simplex_vertex& v : simplex.vertices) {
                glm::vec3 difference = point_m - v.m;

                float dist = dot(difference, direction);

                if(dist <= limit) return false;
            }

            if(glm::dot(point_m, direction) <= limit) return false;

            simplex.vertices.push_back(Simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);
            } else if(size == 1) {
                glm::vec3 line_direction = normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                glm::vec3 rel_origin_pos = -simplex.vertices[1].m;

                glm::vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            } else if(size == 2) {
                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;
            }
        } else {
            int n = simplex_contains(glm::vec3(0, 0, 0), simplex.vertices, iterations > iter_limit);
            if(n == -1) {
                return true;
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;
            }
        }
    }
}


bool Physics_system::GJK(Temporary_collider& a, Temporary_collider& b) {
    std::vector<glm::vec3> a_vertices;
    std::vector<glm::vec3> b_vertices;

    float limit = 0.000001f;
    uint32_t iter_limit = 128;

    vec3 b_rel_pos = vec3(b.position - a.position);
    vec3 a_rel_pos = vec3(0.0f);

    for(vec3 v : a.vertices) {
        a_vertices.push_back(a.orientation * v);
    }
    for(vec3 v : b.vertices) {
        b_vertices.push_back(b.orientation * v + b_rel_pos);
    }
    vec3 rad_a = vec3(a.radius);
    vec3 rad_b = vec3(b.radius);



    Simplex simplex;    

    glm::vec3 direction = glm::normalize(vec3(a.position - b.position));

    vec3 offset = vec3(direction.y, -direction.z, direction.x);

    if(abs(glm::dot(offset, direction)) > 0.95) {
        offset = vec3(direction.z, -direction.y, direction.x);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    auto get_normal = [&](Convex_collider& c, mat3 ori, vec3 dir) -> shape_face* {
        float dd = -FLT_MAX;
        vec3 vec = dir;
        int32_t id = -1;

        uint32_t i = 0;
        for(shape_face& t : c.collision_shape->faces) {
            vec3 normal = ori * t.normal;

            float d = dot(normal, dir);

            if(d > dd) {
                vec = normal;
                dd = d;
                id = i;
            }

            ++i;
        }

        if(id >= 0) return &c.collision_shape->faces[id];
        else return nullptr;
    };

    while(loop) {
        ++iterations;
        
        int size = simplex.vertices.size();
        if(size < 4) {
            if(iterations > iter_limit) return false;

            if(isnan(direction.x)) {
                std::cout << "NAN DIRECTION\n";
                direction = vec3(1, 0, 0);
            }

            glm::vec3 point_a = support_func(a_vertices, rad_a, direction, a.orientation);
            glm::vec3 point_b = support_func(b_vertices, rad_b, -direction, b.orientation);

            glm::vec3 point_m = point_a - point_b;

            for(Simplex_vertex& v : simplex.vertices) {
                glm::vec3 difference = point_m - v.m;

                float dist = dot(difference, direction);

                if(dist <= limit) return false;
            }

            if(glm::dot(point_m, direction) <= limit) return false;

            simplex.vertices.push_back(Simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);
            } else if(size == 1) {
                glm::vec3 line_direction = normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                glm::vec3 rel_origin_pos = -simplex.vertices[1].m;

                glm::vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            } else if(size == 2) {
                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;
            }
        } else {
            int n = simplex_contains(glm::vec3(0, 0, 0), simplex.vertices, iterations > iter_limit);
            if(n == -1) {
                return true;
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;
            }
        }
    }
}

Bounding_box transform_bb(Bounding_box b, vec3 pos, mat3 ori) {
    vec3 center = (b.minimum + b.maximum) * 0.5f;
    vec3 extents = b.maximum - center;

    center = ori * center + pos;
    extents = mat3(abs(ori[0]), abs(ori[1]), abs(ori[2])) * extents;

    b.minimum = center - extents;
    b.maximum = center + extents;

    return b;
};

bool Physics_system::collision(Transform& ta, Bounding_box& a, Transform& tb, Bounding_box& b) {
    ++num_bb_checks;

    if(a.volume < b.volume) {
        vec3 pos = transpose(tb.orientation) * vec3(ta.position - tb.position);
        mat3 ori = transpose(tb.orientation) * ta.orientation;

        Bounding_box na = transform_bb(a, pos, ori);

        bool xb = na.minimum.x < b.maximum.x && b.minimum.x < na.maximum.x;
        bool yb = na.minimum.y < b.maximum.y && b.minimum.y < na.maximum.y;
        bool zb = na.minimum.z < b.maximum.z && b.minimum.z < na.maximum.z;

        return xb && yb && zb;
    } else {
        vec3 pos = transpose(ta.orientation) * vec3(tb.position - ta.position);
        mat3 ori = transpose(ta.orientation) * tb.orientation;

        Bounding_box nb = transform_bb(b, pos, ori);

        bool xb = a.minimum.x < nb.maximum.x && nb.minimum.x < a.maximum.x;
        bool yb = a.minimum.y < nb.maximum.y && nb.minimum.y < a.maximum.y;
        bool zb = a.minimum.z < nb.maximum.z && nb.minimum.z < a.maximum.z;

        return xb && yb && zb;
    }
}

void Physics_system::insert_collision(Manifold& data) {
    std::array<uint32_t, 2> key;
    if(data.a > data.b) {
        key = {data.b, data.a};
    } else key = {data.a, data.b};

    if(collision_table.contains(key)) {
        collision_table[key].push_back(data);
    } else {
        collision_table.emplace(key, std::vector<Manifold>{data});
    }
};

void Physics_system::erase_collisions(uint32_t entity) {
    std::vector<std::array<uint32_t, 2>> delete_k;
    for(auto& [k, f] : collision_table) {
        if(k[0] == entity || k[1] == entity) {
            delete_k.push_back(k);
        }
    }

    for(auto k : delete_k) {
        collision_table.erase(k);
    }
};

void Physics_system::merge_manifolds(Manifold& a, Manifold& b) {
    Manifold ret = a;

    ret.normal = a.normal * (float)a.points.size() + b.normal * (float)b.points.size();
    ret.normal = normalize(ret.normal);

    for(Collision_data& c : b.points) {
        bool insert = true;
        for(int i = 0; i < ret.points.size(); ++i) {
            Collision_data& data_b = ret.points[i];

            vec3 diff_a = data_b.contact_point.a - c.contact_point.a;
            vec3 diff_b = data_b.contact_point.b - c.contact_point.b;

            if(length(diff_a) < contact_sep && length(diff_b) < contact_sep) {
                insert = false;
                break;
            }
        }

        if(insert) ret.points.push_back(c);
    }

    a = ret;
}

Contact_point Physics_system::get_points(Collision_data& data, uint32_t a, uint32_t b) {
    Transform& at = ecs.get_component<Transform>(a);
    Collider& ac = ecs.get_component<Collider>(a);
    
    vec3 pa = at.orientation * data.contact_point.a;
    vec3 pb;
    if(b != 0xFFFFFFFF) {
        Transform& bt = ecs.get_component<Transform>(b);
        Collider& bc = ecs.get_component<Collider>(b);
        pb = bt.orientation * data.contact_point.b + vec3(bt.position - at.position);
    } else pb = data.contact_point.b - at.position;

    return Contact_point{pa, pb};
}

int get_i(ivec3 v, ivec3 size) {
    return (v.z * size.y + v.y) * size.x + v.x;
}

std::vector<uint32_t> Collider::traverse_BVH(Transform& ta, Transform& tb, Bounding_box& bb) {
    std::vector<uint32_t> front_buffer = {0};
    std::vector<uint32_t> back_buffer;
    std::vector<uint32_t> shapes;

    while(true) {
        if(front_buffer.size() == 0) break;

        for(uint32_t i : front_buffer) {
            BVH_node& node = BVH[i];

            if(Physics_system::collision(ta, node.bounding_box, tb, bb)) {
                if(node.children.size() > 1) {
                    back_buffer.push_back(node.children[0]);
                    back_buffer.push_back(node.children[1]);
                } else shapes.push_back(node.children[0]);
            }
        }

        front_buffer = back_buffer;
        back_buffer.clear();
    }

    return shapes;
}

std::vector<uint64_t> Collider::traverse_BVH(Transform& ta, Transform& tb, Collider& cb) {
    std::vector<uint64_t> front_buffer = {0};
    std::vector<uint64_t> back_buffer;
    std::vector<uint64_t> shape_pairs;

    while(true) {
        if(front_buffer.size() == 0) break;

        for(uint64_t i : front_buffer) {
            uint32_t ai = i & 0xFFFFFFFF;
            uint32_t bi = i >> 32;

            BVH_node& node_a = BVH[ai];
            BVH_node& node_b = cb.BVH[bi];

            if(Physics_system::collision(ta, node_a.bounding_box, tb, node_b.bounding_box)) {
                if(node_a.children.size() == 1) {
                    if(node_b.children.size() == 1) {
                        shape_pairs.push_back(uint64_t(node_a.children[0]) | (uint64_t(node_b.children[0]) << 32));
                    } else {
                        back_buffer.push_back(uint64_t(ai) | (uint64_t(node_b.children[0]) << 32));
                        back_buffer.push_back(uint64_t(ai) | (uint64_t(node_b.children[1]) << 32));
                    }
                } else if(node_b.children.size() == 1) {
                    back_buffer.push_back(uint64_t(node_a.children[0]) | (uint64_t(bi) << 32));
                    back_buffer.push_back(uint64_t(node_a.children[1]) | (uint64_t(bi) << 32));
                } else {
                    back_buffer.push_back(uint64_t(node_a.children[0]) | (uint64_t(node_b.children[0]) << 32));
                    back_buffer.push_back(uint64_t(node_a.children[1]) | (uint64_t(node_b.children[0]) << 32));
                    back_buffer.push_back(uint64_t(node_a.children[0]) | (uint64_t(node_b.children[1]) << 32));
                    back_buffer.push_back(uint64_t(node_a.children[1]) | (uint64_t(node_b.children[1]) << 32));
                }
            }
        }

        front_buffer = back_buffer;
        back_buffer.clear();
    }

    return shape_pairs;
}

quat axis_angle(vec3 aa) {
    float len = length(aa);

    float s = sin(len / 2.0f);
    float c = cos(len / 2.0f);

    if(len < 0.0001) {
        s = len;
        c = 1.0f - len * len / 2.0f;
    }

    if(len > 0.0f) aa = normalize(aa);
    quat q = quat(c, aa.x * s, aa.y * s, aa.z * s);

    return q;
}

vec3 axis_angle(quat q) {
    q = normalize(q);
    if(q.w < 0.0) q = -q;
    float sin_half_sq = q.x * q.x + q.y * q.y + q.z * q.z;
    if(sin_half_sq < 1e-8) return vec3(2.0f * q.x, 2.0f * q.y, 2.0f * q.z);

    float angle = acos(clamp(q.w, -1.0f, 1.0f)) * 2.0f;
    float s = sqrt(sin_half_sq);

    vec3 aaa = vec3(q.x, q.y, q.z) / s;
    return aaa * angle;
}

void Physics_system::integrate() {
    for(uint32_t c : collectors[0].entities) {
        Transform& c_transform = ecs.get_component<Transform>(c);
        Collider& c_collider = ecs.get_component<Collider>(c);

        if(!c_collider.is_static) {
            c_transform.position += c_collider.velocity * sub_dt;

            if(c_collider.allow_rotation && glm::length(c_collider.angular_momentum)) {
                glm::vec3 angular_velocity = (c_transform.orientation * c_collider.inverse_inertia_tensor * transpose(c_transform.orientation)) * c_collider.angular_momentum;
                
                float len_av = length(angular_velocity);
                vec3 norm_av = angular_velocity / len_av;
                if(len_av * sub_dt != 0.0f) {
                    glm::mat3 rotation = glm::rotate(len_av * sub_dt, norm_av);
        
                    c_transform.orientation = rotation * c_transform.orientation;
                }
            }
            
            if(c_collider.allow_gravity) {
                vec3 gravity_acceleration = -get_gravity(c_transform.position) * gravity;

                c_collider.velocity += gravity_acceleration * sub_dt;
            }
        }
    }
}

float max_velocity = 5.0f;

void Physics_system::compute_velocities() {
    for(uint32_t c : collectors[0].entities) {
        Transform& c_transform = ecs.get_component<Transform>(c);
        Collider& c_collider = ecs.get_component<Collider>(c);

        if(!c_collider.is_static) {
            c_collider.velocity += c_collider.pos_delta;
            c_collider.angular_momentum += c_collider.am_delta;
        }

        c_collider.pos_delta = vec3(0.0f);
        c_collider.am_delta = vec3(0.0f);
    }
}

std::vector<Return_point> Physics_system::collision(Transform& ta, Collider& ca, Transform& tb, Collider& cb) {
    std::vector<Return_point> ret;

    Return_tag tag;

    if(ca.BVH.size()) {
        if(cb.BVH.size()) {
            std::vector<uint64_t> pairs = ca.traverse_BVH(ta, tb, cb);

            for(uint64_t pair : pairs) {
                uint32_t a = pair & 0xFFFFFFFF;
                uint32_t b = pair >> 32;

                Convex_collider& sa = ca.collision_shapes[a];
                Convex_collider& sb = cb.collision_shapes[b];

                std::vector<Return_point> r = collision(ta, sa, tb, sb, tag);

                ret.insert(ret.end(), r.begin(), r.end());
            }
        } else {
            std::vector<Return_tag> tags;
            std::vector<std::vector<Return_point>> points;
            
            for(Convex_collider& sb : cb.collision_shapes) {
                std::vector<uint32_t> shapes = ca.traverse_BVH(ta, tb, sb.bounding_box);

                for(uint32_t shape : shapes) {
                    Convex_collider& sa = ca.collision_shapes[shape];

                    std::vector<Return_point> r = collision(ta, sa, tb, sb, tag);

                    tags.push_back(tag);
                    points.push_back(r);
                }
            }

            // faces first

            std::unordered_set<ivec3, Hash_coord> set;

            for(uint32_t i = 0; i < tags.size(); ++i) {
                Return_tag& tag = tags[i];
                if(tag.type == COLLISION_TYPE_FACE) {
                    ret.insert(ret.end(), points[i].begin(), points[i].end());

                    set.insert(tag.vid_a[0]);
                    set.insert(tag.vid_a[1]);
                    set.insert(tag.vid_a[2]);
                }
            }

            // then edges
            
            for(uint32_t i = 0; i < tags.size(); ++i) {
                Return_tag& tag = tags[i];
                if(tag.type == COLLISION_TYPE_EDGE) {
                    if(!(set.contains(tag.vid_a[0]) && set.contains(tag.vid_a[1]))) {
                        ret.insert(ret.end(), points[i].begin(), points[i].end());
                    
                        set.insert(tag.vid_a[0]);
                        set.insert(tag.vid_a[1]);
                    }
                }
            }

            // and finally vertices
            
            for(uint32_t i = 0; i < tags.size(); ++i) {
                Return_tag& tag = tags[i];
                if(tag.type == COLLISION_TYPE_VERTEX) {
                    if(!set.contains(tag.vid_a[0])) {
                        ret.insert(ret.end(), points[i].begin(), points[i].end());
                    
                        set.insert(tag.vid_a[0]);
                    }
                }
            }
        }
    } else if(cb.BVH.size()) {
        std::vector<Return_tag> tags;
        std::vector<std::vector<Return_point>> points;
        
        for(Convex_collider& sa : ca.collision_shapes) {
            std::vector<uint32_t> shapes = cb.traverse_BVH(tb, ta, sa.bounding_box);

            for(uint32_t shape : shapes) {
                Convex_collider& sb = cb.collision_shapes[shape];

                std::vector<Return_point> r = collision(ta, sa, tb, sb, tag);

                tags.push_back(tag);
                points.push_back(r);
            }
        }

        // faces first

        std::unordered_set<ivec3, Hash_coord> set;

        for(uint32_t i = 0; i < tags.size(); ++i) {
            Return_tag& tag = tags[i];
            if(tag.type == COLLISION_TYPE_VERTEX) {
                ret.insert(ret.end(), points[i].begin(), points[i].end());

                set.insert(tag.vid_b[0]);
                set.insert(tag.vid_b[1]);
                set.insert(tag.vid_b[2]);
            }
        }

        // then edges
        
        for(uint32_t i = 0; i < tags.size(); ++i) {
            Return_tag& tag = tags[i];
            if(tag.type == COLLISION_TYPE_EDGE) {
                if(!(set.contains(tag.vid_b[0]) && set.contains(tag.vid_b[1]))) {
                    ret.insert(ret.end(), points[i].begin(), points[i].end());
                
                    set.insert(tag.vid_b[0]);
                    set.insert(tag.vid_b[1]);
                }
            }
        }

        // and finally vertices
        
        for(uint32_t i = 0; i < tags.size(); ++i) {
            Return_tag& tag = tags[i];
            if(tag.type == COLLISION_TYPE_FACE) {
                if(!set.contains(tag.vid_b[0])) {
                    ret.insert(ret.end(), points[i].begin(), points[i].end());
                    
                    set.insert(tag.vid_b[0]);
                }
            }
        }
    } else {
        for(Convex_collider& sa : ca.collision_shapes) {
            for(Convex_collider& sb : cb.collision_shapes) {

                std::vector<Return_point> r = collision(ta, sa, tb, sb, tag);
                
                ret.insert(ret.end(), r.begin(), r.end());
            }
        }
    }

    return ret;
}

std::vector<uint32_t> Physics_system::GJK_BVH(Collider& ca, Transform& ta, Collider& cb, Convex_collider& ccb, Transform& tb) {
    std::vector<uint32_t> ret;

    std::vector<uint32_t> colliders = ca.traverse_BVH(ta, tb, ccb.bounding_box);

    for(uint32_t c : colliders) {
        Convex_collider& cca = ca.collision_shapes[c];

        Return_tag tag;
        bool b = GJK(cca, ta, ccb, tb);

        if(b) ret.push_back(c);
    }

    return ret;
}

Manifold Physics_system::create_manifold(std::vector<Return_point> contacts, uint32_t a, uint32_t b, Collider& ca, Transform& ta, Collider& cb, Transform& tb) {
    Manifold manifold;
    
    if(ca.mass <= 0.0f || ca.is_static) {
        manifold.a = b;
        manifold.b = NULL_ENTITY;
    } else if(cb.mass <= 0.0f || cb.is_static) {
        manifold.a = a;
        manifold.b = NULL_ENTITY;
    } else {
        manifold.a = a;
        manifold.b = b;
    }

    for(Return_point& rp : contacts) {
        Collision_data d;

        if(ca.mass <= 0.0f || ca.is_static) {
            rp.normal = -rp.normal;
            pvec3 p = rp.a;
            rp.a = transpose(tb.orientation) * vec3(rp.b);
            rp.b = p + ta.position;
        } else if(cb.mass <= 0.0f || cb.is_static) {
            rp.a = transpose(ta.orientation) * vec3(rp.a);
            rp.b = rp.b + tb.position;
        } else {
            rp.a = transpose(ta.orientation) * vec3(rp.a);
            rp.b = transpose(tb.orientation) * vec3(rp.b);
        }

        d.contact_point.a = rp.a;
        d.contact_point.b = rp.b;
        d.normal = normalize(rp.normal);
        manifold.normal = d.normal;

        manifold.points.push_back(d);
    }
    
    return manifold;
}

void Physics_system::physics_loop() {
    profiler.start();

    num_bb_checks = 0;
    num_gjk_checks = 0;
    
    //integrate();
    
    Particle_system& ps = ecs.get_system<Particle_system>();
    ps.ps_indices.clear();
    ps.ps_vertices.clear();
    
    for(uint32_t c : collectors[0].entities) {
        Transform& c_transform = ecs.get_component<Transform>(c);
        Collider& c_collider = ecs.get_component<Collider>(c);

        c_collider.colliding_normal.clear();
        c_collider.colliding_with.clear();
    }

    std::vector<uint64_t> possible_collisions = broad_phase();

    profiler.step("BROAD PHASE");

    //

    const uint32_t num_threads = 12;
    std::vector<std::thread> threads(num_threads);
    std::vector<std::vector<Manifold>> cdata(num_threads);
    std::vector<std::vector<uint64_t>> threads_collisions(num_threads);

    uint32_t num_collisions = 0;
    float num_per_thread = float(possible_collisions.size()) / num_threads;
    for(uint64_t i : possible_collisions) {
        uint32_t fi = num_collisions % num_threads;
        threads_collisions[fi].push_back(i);
        ++num_collisions;
    }

    auto thread_GJK = [&](uint32_t j) {
        for(uint64_t i : threads_collisions[j]) {
            uint32_t a = i & 0xFFFFFFFF;
            uint32_t b = i >> 32;
            
            Collider& ai = ecs.get_component<Collider>(a);
            Collider& bi = ecs.get_component<Collider>(b);

            uint32_t ii = 0;
            uint32_t jj = 0;

            if(!(ai.is_static && bi.is_static) && !(ai.collision_mask.contains(b) || bi.collision_mask.contains(a))) {
                Collider& ac = ecs.get_component<Collider>(a);
                Transform& at = ecs.get_component<Transform>(a);

                Collider& bc = ecs.get_component<Collider>(b);
                Transform& bt = ecs.get_component<Transform>(b);

                std::vector<Return_point> data = collision(at, ac, bt, bc);

                if(data.size()) {
                    Manifold m = create_manifold(data, a, b, ac, at, bc, bt);
                    cdata[j].push_back(m);
                }
            }
        }
    };

    for(int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread(thread_GJK, i);
    }
    
    for(int i = 0; i < num_threads; ++i) {
        threads[i].join();
    }

    profiler.step("NARROW PHASE");
    
    for(auto& c : cdata) {
        for(auto& manifold : c) {
            insert_collision(manifold);
        }
    }

    prune_manifolds();

    profiler.step("MANIFOLDS");

    //

    bool isnan_before = false;
    for(uint32_t entity : collectors[0].entities) {
        Transform& tf = ecs.get_component<Transform>(entity);

        if(isnan(tf.position.x.fraction)) isnan_before = true;
    }
    

    uint32_t new_size = 0;
    for(auto& [k, d] : collision_table) {
        new_size += d.size();
    }

    collision_constraints.clear();
    collision_constraints.resize(new_size);
    
    uint32_t i = 0;
    for(auto& [k, d] : collision_table) {
        for(Manifold& manifold : d) {
            collision_constraints[i] = Collision_constraint();

            Collision_constraint& cc = collision_constraints[i];
            cc.a = manifold.a;
            cc.b = manifold.b;

            cc.ca = &ecs.get_component<Collider>(cc.a);
            cc.ta = &ecs.get_component<Transform>(cc.a);
            
            if(cc.ca->collect) cc.ca->colliding_with.push_back(cc.b);

            if(cc.b != 0xFFFFFFFF) {
                cc.cb = &ecs.get_component<Collider>(cc.b);
                cc.tb = &ecs.get_component<Transform>(cc.b);
                
                if(cc.cb->collect) cc.cb->colliding_with.push_back(cc.a);
            }

            cc.constraints.clear();
            cc.constraints.resize(manifold.points.size());

            for(int j = 0; j < manifold.points.size(); ++j) {  
                Collision_data& c = manifold.points[j];

                col_constraint col_c;
                
                col_c.normal = c.normal;
                col_c.data = &c;

                if(cc.ca->collect) cc.ca->colliding_normal.push_back(c.normal);

                if(manifold.b != 0xFFFFFFFF) {
                    if(cc.cb->collect) cc.cb->colliding_normal.push_back(-c.normal);
                }

                cc.constraints[j] = col_c;
            }
            ++i;

            cc.mf = &manifold;
        }
    }
    
    profiler.step("CREATING CONSTRAINTS");

    for(int i = 0; i < substeps; ++i) {
        //position_solve(collision_constraints);
        //compute_velocities();
        integrate();

        velocity_solve(collision_constraints);   
    }
    
    profiler.step("SOLVER");
    profiler.loop();
}

void Physics_system::call() {
    Input_system& input_system = ecs.get_system<Input_system>();

    if(sim_active) {
        physics_time += core.delta_time;

        uint32_t count = 0;
        while(physics_time >= physics_step) {
            physics_time -= physics_step;
            physics_loop();
            count += 1;

            if(count >= max_frames) {
                physics_time = 0;
                break;
            }
        }
    }
}

void Physics_system::prune_manifolds() {
    std::vector<std::array<uint32_t, 2>> delete_keys;

    for(auto& [key, data] : collision_table) {
        std::vector<Manifold> new_manifolds = {data[0]};
        
        for(int i = 1; i < data.size(); ++i) {
            bool insert = true;
            for(Manifold& m : new_manifolds) {
                if(dot(m.normal, data[i].normal) > 0.9f) {
                    merge_manifolds(m, data[i]);
                    insert = false;
                    break;
                }
            }
            
            if(insert) new_manifolds.push_back(data[i]);
        }

        std::vector<uint32_t> delete_manifolds;

        uint32_t d = 0;
        uint32_t vv = 0;
        for(Manifold& manifold : new_manifolds) {
            uint32_t max_id;
            float max_pen = FLT_MAX;
            
            for(int i = 0; i < manifold.points.size(); ++i) {
                auto& v = manifold.points[i]; 
        
                v.normal = manifold.normal;
                ++vv;

                Contact_point p = get_points(v, manifold.a, manifold.b);

                vec3 diff = p.a - p.b;
                
                float dot_normal = dot(v.normal, diff);
                float tangent = length(diff - v.normal * dot_normal);

                if((dot_normal > contact_sep) || tangent > contact_sep) {
                    manifold.points.erase(manifold.points.begin() + i);
                    --i;
                } else if(dot_normal < max_pen) {
                    max_id = i;
                    max_pen = dot_normal;
                }
                
                if(manifold.points.size() == 0) {
                    delete_manifolds.push_back(d);
                }
            }
                
            std::vector<Collision_data>& cdata = manifold.points;

            if(cdata.size() > 4) {
                uint32_t priority_points = 0;
                for(int i = 0; i < 4; ++i) {
                    if(!cdata[i].priority) break;
                    ++priority_points;
                }

                std::vector<uint32_t> ids(4);

                ids[0] = max_id;

                float m = -FLT_MAX;
                uint32_t mi;

                uint32_t i = 0;
                for(auto& v : cdata) {
                    if(i != ids[0]) {
                        float dist = length(vec3(v.contact_point.a - cdata[ids[0]].contact_point.a));

                        if(dist > m) {
                            m = dist;
                            mi = i;
                        }
                    }
                    ++i;
                }
                ids[1] = mi; 
                
                m = -FLT_MAX;
                vec3 n = normalize(vec3(cdata[ids[1]].contact_point.a - cdata[ids[0]].contact_point.a));

                i = 0;
                for(auto& v : cdata) {
                    if(i != ids[0] && i != ids[1]) {
                        vec3 diff = vec3(v.contact_point.a - cdata[ids[0]].contact_point.a);
                        float dist = length(diff - n * dot(n, diff));

                        if(dist > m) {
                            m = dist;
                            mi = i;
                        }
                    }
                    ++i;
                }
                ids[2] = mi; 

                vec3 center = vec3(cdata[ids[1]].contact_point.a - cdata[ids[0]].contact_point.a) + vec3(cdata[ids[2]].contact_point.a - cdata[ids[0]].contact_point.a);
                center /= 3;

                m = -FLT_MAX;

                i = 0;
                for(auto& v : cdata) {
                    if(i != ids[0] && i != ids[1] && i != ids[2]) {
                        vec3 diff = vec3(v.contact_point.a - cdata[ids[0]].contact_point.a);
                        float dist = length(diff - center);

                        if(dist > m) {
                            m = dist;
                            mi = i;
                        }
                    }
                    ++i;
                }
                ids[3] = mi; 
                
                std::vector<Collision_data> cd(4);

                for(int i = 0; i < 4; ++i) {
                    cd[i] = cdata[ids[i]];
                }

                cdata = cd;
            }

            ++d;
        }
        
        data = new_manifolds;
        
        int num_deleted = 0;
        for(int dd : delete_manifolds) {
            data.erase(data.begin() + dd - num_deleted);
            ++num_deleted;
        }

        if(data.size() == 0) {
            delete_keys.push_back(key);
        }
    }

    for(auto key : delete_keys) {
        collision_table.erase(key);
    }
}

bool Physics_system::contains(std::vector<vec3> points, vec3 radius, vec3 point) {
    Simplex simplex;

    float limit = 0.0001;

    glm::vec3 direction = glm::normalize(point - points[0]);
    
    glm::vec3 offset = glm::vec3(direction.y, -direction.x, direction.z);

    if(glm::dot(offset, direction) > 0.99) {
        offset = glm::vec3(direction.x, direction.z, -direction.y);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    while(loop) {
        ++iterations;
        if(iterations > 100) return false;
        
        int size = simplex.vertices.size();
        if(size < 4) {
            glm::vec3 point_a = support_func(points, radius, direction);
            glm::vec3 point_b = point;

            glm::vec3 point_m = point_a - point_b;

            for(Simplex_vertex& v : simplex.vertices) {
                glm::vec3 difference = point_m - v.m;

                if(glm::length(difference) < limit) {
                    return false;
                }
            }

            if(glm::dot(point_m, direction) < limit * 2) {
                return false;
            }

            simplex.vertices.push_back(Simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);
            } else if(size == 1) {
                glm::vec3 line_direction = glm::normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                glm::vec3 rel_origin_pos = -simplex.vertices[1].m;

                glm::vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            } else if(size == 2) {
                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0) normal = -normal;

                direction = normal;
            }
        } else {
            int n = simplex_contains(glm::vec3(0, 0, 0), simplex.vertices);
            if(n == -1) {
                return true;
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) < 0.0) normal = -normal;

                direction = normal;
            }
        }
    }
}

float Physics_system::contains_dist(std::vector<vec3> points, vec3 radius, vec3 point) {
    Simplex simplex;

    float limit = 0.00001;

    glm::vec3 direction = glm::normalize(point - points[0]);
    
    glm::vec3 offset = glm::vec3(direction.y, -direction.x, direction.z);

    if(glm::dot(offset, direction) > 0.99) {
        offset = glm::vec3(direction.x, direction.z, -direction.y);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    while(loop) {
        ++iterations;
        if(iterations > 10000) {
            return 0.0f;
        }
        
        int size = simplex.vertices.size();
        if(size < 4) {
            glm::vec3 point_a = support_func(points, radius, direction);
            glm::vec3 point_b = point;

            glm::vec3 point_m = point_a - point_b;

            for(Simplex_vertex& v : simplex.vertices) {
                glm::vec3 difference = point_m - v.m;

                if(dot(difference, direction) < limit) {
                    if(size == 1) {
                        return -length(simplex.vertices[0].m);
                    } else if(size == 2) {
                        glm::vec3 line_direction = simplex.vertices[0].m - simplex.vertices[1].m;
                        float len = length(line_direction);
                        line_direction /= len;

                        glm::vec3 rel_origin_pos = -simplex.vertices[1].m;

                        glm::vec3 closest_point = line_direction * clamp(glm::dot(rel_origin_pos, line_direction), 0.0f, len) + simplex.vertices[1].m;

                        return -length(closest_point);
                    } else if(size == 3) {
                        glm::vec3 barycentric = project_clamp(simplex.vertices[0].m, simplex.vertices[1].m, simplex.vertices[2].m, vec3(0.0f));

                        vec3 m = simplex.vertices[0].m * barycentric.x + simplex.vertices[1].m * barycentric.y + simplex.vertices[2].m * barycentric.z;

                        return -length(m);
                    } else {
                        return 0.0f;
                    }
                }
            }

            simplex.vertices.push_back(Simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);
            } else if(size == 1) {
                glm::vec3 line_direction = glm::normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                glm::vec3 rel_origin_pos = -simplex.vertices[1].m;

                glm::vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            } else if(size == 2) {
                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) < 0.0) normal = -normal;

                direction = normal;
            }
        } else {
            int n = simplex_contains(glm::vec3(0, 0, 0), simplex.vertices);
            if(n == -1) {
                bool loop_epa = true;

                glm::vec3 weights;

                glm::vec3 contact_point_a;
                glm::vec3 contact_point_b;
                glm::vec3 separation_vector;
                glm::vec3 collision_normal;

                Polytope p;
                p.from_simplex(simplex);

                iterations = 0;
                uint32_t pass = 0;

                while(true) {
                    ++iterations;
                    
                    Polytope_return r = p.find_closest_face();

                    if(r.vertices.size() == 0) {
                        std::cout << "ZERO\n";

                        return 0.0f;
                    }

                    direction = r.normal;
                    
                    glm::vec3 point_a = support_func(points, radius, direction);
                    glm::vec3 point_b = point;

                    glm::vec3 point_m = point_a - point_b;

                    float dist = dot(point_m, r.normal);
                    
                    if(iterations > 10000)  {
                        std::cout << "EPA ";
                        return 0.0f;
                    }
                    // end epa

                    float limit_2 = 0.001f;

                    if(abs(dist - dot(r.vertices[0].m, r.normal)) < limit_2) {
                        contact_point_a = r.vertices[0].a * r.weights.x + r.vertices[1].a * r.weights.y + r.vertices[2].a * r.weights.z;
                        contact_point_b = r.vertices[0].b * r.weights.x + r.vertices[1].b * r.weights.y + r.vertices[2].b * r.weights.z;

                        return length(contact_point_a - contact_point_b);
                    } else {
                        p.expand({point_m, point_a, point_b});
                    }
                }
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) < 0.0) normal = -normal;

                direction = normal;
            }
        }
    }
}

std::pair<mat3, vec3> Physics_system::calculate_inertia_tensor(std::vector<vec3> points, vec3 radius, float mass) {
    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : points) {
        glm::vec3 position = vertex;

        vec3 min_pos = position - radius;
        vec3 max_pos = position + radius;

        minimum.x = min(minimum.x, min_pos.x);
        minimum.y = min(minimum.y, min_pos.y);
        minimum.z = min(minimum.z, min_pos.z);

        maximum.x = max(maximum.x, max_pos.x);
        maximum.y = max(maximum.y, max_pos.y);
        maximum.z = max(maximum.z, max_pos.z);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(8);

    vec3 size = area / vec3(sample_points);

    mat3 inertia_tensor;
    inertia_tensor[0] = vec3(0);
    inertia_tensor[1] = vec3(0);
    inertia_tensor[2] = vec3(0);

    int num = 0;

    for(int z = 0; z < sample_points.z; ++z) {
        for(int y = 0; y < sample_points.y; ++y) {
            for(int x = 0; x < sample_points.x; ++x) {
                vec3 p = {x, y, z};
                p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

                bool contains = Physics_system::contains(points, radius, p);

                if(contains) {
                    mat3 cuboid_it = {
                        vec3((1.0f / 12) * (size.y * size.y + size.z * size.z), 0, 0),
                        vec3(0, (1.0f / 12) * (size.x * size.x + size.z * size.z), 0),
                        vec3(0, 0, (1.0f / 12) * (size.x * size.x + size.y * size.y)),
                    };

                    cuboid_it = translate_inertia_tensor(p, cuboid_it, 1.0f);

                    inertia_tensor += cuboid_it;

                    center_pos += p;

                    center_mass += 1;

                    ++num;
                }
            }
        }
    }

    center_pos /= center_mass;

    //std::cout << center_pos << "\n";

    if(num != 0) {
        float multiplier = mass / float(num);

        inertia_tensor[0] *= multiplier;
        inertia_tensor[1] *= multiplier;
        inertia_tensor[2] *= multiplier;
    }

    return {inertia_tensor, center_pos};
}

std::pair<mat3, vec3> Physics_system::calculate_inertia_tensor_volume(std::vector<vec3> points, vec3 radius, float& volume) {
    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : points) {
        glm::vec3 position = vertex;

        vec3 min_pos = position - radius;
        vec3 max_pos = position + radius;

        minimum.x = min(minimum.x, min_pos.x);
        minimum.y = min(minimum.y, min_pos.y);
        minimum.z = min(minimum.z, min_pos.z);

        maximum.x = max(maximum.x, max_pos.x);
        maximum.y = max(maximum.y, max_pos.y);
        maximum.z = max(maximum.z, max_pos.z);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(8);

    vec3 size = area / vec3(sample_points);

    mat3 inertia_tensor;
    inertia_tensor[0] = vec3(0);
    inertia_tensor[1] = vec3(0);
    inertia_tensor[2] = vec3(0);

    int num = 0;

    for(int z = 0; z < sample_points.z; ++z) {
        for(int y = 0; y < sample_points.y; ++y) {
            for(int x = 0; x < sample_points.x; ++x) {
                vec3 p = {x, y, z};
                p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

                bool contains = Physics_system::contains(points, radius, p);

                if(contains) {
                    mat3 cuboid_it = {
                        vec3((1.0f / 12) * (size.y * size.y + size.z * size.z), 0, 0),
                        vec3(0, (1.0f / 12) * (size.x * size.x + size.z * size.z), 0),
                        vec3(0, 0, (1.0f / 12) * (size.x * size.x + size.y * size.y)),
                    };

                    cuboid_it = translate_inertia_tensor(p, cuboid_it, 1.0f);

                    inertia_tensor += cuboid_it;

                    center_pos += p;

                    center_mass += 1;

                    ++num;
                }
            }
        }
    }

    float cell_volume = size.x * size.y * size.z;
    volume = cell_volume * float(num);

    center_pos /= center_mass;

    //std::cout << center_pos << "\n";

    if(num != 0) {
        float multiplier = 1.0f / float(num);

        inertia_tensor[0] *= multiplier;
        inertia_tensor[1] *= multiplier;
        inertia_tensor[2] *= multiplier;
    }

    return {inertia_tensor, center_pos};
}

std::pair<mat3, vec3> Physics_system::calculate_inertia_tensor(std::vector<vec3> points, vec3 radius, float& mass, float density) {
    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : points) {
        glm::vec3 position = vertex;

        vec3 min_pos = position - radius;
        vec3 max_pos = position + radius;

        minimum.x = min(minimum.x, min_pos.x);
        minimum.y = min(minimum.y, min_pos.y);
        minimum.z = min(minimum.z, min_pos.z);

        maximum.x = max(maximum.x, max_pos.x);
        maximum.y = max(maximum.y, max_pos.y);
        maximum.z = max(maximum.z, max_pos.z);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(8);

    vec3 size = area / vec3(sample_points);

    float volume = size.x * size.y * size.z;

    mat3 inertia_tensor;
    inertia_tensor[0] = vec3(0);
    inertia_tensor[1] = vec3(0);
    inertia_tensor[2] = vec3(0);

    int num = 0;

    for(int z = 0; z < sample_points.z; ++z) {
        for(int y = 0; y < sample_points.y; ++y) {
            for(int x = 0; x < sample_points.x; ++x) {
                vec3 p = {x, y, z};
                p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

                bool contains = Physics_system::contains(points, radius, p);

                if(contains) {
                    mat3 cuboid_it = {
                        vec3((1.0f / 12) * (size.y * size.y + size.z * size.z), 0, 0),
                        vec3(0, (1.0f / 12) * (size.x * size.x + size.z * size.z), 0),
                        vec3(0, 0, (1.0f / 12) * (size.x * size.x + size.y * size.y)),
                    };

                    cuboid_it = translate_inertia_tensor(p, cuboid_it, 1.0f);

                    inertia_tensor += cuboid_it;

                    center_pos += p;

                    center_mass += 1;

                    ++num;
                }
            }
        }
    }

    center_pos /= center_mass;

    //std::cout << center_pos << "\n";

    mass = num * volume * density;

    if(num != 0) {
        float multiplier = mass / float(num);

        inertia_tensor[0] *= multiplier;
        inertia_tensor[1] *= multiplier;
        inertia_tensor[2] *= multiplier;
    }

    return {inertia_tensor, center_pos};
}

std::pair<mat3, vec3> Physics_system::calculate_inertia_tensor_flat(std::vector<vec3> points, float mass) {
    vec3 normal = normalize(cross(points[0] - points[2], points[1] - points[2]));

    mat3 rot_mat = rotate_to(normal, vec3(0, 0, 1));

    std::vector<vec3> pp;
    float z = 0;

    for(vec3 v : points) {
        vec3 v1 = rot_mat * v;

        pp.push_back(vec3(v1.x, v1.y, -1.0f));
        pp.push_back(vec3(v1.x, v1.y, 1.0f));
        z = v1.z;
    }

    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : pp) {
        glm::vec3 position = vertex;

        minimum = min(minimum, position);
        maximum = max(maximum, position);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(25);

    mat3 inertia_tensor;
    inertia_tensor[0] = vec3(0);
    inertia_tensor[1] = vec3(0);
    inertia_tensor[2] = vec3(0);

    int num = 0;

    for(int y = 0; y < sample_points.y; ++y) {
        for(int x = 0; x < sample_points.x; ++x) {
            vec3 p = {x, y, 0};
            p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

            bool contains = Physics_system::contains(pp, vec3(0), p);

            if(contains) {
                p = vec3(p.x, p.y, z);
                p = transpose(rot_mat) * p;
                ++num;
                for(int i = 0; i <= 2; ++i) {
                    for(int j = 0; j <= 2; ++j) {
                        if(i == j) inertia_tensor[i][j] += p[(i + 1) % 3] * p[(i + 1) % 3] + p[(i + 2) % 3] * p[(i + 2) % 3];
                        else inertia_tensor[i][j] += -(p[i] * p[j]);
                    }
                }

                center_pos += p;
                center_mass += 1;
            }
        }
    }

    center_pos /= center_mass;

    if(num != 0) {
        float multiplier = mass / float(num);

        inertia_tensor[0] *= multiplier;
        inertia_tensor[1] *= multiplier;
        inertia_tensor[2] *= multiplier;
    }

    return {inertia_tensor, center_pos};
}

std::pair<mat3, vec3> Physics_system::calculate_inertia_tensor_flat(std::vector<vec3> points, float thickness, float& mass, float density) {
    vec3 normal = normalize(cross(points[0] - points[2], points[1] - points[2]));

    mat3 rot_mat = rotate_to(normal, vec3(0, 0, 1));

    std::vector<vec3> pp;
    float z = 0;

    for(vec3 v : points) {
        vec3 v1 = rot_mat * v;

        pp.push_back(vec3(v1.x, v1.y, thickness * -0.5f));
        pp.push_back(vec3(v1.x, v1.y, thickness * 0.5f));
        z = v1.z;
    }

    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : pp) {
        glm::vec3 position = vertex;

        minimum = min(minimum, position);
        maximum = max(maximum, position);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(25);

    mat3 inertia_tensor;
    inertia_tensor[0] = vec3(0);
    inertia_tensor[1] = vec3(0);
    inertia_tensor[2] = vec3(0);
    
    vec3 size = area / vec3(sample_points);
    float volume = size.x * size.y * size.z;

    int num = 0;

    for(int y = 0; y < sample_points.y; ++y) {
        for(int x = 0; x < sample_points.x; ++x) {
            vec3 p = {x, y, 0};
            p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

            bool contains = Physics_system::contains(pp, vec3(0), p);

            if(contains) {
                mat3 cuboid_it = {
                    vec3((1.0f / 12) * (size.y * size.y + size.z * size.z), 0, 0),
                    vec3(0, (1.0f / 12) * (size.x * size.x + size.z * size.z), 0),
                    vec3(0, 0, (1.0f / 12) * (size.x * size.x + size.y * size.y)),
                };

                cuboid_it = translate_inertia_tensor(p, cuboid_it, 1.0f);

                inertia_tensor += cuboid_it;

                center_pos += p;

                center_mass += 1;

                ++num;
            }
        }
    }

    
    center_pos /= center_mass;

    mass = num * volume * density;

    if(num != 0) {
        float multiplier = mass / float(num);

        inertia_tensor[0] *= multiplier;
        inertia_tensor[1] *= multiplier;
        inertia_tensor[2] *= multiplier;
    }

    return {inertia_tensor, center_pos};
}


std::pair<mat3, vec3> Physics_system::calculate_inertia_tensor_flat_volume(std::vector<vec3> points, float thickness, float& volume) {
    vec3 normal = normalize(cross(points[0] - points[2], points[1] - points[2]));

    mat3 rot_mat = rotate_to(normal, vec3(0, 0, 1));

    std::vector<vec3> pp;
    float z = 0;

    for(vec3 v : points) {
        vec3 v1 = rot_mat * v;

        pp.push_back(vec3(v1.x, v1.y, thickness * -0.5f));
        pp.push_back(vec3(v1.x, v1.y, thickness * 0.5f));
        z = v1.z;
    }

    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : pp) {
        glm::vec3 position = vertex;

        minimum = min(minimum, position);
        maximum = max(maximum, position);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(25);

    mat3 inertia_tensor;
    inertia_tensor[0] = vec3(0);
    inertia_tensor[1] = vec3(0);
    inertia_tensor[2] = vec3(0);
    
    vec3 size = area / vec3(sample_points);
    float cell_volume = size.x * size.y * size.z;

    int num = 0;

    for(int y = 0; y < sample_points.y; ++y) {
        for(int x = 0; x < sample_points.x; ++x) {
            vec3 p = {x, y, 0};
            p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

            bool contains = Physics_system::contains(pp, vec3(0), p);

            if(contains) {
                mat3 cuboid_it = {
                    vec3((1.0f / 12) * (size.y * size.y + size.z * size.z), 0, 0),
                    vec3(0, (1.0f / 12) * (size.x * size.x + size.z * size.z), 0),
                    vec3(0, 0, (1.0f / 12) * (size.x * size.x + size.y * size.y)),
                };

                cuboid_it = translate_inertia_tensor(p, cuboid_it, 1.0f);

                inertia_tensor += cuboid_it;

                center_pos += p;

                center_mass += 1;

                ++num;
            }
        }
    }
    
    volume = cell_volume * float(num);

    center_pos /= center_mass;

    //std::cout << center_pos << "\n";

    if(num != 0) {
        float multiplier = 1.0f / float(num);

        inertia_tensor[0] *= multiplier;
        inertia_tensor[1] *= multiplier;
        inertia_tensor[2] *= multiplier;
    }

    return {inertia_tensor, center_pos};
}

mat3 Physics_system::translate_inertia_tensor(vec3 delta, mat3 inertia_tensor, float mass) {
    mat3 tensor = inertia_tensor;

    float squared_delta = dot(delta, delta);

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            tensor[i][j] = inertia_tensor[i][j] + mass * (((i == j) ? squared_delta : 0) - delta[i] * delta[j]);
        }
    }

    return tensor;
}

mat3 Physics_system::translate_inertia_tensor_inverse(vec3 delta, mat3 inertia_tensor, float mass) {
    mat3 tensor = inertia_tensor;

    float squared_delta = dot(delta, delta);

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            tensor[i][j] = inertia_tensor[i][j] - mass * (((i == j) ? squared_delta : 0) - delta[i] * delta[j]);
        }
    }

    return tensor;
}

mat3 Physics_system::add_inertia_tensor(mat3 a, mat3 b) {
    mat3 ret = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            ret[i][j] = a[i][j] + b[i][j];
        }
    }

    return ret;
}

mat3 translate_M(vec3 d, mat3 M, float mass) {
    mat3 t = {
        vec3(d.x * d.x, d.x * d.y, d.x * d.z),
        vec3(d.y * d.x, d.y * d.y, d.y * d.z),
        vec3(d.z * d.x, d.z * d.y, d.z * d.z)
    };

    return M + mass * t;
}

mat3 inv_translate_M(vec3 d, mat3 M, float mass) {
    mat3 t = {
        vec3(d.x * d.x, d.x * d.y, d.x * d.z),
        vec3(d.y * d.x, d.y * d.y, d.y * d.z),
        vec3(d.z * d.x, d.z * d.y, d.z * d.z)
    };

    return M - mass * t;
}


mat3 cuboid_M(vec3 size) {
    return mat3{
        size.x * size.x / 12.0f, 0.0f, 0.0f,
        0.0f, size.y * size.y / 12.0f, 0.0f,
        0.0f, 0.0f, size.z * size.z / 12.0f
    };
}

mat3 Physics_system::inertia_tensor(mat3 M) {
    float trace = M[0][0] + M[1][1] + M[2][2];

    return identity<mat3>() * trace - M;
}

std::pair<mat3, vec3> Physics_system::calculate_M(std::vector<vec3> points, vec3 radius, float& volume) {
    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : points) {
        glm::vec3 position = vertex;

        vec3 min_pos = position - radius;
        vec3 max_pos = position + radius;

        minimum.x = min(minimum.x, min_pos.x);
        minimum.y = min(minimum.y, min_pos.y);
        minimum.z = min(minimum.z, min_pos.z);

        maximum.x = max(maximum.x, max_pos.x);
        maximum.y = max(maximum.y, max_pos.y);
        maximum.z = max(maximum.z, max_pos.z);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(8);

    vec3 size = area / vec3(sample_points);

    float cell_volume = size.x * size.y * size.z;

    mat3 M;
    M[0] = vec3(0);
    M[1] = vec3(0);
    M[2] = vec3(0);

    int num = 0;

    mat3 M0 = cuboid_M(size);

    for(int z = 0; z < sample_points.z; ++z) {
        for(int y = 0; y < sample_points.y; ++y) {
            for(int x = 0; x < sample_points.x; ++x) {
                vec3 p = {x, y, z};
                p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

                bool contains = Physics_system::contains(points, radius, p);

                if(contains) {
                    mat3 M1 = translate_M(p, M0, 1.0f);

                    M += M1;

                    center_mass += 1.0f;
                    center_pos += p;

                    ++num;
                }
            }
        }
    }

    volume = cell_volume * num;

    center_pos /= center_mass;

    if(num != 0) {
        float multiplier = 1.0f / float(num);

        M[0] *= multiplier;
        M[1] *= multiplier;
        M[2] *= multiplier;
    }

    return {M, center_pos};
}

std::pair<mat3, vec3> Physics_system::calculate_M_flat(std::vector<vec3> points, float thickness, float& volume) {
    vec3 normal = normalize(cross(points[0] - points[2], points[1] - points[2]));

    mat3 rot_mat = rotate_to(normal, vec3(0, 0, 1));

    std::vector<vec3> pp;
    float z = 0;

    for(vec3 v : points) {
        vec3 v1 = rot_mat * v;

        pp.push_back(vec3(v1.x, v1.y, thickness * -0.5f));
        pp.push_back(vec3(v1.x, v1.y, thickness * 0.5f));
        z = v1.z;
    }

    vec3 center_pos = vec3(0, 0, 0);
    float center_mass = 0;

    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    for(glm::vec3 vertex : pp) {
        glm::vec3 position = vertex;

        minimum = min(minimum, position);
        maximum = max(maximum, position);
    }

    vec3 area = maximum - minimum;

    ivec3 sample_points = ivec3(25);

    mat3 M;
    M[0] = vec3(0);
    M[1] = vec3(0);
    M[2] = vec3(0);
    
    vec3 size = area / vec3(sample_points);
    float cell_volume = size.x * size.y * size.z;
    
    mat3 M0 = cuboid_M(size);

    int num = 0;

    for(int y = 0; y < sample_points.y; ++y) {
        for(int x = 0; x < sample_points.x; ++x) {
            vec3 p = {x, y, 0};
            p = minimum + area * ((p + 0.5f) / (vec3)sample_points);

            bool contains = Physics_system::contains(pp, vec3(0), p);

            if(contains) {
                mat3 M1 = translate_M(p, M0, 1.0f);

                M += M1;
                
                center_mass += 1.0f;
                center_pos += p;

                ++num;
            }
        }
    }
    
    volume = cell_volume * num;

    center_pos /= center_mass;

    if(num != 0) {
        float multiplier = 1.0f / float(num);

        M[0] *= multiplier;
        M[1] *= multiplier;
        M[2] *= multiplier;
    }

    return {M, center_pos};
}

std::vector<shape_face> Physics_system::triangulate_merge(std::vector<vec3> vertices) {
    vec3 center = vec3(0.0f);
    for(vec3 v : vertices) center += v;
    center /= vertices.size();

    auto v = triangulate(vertices);

    uint32_t triangle_size = 3;

    std::vector<shape_face> faces;

    for(int i = 0; i < v.size() / triangle_size; ++i) {
        shape_face t;
        t.vertices = std::vector<uint32_t>(v.begin() + (i * triangle_size), v.begin() + ((i + 1) * triangle_size));

        vec3 normal = cross(vertices[t.vertices[0]] - vertices[t.vertices[2]], vertices[t.vertices[1]] - vertices[t.vertices[2]]);
        normal = normalize(normal);

        if(dot(normal, center - vertices[t.vertices[0]]) > 0.0f) normal = -normal;
        
        t.normal = normal;

        std::sort(t.vertices.begin(), t.vertices.end());

        faces.push_back(t);
    }

    for(int i = 0; i < faces.size(); ++i) {
        shape_face& face_i = faces[i];
        for(int j = i + 1; j < faces.size(); ++j) {
            shape_face& face_j = faces[j];

            if(dot(face_i.normal, face_j.normal) > 0.99) {
                int i_size = face_i.vertices.size();

                face_i.vertices.insert(face_i.vertices.end(), face_j.vertices.begin(), face_j.vertices.end());
                std::inplace_merge(face_i.vertices.begin(), face_i.vertices.begin() + i_size, face_i.vertices.end());
                auto iter = std::unique(face_i.vertices.begin(), face_i.vertices.end());
                face_i.vertices.erase(iter, face_i.vertices.end());

                faces.erase(faces.begin() + j);
                --j;
            }
        }
    }
    
    for(int i = 0; i < faces.size(); ++i) {
        shape_face& face_i = faces[i];

        mat3 ori = rotate_to(face_i.normal, vec3(0.0f, 0.0f, 1.0f));
        std::vector<vec2> vs;
        for(int i = 0; i < face_i.vertices.size(); ++i) {
            vec3 v = vertices[face_i.vertices[i]];

            vs.push_back((ori * v).xy());
        }

        std::vector<uint32_t> hull = convex_hull(vs);

        std::vector<uint32_t> new_v;
        for(uint32_t i : hull) {
            new_v.push_back(face_i.vertices[i]);
        }
        face_i.vertices = new_v;
    }

    return faces;
}

void Physics_system::initialize_collision_shape(Collision_shape& shape) {
    vec3 radius = vec3(shape.radius);
    if(radius.x == 0.0f) radius = shape.split_radius;

    if(shape.mass != 0.0f) {
        auto s = Physics_system::calculate_inertia_tensor(shape.vertices, radius, shape.mass);
        shape.inertia_tensor = s.first;
        shape.center_of_mass = s.second;
    }

    std::vector<shape_face> faces = triangulate_merge(shape.vertices);

    shape.faces = faces;
}

vec3 Physics_system::initialize_collider(Collider& collider) {
    mat3 total_it = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    float mass = 0;

    vec3 center_pos = {0, 0, 0};
    
    for(Convex_collider& cc : collider.collision_shapes) {
        auto& shape = *cc.collision_shape.get();
        initialize_collision_shape(shape);

        mat3 inertia_tensor = shape.inertia_tensor;
        inertia_tensor = cc.orientation * inertia_tensor * transpose(cc.orientation);

        vec3 center_of_mass = cc.orientation * shape.center_of_mass;

        inertia_tensor = Physics_system::translate_inertia_tensor_inverse(center_of_mass, inertia_tensor, shape.mass);

        inertia_tensor = Physics_system::translate_inertia_tensor(vec3(cc.position) + center_of_mass, inertia_tensor, shape.mass);

        total_it = Physics_system::add_inertia_tensor(total_it, inertia_tensor);

        center_of_mass += cc.position;
        center_pos += center_of_mass * shape.mass;
        mass += shape.mass;
    }

    center_pos /= mass;

    total_it = Physics_system::translate_inertia_tensor_inverse(center_pos, total_it, mass);
    if(collider.allow_rotation) {
        for(Convex_collider& cc : collider.collision_shapes) {
            cc.position -= center_pos;
        }
        mat3 inertia_tensor = total_it;

        float max_inertia = max(inertia_tensor[0][0], max(inertia_tensor[1][1], inertia_tensor[2][2]));
        max_inertia = max_inertia * 0.25f;
        //inertia_tensor[0][0] = max(inertia_tensor[0][0], max_inertia);
        //inertia_tensor[1][1] = max(inertia_tensor[1][1], max_inertia);
        //inertia_tensor[2][2] = max(inertia_tensor[2][2], max_inertia);
        
        collider.inertia_tensor = inertia_tensor;
        
        collider.inverse_inertia_tensor = inverse(collider.inertia_tensor);
    }

    collider.mass = mass;
    
    return center_pos;
    //collider.allow_rotation = true;
}

void Physics_system::initialize_collision_shape(Collision_shape& shape, float density) {
    vec3 radius = vec3(shape.radius);
    if(radius.x == 0.0f) radius = shape.split_radius;

    auto s = Physics_system::calculate_inertia_tensor(shape.vertices, radius, shape.mass, density);
    shape.inertia_tensor = s.first;
    shape.center_of_mass = s.second;

    std::vector<shape_face> faces = triangulate_merge(shape.vertices);

    shape.faces = faces;
}

vec3 Physics_system::initialize_collider(Collider& collider, std::vector<float> density) {
    mat3 total_it = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    float mass = 0;

    vec3 center_pos = {0, 0, 0};
    
    uint32_t i = 0;
    for(Convex_collider& cc : collider.collision_shapes) {
        auto& shape = *cc.collision_shape.get();
        initialize_collision_shape(shape, density[i]);

        mat3 inertia_tensor = shape.inertia_tensor;
        inertia_tensor = cc.orientation * inertia_tensor * transpose(cc.orientation);

        vec3 center_of_mass = cc.orientation * shape.center_of_mass;

        inertia_tensor = Physics_system::translate_inertia_tensor_inverse(center_of_mass, inertia_tensor, shape.mass);

        inertia_tensor = Physics_system::translate_inertia_tensor(vec3(cc.position) + center_of_mass, inertia_tensor, shape.mass);

        total_it = Physics_system::add_inertia_tensor(total_it, inertia_tensor);

        center_of_mass += cc.position;
        center_pos += center_of_mass * shape.mass;
        mass += shape.mass;

        ++i;
    }

    center_pos /= mass;

    total_it = Physics_system::translate_inertia_tensor_inverse(center_pos, total_it, mass);
    if(collider.allow_rotation) {
        for(Convex_collider& cc : collider.collision_shapes) {
            cc.position -= center_pos;
        }
        mat3 inertia_tensor = total_it;

        float max_inertia = max(inertia_tensor[0][0], max(inertia_tensor[1][1], inertia_tensor[2][2]));
        max_inertia = max_inertia * 0.25f;
        inertia_tensor[0][0] = max(inertia_tensor[0][0], max_inertia);
        inertia_tensor[1][1] = max(inertia_tensor[1][1], max_inertia);
        inertia_tensor[2][2] = max(inertia_tensor[2][2], max_inertia);
        
        collider.inertia_tensor = inertia_tensor;
        
        collider.inverse_inertia_tensor = inverse(collider.inertia_tensor);
    }

    collider.mass = mass;
    
    return center_pos;
    //collider.allow_rotation = true;
}

std::vector<vec3> get_tangents(vec3 normal) {
    vec3 tangent = vec3(0.0f, -normal.z, normal.y);
    if(length(tangent) < 0.02f) tangent = vec3(-normal.y, normal.x, 0.0f);
    tangent = normalize(tangent);

    vec3 bitangent = normalize(cross(tangent, normal));

    return {tangent, bitangent};
}

void Collision_constraint::refresh(col_constraint& c) {
    vec3 rel_a;
    vec3 rel_b;

    if(b != NULL_ENTITY) {
        vec3 pa = ta->orientation * vec3(c.data->contact_point.a);
        vec3 pb = tb->orientation * vec3(c.data->contact_point.b); // contact point b is relative to b
        c.constraint_point.b = pb;
        c.constraint_point.a = pa;
        // both constraint points are relative to A's position in global space
        
        c.pos_a = c.constraint_point.a;
        c.pos_b = c.constraint_point.b;
        
        c.normal = c.data->normal;
        c.tangent = c.data->tangent;
        c.bitangent = c.data->bitangent;
        
        //c.normal = tb->orientation * c.data->normal;
        //c.tangent = tb->orientation * c.data->tangent;
        //c.bitangent = tb->orientation * c.data->bitangent;
        
        c.inertiaNa = calculate_inertia(ca, ta, c.normal, c.pos_a);
        c.inertiaTa = calculate_inertia(ca, ta, c.tangent, c.pos_a);
        c.inertiaBa = calculate_inertia(ca, ta, c.bitangent, c.pos_a);
        
        c.inertiaNb = calculate_inertia(cb, tb, -c.normal, c.pos_b);
        c.inertiaTb = calculate_inertia(cb, tb, -c.tangent, c.pos_b);
        c.inertiaBb = calculate_inertia(cb, tb, -c.bitangent, c.pos_b);

        rel_a = c.constraint_point.a;
        rel_b = c.constraint_point.b + (tb->position - ta->position);
    } else {
        vec3 pa = ta->orientation * vec3(c.data->contact_point.a);
        vec3 pb = c.data->contact_point.b - ta->position;
        c.constraint_point.a = pa;
        c.constraint_point.b = pb;
        
        c.pos_a = c.constraint_point.a;
        
        c.normal = c.data->normal;
        c.tangent = c.data->tangent;
        c.bitangent = c.data->bitangent;
        
        c.inertiaNa = calculate_inertia(ca, ta, c.normal, c.pos_a);
        c.inertiaTa = calculate_inertia(ca, ta, c.tangent, c.pos_a);
        c.inertiaBa = calculate_inertia(ca, ta, c.bitangent, c.pos_a);
        
        c.inertiaNb = 0.0f;
        c.inertiaTb = 0.0f;
        c.inertiaBb = 0.0f;
        
        rel_a = c.constraint_point.a;
        rel_b = c.constraint_point.b;
    }

    c.inertiaN = c.inertiaNa + c.inertiaNb;
    c.inertiaT = c.inertiaTa + c.inertiaTb;
    c.inertiaB = c.inertiaBa + c.inertiaBb;

    vec3 diff = rel_b - rel_a;
    c.baumgarteN = dot(diff, c.normal);
    c.baumgarteT = dot(diff, c.tangent);
    c.baumgarteB = dot(diff, c.bitangent);
}

void Collision_constraint::pre_step() {
    if(b == NULL_ENTITY) {
        mat3 inverse_tensor_a = ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation);
        ca->iit_rot = inverse_tensor_a;

        for(col_constraint& constraint : constraints) {
            if(constraint.data->tangent.x == 0.0f && constraint.data->tangent.y == 0.0f && constraint.data->tangent.z == 0.0f) {
                vec3 normal = constraint.data->normal;

                vec3 tangent = vec3(0.0f, -normal.z, normal.y);
                float t_len = length(tangent);
                if(t_len < 0.001f) {
                    tangent = vec3(-normal.y, normal.x, 0.0f);
                    t_len = length(tangent);
                }
                tangent /= t_len;

                vec3 bitangent = normalize(cross(tangent, normal));
                
                constraint.data->tangent = tangent;
                constraint.data->bitangent = bitangent;
            }

            //

            vec3 pa = ta->orientation * vec3(constraint.data->contact_point.a);
            vec3 pb = constraint.data->contact_point.b - ta->position;
            constraint.constraint_point.a = pa;
            constraint.constraint_point.b = pb;
            
            constraint.pos_a = constraint.constraint_point.a;
            
            constraint.normal = constraint.data->normal;
            constraint.tangent = constraint.data->tangent;
            constraint.bitangent = constraint.data->bitangent;
            
            float inv_mass = 1.0f / ca->mass;
            vec3 d;

            d = cross(constraint.pos_a, constraint.normal);
            constraint.inertiaNa = inv_mass + dot(d, inverse_tensor_a * d);

            d = cross(constraint.pos_a, constraint.tangent);
            constraint.inertiaTa = inv_mass + dot(d, inverse_tensor_a * d);

            d = cross(constraint.pos_a, constraint.bitangent);
            constraint.inertiaBa = inv_mass + dot(d, inverse_tensor_a * d);

            //
            
            constraint.inertiaNb = 0.0f;
            constraint.inertiaTb = 0.0f;
            constraint.inertiaBb = 0.0f;
            
            vec3 rel_a = constraint.constraint_point.a;
            vec3 rel_b = constraint.constraint_point.b;

            //

            constraint.inertiaN = constraint.inertiaNa + constraint.inertiaNb;
            constraint.inertiaT = constraint.inertiaTa + constraint.inertiaTb;
            constraint.inertiaB = constraint.inertiaBa + constraint.inertiaBb;

            vec3 diff = rel_b - rel_a;
            constraint.baumgarteN = dot(diff, constraint.normal);
        }
    } else {
        mat3 inverse_tensor_a = ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation);
        mat3 inverse_tensor_b = tb->orientation * cb->inverse_inertia_tensor * transpose(tb->orientation);
        ca->iit_rot = inverse_tensor_a;
        cb->iit_rot = inverse_tensor_b;
        
        for(col_constraint& constraint : constraints) {
            if(constraint.data->tangent.x == 0.0f && constraint.data->tangent.y == 0.0f && constraint.data->tangent.z == 0.0f) {
                vec3 normal = constraint.data->normal;

                vec3 tangent = vec3(0.0f, -normal.z, normal.y);
                float t_len = length(tangent);
                if(t_len < 0.001f) {
                    tangent = vec3(-normal.y, normal.x, 0.0f);
                    t_len = length(tangent);
                }
                tangent /= t_len;

                vec3 bitangent = normalize(cross(tangent, normal));
                
                constraint.data->tangent = tangent;
                constraint.data->bitangent = bitangent;
            }

            //

            vec3 pa = ta->orientation * vec3(constraint.data->contact_point.a);
            vec3 pb = tb->orientation * vec3(constraint.data->contact_point.b); // contact point b is relative to b
            constraint.constraint_point.b = pb;
            constraint.constraint_point.a = pa;
            // both constraint points are relative to A's position in global space
            
            constraint.pos_a = constraint.constraint_point.a;
            constraint.pos_b = constraint.constraint_point.b;
            
            constraint.normal = constraint.data->normal;
            constraint.tangent = constraint.data->tangent;
            constraint.bitangent = constraint.data->bitangent;

            //
            
            float inv_mass = 1.0f / ca->mass;
            vec3 d;

            d = cross(constraint.pos_a, constraint.normal);
            constraint.inertiaNa = inv_mass + dot(d, inverse_tensor_a * d);

            d = cross(constraint.pos_a, constraint.tangent);
            constraint.inertiaTa = inv_mass + dot(d, inverse_tensor_a * d);

            d = cross(constraint.pos_a, constraint.bitangent);
            constraint.inertiaBa = inv_mass + dot(d, inverse_tensor_a * d);
            
            inv_mass = 1.0f / cb->mass;

            d = cross(constraint.pos_b, -constraint.normal);
            constraint.inertiaNb = inv_mass + dot(d, inverse_tensor_b * d);
            
            d = cross(constraint.pos_b, -constraint.tangent);
            constraint.inertiaTb = inv_mass + dot(d, inverse_tensor_b * d);
            
            d = cross(constraint.pos_b, -constraint.tangent);
            constraint.inertiaBb = inv_mass + dot(d, inverse_tensor_b * d);

            vec3 rel_a = constraint.constraint_point.a;
            vec3 rel_b = constraint.constraint_point.b + (tb->position - ta->position);

            constraint.inertiaN = constraint.inertiaNa + constraint.inertiaNb;
            constraint.inertiaT = constraint.inertiaTa + constraint.inertiaTb;
            constraint.inertiaB = constraint.inertiaBa + constraint.inertiaBb;

            vec3 diff = rel_b - rel_a;
            constraint.baumgarteN = dot(diff, constraint.normal);
        }
    }
}

// constraint

void Constraint::pre_step() {
    ca = &ecs.get_component<Collider>(a);
    ta = &ecs.get_component<Transform>(a);
    if(b != NULL_ENTITY) {  
        cb = &ecs.get_component<Collider>(b);
        tb = &ecs.get_component<Transform>(b);
    }
    
    ca->iit_rot = ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation);
    if(b != NULL_ENTITY) cb->iit_rot = tb->orientation * cb->inverse_inertia_tensor * transpose(tb->orientation);

    for(pos_constraint& pc : pos) {
        pc.baumgarte.resize(pc.vs.size());
        pc.inertia_a.resize(pc.vs.size());
        pc.inertia_b.resize(pc.vs.size());
        pc.lambda.resize(pc.vs.size(), 0.0f);

        refresh(pc);
    }

    for(rot_constraint& rc : rot) {
        rc.baumgarte.resize(rc.vs.size());
        rc.inertia_a.resize(rc.vs.size());
        rc.inertia_b.resize(rc.vs.size());
        rc.lambda.resize(rc.vs.size(), 0.0f);
        rc.wvs.resize(rc.vs.size());

        refresh(rc);
    }
}

void Constraint::refresh(pos_constraint& c) {
    vec3 point_a = ta->orientation * vec3(c.a);
    c.ra = point_a;
    c.wa = pvec3(point_a) + ta->position;
    for(int i = 0; i < c.vs.size(); ++i) c.inertia_a[i] = calculate_inertia(ca, ta, c.vs[i], c.ra);

    if(b != NULL_ENTITY) {
        vec3 point_b = tb->orientation * vec3(c.b);
        c.rb = point_b;
        c.wb = pvec3(point_b) + tb->position;
        for(int i = 0; i < c.vs.size(); ++i) c.inertia_b[i] = calculate_inertia(cb, tb, c.vs[i], c.rb);
    } else {
        c.wb = c.b;
    }

    for(int i = 0; i < c.vs.size(); ++i) {
        vec3 diff = c.wa - c.wb;
        float dd = dot(diff, c.vs[i]);
        c.baumgarte[i] = dd;
    }
}

void Constraint::refresh(rot_constraint& c) {
    if(c.c == NULL_ENTITY) {
        for(int i = 0; i < c.vs.size(); ++i) c.wvs[i] = c.vs[i];
    } else {
        Transform& tf = ecs.get_component<Transform>(c.c);
        for(int i = 0; i < c.vs.size(); ++i) c.wvs[i] = tf.orientation * c.vs[i];
    }

    vec3 axis_a = ta->orientation * c.a;
    c.wa = axis_a;
    for(int i = 0; i < c.vs.size(); ++i) c.inertia_a[i] = calculate_inertia(ca, ta, c.wvs[i]);

    if(b != NULL_ENTITY) {
        vec3 axis_b = tb->orientation * c.b;
        c.wb = axis_b;
        for(int i = 0; i < c.vs.size(); ++i) c.inertia_b[i] = calculate_inertia(cb, tb, c.wvs[i]);
    } else {
        c.wb = c.b;
    }

    for(int i = 0; i < c.vs.size(); ++i) {
        vec3 wc = c.wvs[i];

        float diff = dot(c.wa, c.wb);
        diff = acos(clamp(diff, -1.0f, 1.0f));

        vec3 proj_a = normalize(c.wa - wc * dot(c.wa, wc));
        vec3 proj_b = normalize(c.wb - wc * dot(c.wb, wc));

        float angle = acos(clamp(dot(proj_a, proj_b), -1.0f, 1.0f));

        if(dot(cross(proj_a, proj_b), wc) > 0) angle = -angle;

        if(isnan(angle)) angle = 0.0f;

        c.baumgarte[i] = min(abs(angle), diff) * sign(angle);
    }
}

float calculate_inertia(Collider* c, Transform* t, vec3 dir, vec3 point) {
    float inertia = 1.0f / c->mass;

    if(c->allow_rotation && !c->is_static) {
        vec3 d = cross(point, dir);

        mat3 inverse_inertia_tensor = t->orientation * c->inverse_inertia_tensor * transpose(t->orientation);

        float angular_inertia = dot(d, inverse_inertia_tensor * d);
        // amount of change in linear (rotational) velocity in the direction of the contact normal from one unit of impulse

        inertia += angular_inertia;
    }

    return inertia;
}

float calculate_inertia(Collider* c, Transform* t, vec3 dir) {
    float inertia = 0.0f;

    if(c->allow_rotation) {
        mat3 inverse_inertia_tensor = t->orientation * c->inverse_inertia_tensor * transpose(t->orientation);

        vec3 av = inverse_inertia_tensor * dir;

        float angular_inertia = dot(av, dir);
        // amount of change in linear (rotational) velocity in the direction of the contact normal from one unit of impulse

        inertia += abs(angular_inertia);
    }

    return inertia;
}

void Collider::apply_impulse(vec3 impulse, vec3 position) {
    bool b0 = isnan(velocity.x) || isnan(velocity.y) || isnan(velocity.z) || isnan(angular_momentum.x) || isnan(angular_momentum.y) || isnan(angular_momentum.z);

    if(allow_rotation) {
        velocity += impulse / mass;

        vec3 torque = cross(position, impulse);
        angular_momentum += torque;
    } else {
        velocity += impulse / mass;
    }
    
    bool b1 = isnan(velocity.x) || isnan(velocity.y) || isnan(velocity.z) || isnan(angular_momentum.x) || isnan(angular_momentum.y) || isnan(angular_momentum.z);
}

bool ff = false;
vec3 Collider::get_velocity(vec3 position) {
    vec3 linear_velocity = velocity;
    
    if(allow_rotation) {
        vec3 angular_velocity = get_angular_velocity();
        vec3 vel = cross(angular_velocity, position);
        
        linear_velocity += vel;
    }

    ff = false;

    return linear_velocity;
}


vec3 Collider::get_angular_velocity() {
    vec3 angular_velocity = (iit_rot) * angular_momentum;
    return angular_velocity;
}


std::vector<uint32_t> triangulate(std::vector<vec3> vertices) {
    Simplex simplex;

    vec3 center = vec3(0.0f);
    for(vec3 v : vertices) center += v;
    center /= vertices.size();
    for(vec3& v : vertices) v -= center;
    
    std::unordered_set<uint32_t> set;
    for(uint32_t i = 1; i < vertices.size(); ++i) {
        set.emplace(i);
    }

    // 1
    simplex.vertices.push_back(Simplex_vertex{vertices[0], vec3(0)});
    vec3 direction = -glm::normalize(simplex.vertices[0].m);

    // 2
    uint32_t next = support_func(vertices, set, direction);
    simplex.vertices.push_back(Simplex_vertex{vertices[next], vec3(next)});
    
    glm::vec3 line_direction = normalize(simplex.vertices[0].m - simplex.vertices[1].m);
    glm::vec3 rel_origin_pos = -simplex.vertices[1].m;
    glm::vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
    direction = glm::normalize(-closest_point);
    set.erase(next);

    // 3
    next = support_func(vertices, set, direction);
    simplex.vertices.push_back(Simplex_vertex{vertices[next], vec3(next)});

    center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
    glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
    if(glm::dot(normal, -center) <= 0.0f) normal = -normal;
    direction = normal;
    set.erase(next);

    // 4
    next = support_func(vertices, set, direction);
    simplex.vertices.push_back(Simplex_vertex{vertices[next], vec3(next)});
    set.erase(next);

    Polytope p;
    p.from_simplex(simplex);

    for(uint32_t i : set) {
        vec3 vertex = vertices[i];
        p.expand(Simplex_vertex(vertex, vec3(i)));
    }

    std::vector<uint32_t> indices;
    for(Polytope_face pf : p.faces) {
        for(uint32_t i : pf.vertices) {
            Simplex_vertex& v = p.vertices[i];
            indices.push_back(v.a[0]);
        }
    }

    return indices;
}

bool compare_sap(Sap_point& a, Sap_point& b) {
    return a.minimum < b.minimum;
};

struct spacial_data {
    uint32_t i;
    bool is_static = false;
    Bounding_box* bb;
    Transform* t;
};

template<std::size_t num_bits>
struct bit_key {
    std::bitset<num_bits> x;
    std::bitset<num_bits> y;
    std::bitset<num_bits> z;
};

template<std::size_t num_bits>
bit_key<num_bits> operator>>(bit_key<num_bits> bc, std::size_t shift) {
    bc.x >>= shift;
    bc.y >>= shift;
    bc.z >>= shift;

    return bc;
}

template<std::size_t num_bits>
bit_key<num_bits> operator<<(bit_key<num_bits> bc, std::size_t shift) {
    bc.x <<= shift;
    bc.y <<= shift;
    bc.z <<= shift;

    return bc;
}

template<std::size_t num_bits>
struct hash_bits {
    std::size_t operator()(const bit_key<num_bits>& key) const {
        return std::hash<std::bitset<num_bits>>()(key.x) ^ (std::hash<std::bitset<num_bits>>()(key.y) << 16) ^ (std::hash<std::bitset<num_bits>>()(key.z) << 32);
    }
};

template<std::size_t num_bits>
struct equal_bits {
    std::size_t operator()(const bit_key<num_bits>& a, const bit_key<num_bits>& b) const {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

bit_key<72> create_key(ivec3 rel, vec<3, int64_t> sector) {
    bit_key<72> key;
    
    key.x = sector.x;
    key.x <<= 8;
    key.x = key.x | std::bitset<72>(rel.x);
    
    key.y = sector.y;
    key.y <<= 8;
    key.y = key.y | std::bitset<72>(rel.y);

    key.z = sector.z;
    key.z <<= 8;
    key.z = key.z | std::bitset<72>(rel.z);

    return key;
}

void output_bitset(std::bitset<72> b) {
    std::bitset<64> l;
    for(int i = 0; i < 64; ++i) {
        l[i] = b[i + 8];
    }
    std::bitset<8> l2;
    for(int i = 0; i < 8; ++i) {
        l2[i] = b[i];
    }

    int64_t n2 = l2.to_ulong();
    int64_t n = l.to_ulong();
}

std::vector<uint64_t> Physics_system::broad_phase() {
    const std::size_t max_i = 12;
    const float ratio = 4.0f;
    const float start_size = 4.0f;

    std::unordered_set<uint64_t> set;

    std::array<std::unordered_map<bit_key<72>, std::vector<spacial_data>, hash_bits<72>, equal_bits<72>>, max_i> spacial;

    auto insert_into = [&](Collider& c, Transform& t, uint32_t entity) {
        vec3 frac = vec3(t.position.x.fraction, t.position.y.fraction, t.position.b.fraction);
        Bounding_box bb = transform_bb(c.bounding_box, frac, t.orientation);

        vec3 size = bb.maximum - bb.minimum;

        float max_size = max(max(size.x, size.y), size.z);

        std::unordered_map<bit_key<72>, std::vector<spacial_data>, hash_bits<72>, equal_bits<72>>* spacial_scale = nullptr;

        uint32_t ii = 0xFFFFFFFF;
        float bucket_size = start_size;
        for(int i = 0; i < max_i; ++i) {
            if(max_size <= bucket_size || i == max_i - 1) {
                spacial_scale = &spacial[i];
                ii = i;
                break;
            }
            bucket_size *= ratio;
        }

        vec3 min = bb.minimum;
        vec3 max = bb.maximum;

        vec<3, int64_t> sector = {t.position.x.sector, t.position.y.sector, t.position.z.sector};
        if(bucket_size > sector_size) {
            int64_t sector_bucket = bucket_size / sector_size;

            vec<3, int64_t> s_bucket = sector % sector_bucket;
            sector -= s_bucket;

            min += vec3(ivec3(s_bucket) * (int)sector_size);
            max += vec3(ivec3(s_bucket) * (int)sector_size);
        }
        
        ivec3 mmin = ivec3(floor(min / bucket_size));
        ivec3 mmax = ivec3(floor(max / bucket_size));

        spacial_data sd;
        sd.i = entity;
        sd.bb = &c.bounding_box;
        sd.t = &t;
        sd.is_static = c.is_static;

        for(int z = mmin.z; z <= mmax.z; ++z) {
            for(int y = mmin.y; y <= mmax.y; ++y) {
                for(int x = mmin.x; x <= mmax.x; ++x) {
                    ivec3 i = {x, y, z};
                    i *= bucket_size;
                    vec<3, int64_t> i_sector = sector;

                    ivec3 r = i % int(sector_size);
                    if(r.x < 0) r.x += sector_size;
                    if(r.y < 0) r.y += sector_size;
                    if(r.z < 0) r.z += sector_size;

                    auto pi = i;

                    vec<3, int64_t> d = i - r;
                    auto pd = d;
                    d /= int(sector_size);
                    i_sector += d;
                    i = r;

                    bit_key<72> key = create_key(i, i_sector);

                    if(!spacial_scale->contains(key)) {
                        spacial_scale->emplace(key, std::vector<spacial_data>{});
                    }
                    auto& bucket = spacial_scale->at(key);

                    bucket.push_back(sd);
                }       
            }
        }
    };

    for(uint32_t v : collectors[0].entities) {
        Collider& c = ecs.get_component<Collider>(v);
        Transform& t = ecs.get_component<Transform>(v);

        if(!c.init_bb) {
            create_bounding_box(c);
        }

        insert_into(c, t, v);
    }
    
    float bucket_size = start_size;
    for(int k = 0; k < max_i; ++k) {
        auto& s = spacial[k];
        for(auto& [key, v] : s) {
            for(int i = 0; i < v.size(); ++i) {
                auto vi = v[i];

                // loop through all shapes in same level
                for(int j = i + 1; j < v.size(); ++j) {
                    auto vj = v[j];

                    if(!(vi.is_static && vj.is_static) && collision(*vi.t, *vi.bb, *vj.t, *vj.bb)) {
                        uint64_t kk;
                        if(vi.i < vj.i) kk = (uint64_t)vi.i | (uint64_t(vj.i) << 32);
                        else kk = (uint64_t)vj.i | (uint64_t(vi.i) << 32);

                        set.emplace(kk);
                    }
                }

                for(int m = k + 1; m < max_i; ++m) {
                    bit_key<72> key3 = key >> ((m + 1) * 2);
                    key3 = key3 << ((m + 1) * 2);

                    if(spacial[m].contains(key3)) {
                        auto& v2 = spacial[m].at(key3);

                        for(int j = 0; j < v2.size(); ++j) {
                            auto vj = v2[j];

                            if(!(vi.is_static && vj.is_static) && collision(*vi.t, *vi.bb, *vj.t, *vj.bb)) {
                                uint64_t kk;
                                if(vi.i < vj.i) kk = (uint64_t)vi.i | (uint64_t(vj.i) << 32);
                                else kk = (uint64_t)vj.i | (uint64_t(vi.i) << 32);

                                set.emplace(kk);
                            }
                        }
                    }
                }
            }
        }
        bucket_size *= ratio;
    }

    return std::vector<uint64_t>(set.begin(), set.end());
}

struct State {
    vec3 velocity;
    vec3 angular_velocity;
};

void Physics_system::velocity_solve(std::vector<Collision_constraint>& collision_constraints) {
    col_constraint* cs;
    Collision_constraint* ccs;

    auto lambda_apply = [&](Collider* collider, vec3 impulse, vec3 position) {
        vec3 vel = impulse / collider->mass;

        collider->velocity += vel;

        vec3 twirl = cross(position, impulse);

        collider->angular_momentum += twirl;
    };

    auto rot_apply = [&](Collider* collider, vec3 twirl) {
        collider->angular_momentum += twirl;
    };

    for(Collision_constraint& data : collision_constraints) {
        data.pre_step();
        ccs = &data;

        for(col_constraint& cc : data.constraints) {
            cs = &cc;
            
            cc.lambdaN = cc.data->lambdaN;
            cc.lambdaT = cc.data->lambdaT;
            cc.lambdaB = cc.data->lambdaB;

            vec3 normal_impulse = cc.normal * cc.lambdaN;
            vec3 tangent_impulse = cc.tangent * cc.lambdaT;
            vec3 bitangent_impulse = cc.bitangent * cc.lambdaB;

            lambda_apply(data.ca, normal_impulse, cc.pos_a);
            lambda_apply(data.ca, tangent_impulse, cc.pos_a);
            lambda_apply(data.ca, bitangent_impulse, cc.pos_a);

            if(data.b != 0xFFFFFFFF) {
                lambda_apply(data.cb, -normal_impulse, cc.pos_b);
                lambda_apply(data.cb, -tangent_impulse, cc.pos_b);
                lambda_apply(data.cb, -bitangent_impulse, cc.pos_b);
            }
        }
    }

    for(Constraint& data : constraints) {
        data.ca = &ecs.get_component<Collider>(data.a);
        data.ta = &ecs.get_component<Transform>(data.a);
        if(data.b != NULL_ENTITY) {
            data.cb = &ecs.get_component<Collider>(data.b);
            data.tb = &ecs.get_component<Transform>(data.b);
        }

        data.pre_step();


        for(pos_constraint& c : data.pos) {
            uint32_t i = 0;
            for(vec3 v : c.vs) {
                vec3 impulse = v * c.lambda[i];

                if(data.b == NULL_ENTITY) {
                    lambda_apply(data.ca, impulse, c.ra);
                } else {
                    lambda_apply(data.ca, impulse, c.ra);
                    lambda_apply(data.cb, -impulse, c.rb);
                }

                ++i;
            }
        }
        
        for(rot_constraint& c : data.rot) {
            int i = 0;
            for(vec3 v : c.wvs) {
                vec3 twirl = c.lambda[i] * v;
                
                data.ca->angular_momentum += twirl;
                data.cb->angular_momentum -= twirl;

                ++i;
            }
        }
    }

    for(int i = 0; i < iterations; ++i) {
        /*
        for(Constraint& data : constraints) {
            for(pos_constraint& pc : data.pos) {
                if(data.b == NULL_ENTITY) {
                    pc.vel_a = data.ca->get_velocity(pc.ra);
                } else {
                    pc.vel_a = data.ca->get_velocity(pc.ra);
                    pc.vel_b = data.cb->get_velocity(pc.rb);
                }
            }
        }
        */
            
        for(int j = 0; j < constraints.size(); ++j) { 
        //for(Collision_constraint& data : collision_constraints) {
            int start = 0;//core.random.next() % collision_constraints.size();
            int dir = 1;

            if(i % 2 == 1) {
                start = constraints.size() - 1;
                dir = -1;
            }

            Constraint& data = constraints[start + j * dir];

            if(data.b == 0xFFFFFFFF) {
                uint32_t i = 0;
                for(pos_constraint& pc : data.pos) {
                    uint32_t j = 0;
                    for(vec3 v : pc.vs) {
                        vec3 velocity = data.ca->get_velocity(pc.ra);

                        float baumgarte = -pc.baumgarte[j] * pc.spring / physics_step;
                        
                        float L = baumgarte - dot(v, velocity);
                        L /= pc.inertia_a[j];
                        if(do_dampening) L -= pc.softness * pc.lambda[j];
                        float new_lambda = pc.lambda[j] + L;
                        //new_lambda = clamp(new_lambda, -pc.max_impulse, pc.max_impulse);
                        
                        L = new_lambda - pc.lambda[j];
                        pc.lambda[j] = new_lambda;
                        
                        vec3 impulse = v * L;
                        
                        lambda_apply(data.ca, impulse, pc.ra);

                        ++j;
                    }
                    ++i;
                }
            } else {
                uint32_t i = 0;
                for(pos_constraint& pc : data.pos) {
                    uint32_t j = 0;
                    for(vec3 v : pc.vs) {
                        if(data.ca->mass == 1.0f) ff = true;
                        vec3 velocity = data.ca->get_velocity(pc.ra);

                        if(data.cb->mass == 1.0f) ff = true;
                        velocity -= data.cb->get_velocity(pc.rb);

                        float baumgarte = -pc.baumgarte[j] * pc.spring / physics_step;
                        
                        float L = baumgarte - dot(v, velocity);

                        std::cout << baumgarte << " " << (L - baumgarte) << "\n";
                        L /= pc.inertia_a[j] + pc.inertia_b[j];

                        if(do_dampening) L -= pc.softness * pc.lambda[j];
                        float new_lambda = pc.lambda[j] + L;
                        //new_lambda = clamp(new_lambda, -pc.max_impulse, pc.max_impulse);

                        L = new_lambda - pc.lambda[j];
                        pc.lambda[j] = new_lambda;
                        
                        vec3 impulse = v * L;
                        
                        lambda_apply(data.ca, impulse, pc.ra);
                        lambda_apply(data.cb, -impulse, pc.rb);

                        ++j;
                    }
                    ++i;
                } 
                
                i = 0;
                for(rot_constraint& rc : data.rot) {
                    uint32_t j = 0;
                    for(vec3 v : rc.wvs) {
                        vec3 vel_a = data.ca->iit_rot * data.ca->angular_momentum;
                        vec3 vel_b = data.cb->iit_rot * data.cb->angular_momentum;

                        vec3 rel_velocity = vel_a - vel_b;

                        float baumgarte = -rc.baumgarte[j] * rc.spring / physics_step;

                        float L = baumgarte - dot(rel_velocity, v);
                        L /= rc.inertia_a[j] + rc.inertia_b[j];
                        if(do_dampening) L -= rc.softness * rc.lambda[j];
                        float new_lambda = rc.lambda[j] + L;
                        new_lambda = clamp(new_lambda, -rc.max_impulse, rc.max_impulse);

                        L = new_lambda - rc.lambda[j];
                        rc.lambda[j] = new_lambda;
                        
                        vec3 impulse = v * L;
                        
                        rot_apply(data.ca, impulse);
                        rot_apply(data.cb, -impulse);

                        ++j;
                    }
                    ++i;
                }
            }
        }

        for(int j = 0; j < collision_constraints.size(); ++j) { 
        //for(Collision_constraint& data : collision_constraints) {
            uint32_t start = 0;//core.random.next() % collision_constraints.size();
            Collision_constraint& data = collision_constraints[(j + start) % collision_constraints.size()];
            ccs = &data;

            for(col_constraint& cc : data.constraints) {
                cs = &cc;
                
                vec3 velocity;

                float baumgarte = cc.baumgarteN * cc.spring / physics_step;

                if(data.b != 0xFFFFFFFF) {
                    velocity = data.ca->get_velocity(cc.pos_a) - data.cb->get_velocity(cc.pos_b);

                    // normal force

                    float L = baumgarte - dot(cc.normal, velocity);
                    L /= cc.inertiaN;
                    L -= cc.softness * cc.lambdaN;
                    float new_lambda = cc.lambdaN + L;
                    new_lambda = clamp(new_lambda, 0.0f, __FLT_MAX__);
                    L = new_lambda - cc.lambdaN;
                    cc.lambdaN = new_lambda;
                    
                    vec3 impulse = cc.normal * L;
                    
                    lambda_apply(data.ca, impulse, cc.pos_a);
                    lambda_apply(data.cb, -impulse, cc.pos_b);

                    float friction_max = cc.mu * abs(cc.lambdaN);

                    // tangent

                    velocity = data.ca->get_velocity(cc.pos_a) - data.cb->get_velocity(cc.pos_b);

                    L = -dot(cc.tangent, velocity);
                    L /= cc.inertiaT;
                    new_lambda = cc.lambdaT + L;
                    new_lambda = clamp(new_lambda, -friction_max, friction_max);
                    L = new_lambda - cc.lambdaT;
                    cc.lambdaT = new_lambda;

                    impulse = cc.tangent * L;

                    lambda_apply(data.ca, impulse, cc.pos_a);
                    lambda_apply(data.cb, -impulse, cc.pos_b);
                    
                    // bitangent

                    velocity = data.ca->get_velocity(cc.pos_a) - data.cb->get_velocity(cc.pos_b);

                    L = -dot(cc.bitangent, velocity);
                    L /= cc.inertiaB;
                    new_lambda = cc.lambdaB + L;
                    new_lambda = clamp(new_lambda, -friction_max, friction_max);
                    L = new_lambda - cc.lambdaB;
                    cc.lambdaB = new_lambda;

                    impulse = cc.bitangent * L;

                    lambda_apply(data.ca, impulse, cc.pos_a);
                    lambda_apply(data.cb, -impulse, cc.pos_b);
                } else {
                    velocity = data.ca->get_velocity(cc.pos_a);

                    // normal force

                    float L = baumgarte - dot(cc.normal, velocity);
                    L /= cc.inertiaN;
                    L -= cc.softness * cc.lambdaN;
                    float new_lambda = cc.lambdaN + L;
                    new_lambda = clamp(new_lambda, 0.0f, __FLT_MAX__);
                    L = new_lambda - cc.lambdaN;
                    cc.lambdaN = new_lambda;
                    
                    vec3 impulse = cc.normal * L;
                    
                    lambda_apply(data.ca, impulse, cc.pos_a);

                    float friction_max = cc.mu * abs(cc.lambdaN);

                    // tangent

                    velocity = data.ca->get_velocity(cc.pos_a);

                    L = -dot(cc.tangent, velocity);
                    L /= cc.inertiaT;
                    new_lambda = cc.lambdaT + L;
                    new_lambda = clamp(new_lambda, -friction_max, friction_max);
                    L = new_lambda - cc.lambdaT;
                    cc.lambdaT = new_lambda;

                    impulse = cc.tangent * L;

                    lambda_apply(data.ca, impulse, cc.pos_a);
                    
                    // tangent

                    velocity = data.ca->get_velocity(cc.pos_a);

                    L = -dot(cc.bitangent, velocity);
                    L /= cc.inertiaB;
                    new_lambda = cc.lambdaB + L;
                    new_lambda = clamp(new_lambda, -friction_max, friction_max);
                    L = new_lambda - cc.lambdaB;
                    cc.lambdaB = new_lambda;

                    impulse = cc.bitangent * L;

                    lambda_apply(data.ca, impulse, cc.pos_a);
                }
            }
        }

        if(do_DOF) {
            for(DOF_constraint& c : dof_constraints) {
                c.apply(sub_dt);
            }
        }
    }

    for(Collision_constraint& data : collision_constraints) {
        for(col_constraint& cc : data.constraints) {
            cc.data->lambdaN = cc.lambdaN;
            cc.data->lambdaT = cc.lambdaT;
            cc.data->lambdaB = cc.lambdaB;
        }
    }
}

void Physics_system::apply_position(Collider* c, Transform* t, vec3 lambda, vec3 point) {
    vec3 d = lambda / c->mass;
    t->position += d;

    mat3 current_iit = t->orientation * c->inverse_inertia_tensor * transpose(t->orientation);

    vec3 am_delta = cross(point, lambda);
    vec3 delta_rotation = current_iit * am_delta;

    /*
    c->am_delta += am_delta;
    c->pos_delta += d;

    float len = length(delta_rotation);
    if(len != 0.0f) t->orientation = mat3(rotate(len, delta_rotation / len)) * t->orientation;
    */

    float len = length(d);
    if(len != 0.0f) {
        vec3 delta_d = d / sub_dt;
        c->pos_delta += delta_d;
    }
    
    len = length(delta_rotation);
    if(len != 0.0f) {
        vec3 delta_d = delta_rotation / sub_dt;
        c->am_delta += (t->orientation * c->inertia_tensor * transpose(t->orientation)) * delta_d;
    }
    if(len != 0.0f) t->orientation = mat3(rotate(len, delta_rotation / len)) * t->orientation;
}

void Physics_system::apply_rotation(Collider* c, Transform* t, vec3 lambda) {
    mat3 current_iit = t->orientation * c->inverse_inertia_tensor * transpose(t->orientation);

    vec3 am_delta = lambda;
    vec3 delta_rotation = current_iit * lambda;
    
    c->am_delta += am_delta;
    
    float len = length(delta_rotation);
    if(len != 0.0f) t->orientation = mat3(rotate(len, delta_rotation / len)) * t->orientation;
}

mat3 rotate_to(vec3 a, vec3 b) {
    vec3 cross_p = cross(a, b);
    float d = dot(a, b);
    float f = acos(d);
    vec3 n = normalize(cross_p);

    if(isinf(n.x) || isnan(n.x) || f == 0 || isnan(f)) {
        mat3 rot_mat = identity<mat3>();
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

/*
mat3 rotate_to(vec3 a, vec3 b, bool o) {
    vec3 cross_p = cross(b, a);
    float d = dot(a, b);
    float f = acos(d);
    vec3 n = normalize(cross_p);

    if(o) std::cout << f << " " << n << "\n";

    return rotate(f, n);
}
*/

void create_mesh_from_collider(uint32_t entity, vec3 color) {
    Collider& c = ecs.get_component<Collider>(entity);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;
    
    Mesh_component mc;
    std::shared_ptr<Mesh> mesh(new Mesh);
    
    ivec4 tex = ivec4(16, 128, 32, 32);
    
    for(Convex_collider& cc : c.collision_shapes) {
        std::vector<vec3> vertices;

        for(vec3 v : cc.collision_shape->vertices) {
            v = cc.orientation * v + vec3(cc.position);
            vertices.push_back(v);
        }

        vec3 center = vec3(0.0f);
        for(vec3 v : vertices) center += v;
        center /= vertices.size();

        std::vector<uint32_t> indices = triangulate(vertices);

        // create mesh

        uint32_t start = mesh_vertices.size();

        for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
            std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

            vec3 v0 = vertices[triangle_indices[0]];
            vec3 v1 = vertices[triangle_indices[1]];
            vec3 v2 = vertices[triangle_indices[2]];

            vec3 n0;
            vec3 n1;
            vec3 n2;

            vec3 normal = normalize(cross(v0 - v2, v1 - v2));

            if(dot(normal, center - v0) > 0) {
                normal = -normal;
                vec3 temp = v0;
                v0 = v1;
                v1 = temp;

                temp = n0;
                n0 = n1;
                n1 = temp;
            }

            n0 = normal;
            n1 = normal;
            n2 = normal;

            //

            normal = normalize(cross(v0 - v2, v1 - v2));

            normal /= max(abs(normal.x), max(abs(normal.y), abs(normal.z)));
            ivec3 n = round(normal);

            mat3 ori = directional[n];
            vec3 x = ori[1];
            vec3 y = ori[2];

            vec2 tx0 = vec2(dot(x, v0), dot(y, v0)) / 16.0f;
            vec2 tx1 = vec2(dot(x, v1), dot(y, v1)) / 16.0f;
            vec2 tx2 = vec2(dot(x, v2), dot(y, v2)) / 16.0f;

            //

            Mesh_vertex mv;
            mv.bone_weights = vec4(color, 1.0f);
            mv.bone_ids = tex;

            mv.tex_coords = tx0;
            mv.position = v0;
            mv.normal = n0;
            mesh_vertices.push_back(mv);
            mesh_indices.push_back(start + triangle * 3);
            
            mv.tex_coords = tx1;
            mv.position = v1;
            mv.normal = n1;
            mesh_vertices.push_back(mv);
            mesh_indices.push_back(start + triangle * 3 + 1);
            
            mv.tex_coords = tx2;
            mv.position = v2;
            mv.normal = n2;
            mesh_vertices.push_back(mv);
            mesh_indices.push_back(start + triangle * 3 + 2);
        }
    }

    

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    mc.cull = false;
    
    mc.texture = core.textures["tilesheet"];

    ecs.insert_component(entity, mc);
}

void create_mesh_from_vertices(uint32_t entity, vec3 color, std::vector<vec3>& vertices) {
    Collider& c = ecs.get_component<Collider>(entity);

    std::vector<uint32_t> indices = triangulate(vertices);

    vec3 center = vec3(0.0f);
    for(vec3 v : vertices) center += v;
    center /= vertices.size();

    // create mesh

    Mesh_component mc;
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
        std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

        vec3 v0 = vertices[triangle_indices[0]];
        vec3 v1 = vertices[triangle_indices[1]];
        vec3 v2 = vertices[triangle_indices[2]];

        vec3 n0;
        vec3 n1;
        vec3 n2;

        vec3 normal = normalize(cross(v0 - v2, v1 - v2));

        if(dot(normal, center - v0) > 0) {
            normal = -normal;
            vec3 temp = v0;
            v0 = v1;
            v1 = temp;

            temp = n0;
            n0 = n1;
            n1 = temp;
        }

        n0 = normal;
        n1 = normal;
        n2 = normal;

        Mesh_vertex mv;
        mv.tex_coords = vec2(0.5f);

        mv.position = v0;
        mv.normal = n0;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3);
        
        mv.position = v1;
        mv.normal = n1;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 1);
        
        mv.position = v2;
        mv.normal = n2;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 2);
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    mc.cull = false;
    
    Format format = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
    std::vector<uint8_t> pixels = {uint8_t(color.x * 255), uint8_t(color.y * 255), uint8_t(color.z * 255), uint8_t(255)};
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(Texture(pixels.data(), uvec3(1, 1, 1), GL_TEXTURE_2D, format));
    
    mc.texture = texture;

    ecs.insert_component(entity, mc);
}


void modify_mesh_from_vertices(uint32_t entity, vec3 color, std::vector<vec3>& vertices) {
    std::vector<uint32_t> indices = triangulate(vertices);

    // create mesh
    
    vec3 center = vec3(0.0f);
    for(vec3 v : vertices) center += v;
    center /= vertices.size();

    Mesh_component& mc = ecs.get_component<Mesh_component>(entity);
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
        std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

        vec3 v0 = vertices[triangle_indices[0]];
        vec3 v1 = vertices[triangle_indices[1]];
        vec3 v2 = vertices[triangle_indices[2]];

        vec3 n0;
        vec3 n1;
        vec3 n2;

        vec3 normal = normalize(cross(v0 - v2, v1 - v2));

        if(dot(normal, center - v0) > 0) {
            normal = -normal;
            vec3 temp = v0;
            v0 = v1;
            v1 = temp;

            temp = n0;
            n0 = n1;
            n1 = temp;
        }

        n0 = normal;
        n1 = normal;
        n2 = normal;

        Mesh_vertex mv;
        mv.tex_coords = vec2(0.5f);

        mv.position = v0;
        mv.normal = n0;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3);
        
        mv.position = v1;
        mv.normal = n1;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 1);
        
        mv.position = v2;
        mv.normal = n2;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 2);
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    mc.cull = false;
    
    Format format = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
    std::vector<uint8_t> pixels = {uint8_t(color.x * 255), uint8_t(color.y * 255), uint8_t(color.z * 255), uint8_t(255)};
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(Texture(pixels.data(), uvec3(1, 1, 1), GL_TEXTURE_2D, format));
    
    mc.texture = texture;
}

void create_cube_mesh(uint32_t entity, vec3 size, vec3 color, ivec4 range, float scale) {
    Collider& c = ecs.get_component<Collider>(entity);

    std::vector<vec3> vertices = {
        vec3(-1, -1, -1),
        vec3(1, -1, -1),
        vec3(-1, 1, -1),
        vec3(1, 1, -1),
        vec3(-1, -1, 1),
        vec3(1, -1, 1),
        vec3(-1, 1, 1),
        vec3(1, 1, 1),
    };

    for(vec3& v : vertices) v *= size;

    std::vector<uint32_t> indices = {
        2, 0, 4, 2, 4, 6,
        1, 3, 7, 1, 7, 5,
        0, 1, 5, 0, 5, 4,
        3, 2, 6, 3, 6, 7,
        2, 3, 1, 2, 1, 0, 
        4, 5, 7, 4, 7, 6
    };

    // create mesh

    Mesh_component mc;
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
        std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

        vec3 v0 = vertices[triangle_indices[0]];
        vec3 v1 = vertices[triangle_indices[1]];
        vec3 v2 = vertices[triangle_indices[2]];

        vec3 n0;
        vec3 n1;
        vec3 n2;

        vec3 normal = normalize(cross(v0 - v2, v1 - v2));
        vec3 tex_x = normalize(cross(normal, vec3(0, 0, 1)));
        if(isnan(tex_x.x)) tex_x = normalize(cross(normal, vec3(0, 1, 0)));
        vec3 tex_y = normalize(cross(normal, tex_x));

        if(dot(normal, -v0) > 0) {
            normal = -normal;
            vec3 temp = v0;
            v0 = v1;
            v1 = temp;

            temp = n0;
            n0 = n1;
            n1 = temp;
        }

        n0 = normal;
        n1 = normal;
        n2 = normal;

        Mesh_vertex mv;
        mv.tex_coords = vec2(0.5f);

        mv.position = v0;
        mv.normal = n0;
        mv.tex_coords = vec2(dot(tex_x, mv.position - v0), dot(tex_y, mv.position - v0));
        mv.tex_coords *= vec2(16.0f) / vec2(range.zw());
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3);
        
        mv.position = v1;
        mv.normal = n1;
        mv.tex_coords = vec2(dot(tex_x, mv.position - v0), dot(tex_y, mv.position - v0));
        mv.tex_coords *= vec2(16.0f) / vec2(range.zw()) / scale;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 1);
        
        mv.position = v2;
        mv.normal = n2;
        mv.tex_coords = vec2(dot(tex_x, mv.position - v0), dot(tex_y, mv.position - v0));
        mv.tex_coords *= vec2(16.0f) / vec2(range.zw()) / scale;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 2);
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    std::shared_ptr<Texture> texture = create_image_texture("res/textures/tilesheet.png", range, color);
    
    mc.texture = texture;

    ecs.insert_component(entity, mc);
}

void create_cube_mesh(uint32_t entity, vec3 size, vec3 color, std::vector<ivec4> range, std::shared_ptr<Texture> texture) {
    Collider& c = ecs.get_component<Collider>(entity);

    std::vector<vec3> vertices = {
        vec3(-1, -1, -1),
        vec3(1, -1, -1),
        vec3(-1, 1, -1),
        vec3(1, 1, -1),
        vec3(-1, -1, 1),
        vec3(1, -1, 1),
        vec3(-1, 1, 1),
        vec3(1, 1, 1),
    };

    for(vec3& v : vertices) v *= size;

    std::vector<uint32_t> indices = {
        2, 0, 4, 2, 4, 6,
        1, 3, 7, 1, 7, 5,
        0, 1, 5, 0, 5, 4,
        3, 2, 6, 3, 6, 7,
        2, 3, 1, 2, 1, 0, 
        4, 5, 7, 4, 7, 6
    };

    std::vector<vec2> tex_coords = {
        vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f), 
        vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f), 
        vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f), 
        vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f), 
        vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f), 
        vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f)
    };

    // create mesh

    Mesh_component mc;
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
        vec4 tex_range = range[triangle / 2];

        std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

        vec3 v0 = vertices[triangle_indices[0]];
        vec3 v1 = vertices[triangle_indices[1]];
        vec3 v2 = vertices[triangle_indices[2]];

        vec3 n0;
        vec3 n1;
        vec3 n2;

        vec3 normal = normalize(cross(v0 - v2, v1 - v2));
        vec3 tex_x = normalize(cross(normal, vec3(0, 0, 1)));
        if(isnan(tex_x.x)) tex_x = normalize(cross(normal, vec3(0, 1, 0)));
        vec3 tex_y = normalize(cross(normal, tex_x));

        if(dot(normal, -v0) > 0) {
            normal = -normal;
            vec3 temp = v0;
            v0 = v1;
            v1 = temp;

            temp = n0;
            n0 = n1;
            n1 = temp;
        }

        n0 = normal;
        n1 = normal;
        n2 = normal;

        Mesh_vertex mv;
        mv.tex_coords = vec2(0.5f);

        mv.position = v0;
        mv.normal = n0;
        mv.tex_coords = tex_coords[triangle * 3] * tex_range.zw() + tex_range.xy();
        mv.tex_coords /= vec2(texture->size.xy());
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3);
        
        mv.position = v1;
        mv.normal = n1;
        mv.tex_coords = tex_coords[triangle * 3 + 1] * tex_range.zw() + tex_range.xy();
        mv.tex_coords /= vec2(texture->size.xy());
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 1);
        
        mv.position = v2;
        mv.normal = n2;
        mv.tex_coords = tex_coords[triangle * 3 + 2] * tex_range.zw() + tex_range.xy();
        mv.tex_coords /= vec2(texture->size.xy());;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 2);
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    
    mc.texture = texture;

    ecs.insert_component(entity, mc);
}

vec3 get_ellipsoid_normal(vec3 pos, vec3 radii) {
    return normalize(vec3(pos.x / (radii.x * radii.x), pos.y / (radii.y * radii.y), pos.z / (radii.z * radii.z)));
}

void create_sphere_mesh(uint32_t entity, vec3 size, vec3 color) {
    std::vector<vec3> directions = {
        vec3(1, 0, 0),
        vec3(-1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, -1, 0),
        vec3(0, 0, 1),
        vec3(0, 0, -1),
    };

    std::vector<vec3> base_vertices = {
        vec3(0, 0, 0),
        vec3(1, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 1, 0)
    };

    std::vector<vec3> vertices;
    std::vector<uint32_t> indices;
    int num_tiles = 4;//clamp(int(round(max(size.x * 2, max(size.y * 2, size.z * 2)))), 4, 16);

    for(vec3 d : directions) {
        mat3 orientation = rotate_to(vec3(0, 0, 1), d);

        for(int x = 0; x < num_tiles; ++x) {
            for(int y = 0; y < num_tiles; ++y) {
                vec3 rel_pos = vec3(-float(num_tiles) * 0.5f + x, -float(num_tiles) * 0.5f + y, num_tiles / 2);

                for(vec3 b : base_vertices) {
                    b = orientation * (b + rel_pos);
                    b /= float(num_tiles * 0.5f);
                    b = normalize(b) * size;

                    indices.push_back(vertices.size());
                    vertices.push_back(b);
                }
            }
        }
    }
    
    Mesh_component mc;
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
        std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

        vec3 v0 = vertices[triangle_indices[0]];
        vec3 v1 = vertices[triangle_indices[1]];
        vec3 v2 = vertices[triangle_indices[2]];

        vec3 n0;
        vec3 n1;
        vec3 n2;

        vec3 normal = normalize(cross(v0 - v2, v1 - v2));
        if(abs(normal.x) > max(abs(normal.y), abs(normal.z))) normal = vec3(normal.x / abs(normal.x), 0, 0);
        else if(abs(normal.y) > max(abs(normal.x), abs(normal.z))) normal = vec3(0, normal.y / abs(normal.y), 0);
        else if(abs(normal.z) > max(abs(normal.x), abs(normal.y))) normal = vec3(0, 0, (normal.z / abs(normal.z)));

        vec3 tex_x = normalize(cross(normal, vec3(0, 0, 1)));
        if(isnan(tex_x.x)) tex_x = normalize(cross(normal, vec3(0, 1, 0)));
        vec3 tex_y = normalize(cross(normal, tex_x));

        n0 = get_ellipsoid_normal(v0, size);
        n1 = get_ellipsoid_normal(v1, size);
        n2 = get_ellipsoid_normal(v2, size);

        Mesh_vertex mv;
        mv.tex_coords = vec2(0.5f);

        vec3 avg_pos = v0 + v1 + v2;
        avg_pos /= 3.0f;

        mv.position = v0;
        mv.normal = n0;
        mv.tex_coords = vec2(dot(tex_x, mv.position), dot(tex_y, mv.position));
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3);
        
        mv.position = v1;
        mv.normal = n1;
        mv.tex_coords = vec2(dot(tex_x, mv.position), dot(tex_y, mv.position));
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 1);
        
        mv.position = v2;
        mv.normal = n2;
        mv.tex_coords = vec2(dot(tex_x, mv.position), dot(tex_y, mv.position));
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 2);
    }
    
    for(Mesh_vertex& mv : mesh_vertices) {
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    std::shared_ptr<Texture> texture = create_image_texture("res/textures/tilesheet.png", ivec4(16, 160, 16, 16), color);
    
    mc.texture = texture;

    ecs.insert_component(entity, mc);
}

void create_torus_mesh(uint32_t entity, vec4 size, ivec2 num_quads, vec3 color) {
    auto torus_sdf = [&](vec3 pos, vec3 radii) {
        vec2 p = vec2(length(pos.xy()) - radii.x, pos.z);
        vec2 pp = abs(p) / radii.yz();
        float n = 2.0;
        float m = pow(pow(pp.x, n) + pow(pp.y, n), 1.0/n);
        float d = m - 1.0;
        // Scale back to world space
        float k = min(radii.x, radii.y);
        return d * k;
    };

    auto torus_normal = [&](vec3 pos, vec3 radii, float delta) {
        vec3 p0 = pos;
        vec3 px = p0 + vec3(delta, 0.0f, 0.0f);
        vec3 py = p0 + vec3(0.0f, delta, 0.0f);
        vec3 pz = p0 + vec3(0.0f, 0.0f, delta);

        float v0 = torus_sdf(pos, radii);
        float vx = torus_sdf(px, radii);
        float vy = torus_sdf(py, radii);
        float vz = torus_sdf(pz, radii);

        return normalize(vec3(vx - v0, vy - v0, vz - v0) / delta);
    };
    
    std::vector<vec3> base_vertices = {
        vec3(0, 0, 0),
        vec3(1, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 1, 0)
    };

    std::vector<vec3> vertices;
    std::vector<uint32_t> indices;

    vec3 offset = vec3(0.0, size.w / 2, 0.0);

    for(int x = 0; x < num_quads.x; ++x) {
        for(int y = 0; y < num_quads.y; ++y) {
            for(vec3 b : base_vertices) {
                float angle_x = float(x + b.x) / num_quads.x * M_PI * 2.0;
                float angle_y = float(y + b.y) / num_quads.y * M_PI * 2.0;

                vec2 center = vec2(cos(angle_x), sin(angle_x));

                vec3 v = vec3(vec2(center * cos(angle_y) * size.y + center * size.x), sin(angle_y) * size.z);

                if(x < num_quads.x / 2) v += offset;
                else v -= offset;

                indices.push_back(vertices.size());
                vertices.push_back(v);
            }
        }
    }

    // side bars
    for(int y = 0; y < num_quads.y; ++y) {
        for(vec3 b : base_vertices) {
            float angle_y = float(y + b.y) / num_quads.y * M_PI * 2.0;

            vec2 center = vec2(1.0, 0.0);

            vec3 v = vec3(vec2(center * cos(angle_y) * size.y + center * size.x), sin(angle_y) * size.z);

            if(b.x == 1) v += offset;
            else v -= offset;
            
            indices.push_back(vertices.size());
            vertices.push_back(v);
        }
    }
    
    for(int y = 0; y < num_quads.y; ++y) {
        for(vec3 b : base_vertices) {
            float angle_y = float(y + b.y) / num_quads.y * M_PI * 2.0;

            vec2 center = vec2(-1.0, 0.0);

            vec3 v = vec3(vec2(center * cos(angle_y) * size.y + center * size.x), sin(angle_y) * size.z);

            if(b.x == 0) v += offset;
            else v -= offset;
            
            indices.push_back(vertices.size());
            vertices.push_back(v);
        }
    }
    
    Mesh_component mc;
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
        std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

        vec3 v0 = vertices[triangle_indices[0]];
        vec3 v1 = vertices[triangle_indices[1]];
        vec3 v2 = vertices[triangle_indices[2]];

        vec3 n0;
        vec3 n1;
        vec3 n2;

        vec3 normal = normalize(cross(v0 - v2, v1 - v2));
        if(abs(normal.x) > max(abs(normal.y), abs(normal.z))) normal = vec3(normal.x / abs(normal.x), 0, 0);
        else if(abs(normal.y) > max(abs(normal.x), abs(normal.z))) normal = vec3(0, normal.y / abs(normal.y), 0);
        else if(abs(normal.z) > max(abs(normal.x), abs(normal.y))) normal = vec3(0, 0, (normal.z / abs(normal.z)));

        vec3 tex_x = normalize(cross(normal, vec3(0, 0, 1)));
        if(isnan(tex_x.x)) tex_x = normalize(cross(normal, vec3(0, 1, 0)));
        vec3 tex_y = normalize(cross(normal, tex_x));

        vec3 bar_offset = vec3(0.0f, size.w * 0.5f, 0.0f);

        n0 = torus_normal(v0 + bar_offset * (-1.0f + 2.0f * (v0.y < 0.0f)), size, 0.01);
        n1 = torus_normal(v1 + bar_offset * (-1.0f + 2.0f * (v1.y < 0.0f)), size, 0.01);
        n2 = torus_normal(v2 + bar_offset * (-1.0f + 2.0f * (v2.y < 0.0f)), size, 0.01);

        Mesh_vertex mv;
        mv.tex_coords = vec2(0.5f);

        vec3 avg_pos = v0 + v1 + v2;
        avg_pos /= 3.0f;

        mv.position = v0;
        mv.normal = n0;
        mv.tex_coords = vec2(dot(tex_x, mv.position), dot(tex_y, mv.position));
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3);
        
        mv.position = v1;
        mv.normal = n1;
        mv.tex_coords = vec2(dot(tex_x, mv.position), dot(tex_y, mv.position));
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 1);
        
        mv.position = v2;
        mv.normal = n2;
        mv.tex_coords = vec2(dot(tex_x, mv.position), dot(tex_y, mv.position));
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 2);
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    std::shared_ptr<Texture> texture = create_image_texture("res/textures/tilesheet.png", ivec4(16, 160, 16, 16), color);
    
    mc.texture = texture;

    ecs.insert_component(entity, mc);
}

vec3 get_gravity(pvec3 pos) {
    Physics_system& ps = ecs.get_system<Physics_system>();
    vec3 rel_pos = vec3(pos - ps.gravity_center);
    rel_pos = transpose(ps.gravity_orientation) * rel_pos;
    return ps.gravity_orientation * normalize(vec3(rel_pos.x / (ps.gravity_aspect.x * ps.gravity_aspect.x), rel_pos.y / (ps.gravity_aspect.y * ps.gravity_aspect.y), rel_pos.z / (ps.gravity_aspect.z * ps.gravity_aspect.z)));
}

float Physics_system::shape_cast(Collider& shape, mat3 orientation, pvec3 start, vec3 direction, float step, uint32_t* hit, vec3* normal) {
    Transform rel_transform;
    rel_transform.position = vec3(0.0);
    rel_transform.orientation = orientation;

    create_bounding_box(shape);

    float delta = step;

    /*for(vec3 v : shape.vertices) {
        delta = max(length(v), delta);
    }*/

    float len = 10.0f;
    vec3 norm = normalize(direction);
    bool decrease = false;

    float pos = 0.0;
    
    bool collide = false;
        
    
    while(abs(delta) > 0.001f && (pos + delta <= len || collide)) {
        pos += delta;

        Transform t;
        t.position = start + pvec3(norm * pos);
        t.orientation = orientation;
        
        Collider cc = shape;

        for(Convex_collider& convex_collider : cc.collision_shapes) {
            convex_collider.bounding_box.minimum += t.position;
            convex_collider.bounding_box.maximum += t.position;
        }

        collide = false;


        for(uint32_t entity : collectors[0].entities) {
            Collider& ec = ecs.get_component<Collider>(entity);
            Transform& et = ecs.get_component<Transform>(entity);
            
            std::vector<Return_point> data = collision(t, cc, et, ec);

            if(data.size()) {
                collide = true;
                if(hit) *hit = entity;
                if(normal) *normal = data[0].normal;

                goto end_loop;
            }
        }

        end_loop:

        if(delta > 0) {   
            if(collide) {
                delta = -delta * 0.5f;
                decrease = true;
            } else if(decrease) {
                delta *= 0.5f;
            }
        } else {  
            if(!collide) {
                delta = -delta * 0.5f;
            } else if(decrease) {
                delta *= 0.5f;
            }
        }
    }

    return pos;
}

bool Physics_system::raycast(pvec3 start, vec3 direction, float step, float dist, std::unordered_set<uint32_t>& mask, uint32_t* hit, uint32_t* shape_hit, vec3* normal, pvec3* point, float inflate, vec3 axis1, vec3 axis2) {
    std::shared_ptr<Collision_shape> shape(new Collision_shape);
    Collider collider;
    collider.collision_shapes.resize(1);
    collider.collision_shapes[0].collision_shape = shape;

    *hit = NULL_ENTITY;

    vec3 norm = normalize(direction);
    bool decrease = false;

    float pos = 0.0;
    float length = step;

    float prev_pos = 0.0f;
    float prev_length = 0.0f;
    
    bool collide = false;

    float precision = 0.001f;
    bool collide_this_step;

    std::vector<vec3> sweep_shape = {vec3(0)};
    
    while(abs(length) > precision && pos + length <= dist) {
        collide_this_step = false;

        Transform t;
        t.position = start + pvec3(direction * pos);
        t.orientation = identity<mat3>();

        std::vector<vec3> vs = {
            vec3(0.0f),
            direction * length
        };

        std::vector<vec3> vs2;
        vs2.reserve(vs.size() * sweep_shape.size());

        for(vec3 v0 : vs) {
            for(vec3 v1 : sweep_shape) {
                vs2.push_back(v0 + v1);
            }
        }

        collider.collision_shapes[0].collision_shape->vertices = vs2;
        collider.collision_shapes[0].collision_shape->radius = inflate;
        
        create_bounding_box(collider);

        for(uint32_t entity : collectors[0].entities) {
            if(!mask.contains(entity)) {
                Collider& ec = ecs.get_component<Collider>(entity);
                Transform& et = ecs.get_component<Transform>(entity);
                //if(!ec.is_static) {
                    if(collision(et, ec.bounding_box, t, collider.bounding_box)) {
                        if(ec.BVH.size()) {
                            std::vector<uint32_t> shapes = GJK_BVH(ec, et, collider, collider.collision_shapes[0], t);

                            if(shapes.size()) {
                                *hit = entity;
                                *shape_hit = shapes[0];
                                
                                collide_this_step = true;
                                goto end_loop;
                            }
                        } else {
                            uint32_t c = 0;
                            for(Convex_collider& ce : ec.collision_shapes) {
                                for(Convex_collider& convex : collider.collision_shapes) {
                                    if(collision(et, ce.bounding_box, t, convex.bounding_box)) {
                                        bool b = GJK(ce, et, convex, t);

                                        if(b) {
                                            *hit = entity;
                                            *shape_hit = c;
                                            
                                            collide_this_step = true;
                                            goto end_loop;
                                        }
                                    }
                                }
                                ++c;
                            }
                        }
                    }
                //}
            }
        }

        end_loop:

        if(collide_this_step) {
            collide = true;
            decrease = true;
        }

        if(decrease) {
            if(collide_this_step) {
                prev_pos = pos;
                prev_length = length;

                length *= 0.5f;
            } else {
                pos += length;
                
                length *= 0.5f;
            }
        } else {
            pos += length;
        }
    }

    // get normal
    
    Transform t;
    t.position = start + pvec3(direction * prev_pos);
    t.orientation = identity<mat3>();

    std::vector<vec3> vs = {
        vec3(0.0f),
        direction * prev_length
    };

    std::vector<vec3> vs2;
    vs2.reserve(vs.size() * sweep_shape.size());

    for(vec3 v0 : vs) {
        for(vec3 v1 : sweep_shape) {
            vs2.push_back(v0 + v1);
        }
    }

    collider.collision_shapes[0].collision_shape->vertices = vs2;
    collider.collision_shapes[0].collision_shape->radius = inflate;

    *point = start + pvec3(direction * pos);

    create_bounding_box(collider);

    for(uint32_t entity : collectors[0].entities) {
        if(!mask.contains(entity)) {
            Collider& ec = ecs.get_component<Collider>(entity);
            Transform& et = ecs.get_component<Transform>(entity);
            //if(!ec.is_static) {
                if(collision(et, ec.bounding_box, t, collider.bounding_box)) {
                    auto rp = collision(et, ec, t, collider);

                    if(rp.size()) {
                        *normal = -rp[0].normal;
                    }
                }
            //}
        }
    }

    if(dot(*normal, direction) > 0.0f) *normal = -*normal;

    return collide;
}

void Physics_system::create_bounding_box(Temporary_collider& collider) {
    Bounding_box bb;

    bb.maximum = vec3(-FLT_MAX);
    bb.minimum = vec3(FLT_MAX);
    
    for(vec3 v : collider.vertices) {
        bb.maximum = max(bb.maximum, v + collider.radius);
        bb.minimum = min(bb.minimum, v - collider.radius);
    }

    collider.bounding_box = bb;
}

bool Physics_system::raycast(pvec3 start, vec3 direction, float step, float dist, std::vector<Temporary_collider>& colliders, uint32_t* hit = nullptr, vec3* normal = nullptr, pvec3* point = nullptr) {
    return false;

    for(Temporary_collider& collider : colliders) {
        create_bounding_box(collider);
    }

    vec3 norm = normalize(direction);
    bool decrease = false;

    float pos = 0.0;
    float length = step;
    
    bool collide = false;

    float precision = 0.001f;
    
    Temporary_collider shape;
    while(abs(length) > precision && (pos + length <= dist || collide)) {
        bool collide_this_step = false;

        shape.position = start + pvec3(direction * pos);
        shape.orientation = identity<mat3>();

        shape.vertices = {
            vec3(0.0f),
            direction * length
        };
        shape.radius = 0.0f;

        create_bounding_box(shape);
        Transform st = {shape.position, shape.orientation};

        for(uint32_t i = 0; i < colliders.size(); ++i) {
            Temporary_collider& collider = colliders[i];
            Transform ct = {collider.position, collider.orientation};

            if(collision(st, shape.bounding_box, ct, collider.bounding_box)) {
                bool b = GJK(shape, collider);

                if(b) {
                    *hit = i;
                    
                    collide = true;
                    collide_this_step = true;
                    goto end_loop;
                }

                /*
                if(ec.BVH.size()) {
                    for(Convex_collider& convex : collider.collision_shapes) {
                        std::vector<uint32_t> colliders = ec.traverse_BVH(convex.bounding_box);
                        
                        for(uint32_t c : colliders) {
                            Convex_collider& ce = ec.collision_shapes[c];

                            if(collision(convex.bounding_box, ce.bounding_box)) {
                                bool b = GJK(ce, et, convex, t);

                                if(b) {
                                    *hit = entity;
                                    *shape_hit = c;

                                    collide = true;
                                    collide_this_step = true;
                                    goto end_loop;
                                }
                            }
                        }
                    }
                } */ 
            }
        }

        end_loop:

        if(collide_this_step) decrease = true;

        if(decrease) {
            if(collide_this_step) {
                length *= 0.5f;
            } else {
                pos += length;
                length *= 0.5f;
            }
        } else {
            pos += length;
        }
    }

    *point = start + pvec3(direction * pos);
    return collide;
}

void DOF_constraint::apply(float dt) {
    Collider* ca = &ecs.get_component<Collider>(a);
    Transform* ta = &ecs.get_component<Transform>(a);
    Collider* cb = &ecs.get_component<Collider>(b);
    Transform* tb = &ecs.get_component<Transform>(b);
    
    vec3 axis = tb->orientation * locked_axis;

    vec3 angular_velocity = (ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation)) * ca->angular_momentum;

    float d = dot(angular_velocity, axis);

    angular_velocity -= (1.0f - exp(-factor * dt)) * d * axis;

    ca->angular_momentum = ta->orientation * ca->inertia_tensor * transpose(ta->orientation) * angular_velocity;
}

// convex hull 2d

glm::vec2 segment_project(glm::vec2 a, glm::vec2 b, glm::vec2 c, vec2& p) {
    vec2 line_axis = b - a;
    float dist_c = 1.0f / length(line_axis);
    line_axis *= dist_c;

    vec2 normal = vec2(line_axis.y, -line_axis.x);

    vec2 cc = -a;
    cc = cc - normal * dot(normal, cc);
    cc += a;

    vec2 da = a - cc;
    vec2 db = b - cc;

    float dist_a = length(da);
    float dist_b = length(db);
    
    dist_a *= dist_c;
    dist_b *= dist_c;

    if(dist_a > dist_b && dist_a > 1.0f) {
        dist_a = 1.0f;
        dist_b = 0.0f;
    } else if(dist_b > dist_a && dist_b > 1.0f) {
        dist_a = 0.0f;
        dist_b = 1.0f;
    }

    p = cc;

    return {dist_b, dist_a};
}

void get_normal(vec2 a, vec2 b, vec2 r, vec2& normal, vec2& center) {
    vec2 v = a - b;

    normal = normalize(vec2(v.y, -v.x));
    if(dot(normal, r - a) > 0) normal = -normal;
    center = (a + b) * 0.5f;
}

struct Polygon_return {
    std::vector<vec2> vertices;
    vec2 normal;
    vec2 weights = vec2(-1.0f);
};

struct Polygon_edge {
    std::vector<uint32_t> vertices;
    vec2 normal;
    float distance;
};

struct Polygon {
    std::vector<vec2> vertices;
    std::vector<Polygon_edge> edges;
    vec2 sum = vec2(0.0f);
    
    Polygon_return find_closest_face() {
        Polygon_return ret;

        float min_dist = FLT_MAX;
        int edge_i = -1;
        for(int i = 0; i < edges.size(); ++i) {
            Polygon_edge& edge = edges[i];

            if(edge.distance < min_dist) {
                min_dist = edge.distance;
                edge_i = i;
            }
        }

        if(edge_i != -1) {
            Polygon_edge& edge = edges[edge_i];
            uint32_t a = edge.vertices[0];
            uint32_t b = edge.vertices[1];

            vec2 va = vertices[a];
            vec2 vb = vertices[b];

            vec2 center;

            vec2 w = segment_project(va, vb, vec2(0.0f), center);

            if(w.x != -1) {
                ret.vertices = {va, vb};
                vec2 c;
                get_normal(va, vb, sum / float(vertices.size()), ret.normal, c);
                //ret.normal = edge.normal;
                ret.weights = w;
            }
        }

        return ret;
    }

    void insert_edge(uint32_t a, uint32_t b, std::size_t insert) {
        vec2 pa = vertices[a];
        vec2 pb = vertices[b];

        vec2 normal;
        vec2 center;

        Polygon_edge e;
        e.vertices = {a, b};
        e.normal = normalize(pa - pb);
        e.normal = {e.normal.y, -e.normal.x};

        if(dot(pa - (sum / float(vertices.size())), e.normal) < 0.0f) {
            e.normal = -e.normal;
            e.vertices = {b, a};
        }

        e.distance = abs(dot(normal, -center));

        edges.insert(edges.begin() + insert, e);
    }

    void expand(vec2 vertex) {
        uint32_t v_n = vertices.size();
        vertices.push_back(vertex);

        vec2 prev_sum = sum;
        sum += vertex;
        
        std::vector<uint32_t> edges_seen;
        std::vector<uint32_t> vertices_seen;
        for(int i = 0; i < edges.size(); ++i) {
            Polygon_edge& e = edges[i];

            vec2 diff = vertex - vertices[e.vertices[0]];

            if(dot(e.normal, diff) > 0.0f) {
                edges_seen.push_back(i);
            }
        }

        if(edges_seen.size()) {
            std::vector<uint32_t> prev_es = edges_seen;

            uint32_t start = edges_seen[0];
            
            if(edges_seen[0] == 0 && edges_seen.back() == edges.size() - 1) {
                uint32_t i = 0;
                while(true) {
                    if(edges_seen[i] != i) break;
                    ++i;
                }
                edges_seen.insert(edges_seen.end(), edges_seen.begin(), edges_seen.begin() + i);
                edges_seen.erase(edges_seen.begin(), edges_seen.begin() + i);
            }

            for(uint32_t i : edges_seen) {
                Polygon_edge& e = edges[i];
                vertices_seen.push_back(e.vertices[0]);
                vertices_seen.push_back(e.vertices[1]);
            }
            
            int i = 0;
            for(uint32_t edge : prev_es) {
                edges.erase(edges.begin() + edge - i);
                ++i;
            }

            int in = 0;
            for(uint32_t vertex : vertices_seen) {
                if(std::count(vertices_seen.begin(), vertices_seen.end(), vertex) == 1) {
                    uint32_t a = vertex;
                    
                    insert_edge(a, v_n, start + in);
                    ++in;
                }
            }
        }
    }
};

Polygon from_simplex(std::vector<vec2> vertices) {
    Polygon p;
    p.vertices = vertices;

    for(int i = 0; i < 3; ++i) {
        p.sum += p.vertices[i];
    }

    std::vector<uint32_t> ids = {0, 1, 2};

    vec2 center = p.sum / 3.0f;
    if(cross(vec3(p.vertices[0] - p.vertices[2], 0.0f), vec3(p.vertices[1] - p.vertices[2], 0.0f)).z > 0.0f) {
        ids = {2, 1, 0};
    }
    
    for(int i = 0; i < 3; ++i) {
        uint32_t a = i;
        uint32_t b = (i + 1) % 3;

        p.insert_edge(ids[a], ids[b], p.edges.size());
    }

    return p;
}

vec2 support_func(std::vector<vec2> points, vec2 direction) {
    vec2 ret;
    float min_dist = -FLT_MAX;
    for(vec2 v : points) {
        float dist = dot(v, direction);

        if(dist > min_dist) {
            min_dist = dist;
            ret = v;
        }
    }
    

    return ret;
}

/*

    std::unordered_set<uint32_t> set;
    for(uint32_t i = 1; i < vertices.size(); ++i) {
        set.emplace(i);
    }

    // 1
    simplex.vertices.push_back(Simplex_vertex{vertices[0], vec3(0)});
    vec3 direction = -glm::normalize(simplex.vertices[0].m);

    // 2
    uint32_t next = support_func(vertices, set, direction);
    simplex.vertices.push_back(Simplex_vertex{vertices[next], vec3(next)});
    
    glm::vec3 line_direction = normalize(simplex.vertices[0].m - simplex.vertices[1].m);
    glm::vec3 rel_origin_pos = -simplex.vertices[1].m;
    glm::vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
    direction = glm::normalize(-closest_point);
    set.erase(next);

    // 3
    next = support_func(vertices, set, direction);
    simplex.vertices.push_back(Simplex_vertex{vertices[next], vec3(next)});

    glm::vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
    glm::vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
    if(glm::dot(normal, -center) <= 0.0f) normal = -normal;
    direction = normal;
    set.erase(next);

    // 4
    next = support_func(vertices, set, direction);
    simplex.vertices.push_back(Simplex_vertex{vertices[next], vec3(next)});
    set.erase(next);
    */

std::vector<uint32_t> convex_hull(std::vector<vec2> points) {
    std::vector<uint32_t> ret;
    vec2 center = vec2(0.0f);
    for(vec2 v : points) center += v;
    center /= points.size();

    for(vec2& v : points) v -= center;

    if(points.size() > 3) {
        std::vector<uint32_t> ids;

        std::unordered_set<uint32_t> set;
        for(uint32_t i = 1; i < points.size(); ++i) {
            set.emplace(i);
        }

        ids = {0};
        vec2 direction = -normalize(points[0]);

        uint32_t next = support_func(points, set, direction);
        ids.push_back(next);
        set.erase(next);

        vec2 line_dir = normalize(points[ids[0]] - points[ids[1]]);
        vec2 rel_origin = -points[ids[1]];
        vec2 closest_point = line_dir * dot(line_dir, rel_origin) + points[ids[1]];
        direction = -normalize(closest_point);
        
        next = support_func(points, set, direction);
        ids.push_back(next);
        set.erase(next);

        Polygon p = from_simplex({points[ids[0]], points[ids[1]], points[ids[2]]});

        for(uint32_t s : set) {
            ids.push_back(s);
            vec2 point = points[s];

            p.expand(point);
        }

        std::vector<vec2> vs;
        for(Polygon_edge& pe : p.edges) {
            ret.push_back(ids[pe.vertices[0]]);
        }
    } else {
        for(int i = 0; i < points.size(); ++i) {
            ret.push_back(i);
        }
    }

    return ret;
}

std::vector<vec2> convex_hull2(std::vector<vec2> points) {
    std::vector<vec2> ret;
    vec2 center = vec2(0.0f);

    if(points.size() > 3) {
        std::vector<uint32_t> ids;

        std::unordered_set<uint32_t> set;
        for(uint32_t i = 1; i < points.size(); ++i) {
            set.emplace(i);
        }

        ids = {0};
        vec2 direction = -normalize(points[0]);

        uint32_t next = support_func(points, set, direction);
        ids.push_back(next);
        set.erase(next);

        vec2 line_dir = normalize(points[ids[0]] - points[ids[1]]);
        vec2 rel_origin = -points[ids[1]];
        vec2 closest_point = line_dir * dot(line_dir, rel_origin) + points[ids[1]];
        direction = -normalize(closest_point);
        
        next = support_func(points, set, direction);
        ids.push_back(next);
        set.erase(next);

        Polygon p = from_simplex({points[ids[0]], points[ids[1]], points[ids[2]]});

        for(uint32_t s : set) {
            ids.push_back(s);
            vec2 point = points[s];

            p.expand(point);
        }

        std::vector<vec2> vs;
        for(Polygon_edge& pe : p.edges) {
            ret.push_back(p.vertices[pe.vertices[0]]);
            ret.push_back(p.vertices[pe.vertices[1]]);
        }
    } else {

    }

    return ret;
}


