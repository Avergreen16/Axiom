#include <ui/widget_base.hpp>

namespace axiom {

struct window_widget : widget {
    vec3 header_color;

    uint header = 24;
    uint resize_border = 6;
    float shadow_width = 6;
    std::string label;

    bool hover_close = false;
    uint operation = NULL_OPERATION;

    void handle_inputs();
    void mesh();
    void init();
    capture_data handle_capture();

    static ulong insert(std::string label, ivec2 size, ivec2 position, vec3 color);
};

}