#include <ui/widgets/debug_widget.hpp>
#include <ui/system.hpp>

#include <GLFW/glfw3.h>

namespace axiom {

void debug_widget::mesh() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    if(dirty) {
        auto& pw = ui_system.widgets[parent];

        std::vector<ui_vertex> total_ret;

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        vec4 range = ui_system.get_range(self);

        // panel
        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * size, z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(color, 1.0f);
            v.data = 1;

            //v.range = range;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        vertices_before = total_ret;
        dirty = false;
    }
}

uint64_t debug_widget::insert(vec2 size, float max_width, vec3 color) {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    debug_widget widget;
    widget.size = size;
    widget.color = color;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    widget.min_width = size.x;
    widget.max_width = max_width;
    widget.min_height = size.y;
    widget.max_height = size.y;

    return ui_system.insert_widget(widget);
}

}