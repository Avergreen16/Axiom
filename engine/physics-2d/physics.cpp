#include <physics-2d/physics.hpp>
#include <include/ecs.hpp>
#include <include/math.hpp>

#include <physics-2d/collider.hpp>

namespace axiom {

transform2d null_transform = {vec2(0.0f), glm::identity<glm::mat2>()};

physics_system2d::physics_system2d() {
    axiom::signature s = axiom::global_core.ecs->update_signature<collider2d>();
    axiom::global_core.ecs->update_signature<transform2d>(s);
    collectors.push_back(collector{s, false});
}

vec2 physics_system2d::transform_vertices(transform2d& t, collision_shape2d& c, std::vector<vertex_element2d>& vertices, vec2 origin) {
    vec2 center = vec2(0.0f);

    vec2 o = t.position - origin;

    for(vertex_element2d vertex : c.vertices) {
        vertex.center = t.orientation * (c.orientation * vertex.center + c.position) + o;
        vertex.orientation = t.orientation * vertex.orientation;

        vertices.push_back(vertex);

        center += vertex.center;
    }

    return center / float(c.vertices.size());
}

vec2 physics_syst(std::vector<vec2>& vertices, vec2 radius, vec2 direction) {
    float max_dot = -FLT_MAX;
    vec2 return_vertex;

    for(vec2 v : vertices) {
        float dot_v = glm::dot(direction, v);

        if(dot_v > max_dot) {
            max_dot = dot_v;
            return_vertex = v;
        }
    }

    float s = sqrt(radius.x * radius.x * direction.x * direction.x + radius.y * radius.y * direction.y * direction.y);
    if(s > 0.0f) {
        vec2 ellipsoid = vec2(radius.x * radius.x * direction.x, radius.y * radius.y * direction.y) / s;
        return_vertex += ellipsoid;
    }

    return return_vertex;
}

vec2 physics_syst(std::vector<vec2>& vertices, vec2 radius, vec2 direction, glm::mat2 matrix) {
    float max_dot = -FLT_MAX;
    vec2 return_vertex;

    for(vec2 v : vertices) {
        float dot_v = glm::dot(direction, v);

        if(dot_v > max_dot) {
            max_dot = dot_v;
            return_vertex = v;
        }
    }

    //direction = transpose(matrix) * direction;

    /*
    float s = sqrt(radius.x * radius.x * direction.x * direction.x + radius.y * radius.y * direction.y * direction.y);
    if(s > 0.0f) {
        vec2 ellipsoid = vec2(radius.x * radius.x * direction.x, radius.y * radius.y * direction.y) / s;
        ellipsoid = matrix * ellipsoid;

        return_vertex += ellipsoid;
    }
    */
    return_vertex += direction * radius.x;

    return return_vertex;
}

struct simplex_vertex {
    vec2 m;
    vec2 a;
    vec2 b;
};

glm::vec2 segment_project(glm::vec2 a, glm::vec2 b, glm::vec2 c, vec2& p) {
    vec2 line_axis = b - a;
    float dist_c = 1.0f / length(line_axis);
    line_axis *= dist_c;

    vec2 normal = vec2(line_axis.y, -line_axis.x);

    vec2 cc = -a;
    cc = cc - normal * glm::dot(normal, cc);
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

struct simplex {
    std::vector<simplex_vertex> vertices;

    uint32_t find_closest_face(glm::vec2& weights, glm::vec2& dir) {
        float dist = FLT_MAX;
        uint32_t f = -1;
        weights = vec2(-1);

        for(int i = 0; i < 2; ++i) {
            vec2 p;

            vec2 w = segment_project(vertices[(i <= 0) ? 1 : 0].m, vertices[(i <= 1) ? 2 : 1].m, vec2(0.0f), p);

            if(w.x != -1) {
                float length_p = glm::length(p);

                if(length_p < dist) {
                    dir = vertices[(i <= 0) ? 1 : 0].m - vertices[(i <= 1) ? 2 : 1].m;
                    dir = normalize(vec2(dir.y, -dir.x));

                    dist = length_p;
                    f = i;
                    weights = w;
                }
            }
        }

        return f;
    }
};

int simplex_contains(glm::vec2 p, std::vector<simplex_vertex>& points) {
    vec2 centroid;
    vec2 normal;

    get_normal(points[1].m, points[2].m, points[0].m, normal, centroid);
    float d0 = glm::dot(p - centroid, normal);

    get_normal(points[0].m, points[2].m, points[1].m, normal, centroid);
    float d1 = glm::dot(p - centroid, normal);

    //get_normal(points[0].m, points[1].m, points[2].m, normal, centroid);
    //float d2 = glm::dot(p - centroid, normal);

    if(d0 > 0 && d0 > d1) return 0;
    if(d1 > 0 && d1 > d0) return 1;
    //if(d2 > 0 && d2 > glm::max(d0, d1)) return 2;

    return -1;
}

struct polygon_return {
    std::vector<simplex_vertex> vertices;
    vec2 normal;
    vec2 weights = vec2(-1.0f);
};

struct polygon_edge {
    std::vector<uint32_t> vertices;
    vec2 normal;
    float distance;
};

struct polygon {
    std::vector<simplex_vertex> vertices;
    std::vector<polygon_edge> edges;
    vec2 sum = vec2(0.0f);

    polygon_return find_closest_face() {
        polygon_return ret;

        float min_dist = FLT_MAX;
        int edge_i = -1;
        for(int i = 0; i < edges.size(); ++i) {
            polygon_edge& edge = edges[i];

            if(edge.distance < min_dist) {
                min_dist = edge.distance;
                edge_i = i;
            }
        }

        if(edge_i != -1) {
            polygon_edge& edge = edges[edge_i];
            uint32_t a = edge.vertices[0];
            uint32_t b = edge.vertices[1];

            simplex_vertex va = vertices[a];
            simplex_vertex vb = vertices[b];

            vec2 center;

            vec2 w = segment_project(va.m, vb.m, vec2(0.0f), center);

            if(w.x != -1) {
                ret.vertices = {va, vb};
                vec2 c;
                get_normal(va.m, vb.m, sum / float(vertices.size()), ret.normal, c);
                //ret.normal = edge.normal;
                ret.weights = w;
            }
        }

        return ret;
    }

    void insert_edge(uint32_t a, uint32_t b) {
        vec2 pa = vertices[a].m;
        vec2 pb = vertices[b].m;

        vec2 normal;
        vec2 center;
        get_normal(pa, pb, sum / float(vertices.size()), normal, center);

        polygon_edge e;
        e.vertices = {a, b};
        e.normal = normal;
        e.distance = abs(dot(normal, -center));

        edges.push_back(e);
    }

    void expand(simplex_vertex vertex) {
        uint32_t v_n = vertices.size();
        vertices.push_back(vertex);

        vec2 prev_sum = sum;
        sum += vertex.m;

        std::vector<uint32_t> edges_seen;
        std::vector<uint32_t> vertices_seen;
        for(int i = 0; i < edges.size(); ++i) {
            polygon_edge& e = edges[i];

            vec2 diff = vertex.m - vertices[e.vertices[0]].m;

            if(dot(e.normal, diff) > 0.0f) {
                edges_seen.push_back(i);
                vertices_seen.push_back(e.vertices[0]);
                vertices_seen.push_back(e.vertices[1]);
            }
        }

        std::sort(edges_seen.begin(), edges_seen.end());

        int i = 0;
        for(uint32_t edge : edges_seen) {
            edges.erase(edges.begin() + edge - i);
            ++i;
        }

        for(uint32_t vertex : vertices_seen) {
            if(std::count(vertices_seen.begin(), vertices_seen.end(), vertex) == 1) {
                uint32_t a = vertex;

                insert_edge(a, v_n);
            }
        }
    }
};

polygon from_simplex(simplex& s) {
    polygon p;
    p.vertices = s.vertices;

    for(int i = 0; i < 3; ++i) {
        p.sum += p.vertices[i].m;
    }

    for(int i = 0; i < 3; ++i) {
        uint32_t a = i;
        uint32_t b = (i + 1) % 3;

        p.insert_edge(a, b);
    }

    return p;
}

std::vector<collision_data2d> physics_system2d::collision(collision_input& input) {
    std::vector<collision_data2d> ret;

    return_tag2d tag;

    if(input.ca->BVH.size()) {
        if(input.cb->BVH.size()) {
            std::vector<uint64_t> pairs = traverse_BVH(*input.ta, input.ca->BVH, *input.tb, input.cb->BVH);

            for(uint64_t pair : pairs) {
                uint32_t a = pair & 0xFFFFFFFF;
                uint32_t b = pair >> 32;

                collision_shape2d& sa = input.ca->shapes[a];
                collision_shape2d& sb = input.cb->shapes[b];

                std::vector<collision_data2d> r = collision(*input.ta, sa, *input.tb, sb, tag);
                ret.insert(ret.end(), r.begin(), r.end());
            }
        } else {
            std::vector<return_tag2d> tags;
            std::vector<std::vector<collision_data2d>> data;

            for(collision_shape2d& sb : input.cb->shapes) {
                std::vector<uint32_t> shapes = traverse_BVH(*input.ta, input.ca->BVH, *input.tb, sb.bounding_box);

                for(uint32_t shape : shapes) {
                    collision_shape2d& sa = input.ca->shapes[shape];

                    std::vector<collision_data2d> r = collision(*input.ta, sa, *input.tb, sb, tag);

                    tags.push_back(tag);
                    data.push_back(r);
                }
            }

            // edges

            std::unordered_set<ivec2, hash_coord> set;

            for(uint32_t i = 0; i < tags.size(); ++i) {
                return_tag2d& tag = tags[i];
                if(tag.type == axiom::collision_type2d::EDGE) {
                    ret.insert(ret.end(), data[i].begin(), data[i].end());

                    set.insert(tag.va[0]);
                    set.insert(tag.va[1]);
                }
            }

            // vertices

            for(uint32_t i = 0; i < tags.size(); ++i) {
                return_tag2d& tag = tags[i];
                if(tag.type == axiom::collision_type2d::VERTEX) {
                    if(set.find(tag.va[0]) == set.end()) {
                        ret.insert(ret.end(), data[i].begin(), data[i].end());

                        set.insert(tag.va[0]);
                    }
                }
            }
        }
    } else if(input.cb->BVH.size()) {
        std::vector<return_tag2d> tags;
        std::vector<std::vector<collision_data2d>> data;

        for(collision_shape2d& sa : input.ca->shapes) {
            std::vector<uint32_t> shapes = traverse_BVH(*input.tb, input.cb->BVH, *input.ta, sa.bounding_box);

            for(uint32_t shape : shapes) {
                collision_shape2d& sb = input.cb->shapes[shape];

                std::vector<collision_data2d> r = collision(*input.ta, sa, *input.tb, sb, tag);

                tags.push_back(tag);
                data.push_back(r);
                //ret.insert(ret.end(), r.begin(), r.end());
            }
        }

        // edges

        std::unordered_set<ivec2, hash_coord> set;

        for(uint32_t i = 0; i < tags.size(); ++i) {
            return_tag2d& tag = tags[i];
            if(tag.type == axiom::collision_type2d::VERTEX) {
                ret.insert(ret.end(), data[i].begin(), data[i].end());

                set.insert(tag.vb[0]);
                set.insert(tag.vb[1]);
            }
        }

        // vertices

        for(uint32_t i = 0; i < tags.size(); ++i) {
            return_tag2d& tag = tags[i];
            if(tag.type == axiom::collision_type2d::EDGE) {
                if(set.find(tag.vb[0]) == set.end()) {
                    ret.insert(ret.end(), data[i].begin(), data[i].end());

                    set.insert(tag.vb[0]);
                }
            }
        }
    } else {
        for(collision_shape2d& sa : input.ca->shapes) {
            for(collision_shape2d& sb : input.cb->shapes) {
                std::vector<collision_data2d> r = collision(*input.ta, sa, *input.tb, sb, tag);

                ret.insert(ret.end(), r.begin(), r.end());
            }
        }
    }

    return ret;
}

std::vector<collision_data2d> physics_system2d::collision(transform2d& ta, collision_shape2d& ca, transform2d& tb, collision_shape2d& cb, return_tag2d& tag) {
    std::vector<collision_data2d> data;

    std::vector<vertex_element2d> a_vertices;
    std::vector<vertex_element2d> b_vertices;

    float limit = 0.00001;

    vec2 center_a = transform_vertices(ta, ca, a_vertices, ta.position);
    vec2 center_b = transform_vertices(tb, cb, b_vertices, ta.position);

    simplex simplex;

    vec2 direction = glm::normalize(center_b - center_a);
    vec2 dd = direction;

    vec2 offset = vec2(direction.y, -direction.x);

    if(glm::dot(offset, direction) > 0.99) {
        offset = vec2(direction.x, -direction.y);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    while(loop) {
        ++iterations;
        if(iterations > 100) return {};

        if(glm::isnan(direction.x)) direction = vec2(0, 1);

        int size = simplex.vertices.size();

        if(size < 3) {
            vec2 point_a = support(direction, a_vertices);
            vec2 point_b = support(-direction, b_vertices);

            vec2 point_m = point_a - point_b;

            for(simplex_vertex& v : simplex.vertices) {
                vec2 difference = point_m - v.m;

                if(glm::dot(difference, direction) < limit) return {};
            }

            if(glm::dot(point_m, direction) < limit) return {};

            simplex.vertices.push_back(simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);
            } else if(size == 1) {
                vec2 line_direction = glm::normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                vec2 rel_origin_pos = -simplex.vertices[1].m;

                vec2 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            }
        } else {
            int n = simplex_contains(vec2(0, 0), simplex.vertices);
            if(n == -1) {
                polygon p = from_simplex(simplex);

                iterations = 0;

                while(true) {
                    ++iterations;
                    if(iterations > 100) {
                        std::cout << "ERROR: ITERATION\n";
                        return {};
                    }
                    polygon_return r = p.find_closest_face();

                    if(r.vertices.size() == 0) {
                        std::cout << "ERROR: ZERO\n";
                        std::cout << p.edges.size() << "\n";
                        return {};
                    }

                    direction = r.normal;

                    vec2 point_a = support(direction, a_vertices);
                    vec2 point_b = support(-direction, b_vertices);

                    vec2 point_m = point_a - point_b;

                    float dist = glm::dot(point_m, r.normal);

                    float limit_2 = 0.0001;

                    if(abs(dist - glm::dot(r.vertices[0].m, r.normal)) < limit_2) {
                        vec2 cp_a = r.vertices[0].a * r.weights.x + r.vertices[1].a * r.weights.y;
                        vec2 cp_b = r.vertices[0].b * r.weights.x + r.vertices[1].b * r.weights.y;

                        vec2 separation_vector = cp_b - cp_a;

                        vec2 collision_normal;

                        vec2 main_dir = center_a - center_b;

                        if(r.vertices[0].a != r.vertices[1].a) {
                            vec2 sv = r.vertices[0].a - r.vertices[1].a;
                            collision_normal = normalize(vec2(sv.y, -sv.x));

                            if(dot(collision_normal, main_dir) < 0.0f) collision_normal = -collision_normal;

                            tag.type = axiom::collision_type2d::EDGE;
                            tag.va[0] = ivec2(r.vertices[0].a * 8.0f);
                            tag.va[1] = ivec2(r.vertices[1].a * 8.0f);
                            tag.vb[0] = ivec2(r.vertices[0].b * 8.0f);
                        } else {
                            vec2 sv = r.vertices[0].b - r.vertices[1].b;
                            collision_normal = normalize(vec2(sv.y, -sv.x));

                            if(dot(collision_normal, main_dir) < 0.0f) collision_normal = -collision_normal;

                            tag.type = axiom::collision_type2d::VERTEX;
                            tag.va[0] = ivec2(r.vertices[0].a * 8.0f);
                            tag.vb[0] = ivec2(r.vertices[0].b * 8.0f);
                            tag.vb[1] = ivec2(r.vertices[1].b * 8.0f);
                        }

                        if(glm::isnan(collision_normal.x)) {
                            std::cout << "ERROR: ISNAN\n";
                            return {};
                        }

                        // clipping

                        vec2 pa = support(-collision_normal, a_vertices);
                        float da0 = FLT_MAX;
                        float da1 = -FLT_MAX;

                        vec2 pb = support(collision_normal, b_vertices);
                        float db0 = FLT_MAX;
                        float db1 = -FLT_MAX;

                        vec2 sideways = {collision_normal.y, -collision_normal.x};

                        float margin = 0.01f;

                        for(int i = 0; i < a_vertices.size(); ++i) {
                            vertex_element2d& v = a_vertices[i];
                            float d = glm::dot(v.center, sideways);

                            bool c = glm::dot(v.center - pa, -collision_normal) > -margin;

                            if(c) {
                                if(d < da0) da0 = d;
                                if(d > da1) da1 = d;
                            }
                        }

                        for(int i = 0; i < b_vertices.size(); ++i) {
                            vertex_element2d& v = b_vertices[i];
                            float d = glm::dot(v.center, sideways);

                            bool c = glm::dot(v.center - pb, collision_normal) > -margin;

                            if(c) {
                                if(d < db0) db0 = d;
                                if(d > db1) db1 = d;
                            }
                        }

                        float c0 = glm::max(da0, db0);
                        float c1 = glm::min(da1, db1);

                        vec2 a2 = pa + sideways * (c0 - glm::dot(pa, sideways));
                        vec2 a3 = pa + sideways * (c1 - glm::dot(pa, sideways));

                        vec2 b2 = pb + sideways * (c0 - glm::dot(pb, sideways));
                        vec2 b3 = pb + sideways * (c1 - glm::dot(pb, sideways));

                        if(da0 <= db1 && db0 <= da1) {
                            collision_data2d collision_data2d;
                            collision_data2d.collide = true;
                            collision_data2d.a = 0;
                            collision_data2d.b = 0;
                            collision_data2d.pa = a2;
                            collision_data2d.pb = b2;
                            collision_data2d.normal = collision_normal;

                            data.push_back(collision_data2d);

                            collision_data2d.pa = a3;
                            collision_data2d.pb = b3;

                            data.push_back(collision_data2d);
                        } else {
                            collision_data2d collision_data2d;
                            collision_data2d.collide = true;
                            collision_data2d.a = 0;
                            collision_data2d.b = 0;
                            collision_data2d.pa = cp_a;
                            collision_data2d.pb = cp_b;
                            collision_data2d.normal = collision_normal;

                            data.push_back(collision_data2d);
                        }

                        return data;
                    } else {
                        p.expand({point_m, point_a, point_b});
                    }
                }

                return {};
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                vec2 line_direction = glm::normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                vec2 rel_origin_pos = -simplex.vertices[1].m;

                vec2 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            }
        }
    }
}

bool physics_system2d::collision_point(std::vector<vertex_element2d> vertices, vec2 point) {
    std::vector<vertex_element2d> a_vertices;

    float limit = 0.00001;

    for(vertex_element2d v : vertices) {

        v.center -= point;
        a_vertices.push_back(v);
    }

    simplex simplex;

    vec2 direction = glm::normalize(a_vertices[0].center);

    vec2 offset = vec2(direction.y, -direction.x);

    if(glm::dot(offset, direction) > 0.99) {
        offset = vec2(direction.x, -direction.y);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    while(loop) {
        ++iterations;
        if(iterations > 100) return {};

        int size = simplex.vertices.size();
        if(size < 3) {
            vec2 point_a = support(direction, a_vertices);

            for(simplex_vertex& v : simplex.vertices) {
                vec2 difference = point_a - v.m;

                if(glm::length(difference) < limit) return {};
            }

            if(glm::dot(point_a, direction) < limit * 2) return {};

            simplex.vertices.push_back(simplex_vertex{point_a, vec2(0.0f), vec2(0.0f)});

            if(size == 0) {
                float dir_len = glm::length(point_a);
                if(dir_len != 0.0f) direction = -point_a / dir_len;
                else direction = vec2(direction.y, -direction.x);
            } else if(size == 1) {
                vec2 line_direction = glm::normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                vec2 rel_origin_pos = -simplex.vertices[1].m;

                vec2 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                float dir_len = glm::length(closest_point);
                if(dir_len != 0.0f) direction = -closest_point / dir_len;
                else direction = vec2(direction.y, -direction.x);
            }
        } else {
            int n = simplex_contains(vec2(0, 0), simplex.vertices);
            if(n == -1) return true;
            else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                vec2 line_direction = glm::normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                vec2 rel_origin_pos = -simplex.vertices[1].m;

                vec2 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                //direction = glm::normalize(-closest_point);
                float dir_len = glm::length(closest_point);
                if(dir_len != 0.0f) direction = -closest_point / dir_len;
                else direction = vec2(direction.y, -direction.x);
            }
        }
    }
}

void physics_system2d::insert_collision(collision_data2d c) {
    uint64_t a = uint64_t(c.a) | (uint64_t(c.b) << 32);

    if(collision_table.find(a) == collision_table.end()) {
        collision_table.emplace(a, std::vector<collision_data2d>());
    }

    std::vector<collision_data2d>& v = collision_table[a];

    for(int i = v.size() - 1; i >= 0; --i) {
        collision_data2d& d = v[i];
        vec2 diff_a = d.pa - c.pa;
        vec2 diff_b = d.pb - c.pb;

        if(length(diff_a) < contact_sep && length(diff_b) < contact_sep) return;
    }

    v.push_back(c);
}

bool compare_x(sap_point& a, sap_point& b) {
    float aa = (a.is_start) ? a.start.x : a.end.x;
    float bb = (b.is_start) ? b.start.x : b.end.x;
    return aa < bb;
}

template<typename Iter, typename Compare>
void insertion_sort(Iter begin, Iter end, Compare comp) {
    for(Iter i = begin + 1; i < end; ++i) {
        auto k = *i;
        Iter j = i;
        while(j > begin && comp(k, *(j - 1))) {
            *j = *(j - 1);
            --j;
        }
        *j = k;
    }
}

bounding_box2d physics_system2d::transform(transform2d& t, bounding_box2d& b) {
    bounding_box2d ret;

    vec2 center = (b.minimum + b.maximum) * 0.5f;
    vec2 r = b.maximum - center;

    center = t.orientation * center + t.position;

    glm::mat2 m = {abs(t.orientation[0]), abs(t.orientation[1])};

    r = m * r;

    ret.minimum = center - r;
    ret.maximum = center + r;

    return ret;
}

bool physics_system2d::collision(transform2d& ta, bounding_box2d& a, transform2d& tb, bounding_box2d& b) {
    transform2d t;
    t.position = transpose(ta.orientation) * (tb.position - ta.position);
    t.orientation = transpose(ta.orientation) * tb.orientation;

    bounding_box2d nb = transform(t, b);

    return collision(a, nb);
}

bool physics_system2d::collision(bounding_box2d& a, bounding_box2d& b) {
    bool bx = a.minimum.x < b.maximum.x && b.minimum.x < a.maximum.x;
    bool by = a.minimum.y < b.maximum.y && b.minimum.y < a.maximum.y;

    return bx && by;
}

struct spacial_data {
    uint32_t i;
    bounding_box2d bb;
};

std::vector<uint64_t> physics_system2d::broad_phase(std::vector<input_data>& input) {
    const std::size_t max_i = 4;
    const float ratio = 4.0f;
    const float start_size = 4.0f;

    std::unordered_set<uint64_t> set;

    std::array<std::unordered_map<ivec2, std::vector<spacial_data>, hash_coord>, max_i> spacial;

    auto insert_into = [&](input_data& ii) {
        bounding_box2d bb = transform(*ii.transform, ii.bounding_box);

        vec2 size = bb.maximum - bb.minimum;

        float max_size = glm::max(size.x, size.y);

        std::unordered_map<ivec2, std::vector<spacial_data>, hash_coord>* spacial_scale;

        float bucket_size = start_size;
        for(int i = 0; i < max_i; ++i) {
            if(max_size <= bucket_size || i == max_i - 1) {
                spacial_scale = &spacial[i];
                break;
            }
            bucket_size *= ratio;
        }

        vec2 min = bb.minimum / bucket_size;
        vec2 max = bb.maximum / bucket_size;

        ivec2 mmin = ivec2(floor(min));
        ivec2 mmax = ivec2(floor(max));

        spacial_data sd;
        sd.i = ii.id;
        sd.bb = bb;

        for(int y = mmin.y; y <= mmax.y; ++y) {
            for(int x = mmin.x; x <= mmax.x; ++x) {
                ivec2 i = {x, y};
                auto& bucket = spacial_scale->operator[](i);

                bucket.push_back(sd);
            }
        }
    };

    for(auto& ii : input) {
        insert_into(ii);
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

                    if(collision(vi.bb, vj.bb)) {
                        uint64_t kk;
                        if(vi.i < vj.i) kk = (uint64_t)vi.i | (uint64_t(vj.i) << 32);
                        else kk = (uint64_t)vj.i | (uint64_t(vi.i) << 32);

                        set.emplace(kk);
                    }
                }

                // loop through all shapes in levels above

                //if(!vi.bb->flag) {
                vec2 key2 = key;
                for(int m = k + 1; m < max_i; ++m) {
                    key2 /= ratio;
                    ivec2 k2 = floor(key2);
                    if(spacial[m].find(k2) != spacial[m].end()) {
                        auto& v2 = spacial[m].at(k2);

                        for(int j = 0; j < v2.size(); ++j) {
                            auto vj = v2[j];

                            if(collision(vi.bb, vj.bb)) {
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

std::vector<uint32_t> physics_system2d::traverse_BVH(transform2d& ta, std::vector<BVH_node2d>& ca, transform2d& tb, bounding_box2d& bb) {
    std::vector<uint32_t> front_buffer = {0};
    std::vector<uint32_t> back_buffer;
    std::vector<uint32_t> shapes;

    while(true) {
        if(front_buffer.size() == 0) break;

        for(uint32_t i : front_buffer) {
            BVH_node2d& node = ca[i];

            if(physics_system2d::collision(ta, node.bounding_box, tb, bb)) {
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

uint32_t depth = 0;

std::vector<uint64_t> physics_system2d::traverse_BVH(transform2d& ta, std::vector<BVH_node2d>& ca, transform2d& tb, std::vector<BVH_node2d>& cb) {
    std::vector<uint64_t> front_buffer = {0};
    std::vector<uint64_t> back_buffer;
    std::vector<uint64_t> shape_pairs;

    while(true) {
        if(front_buffer.size() == 0) break;

        for(uint64_t i : front_buffer) {
            uint32_t ai = i & 0xFFFFFFFF;
            uint32_t bi = i >> 32;

            BVH_node2d& node_a = ca[ai];
            BVH_node2d& node_b = cb[bi];

            if(physics_system2d::collision(ta, node_a.bounding_box, tb, node_b.bounding_box)) {
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

void physics_system2d::physics_loop() {
    PROFILE_SCOPE("physics");
    
    std::vector<input_data> input;

    for(uint32_t a : collectors[0].entities) {
        collider2d& ac = axiom::global_core.ecs->get_component<collider2d>(a);
        transform2d& at = axiom::global_core.ecs->get_component<transform2d>(a);

        ac.colliding_with.clear();
        ac.colliding_normal.clear();

        if(ac.bounding_box.minimum.x == FLT_MAX) ac.create_bounding_box();

        //mesh& m = axiom::global_core.ecs->get_component<mesh>(a);

        //

        input_data ii;
        ii.bounding_box = ac.bounding_box;
        ii.transform = &at;
        ii.id = a;

        input.push_back(ii);
    }

    std::vector<uint64_t> collisions = broad_phase(input);

    const uint32_t num_threads = 12;
    std::vector<std::thread> threads(num_threads);
    std::vector<std::vector<collision_data2d>> cdata(num_threads);
    std::vector<std::vector<uint64_t>> threads_collisions(num_threads);

    uint32_t num_collisions = 0;
    float num_per_thread = float(collisions.size()) / num_threads;
    for(uint64_t i : collisions) {
        float f = float(num_collisions) / num_per_thread;
        uint32_t fi = glm::min(uint32_t(f), num_threads - 1);

        threads_collisions[fi].push_back(i);

        ++num_collisions;
    }

    int N = 8;

    bool bb = false;

    auto thread_GJK = [&](uint32_t t) {

        std::vector<uint64_t> cache;
        uint32_t ii = 0;
        for(uint64_t i : threads_collisions[t]) {
            ++ii;
            cache.push_back(i);

            if(cache.size() == N || ii >= threads_collisions[t].size() - 1) {
                std::vector<collision_input> inputs;
                for(int j = 0; j < N; ++j) {
                    if(j < cache.size()) {
                        uint32_t a = cache[j] & NULL_ENTITY;
                        uint32_t b = cache[j] >> 32;

                        collider2d& ca = axiom::global_core.ecs->get_component<collider2d>(a);
                        transform2d& ta = axiom::global_core.ecs->get_component<transform2d>(a);

                        collider2d& cb = axiom::global_core.ecs->get_component<collider2d>(b);
                        transform2d& tb = axiom::global_core.ecs->get_component<transform2d>(b);

                        if(ca.non_colliding.find(b) != ca.non_colliding.end() || cb.non_colliding.find(a) != cb.non_colliding.end() ) continue;

                        collision_input ci;
                        ci.a = a;
                        ci.b = b;
                        ci.ca = &ca;
                        ci.ta = &ta;
                        ci.cb = &cb;
                        ci.tb = &tb;

                        inputs.push_back(ci);
                    }
                }

                for(int j = 0; j < inputs.size(); ++j) {
                    std::vector<collision_data2d> cv = collision(inputs[j]);

                    if(cv.size()) {
                        for(collision_data2d& c : cv) {
                            collision_input& ci = inputs[j];

                            //mesh& am = axiom::global_core.ecs->get_component<mesh>(ci.a);
                            //mesh& bm = axiom::global_core.ecs->get_component<mesh>(ci.b);

                            bool insert = true;

                            if(ci.cb->is_static) {
                                if(ci.ca->is_static) insert = false;
                                else {
                                    c.pa = transpose(ci.ta->orientation) * c.pa;
                                    c.pb = ci.ta->position + c.pb;
                                    c.a = ci.a;
                                    c.b = NULL_ENTITY;
                                }
                            } else if(ci.ca->is_static) {
                                c.a = ci.b;
                                c.b = NULL_ENTITY;

                                vec2 temp = c.pa;
                                c.pa = transpose(ci.tb->orientation) * (c.pb + (ci.ta->position - ci.tb->position));
                                c.pb = ci.ta->position + temp;
                                c.normal = -c.normal;
                            } else {
                                c.pa = transpose(ci.ta->orientation) * (c.pa);
                                c.pb = transpose(ci.tb->orientation) * (c.pb + (ci.ta->position - ci.tb->position));
                                c.a = ci.a;
                                c.b = ci.b;
                            }

                            if(insert) cdata[t].push_back(c);
                        }
                    }
                }

                cache.clear();
            }
        }
    };

    for(int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread(thread_GJK, i);
    }

    for(int i = 0; i < num_threads; ++i) {
        threads[i].join();
    }

    for(auto& c : cdata) {
        for(collision_data2d& collision_data2d : c) {
            insert_collision(collision_data2d);
        }
    }

    collision_constraint2ds.clear();
    collision_constraint2ds.resize(collision_table.size());

    uint32_t i = 0;
    for(auto& [k, d] : collision_table) {
        collision_constraint2d cc;
        uint32_t a = k & 0xFFFFFFFF;
        uint32_t b = k >> 32;
        cc.a = a;
        cc.b = b;

        cc.ca = &axiom::global_core.ecs->get_component<collider2d>(cc.a);
        cc.ta = &axiom::global_core.ecs->get_component<transform2d>(cc.a);
        cc.ca->colliding_with.push_back(cc.b);
        cc.ca->colliding_normal.push_back(d[0].normal);

        if(cc.b != NULL_ENTITY) {
            cc.cb = &axiom::global_core.ecs->get_component<collider2d>(cc.b);
            cc.tb = &axiom::global_core.ecs->get_component<transform2d>(cc.b);
            cc.cb->colliding_with.push_back(cc.a);
            cc.cb->colliding_normal.push_back(-d[0].normal);
        }

        //

        for(int i = 0; i < d.size(); ++i) {
            col_constraint col;

            collision_data2d& c = d[i];

            col.d = &c;

            cc.constraints.push_back(col);
        }
        collision_constraint2ds[i] = cc;
        ++i;
    }

    //

    //

    uint32_t start = constraints.size();

    //
    for(int i = 0; i < substeps; ++i) {
        velocity_solve();

        integrate();
    }

    // prune

    std::vector<uint64_t> remove_table;

    for(collision_constraint2d& c : collision_constraint2ds) {
        uint64_t key = uint64_t(c.a) | (uint64_t(c.b) << 32);
        auto& d = collision_table[key];

        std::vector<uint32_t> n_erase;

        for(col_constraint& cc : c.constraints) {
            vec2 distance = cc.pa - cc.pb;

            float dot_normal = glm::dot(distance, cc.d->normal);
            float v = length(distance - cc.d->normal * dot_normal);

            if(dot_normal > contact_sep || v > contact_sep) {

                n_erase.push_back((uint64_t(cc.d) - uint64_t(d.data())) / sizeof(collision_data2d));
            }
        }

        uint32_t i = 0;
        for(uint32_t n : n_erase) {
            d.erase(d.begin() + (n - i));
            ++i;
        }

        if(d.size() == 0) remove_table.push_back(key);
    }

    for(uint64_t k : remove_table) collision_table.erase(k);
}

void physics_system2d::integrate() {
    for(uint32_t entity : collectors[0].entities) {
        transform2d& ta = axiom::global_core.ecs->get_component<transform2d>(entity);
        collider2d& ca = axiom::global_core.ecs->get_component<collider2d>(entity);

        if(!ca.is_static) {
            ta.position += ca.velocity * sub_dt;

            if(ca.allow_rotation) {
                glm::mat2 rotation = rotate(ca.angular_velocity * sub_dt, vec3(0, 0, 1));
                ta.orientation = rotation * ta.orientation;
            }
        }

        if(!ca.is_static) {
            if(ca.allow_gravity) {
                vec2 g = get_gravity(ta.position) * -20.0f;

                ca.velocity += g * sub_dt;
            }
        }
    }
}

void physics_system2d::call() {
    //Input_system& input_system = axiom::global_core.ecs->get_system<Input_system>();

    if(sim_active) {
        physics_time += axiom::global_core.ecs->delta_time;

        uint32_t frames = 0;

        while(physics_time >= physics_step) {
            physics_loop();
            physics_time -= physics_step;
            ++frames;

            if(frames >= max_frames) {
                physics_time = glm::min(physics_time, physics_step);
                break;
            }
        }
    }
}


void physics_system2d::apply_impulse(collider2d* c, vec2 impulse, vec2 point) {
    c->velocity += impulse / c->mass;
    if(c->allow_rotation) c->angular_velocity += cross(vec3(point, 0.0f), vec3(impulse, 0.0f)).z / c->inertia;
}

void constraint_distance::get_points() {
    pos_a = ta->orientation * pa + ta->position;

    if(b == NULL_ENTITY) {
        pos_b = pb;
    } else {
        pos_b = tb->orientation * pb + tb->position;
    }
}

void constraint_distance::get_values() {
    vec2 difference = pos_a - pos_b;
    float len = length(difference);

    jacobian = difference / len;
    if(len == 0) jacobian = vec2(0.0f, 1.0f);

    //lambda = 0.0f;

    // inertia
    inertia = physics_system2d::calculate_inverse_mass(ca, ta, jacobian, pos_a - ta->position);
    if(b != NULL_ENTITY) inertia += physics_system2d::calculate_inverse_mass(cb, tb, -jacobian, pos_b - tb->position);

    // baumgarte

    baumgarte = -len;
}

void physics_system2d::velocity_solve() {
    float spring = 0.5f;
    float softness = 0.005f;

    float spring_constraint = 0.5f;
    float softness_constraint = 0.005f;
    float factor = 1.0f / physics_step;
    float factor_constraint = 1.0f / physics_step;

    for(collision_constraint2d& data : collision_constraint2ds) {
        data.get_points();
        data.get_value();

        float baumgarte_max = 1.0f;

        for(col_constraint& c : data.constraints) {
            c.baumgarteN = glm::max(c.baumgarteN, -baumgarte_max);

            c.lambdaN = c.d->lambdaN;
            c.lambdaT = c.d->lambdaT;

            if(data.b == NULL_ENTITY) {
                vec2 impulse = c.normal * c.lambdaN;

                apply_impulse(data.ca, impulse, c.pa - data.ta->position);

                vec2 friction_impulse = c.tangent * c.lambdaT;

                apply_impulse(data.ca, friction_impulse, c.pa - data.ta->position);
            } else {
                vec2 impulse = c.normal * c.lambdaN;

                apply_impulse(data.ca, impulse, c.pa - data.ta->position);
                apply_impulse(data.cb, -impulse, c.pb - data.tb->position);

                vec2 friction_impulse = c.tangent * c.lambdaT;

                apply_impulse(data.ca, friction_impulse, c.pa - data.ta->position);
                apply_impulse(data.cb, -friction_impulse, c.pb - data.tb->position);
            }
        }
    }

    for(constraint2d& data : constraints) {
        data.ca = &axiom::global_core.ecs->get_component<collider2d>(data.a);
        data.ta = &axiom::global_core.ecs->get_component<transform2d>(data.a);
        if(data.b != NULL_ENTITY) {
            data.cb = &axiom::global_core.ecs->get_component<collider2d>(data.b);
            data.tb = &axiom::global_core.ecs->get_component<transform2d>(data.b);
        }

        data.get_points();
        data.get_values();


        for(pos_constraint& c : data.pos) {
            uint32_t i = 0;
            for(vec2 v : c.vs) {
                vec2 impulse = v * c.lambda[i];

                if(data.b == NULL_ENTITY) {
                    apply_impulse(data.ca, impulse, c.pa - data.ta->position);
                } else {
                    apply_impulse(data.ca, impulse, c.pa - data.ta->position);
                    apply_impulse(data.cb, -impulse, c.pb - data.tb->position);
                }

                ++i;
            }
        }

        for(rot_constraint& c : data.rot) {
            float twirl = c.lambda;

            data.ca->angular_velocity += twirl / data.ca->inertia;
            if(data.b != NULL_ENTITY) data.cb->angular_velocity -= twirl / data.cb->inertia;
        }
    }

    float distance_stiffness = 0.2f;
    float shape_stiffness = 0.0f;

    float target_length = 0.0f;
    float current_length = 0.0f;

    for(int i = 0; i < velocity_iterations; ++i) {
        for(collision_constraint2d& data : collision_constraint2ds) {
            for(col_constraint& cc : data.constraints) {

                vec2 velocity = calculate_point_velocity(data.ca, cc.pa - data.ta->position);

                float diff = (cc.baumgarteN) * spring * factor;

                if(cc.d->b == NULL_ENTITY) {
                    float inertia = cc.inertiaNa;

                    float v = glm::dot(velocity, cc.d->normal);

                    float L = -v - diff;
                    L /= inertia;

                    vec2 limits = vec2(0.0f, FLT_MAX);

                    float new_lambda = cc.lambdaN + L;
                    new_lambda = glm::clamp(new_lambda, limits.x, limits.y);
                    L = new_lambda - cc.lambdaN;
                    cc.lambdaN = new_lambda;

                    vec2 impulse = cc.d->normal * L;

                    apply_impulse(data.ca, impulse, cc.pa - data.ta->position);

                    // friction

                    float normal_magnitude = length(impulse);

                    velocity = calculate_point_velocity(data.ca, cc.pa - data.ta->position);

                    vec2 tangent_vector = vec2(cc.d->normal.y, -cc.d->normal.x);
                    float tangent_velocity = glm::dot(velocity, tangent_vector);

                    float inverse_mass = cc.inertiaTa;

                    float mu = 0.9f;

                    float max_friction = abs(mu * cc.lambdaN);

                    float new_lambdaT = cc.lambdaT - tangent_velocity / inverse_mass;
                    new_lambdaT = glm::clamp(new_lambdaT, -max_friction, max_friction);
                    L = new_lambdaT - cc.lambdaT;
                    cc.lambdaT = new_lambdaT;

                    float Pt = L;

                    vec2 friction_impulse = tangent_vector * Pt;

                    apply_impulse(data.ca, friction_impulse, cc.pa - data.ta->position);
                } else {
                    float inertia = cc.inertiaNa + cc.inertiaNb;

                    velocity -= calculate_point_velocity(data.cb, cc.pb - data.tb->position);

                    float v = glm::dot(velocity, cc.normal);

                    float L = -v - diff;
                    L /= inertia;
                    L -= softness * cc.lambdaN;

                    vec2 limits = vec2(0.0f, FLT_MAX);

                    float new_lambda = cc.lambdaN + L;
                    new_lambda = glm::clamp(new_lambda, limits.x, limits.y);
                    L = new_lambda - cc.lambdaN;
                    cc.lambdaN = new_lambda;

                    vec2 impulse = cc.normal * L;

                    apply_impulse(data.ca, impulse, cc.pa - data.ta->position);
                    apply_impulse(data.cb, -impulse, cc.pb - data.tb->position);

                    // friction

                    float normal_magnitude = length(impulse);

                    velocity = calculate_point_velocity(data.ca, cc.pa - data.ta->position);
                    velocity -= calculate_point_velocity(data.cb, cc.pb - data.tb->position);

                    float tangent_velocity = glm::dot(velocity, cc.tangent);

                    inertia = cc.inertiaTa + cc.inertiaTb;

                    float mu = 0.9f;

                    float max_friction = abs(mu * cc.lambdaN);

                    float new_lambdaT = cc.lambdaT - tangent_velocity / inertia;
                    new_lambdaT = glm::clamp(new_lambdaT, -max_friction, max_friction);
                    L = new_lambdaT - cc.lambdaT;
                    cc.lambdaT = new_lambdaT;

                    float Pt = L;

                    vec2 friction_impulse = cc.tangent * Pt;

                    apply_impulse(data.ca, friction_impulse, cc.pa - data.ta->position);
                    apply_impulse(data.cb, -friction_impulse, cc.pb - data.tb->position);
                }
            }
        }

        for(int j = 0; j < constraints.size(); ++j) { 
        //for(collision_constraint2d& data : collision_constraint2ds) {
            int start = 0;//core.random.next() % collision_constraint2ds.size();
            int dir = 1;

            if(i % 2 == 1) {
                start = constraints.size() - 1;
                dir = -1;
            }

            constraint2d& data = constraints[start + j * dir];

            float max_grab = FLT_MAX;

            for(pos_constraint& c : data.pos) {
                //max_grab = c.limit;

                uint32_t i = 0;
                for(vec2 v : c.vs) {
                    float bg = -c.baumgarte[i] * spring_constraint * factor_constraint;

                    float inertia = c.inertia_a[i];

                    vec2 velocity = calculate_point_velocity(data.ca, c.pa - data.ta->position);

                    vec2 diff = c.pa - c.pb;
                    float len = length(diff);
                    vec2 vv = diff / len;

                    if(data.b == NULL_ENTITY) {
                        vec2 vel = velocity;
                        //if(c.tolerance != 0.0f) vel = vv * glm::dot(vel, vv);

                        float L = -dot(vel, v) + bg;
                        L /= inertia;
                        L -= softness_constraint * c.lambda[i];

                        float new_lambda = c.lambda[i] + L;

                        //new_lambda = glm::clamp(new_lambda, -max_grab / inertia, max_grab / inertia);
                        L = new_lambda - c.lambda[i];
                        c.lambda[i] = new_lambda;

                        vec2 impulse = v * L;

                        apply_impulse(data.ca, impulse, c.pa - data.ta->position);
                    } else {
                        inertia += c.inertia_b[i];

                        velocity -= calculate_point_velocity(data.cb, c.pb - data.tb->position);

                        vec2 vel = velocity;       
                        //if(c.tolerance != 0.0f) vel = vv * glm::dot(vel, vv);

                        float L = -dot(vel, v) + bg;
                        L /= inertia;
                        L -= softness_constraint * c.lambda[i];

                        float new_lambda = c.lambda[i] + L;

                        //new_lambda = glm::clamp(new_lambda, -max_grab / inertia, max_grab / inertia);
                        L = new_lambda - c.lambda[i];
                        c.lambda[i] = new_lambda;

                        //c.lambda[i] = 0.0f;
                        //L = 0.0f;

                        vec2 impulse = v * L;

                        apply_impulse(data.ca, impulse, c.pa - data.ta->position);
                        apply_impulse(data.cb, -impulse, c.pb - data.tb->position);
                    }

                    ++i;
                }
            }

            for(rot_constraint& c : data.rot) {
                float angular_delta = -c.baumgarte;
                float inertia = c.inertia;

                if(data.b == NULL_ENTITY) {
                    float bg = angular_delta * spring_constraint * factor_constraint;

                    float angular_velocity = data.ca->angular_velocity;

                    float L = -angular_velocity + bg;
                    L /= inertia;
                    L -= softness_constraint * c.lambda;
                    float new_lambda = c.lambda + L;

                    //new_lambda = glm::clamp(new_lambda, -max_grab / c.inertia, max_grab / c.inertia);
                    L = new_lambda - c.lambda;
                    c.lambda = new_lambda;

                    data.ca->angular_velocity += L / data.ca->inertia;
                } else {
                    float bg = angular_delta * spring_constraint * factor_constraint;

                    float angular_velocity = data.ca->angular_velocity - data.cb->angular_velocity;

                    float L = -angular_velocity + bg;
                    L /= inertia;
                    L -= softness_constraint * c.lambda;
                    float new_lambda = c.lambda + L;

                    //new_lambda = glm::clamp(new_lambda, -max_grab / c.inertia, max_grab / c.inertia);
                    L = new_lambda - c.lambda;
                    c.lambda = new_lambda;

                    data.ca->angular_velocity += L / data.ca->inertia;
                    data.cb->angular_velocity -= L / data.cb->inertia;
                }
            }
        }
    }

    for(collision_constraint2d& c : collision_constraint2ds) {
        for(col_constraint& cc : c.constraints) {
            cc.d->lambdaN = cc.lambdaN;
            cc.d->lambdaT = cc.lambdaT;
        }
    }
}

vec2 angular_to_linear(vec2 pos, float angular_velocity) {
    return vec2(pos.y, -pos.x) * angular_velocity;
}

vec2 physics_system2d::calculate_inertia(collision_shape2d& c) {
    ivec2 num_points = ivec2(16);

    transform2d temp;
    temp.position = vec2(0.0f);
    temp.orientation = glm::identity<glm::mat2>();

    vec2 minimum = vec2(FLT_MAX, FLT_MAX);
    vec2 maximum = vec2(-FLT_MAX, -FLT_MAX);

    //

    axiom::bounding_box2d bb;
    bb.minimum = vec2(FLT_MAX, FLT_MAX);
    bb.maximum = vec2(-FLT_MAX, -FLT_MAX);
        
    vec2 min_x = support(vec2(-1.0f, 0.0f), c.vertices);
    vec2 max_x = support(vec2(1.0f, 0.0f), c.vertices);
    vec2 min_y = support(vec2(0.0f, -1.0f), c.vertices);
    vec2 max_y = support(vec2(0.0f, 1.0f), c.vertices);
    
    bb.minimum = vec2(min_x.x, min_y.y);
    bb.maximum = vec2(max_x.x, max_y.y);

    minimum = glm::min(minimum, bb.minimum);
    maximum = glm::max(maximum, bb.maximum);

    //

    vec2 size = (maximum - minimum) / vec2(num_points);

    vec2 accum = vec2(0.0f);
    float number = 0.0f;

    float inertia = 0.0f;

    float single_inertia = 1.0f / 12 * (size.x * size.x + size.y * size.y);

    for(int y = 0; y < num_points.y; ++y) {
        for(int x = 0; x < num_points.x; ++x) {
            vec2 point = minimum + vec2(x + 0.5f, y + 0.5f) / vec2(num_points) * (maximum - minimum);
            vec2 p = transpose(c.orientation) * (point - c.position);
            bool is_inside = collision_point(c.vertices, p);

            if(is_inside) {
                ++number;
                accum += point;

                float dist = length(point);
                inertia += single_inertia + (dist * dist);
            }
        }
    }

    vec2 center = accum / number;

    //

    inertia /= number;
    inertia *= c.mass;
    c.inertia = inertia;

    return center;
}

vec2 physics_system2d::calculate_inertia(collider2d& c) {
    ivec2 num_points = ivec2(16);

    vec2 center = vec2(0.0f);
    c.mass = 0.0f;
    c.inertia = 0.0f;

    for(collision_shape2d& cs : c.shapes) {
        vec2 com = calculate_inertia(cs);

        center += com * cs.mass;

        c.mass += cs.mass;
        if(c.allow_rotation) c.inertia += cs.inertia;
    }

    center /= c.mass;

    //

    for(auto& cs : c.shapes) cs.position -= center;

    float dist = length(center);
    c.inertia -= dist * dist * c.mass;

    return center;
}

vec2 physics_system2d::calculate_point_velocity(collider2d* c, vec2 point) {
    vec2 velocity;
    float angular_velocity;

    velocity = c->velocity;
    angular_velocity = c->angular_velocity;
    velocity += vec2(cross(vec3(0.0f, 0.0f, angular_velocity), vec3(point, 0.0f)));;

    return velocity;
}

float physics_system2d::calculate_inverse_mass(collider2d* c, transform2d* t, vec2 impulse_dir, vec2 point) {
    float inverse_mass = 1.0f / c->mass;

    if(c->allow_rotation) {
        //d = cross(constraint.pos_a, constraint.normal);
        //constraint.inertiaNa = inv_mass + glm::dot(d, inverse_tensor_a * d);
        /*
        float torque_per_unit = length(cross(vec3(point, 0.0f), vec3(impulse_dir, 0.0f)));

        float angular_velocity = torque_per_unit / c->inertia;

        vec2 linear_velocity = vec2(cross(vec3(0.0f, 0.0f, angular_velocity), vec3(point, 0.0f)));

        float angular_inertia = glm::dot(linear_velocity, impulse_dir);
        */

        float d = cross(vec3(point, 0.0f), vec3(impulse_dir, 0.0f)).z;
        float i = d * (d / c->inertia);
        
        inverse_mass += i;
    }

    return inverse_mass;
}

void collision_constraint2d::get_points() {
    for(col_constraint& c : constraints) {
        vec2 point_a = ta->orientation * c.d->pa + ta->position;

        c.pa = point_a;

        if(c.d->b == NULL_ENTITY) {
            c.pb = c.d->pb;
        } else {
            vec2 point_b = tb->orientation * c.d->pb + tb->position;

            c.pb = point_b;
        }
    }
}

void collision_constraint2d::get_value() {
    for(col_constraint& c : constraints) {
        vec2 diff = c.pa - c.pb;

        c.tangent = vec2(c.d->normal.y, -c.d->normal.x);
        c.normal = c.d->normal;

        c.inertiaNa = physics_system2d::calculate_inverse_mass(ca, ta, c.normal, c.pa - ta->position);
        c.inertiaTa = physics_system2d::calculate_inverse_mass(ca, ta, c.tangent, c.pa - ta->position);

        if(b != NULL_ENTITY) {
            c.inertiaNb = physics_system2d::calculate_inverse_mass(cb, tb, c.normal, c.pb - tb->position);
            c.inertiaTb = physics_system2d::calculate_inverse_mass(cb, tb, c.tangent, c.pb - tb->position);
        }

        c.baumgarteN = glm::dot(diff, c.normal);
        c.baumgarteT = glm::dot(diff, c.tangent);
    }
}

void collision_constraint2d::refresh(col_constraint& c) {
    vec2 point_a = ta->orientation * c.d->pa + ta->position;
    c.pa = point_a;
    c.inertiaNa = physics_system2d::calculate_inverse_mass(ca, ta, c.normal, c.pa - ta->position);
    c.inertiaTa = physics_system2d::calculate_inverse_mass(ca, ta, c.tangent, c.pa - ta->position);

    if(c.d->b == NULL_ENTITY) {
        c.pb = c.d->pb;
    } else {
        vec2 point_b = tb->orientation * c.d->pb + tb->position;
        c.pb = point_b;
        c.inertiaNb = physics_system2d::calculate_inverse_mass(cb, tb, c.normal, c.pb - tb->position);
        c.inertiaTb = physics_system2d::calculate_inverse_mass(cb, tb, c.tangent, c.pb - tb->position);
    }

    vec2 diff = c.pa - c.pb;
    c.baumgarteN = glm::dot(diff, c.normal);
    c.baumgarteT = glm::dot(diff, c.tangent);
}

void collision_constraint2d::refresh_C(col_constraint& c) {
    vec2 point_a = ta->orientation * c.d->pa + ta->position;
    c.pa = point_a;

    if(c.d->b == NULL_ENTITY) {
        c.pb = c.d->pb;
    } else {
        vec2 point_b = tb->orientation * c.d->pb + tb->position;
        c.pb = point_b;
    }

    vec2 diff = c.pa - c.pb;
    c.baumgarteN = glm::dot(diff, c.normal);
    c.baumgarteT = glm::dot(diff, c.tangent);
}

void constraint2d::refresh(pos_constraint& c) {
    vec2 point_a = ta->orientation * c.a + ta->position;
    c.pa = point_a;
    for(int i = 0; i < c.vs.size(); ++i) c.inertia_a[i] = physics_system2d::calculate_inverse_mass(ca, ta, c.vs[i], c.pa - ta->position);

    if(b != NULL_ENTITY) {
        vec2 point_b = tb->orientation * c.b + tb->position;
        c.pb = point_b;
        for(int i = 0; i < c.vs.size(); ++i) c.inertia_b[i] = physics_system2d::calculate_inverse_mass(cb, tb, c.vs[i], c.pb - tb->position);
    }

    for(int i = 0; i < c.vs.size(); ++i) {
        vec2 diff = c.pa - c.pb;
        float dd = glm::dot(diff, c.vs[i]);
        c.baumgarte[i] = dd;
    }
}

void constraint2d::get_points() {
    for(pos_constraint& pc : pos) {
        pc.pa = ta->orientation * pc.a + ta->position;

        if(b != NULL_ENTITY) {
            pc.pb = tb->orientation * pc.b + tb->position;
        } else {
            pc.pb = pc.b;
        }
    }
}

void constraint2d::get_values() {
    for(pos_constraint& pc : pos) {
        pc.inertia_a.resize(pc.vs.size());
        pc.inertia_b.resize(pc.vs.size());
        pc.baumgarte.resize(pc.vs.size());
        pc.lambda.resize(pc.vs.size(), 0.0f);

        vec2 diff = pc.pa - pc.pb;

        for(int i = 0; i < pc.vs.size(); ++i) {
            vec2 v = pc.vs[i];
            float v_diff = glm::dot(diff, v);
            pc.baumgarte[i] = v_diff;

            pc.inertia_a[i] = physics_system2d::calculate_inverse_mass(ca, ta, v, pc.pa - ta->position);
            if(b != NULL_ENTITY) pc.inertia_b[i] = physics_system2d::calculate_inverse_mass(cb, tb, v, pc.pb - tb->position);
            
            //pc.lambda[i] = 0.0f;
        }
    }

    for(rot_constraint& rc : rot) {
        vec2 dir_a = ta->orientation * rc.a;
        vec2 dir_b;

        rc.inertia = 0.0f;


        if(b == NULL_ENTITY) dir_b = rc.b;
        else {
            dir_b = tb->orientation * rc.b;
            rc.inertia += 1.0f / cb->inertia;
        }

        float angle = acos(glm::clamp(dot(dir_a, dir_b), -1.0f, 1.0f));

        if(cross(vec3(dir_a, 0), vec3(dir_b, 0)).z > 0) {
            angle = -angle;
        }

        rc.baumgarte = angle;
        rc.inertia += 1.0f / ca->inertia;
    }
}

vec2 get_gravity(vec2 pos) {
    physics_system2d& ps = axiom::global_core.ecs->get_system<physics_system2d>();

    //vec2 rel_pos = pos;
    //return normalize(vec2(rel_pos.x / (ps.gravity_aspect.x * ps.gravity_aspect.x), rel_pos.y / (ps.gravity_aspect.y * ps.gravity_aspect.y)));
    return vec2(0.0f, 1.0f);
}

std::vector<collision_data2d> physics_system2d::collide(transform2d t, std::vector<vertex_element2d> vs) {
    std::vector<collision_data2d> ret;

    collider2d c;
    collision_shape2d cs;
    cs.vertices = vs;
    c.shapes.push_back(cs);
    c.create_bounding_box();

    for(uint32_t entity : collectors[0].entities) {
        collider2d& ca = axiom::global_core.ecs->get_component<collider2d>(entity);
        transform2d& ta = axiom::global_core.ecs->get_component<transform2d>(entity);

        if(collision(ta, ca.bounding_box, t, c.bounding_box)) {
            collision_input ci;
            ci.ta = &ta;
            ci.ca = &ca;
            ci.a = entity;

            ci.tb = &t;
            ci.cb = &c;
            ci.b = 0xFFFFFFFF;

            auto cs = collision(ci);

            for(auto& cc : cs) {
                cc.a = ci.a;
                cc.b = ci.b;

                if(ci.ca->is_static) {
                    cc.a = NULL_ENTITY;

                    cc.pa = ci.ta->position + cc.pa;
                    cc.pb = transpose(ci.tb->orientation) * (cc.pb + (ci.ta->position - ci.tb->position));
                } else {
                    cc.pa = transpose(ci.ta->orientation) * (cc.pa);
                    cc.pb = transpose(ci.tb->orientation) * (cc.pb + (ci.ta->position - ci.tb->position));
                }
            }

            ret.insert(ret.end(), cs.begin(), cs.end());
        }
    }

    return ret;
}

}