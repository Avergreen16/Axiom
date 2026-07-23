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

struct Bounding_box {
    vec3 minimum = vec3(FLT_MAX);
    vec3 maximum = vec3(-FLT_MAX);

    float volume;
};

struct shape_face {
    std::vector<uint32_t> vertices;
    vec3 normal;
};

struct Collision_shape {
    std::vector<vec3> vertices;
    std::vector<shape_face> faces;
    
    float mass = 0.0f;
    float radius = 0.0f;
    vec3 split_radius = vec3(0.0f);
    mat3 inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };
    mat3 inverse_inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    vec3 center_of_mass;
};

struct Convex_collider {
    vec3 position = {0, 0, 0};
    mat3 orientation = identity<mat3>();

    std::shared_ptr<Collision_shape> collision_shape;

    Bounding_box bounding_box = {glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX)};
};

struct BVH_node {
    Bounding_box bounding_box;
    bool split = true;
    std::vector<uint32_t> children;
};

struct Collider {
    std::vector<Convex_collider> collision_shapes;
    float mass = 0;
    glm::mat3 inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };
    glm::mat3 inverse_inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    mat3 iit_rot;

    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 angular_momentum = glm::vec3(0.0f);

    float friction_coefficient = 0.5;
    float bounce_coefficient = 0.0f;
    bool allow_rotation = false;
    bool allow_gravity = true; 
    bool sleeping = false; 
    bool locked = false;

    bool collect = false;
    std::vector<vec3> colliding_normal;
    std::vector<uint32_t> colliding_with;

    bool is_static = false;

    Bounding_box bounding_box = {glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX)};
    bool init_bb = false;

    vec3 am_delta = vec3(0.0f);
    vec3 pos_delta = vec3(0.0f);

    std::vector<BVH_node> BVH;

    std::unordered_set<uint32_t> collision_mask;

    void apply_impulse(vec3 impulse, vec3 position);

    vec3 get_velocity(vec3 position);
    
    vec3 get_angular_velocity();

    void create_BVH(ivec3 v = ivec3(0.0));

    std::vector<uint64_t> traverse_BVH(Transform& ta, Transform& tb, Collider& cb);
    std::vector<uint32_t> traverse_BVH(Transform& ta, Transform& tb, Bounding_box& bb);
};

struct Contact_point {
    pvec3 a;
    pvec3 b;
};

struct Collision_data {
    bool priority = false;

    Contact_point contact_point;

    glm::vec3 normal;
    vec3 tangent = vec3(0.0f, 0.0f, 0.0f);
    vec3 bitangent = vec3(0.0f, 0.0f, 0.0f);

    uint32_t frames = 0;

    float lambdaN = 0.0f;
    float lambdaT = 0.0f;
    float lambdaB = 0.0f;

    float deltaT = 0.0f;
    float deltaB = 0.0f;
    float deltaN = 0.0f;

    vec2 lambda_tolerance = vec2(0.0f);
};

struct Manifold {
    vec3 normal;
    uint32_t a = 0xFFFFFFFF;
    uint32_t b = 0xFFFFFFFF;

    std::vector<Collision_data> points;
};

enum collision_type{COLLISION_TYPE_FACE, COLLISION_TYPE_EDGE, COLLISION_TYPE_VERTEX};
struct Return_point {
    pvec3 a;
    pvec3 b;
    vec3 normal;
};

struct Return_tag {
    collision_type type;
    ivec3 vid_a[3];
    ivec3 vid_b[3];
};

struct hash_uvec2 {
    std::size_t operator()(uvec2 v) const {
        return hash(v);
    }
};

struct col_constraint {
    Collision_data* data;
    Contact_point constraint_point;

    vec3 va;
    vec3 vb;

    vec3 pos_a;
    vec3 pos_b;

    vec3 normal;
    vec3 tangent;
    vec3 bitangent;

    float lambdaN = 0.0f;
    float lambdaT = 0.0f;
    float lambdaB = 0.0f;

    float inertiaNa;
    float inertiaTa;
    float inertiaBa;
    float inertiaNb;
    float inertiaTb;
    float inertiaBb;
    float inertiaN;
    float inertiaT;
    float inertiaB;

    float baumgarteN = 0.0f;
    float baumgarteT = 0.0f;
    float baumgarteB = 0.0f;

    float prev_lambdaT;
    float prev_lambdaB;
    float normal_force;

    bool apply_friction = false;

    float spring = 0.35f;
    float softness = 0.005f;
    float mu = 0.8f;
};

std::vector<vec3> get_tangents(vec3 normal);

struct Collision_constraint {
    uint32_t a = 0xFFFFFFFF;
    uint32_t b = 0xFFFFFFFF;
    
    Collider* ca;
    Collider* cb;
    Transform* ta;
    Transform* tb;
    
    std::vector<col_constraint> constraints;
    
    Manifold* mf;

    void pre_step();
    void refresh(col_constraint& c);
};

struct Hash_ptr_pair {
    std::size_t operator()(const std::array<uint32_t, 2>& a) const {
        uint64_t pa = (uint64_t)a[0];
        uint64_t pb = (uint64_t)a[1];

        return pa ^ pb;
    }
};

using Collision_table = std::unordered_map<std::array<uint32_t, 2>, std::vector<Manifold>, Hash_ptr_pair>;

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

struct Physics_Visualizer {
    std::vector<vec3> vertices_a;
    std::vector<vec3> vertices_b;
    std::vector<GJK_step> steps;
    pvec3 pos = pvec3(pnum(0xC3500001E20BE2, 0xEC), pnum(-0x752FFFFEDEC2A3, 0xFE), pnum(0x9C400000909141, 0x8D));
};


/*
Profiler profiler;
Physics_Visualizer visualizer;
int step = 0;

bool sim_active = true;

// parameters
float fps = 60.0f;
float physics_step = 1.0f / fps;
float sub_dt = physics_step / substeps;
int max_frames = 1;
float physics_time = 0.0f;

float contact_sep = 0.0625f;
uint32_t iterations = 6;
uint32_t substeps = 4;

//

vec3 gravity_aspect = vec3(1, 1, 1);
pvec3 gravity_center = vec3(0.0f);
mat3 gravity_orientation = identity<mat3>();
float gravity = 19.62f;

//

Collision_table collision_table;

std::vector<Collision_constraint> collision_constraints;
std::vector<Constraint> constraints;
std::vector<DOF_constraint> dof_constraints;

std::vector<Debug_point> debug_points;

//
*/

struct Physics_system : System {
    Profiler profiler;
    Physics_Visualizer visualizer;
    int step = 0;
    
    bool sim_active = true;

    // parameters
    float fps = 60.0f;
    float physics_step = 1.0f / fps;
    int max_frames = 1;
    float physics_time = 0.0f;

    float contact_sep = 0.0625f;
    uint32_t iterations = 4;
    uint32_t substeps = 4;
    float sub_dt = physics_step / substeps;

    //

    vec3 gravity_aspect = vec3(1, 1, 1);
    pvec3 gravity_center = vec3(0.0f);
    mat3 gravity_orientation = identity<mat3>();
    float gravity = 19.62f;
    bool do_DOF = true;
    bool do_dampening = true;

    //

    Collision_table collision_table;

    std::vector<Collision_constraint> collision_constraints;
    std::vector<Constraint> constraints;
    std::vector<DOF_constraint> dof_constraints;

    std::vector<Debug_point> debug_points;

    //

    Physics_system() {
        Signature s = ecs.update_signature<Collider>();
        ecs.update_signature<Transform>(s);
        collectors.push_back(Collector(s));

        std::vector<ivec3> vs = {
            ivec3(-1, -1, -1),
            ivec3(0, -1, -1),
            ivec3(1, -1, -1),
            ivec3(-1, 0, -1),
            ivec3(0, 0, -1),
            ivec3(1, 0, -1),
            ivec3(-1, 1, -1),
            ivec3(0, 1, -1),
            ivec3(1, 1, -1),

            ivec3(-1, -1, 0),
            ivec3(0, -1, 0),
            ivec3(1, -1, 0),
            ivec3(-1, 0, 0),
            //ivec3(0, 0, 0),
            ivec3(1, 0, 0),
            ivec3(-1, 1, 0),
            ivec3(0, 1, 0),
            ivec3(1, 1, 0),
            
            ivec3(-1, -1, 1),
            ivec3(0, -1, 1),
            ivec3(1, -1, 1),
            ivec3(-1, 0, 1),
            ivec3(0, 0, 1),
            ivec3(1, 0, 1),
            ivec3(-1, 1, 1),
            ivec3(0, 1, 1),
            ivec3(1, 1, 1),
        };

        //

        for(ivec3 v : vs) {
            vec3 normal = normalize(vec3(v));
            
            mat3 o1 = rotate_to(vec3(1.0f, 0.0f, 0.0f), normal, vec3(0.0f, 0.0f, 1.0f));
            vec3 n = o1[0];
            mat3 o2 = rotate_to(n, normal);

            mat3 ori = o2 * o1;

            directional.emplace(v, ori);
        }
    }

    uint32_t loop_num = 0;

    std::vector<vec3> tris = {vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)};

    std::vector<uint64_t> broad_phase();

    static std::vector<shape_face> triangulate_merge(std::vector<vec3> vertices);

    static void create_bounding_box(Collider& collider);
    static void create_bounding_box(Temporary_collider& collider);
    static Bounding_box create_bounding_box(std::vector<vec3>& vertices);
    //Bounding_box create_bounding_box(Collision_shape& collider, Transform& transform);

    static std::vector<Return_point> collision(Transform& ta, Convex_collider& ca, Transform& tb, Convex_collider& cb, Return_tag& tag);
    static std::vector<Return_point> collision(Transform& ta, Collider& ca, Transform& tb, Collider& cb);

    static bool GJK(Convex_collider& ca, Transform& ta, Convex_collider& cb, Transform& tb);
    static bool GJK(Temporary_collider& a, Temporary_collider& b);
    static std::vector<uint32_t> GJK_BVH(Collider& ca, Transform& ta, Collider& cb, Convex_collider& ccb, Transform& tb);

    static Manifold create_manifold(std::vector<Return_point> contacts, uint32_t a, uint32_t b, Collider& ca, Transform& ta, Collider& cb, Transform& tb);

    static bool collision(Transform& ta, Bounding_box& a, Transform& tb, Bounding_box& b);

    void insert_collision(Manifold& data);

    void prune_manifolds();
    
    void erase_collisions(uint32_t shape);

    void merge_manifolds(Manifold& a, Manifold& b);
    Contact_point get_points(Collision_data& data, uint32_t a, uint32_t b);

    static bool contains(std::vector<vec3> points, vec3 radius, vec3 point);
    static float contains_dist(std::vector<vec3> points, vec3 radius, vec3 point);

    static std::pair<mat3, vec3> calculate_inertia_tensor(std::vector<vec3> points, vec3 radius, float mass);
    static std::pair<mat3, vec3> calculate_inertia_tensor(std::vector<vec3> points, vec3 radius, float& mass, float density);
    static std::pair<mat3, vec3> calculate_inertia_tensor_volume(std::vector<vec3> points, vec3 radius, float& volume);
    
    static std::pair<mat3, vec3> calculate_inertia_tensor_flat(std::vector<vec3> points, float mass);
    static std::pair<mat3, vec3> calculate_inertia_tensor_flat(std::vector<vec3> points, float thickness, float& mass, float density);
    static std::pair<mat3, vec3> calculate_inertia_tensor_flat_volume(std::vector<vec3> points, float thickness, float& volume);
    
    static std::pair<mat3, vec3> calculate_M(std::vector<vec3> points, vec3 radius, float& volume);
    static std::pair<mat3, vec3> calculate_M_flat(std::vector<vec3> points, float thickness, float& volume);
    static mat3 inertia_tensor(mat3 M);
    
    static mat3 translate_inertia_tensor(vec3 delta, mat3 inertia_tensor, float mass);

    static mat3 translate_inertia_tensor_inverse(vec3 delta, mat3 inertia_tensor, float mass);

    static mat3 add_inertia_tensor(mat3 a, mat3 b);

    static void initialize_collision_shape(Collision_shape& shape);
    static vec3 initialize_collider(Collider& collider);
    
    static void initialize_collision_shape(Collision_shape& shape, float density);
    static vec3 initialize_collider(Collider& collider, std::vector<float> density);

    void integrate();
    void compute_velocities();
    void velocity_solve(std::vector<Collision_constraint>& collision_constraints);
    //void position_solve(std::vector<Collision_constraint>& collision_constraints);
    
    void apply_position(Collider* ca, Transform* ta, vec3 delta_pos, vec3 rel_pos);
    void apply_rotation(Collider* c, Transform* t, vec3 delta);

    void physics_loop();

    void call();

    float shape_cast(Collider& shape, mat3 orientation, pvec3 start, vec3 direction, float step, uint32_t* hit = nullptr, vec3* normal = nullptr);

    bool raycast(pvec3 start, vec3 direction, float step, float dist, std::unordered_set<uint32_t>& mask, uint32_t* hit = nullptr, uint32_t* shape_hit = nullptr, vec3* normal = nullptr, pvec3* point = nullptr, float inflate = 0.0f, vec3 axis1 = vec3(0.0f), vec3 axis2 = vec3(0.0f));
    static bool raycast(pvec3 start, vec3 direction, float step, float dist, std::vector<Temporary_collider>& colliders, uint32_t* hit = nullptr, vec3* normal = nullptr, pvec3* point = nullptr);
};

float calculate_inertia(Collider* c, Transform* t, vec3 dir, vec3 point);
float calculate_inertia(Collider* c, Transform* t, vec3 dir);

std::vector<uint32_t> triangulate(std::vector<vec3> vertices);
void create_mesh_from_collider(uint32_t entity, vec3 color = vec3(0.3f));
void create_mesh_from_vertices(uint32_t entity, vec3 color, std::vector<vec3>& vertices);
void modify_mesh_from_vertices(uint32_t entity, vec3 color, std::vector<vec3>& vertices);
void create_cube_mesh(uint32_t entity, vec3 size, vec3 color = vec3(0.3f), ivec4 range = ivec4(16, 128, 16, 16), float scale = 1.0f);
void create_cube_mesh(uint32_t entity, vec3 size, vec3 color, std::vector<ivec4> range, std::shared_ptr<Texture> texture);
void create_sphere_mesh(uint32_t entity, vec3 size, vec3 color = vec3(0.3f));
void create_torus_mesh(uint32_t entity, vec4 size, ivec2 num_quads, vec3 color = vec3(0.3f));

std::vector<uint32_t> convex_hull(std::vector<vec2> points);
std::vector<vec2> convex_hull2(std::vector<vec2> points);
glm::vec3 project_clamp(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 point);

Bounding_box transform_bb(Bounding_box b, vec3 pos, mat3 ori);