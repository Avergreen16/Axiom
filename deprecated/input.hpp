#pragma once

#include "ecs.hpp"
#include "physics.hpp"
#include "particles.hpp"

vec3 get_color(float a);
vec3 get_color_hsv(float h, float s, float v);
std::shared_ptr<Texture> create_image_texture(std::string path, ivec4 range = ivec4(0, 0, 0x7FFFFFFF, 0x7FFFFFFF), vec3 color = vec3(1.0f));

enum Camera_mode{CAMERA_FREECAM, CAMERA_PLAYER};

struct Input_system : System {
    uint32_t player_camera;
    uint32_t player_collider = NULL_ENTITY;
    Camera_mode camera_mode = CAMERA_FREECAM;

    vec4 view_range = vec4(0.0f);

    std::unordered_set<uint32_t> raycast_mask;

    vec2 character_size = vec2(1.0f, 3.5625f);
    
    bool debug_mode = false;
    
    float debug_speed = 3.5f;
    
    float light_factor = 1.0f;

    bool gui_captured = false;

    //

    bool text_capture = false;

    uint32_t pic = 0;
    uint32_t id = 0xFFFFFFFF;
    
    uint32_t held_object = NULL_ENTITY;
    uint32_t held_constraint = NULL_ENTITY;
    float held_dist = 0.0f;
    float held_force = 10.0f;

    uint32_t object_target = NULL_ENTITY;
    bool select_target = false;

    int num_links = 8;
    float object_scale = 1.0f;
    float link_length = 1.0f;
    float link_width = 0.33f;
    ivec3 num_cubes = ivec3(4);

    //

    Input_system();

    void create_player();

    void call();

    void set_camera_mode(Camera_mode mode);
};

void create_chain();

void create_capsule();

void summon_character();

void create_crates();

void summon_statue();

void create_platform();