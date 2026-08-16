#include "physics.hpp"
#include "core.hpp"
#include "input.hpp"

#include <numeric>

// sparse matrix

std::atomic<int> num_bb_checks = 0;
std::atomic<int> num_gjk_checks = 0;

std::unordered_map<ivec3, mat3, Hash_coord> directional; 

//

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


int get_i(ivec3 v, ivec3 size) {
    return (v.z * size.y + v.y) * size.x + v.x;
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

struct State {
    vec3 velocity;
    vec3 angular_velocity;
};


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

/*


glm::vec3 triangle_project(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3& point) {
    vec3 e0 = a - c;
    vec3 e1 = b - c;

    float v = dot(c, e0);
    float w = dot(c, e1);
    float x = dot(e0, e0);
    float y = dot(e1, e1);
    float z = dot(e0, e1);

    float denom = (x * y - z * z);
    float alpha = (w * z - v * y) / denom;
    float beta = (v * z - w * x) / denom;
    float gamma = 1.0f - alpha - beta;

    point = a * alpha + b * beta + c * gamma;

    return {alpha, beta, gamma};
}*/