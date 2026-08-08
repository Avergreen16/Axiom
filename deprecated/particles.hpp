#pragma once

#include "wrapper.hpp"
#include "pnum.hpp"
#include "ecs.hpp"

enum Settings_bits{S_ORIENT_BIT = 0x1, S_STABLE_XY_BIT = 0x2, S_STABLE_Z_BIT = 0x4, S_FADE_BIT = 0x8, S_CONSTANT_SIZE_BIT = 0x10, S_AXIS_BIT = 0x20, S_DEPTH_BIT = 0x40, S_NO_SHADOW_BIT = 0x80};

struct Particle {
    pvec3 position;

    glm::vec3 velocity;
    float gravity;

    double max_lifetime;
    double current_lifetime;

    glm::vec2 start_size;
    glm::vec2 end_size;

    glm::vec2 tex_coord;
    glm::vec2 tex_size;

    glm::vec4 start_color;
    glm::vec4 end_color;

    uint32_t settings;
    glm::vec3 orient_center;

    bool depth_test = true;
};

struct Star {
    Particle particle;
    float brightness;
};

struct Particle_data {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec2 size;
    glm::vec2 tex_coord;
    glm::vec2 tex_size;
    glm::vec3 orient_center;
    uint32_t settings;

    Particle_data(glm::vec3 pos_, glm::vec4 color_, glm::vec2 size_, glm::vec2 tex_coord_, glm::vec2 tex_size_, uint32_t settings_, glm::vec3 orient_center_ = {0, 0, 0}) {
        pos = pos_;
        color = color_;
        size = size_;
        tex_coord = tex_coord_;
        tex_size = tex_size_;
        settings = settings_;
        orient_center = orient_center_;
    }

    Particle_data() = default;
};

struct Particle_sort {
    uint32_t id;
    float z;

    bool operator<(const Particle_sort& p) const {
        return z < p.z;
    }
};

struct Particle_system : System {
    std::vector<Particle> particles;
    std::vector<Particle> icons;

    float scatter = 0.0f;
    std::vector<Star> stars;
    std::vector<vec4> star_sprites = {
        vec4(0, 32, 1, 1),
        vec4(1, 32, 3, 3),
        vec4(4, 32, 5, 5)
    };
    
    std::vector<vec3> ps;
    pvec3 rel_pos;

    std::vector<vec3> ps_vertices;
    std::vector<uint32_t> ps_indices;

    ivec3 pos_ = vec3(__FLT_MAX__, 0, 0);

    Particle_system();

    void insert_icon(pvec3 pos, vec4 color, vec2 size, vec4 tex_range, uint32_t settings);
    void insert_particle(pvec3 pos, float lifetime, vec4 start_color, vec4 end_color, vec2 start_size, vec2 end_size, bool gravity, vec4 tex_range, uint32_t settings, vec3 velocity);
    void insert_star(pvec3 pos, vec4 color, float brightness);

    void call();
};