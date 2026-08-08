#pragma once

#include <include/math.hpp>

#include <set>

namespace axiom {

/*
struct clipping_plane2d {
    vec2 origin;
    vec2 normal;
};
*/

struct vertex_element3d {
    vec3 center;
    vec3 radii = vec3(0.0f);
    mat3 orientation = glm::identity<mat3>();

    //std::vector<clipping_plane2d> planes;
};

/*
struct bounding_box2d {
    vec2 minimum = vec2(FLT_MAX, FLT_MAX);
    vec2 maximum = vec2(-FLT_MAX, -FLT_MAX);
};

struct BVH_node2d {
    axiom::bounding_box2d bounding_box;
    std::vector<uint> children;
};

struct collision_shape2d {
    axiom::bounding_box2d bounding_box;

    //

    std::vector<vertex_element2d> vertices;
    vec2 radius = vec2(0.0f);

    float mass;
    float inertia;

    //

    vec2 position = vec2(0.0f);
    mat2 orientation = glm::identity<glm::mat2>();
};

struct collider2d {
    bounding_box2d bounding_box;
    std::vector<BVH_node2d> BVH;

    //

    std::vector<collision_shape2d> shapes;

    float mass;
    float inertia = FLT_MAX;

    vec2 velocity = vec2(0.0f);
    float angular_velocity = 0.0f;
    
    vec2 beta_velocity = vec2(0.0f);
    float beta_angular_velocity = 0.0f;

    //

    std::set<uint> non_colliding;

    bool allow_gravity = true;
    bool allow_rotation = true;
    bool is_static = false;

    std::vector<uint> colliding_with;
    std::vector<vec2> colliding_normal;

    //

    void create_bounding_box();
    void create_BVH();
};
*/

vec3 support(vec3 direction, vec3 center, mat3 orientation, vec3 radii);
vec3 support(vec3 direction, std::vector<vertex_element3d> ellipsoids);

}