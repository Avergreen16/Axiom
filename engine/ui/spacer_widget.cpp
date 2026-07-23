#include <ui/spacer_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

uint64_t spacer_widget::insert(vec2 min_size, vec2 max_size, bool visual) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    spacer_widget widget;
    widget.min_width = min_size.x;
    widget.max_width = max_size.x;
    widget.min_height = min_size.y;
    widget.max_height = max_size.y;
    widget.weight_width = 0.001f;
    widget.weight_height = 0.001f;

    widget.visual = visual;

    if(widget.min_width == widget.max_width) widget.size.x = widget.min_width;
    if(widget.min_height == widget.max_height) widget.size.y = widget.min_height;
    
    widget.layout_mode = axiom::layout_mode::VOID;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget);
}

void spacer_widget::mesh() {
    if(dirty) {
        vertices_before.clear();
        dirty = false;

        if(visual) {
            ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
            ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
            ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
            ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
            std::vector<ui_vertex> ret = {a, b, d, a, d, c};

            vec4 range = vec4(position.x, position.y + size.y * 0.5f, size.x, 1.0f);
            range = round(range);
            
            for(ui_vertex& v : ret) {
                v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(1.0f, 1.0f, 1.0f, 0.5f);
                v.data = 0x1;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }
    }
}

}