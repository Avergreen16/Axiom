#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct message_widget : widget {
    vec3 color;
    vec2 border;
    ulong timestamp;

    float tail_size = 0.0f;
    uint tail_settings = 0;
    //bool tail = false;

    void handle_inputs();
    void mesh();
    void get_y();
    void set_str(std::string str);
    void init();

    static ulong insert(std::string str, axiom::text_alignment alg, vec2 width, vec3 color, vec2 border, ulong timestamp, uint tail_settings = 0);
};

}