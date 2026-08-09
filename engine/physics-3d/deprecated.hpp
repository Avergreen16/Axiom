#pragma once
#include "ecs.hpp"
#include "wrapper.hpp"
#include "pnum.hpp"
#include "random.hpp"
#include "utility.hpp"

mat3 rotate_to(vec3 a, vec3 b);
mat3 rotate_to(vec3 a, vec3 b, vec3 axis);

vec3 get_gravity(pvec3 pos);

mat3 translate_M(vec3 d, mat3 M, float mass);
mat3 inv_translate_M(vec3 d, mat3 M, float mass);
mat3 cuboid_M(vec3 size);

extern std::unordered_map<ivec3, mat3, Hash_coord> directional; 



struct hash_uvec2 {
    std::size_t operator()(uvec2 v) const {
        return hash(v);
    }
};


std::vector<vec3> get_tangents(vec3 normal);

struct Input_state {
    vec3 movement_vec;
    vec3 forward_vec;
    vec3 up_vec;
};

struct Collider_state {
    uint32_t id;

    pvec3 position;
    mat3 orientation;
    vec3 velocity;
    vec3 angular_momentum;
};

struct Physics_state {
    std::unordered_map<uint32_t, Collider_state> states;
    Collision_table ct;
};

struct pos_constraint {
    // object space
    vec3 a;
    pvec3 b;
    
    // relative space
    vec3 ra;
    vec3 rb;

    // world space
    pvec3 wa;
    pvec3 wb;
    
    // velocities;
    vec3 vel_a;
    vec3 vel_b;

    std::vector<vec3> vs;

    std::vector<float> baumgarte;
    std::vector<float> inertia_a;
    std::vector<float> inertia_b;
    std::vector<float> lambda;
    
    float spring = 0.35f;
    float softness = 0.005f;
    float max_impulse = FLT_MAX;
    bool is_grab = false;
};

struct rot_constraint {
    // vectors to be aligned in the space of their object
    vec3 a;
    vec3 b;

    // vectors in world space
    vec3 wa;
    vec3 wb;

    // vectors to rotate along
    uint32_t c = NULL_ENTITY;
    std::vector<vec3> vs;
    std::vector<vec3> wvs;
    
    std::vector<float> baumgarte;
    std::vector<float> inertia_a;
    std::vector<float> inertia_b;
    std::vector<float> lambda;
    
    float spring = 0.35f;
    float softness = 0.005f;
    float max_impulse = FLT_MAX;
};

struct Constraint {
    uint32_t a = NULL_ENTITY;
    uint32_t b = NULL_ENTITY;

    Collider* ca;
    Collider* cb;
    Transform* ta;
    Transform* tb;

    std::vector<pos_constraint> pos;
    std::vector<rot_constraint> rot;

    std::vector<float> lambda = {};

    void pre_step();
    void refresh(pos_constraint& c);
    void refresh(rot_constraint& c);
};

struct DOF_constraint {
    uint32_t a;
    uint32_t b;

    vec3 locked_axis;
    float factor = 0.9f;

    void apply(float dt);
};

struct Physics_step_data {
    vec3 velocity = vec3(0);
    vec3 angular_momentum = vec3(0);
};

struct Prev_data {
    pvec3 pos;
    mat3 orientation;
};

struct Debug_point {
    pvec3 point;
};

struct Sap_point {
    float minimum;
    float maximum;
    uint32_t id;
    Collider* collider;
};

struct Temporary_collider {
    pvec3 position;
    mat3 orientation;
    std::vector<vec3> vertices;
    float radius;

    Bounding_box bounding_box;
};

struct GJK_step {
    std::vector<uint32_t> is;
    std::vector<vec3> vs;
    uint32_t type;
};


std::vector<uint32_t> convex_hull(std::vector<vec2> points);
std::vector<vec2> convex_hull2(std::vector<vec2> points);
glm::vec3 project_clamp(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 point);