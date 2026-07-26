#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct message_widget : widget {
    vec3 color;
    vec2 border;
    ulong timestamp;
    std::string sender;

    float tail_size = 0.0f;
    uint tail_settings = 0;

    bool hover = false;
    bool inserted = false;
    //bool tail = false;

    void handle_inputs();
    void mesh();
    void get_y();
    void set_str(std::string str);
    void init();

    axiom::capture_data handle_capture();

    static ulong insert(std::string sender, ulong timestamp, std::string message, axiom::text_alignment alg, vec2 width, vec3 color, vec2 border, uint tail_settings = 0);
};

}