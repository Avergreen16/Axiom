#pragma once

#include "ecs.hpp"

struct Animation_frame {
    std::vector<ivec4> regions;
    float duration;
};

struct Animation_bb {
    std::vector<Animation_frame> frames;
    uint32_t type = 0;
};

struct Billboard_animation {
    std::shared_ptr<Texture> texture;
    std::vector<Animation_bb> animations;
    float current_animation = 0;
    uint32_t current_frame = 0;
    float current_time = 0.0f;

    vec2 size;
    float height = 0.0f;
    bool use_axis = true;

    float sway_dist = 0.0f;
    float sway_period = 0.0f;
    float sway_offset = 0.0f;

    void set_animation(uint32_t id);
};