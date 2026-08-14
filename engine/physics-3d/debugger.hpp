#pragma once
#include <include/math.hpp>

#include <deque>

namespace axiom {

struct gjk_step {
    vec3 input_point;
    vec3 search_direction;
    vec3 search_origin;
    
    uint erase_index = 0xFFFFFFFF;
};

struct epa_step {
    vec3 input_point;
    vec3 search_direction;
    vec3 search_origin;

    std::vector<uint> erase_triangles;
};

struct collision_event {
    ulong frame;
    uint collider_a;
    uint collider_b;
    uint shape_a;
    uint shape_b;

    std::vector<gjk_step> gjk;
    std::vector<epa_step> epa;
};

struct debugger_frame {
    std::vector<collision_event> collision_events;
};

struct physics_debugger {
    std::unordered_map<ulong, debugger_frame> frames;
    std::deque<ulong> frame_list;
    uint frame_buffer = 32;

    void insert_frame(ulong id, debugger_frame frame);
    std::vector<uint> search_collision_collider(ulong frame, uint collider);
    std::vector<uint> search_collision_collider(ulong frame, uint collider_a, uint collider_b);
};

}