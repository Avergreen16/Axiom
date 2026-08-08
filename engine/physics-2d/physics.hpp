#pragma once

#include <include/scene.hpp>
#include <include/ecs.hpp>

#include <physics-2d/collider.hpp>

namespace axiom {

//

struct collision_data {
    bool collide = false;

    uint32_t a;
    uint32_t b;

    vec2 pa;
    vec2 pb;

    vec2 normal;

    float lambdaN = 0.0f;
    float lambdaT = 0.0f;

    float deltaT = 0.0f;
    float deltaN = 0.0f;
    float sumN = 0.0f;
};

struct col_constraint {
    collision_data *d;

    vec2 pa;
    vec2 pb;

    vec2 normal;
    vec2 tangent;

    float lambdaN = 0.0f;
    float lambdaT = 0.0f;

    float pos_lambdaN = 0.0f;

    float inertiaNa = 0.0f;
    float inertiaNb = 0.0f;

    float inertiaTa = 0.0f;
    float inertiaTb = 0.0f;

    float baumgarteN;
    float baumgarteT;
};

struct collision_constraint {
    uint32_t a;
    uint32_t b;

    collider2d *ca;
    transform2d *ta;
    collider2d *cb;
    transform2d *tb;

    std::vector<col_constraint> constraints;

    void get_points();
    void get_value();

    void refresh(col_constraint& c);
    void refresh_C(col_constraint& c);
};

//

struct constraint_distance {
    uint32_t a = NULL_ENTITY;
    uint32_t b = NULL_ENTITY;

    vec2 pa;
    vec2 pb;

    vec2 pos_a;
    vec2 pos_b;

    collider2d *ca;
    transform2d *ta;
    collider2d *cb;
    transform2d *tb;

    vec2 jacobian;
    float lambda;
    float inertia;
    float baumgarte;

    void get_points();
    void get_values();
};

struct pos_constraint {
    vec2 a;
    vec2 b;

    vec2 pa;
    vec2 pb;

    std::vector<vec2> vs;
    std::vector<float> inertia_a;
    std::vector<float> inertia_b;
    std::vector<float> baumgarte;
    std::vector<float> lambda;
    std::vector<float> pos_lambda;

    float compliance = 0.0001f;
    float limit = FLT_MAX;

    bool is_hold = false;
};

struct rot_constraint {
    // vectors to be aligned in the space of their object
    vec2 a;
    vec2 b;

    // vectors in world space
    vec2 va;
    vec2 vb;

    float baumgarte;
    float inertia;
    float lambda = 0.0f;
};

struct constraint {
    uint32_t a = NULL_ENTITY;
    uint32_t b = NULL_ENTITY;

    collider2d *ca;
    transform2d *ta;
    collider2d *cb;
    transform2d *tb;

    std::vector<pos_constraint> pos;
    std::vector<rot_constraint> rot;

    void get_points();
    void get_values();

    void refresh(pos_constraint& c);
};

struct sap_point {
    vec2 start;
    vec2 end;
    uint32_t id;
    bool is_start = true;
};

struct collision_input {
    uint32_t a;
    uint32_t b;

    collider2d *ca;
    transform2d *ta;
    collider2d *cb;
    transform2d *tb;
};

struct input_data {
    bounding_box2d bounding_box;
    transform2d *transform;
    uint32_t id;
};

enum class collision_type {
    EDGE, VERTEX
};
struct return_tag {
    collision_type type;
    ivec2 va[2];
    ivec2 vb[2];
};

extern transform2d null_transform;

struct physics_system : axiom::system {
    bool sim_active = true;

    // parameters
    float fps = 60.0f;
    uint32_t velocity_iterations = 4;
    uint32_t position_iterations = 0;
    uint32_t substeps = 4;
    float contact_sep = 0.02f;
    float static_dist = 0.0625f;
    float penetration_threshold = FLT_MAX;
    uint32_t iteration_threshold = 3;
    float softness_duration = 1.0f;

    float physics_step = 1.0f / fps;
    float sub_dt = physics_step / substeps;
    uint32_t max_frames = 1;
    float physics_time = 0.0f;

    //

    std::unordered_map<uint64_t, std::vector<collision_data>> collision_table;

    std::vector<constraint> constraints;
    std::vector<constraint_distance> constraints_distance;
    std::vector<collision_constraint> collision_constraints;

    vec2 gravity_aspect = vec2(1.0f, 1.0f);

    std::unordered_set<uint32_t> inserted_sap;
    std::vector<sap_point> sap_points;

    //

    physics_system();

    // static std::vector<std::vector<collision_data>> collision(std::vector<collision_input>& input);
    static std::vector<collision_data> collision(collision_input& input);
    static std::vector<collision_data> collision(transform2d& ta, collision_shape2d& ca, transform2d& tb, collision_shape2d& cb, return_tag& tag);

    static bounding_box2d transform(transform2d& t, bounding_box2d& b);
    static bool collision(transform2d& ta, bounding_box2d& a, transform2d& tb, bounding_box2d& b);
    static bool collision(bounding_box2d& a, bounding_box2d& b);

    static std::vector<uint32_t> traverse_BVH(transform2d& ta, std::vector<BVH_node2d>& ca, transform2d& tb, bounding_box2d& bb);
    static std::vector<uint64_t> traverse_BVH(transform2d& ta, std::vector<BVH_node2d>& ca, transform2d& tb, std::vector<BVH_node2d>& cb);

    //

    static bool collision_point(std::vector<vertex_element2d> vertices, vec2 point);

    static vec2 transform_vertices(transform2d& t, collision_shape2d& c, std::vector<vertex_element2d>& vertices, vec2 origin);

    static vec2 support_func(std::vector<vec2>& vertices, vec2 radius, vec2 direction);
    static vec2 support_func(std::vector<vec2>& vertices, vec2 radius, vec2 direction, mat2 matrix);

    void insert_collision(collision_data c);

    void velocity_solve();

    static vec2 calculate_inertia(collision_shape2d& c);
    static vec2 calculate_inertia(collider2d& c);

    std::vector<uint64_t> broad_phase(std::vector<input_data>& input);

    static vec2 calculate_point_velocity(collider2d *c, vec2 point);

    static float calculate_inverse_mass(collider2d *c, transform2d *t, vec2 impulse_dir, vec2 point);

    static void apply_impulse(collider2d *c, vec2 impulse, vec2 point);

    void integrate();

    void physics_loop();

    void call();

    std::vector<collision_data> collide(transform2d t, std::vector<vertex_element2d> vs);
};

vec2 get_gravity(vec2 pos);

void get_normal(vec2 a, vec2 b, vec2 r, vec2& normal, vec2& center);

}