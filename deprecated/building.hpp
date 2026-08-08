#pragma once

#include "ecs.hpp"
#include "physics.hpp"
#include "core.hpp"

enum building_mode{BUILDING_MODE_PLACE, BUILDING_MODE_SIZE_BASE, BUILDING_MODE_SIZE_HEIGHT};

vec3 random_color(float seed);

struct Primitive {
    std::vector<vec3> vertices;
    std::vector<uint32_t> triangles;
    std::vector<shape_face> faces;
    mat3 M;
    vec3 center_of_mass;
    float volume;
};

struct Brick {
    uint32_t primitive_id;
    ivec3 position;
    ivec3 size;
    vec3 color;

    ivec3 orientation_x;
    ivec3 orientation_y;
    ivec3 orientation_z;
};

struct Build {
    vec3 center_of_mass = vec3(0.0f);
    std::vector<Brick> bricks;
};

struct Building_system : System {
    std::vector<Primitive> primitives;

    bool building_active = false;
    building_mode mode = BUILDING_MODE_PLACE;

    uint32_t building_reference = NULL_ENTITY;
    uint32_t building_entity = NULL_ENTITY;
    uint32_t active_primitive = 0;
    ivec3 active_size;
    ivec3 active_offset;
    vec3 axis;

    ivec3 reference_pos;
    vec3 rel_origin;
    vec3 rel_normal;
    float dist;

    vec3 ori_x;
    vec3 ori_y;
    vec3 ori_z;

    bool axis_lock = true;
    bool reset_b = false;

    vec3 current_color = random_color(core.random()) * 0.65f + 0.35f;

    Building_system();

    void mesh_primitive(uint32_t entity, vec3 color, std::vector<vec3> vertices, std::vector<uint32_t> indices);

    void call();

    void mesh_build(uint32_t entity);

    void create_build_collider(uint32_t entity);
};