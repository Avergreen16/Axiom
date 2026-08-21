#include <ui/checkbox_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void checkbox_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    if(includes(ui_system.window->cursor_pos, vec4(position, position + size)) && ui_system.hover_capture == self && (ui_system.click_capture == NULL_WIDGET || ui_system.click_capture == self)) {
        hovered = true;
        if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
           checked = !checked;
        }
    } else {
        hovered = false;
    }

    if(hovered) ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;
    
    callback(*this);
}

void checkbox_widget::mesh() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    if(dirty) {
        vec4 view_range = ui_system.get_range(self);

        dirty = false;

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        // panel
        float border = 3.0f;
        std::vector<vec4> ranges = {
            vec4(2.0f, 2.0f, size.x - 4.0f, size.y - 4.0f),
            vec4(0.0f, 0.0f, 1.0f, size.y),
            vec4(size.x - 1, 0.0f, 1.0f, size.y),
            vec4(0.0f, 0.0f, size.y, 1.0f),
            vec4(0.0f, size.y - 1, size.x, 1.0f),
        };
        std::vector<vec4> colors = {
            vec4(color, 0.0f),
            vec4(color, 1.0f),
            vec4(color, 1.0f),
            vec4(color, 1.0f),
            vec4(color, 1.0f),
        };

        if(checked) colors[0] = vec4(color, 1.0f);
        
        vertices_before.clear();

        for(int i = 0; i < ranges.size(); ++i) {
            std::vector<ui_vertex> ret = {a, b, d, a, d, c};
            vec4 range = ranges[i];
            vec4 color = colors[i];

            for(ui_vertex& v : ret) {
                v.pos = vec3(position + range.xy() + v.pos.xy() * range.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = color;
                v.data = 0x1;

                v.range = view_range;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }

        icon_size = icon.zw();

        if(icon.z != 0.0f) {
            std::vector<ui_vertex> ret = {a, b, d, a, d, c};
            vec2 pos = position + (size - icon_size) * 0.5f;
            pos = round(pos);

            for(ui_vertex& v : ret) {
                v.pos = vec3(pos + v.pos.xy() * icon_size, z);
                v.tex_pos = v.tex_pos * icon.zw() + icon.xy();
                v.color = vec4(1.0f);
                v.data = 0x1;
                
                v.range = view_range;
            }

            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }
        
        std::vector<ui_vertex> text_vertices;
        vec2 text_pos;
        if(text.size()) {
            text_vertices = text[0]->mesh();
            text_pos = position + (size - text[0]->size) * 0.5f;
            
            std::vector<ui_vertex> vs = text_vertices;
            for(ui_vertex& v : vs) {
                v.pos += vec3(round(text_pos), z);
                
                v.range = view_range;
            }
            vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
        }
    }
}

void checkbox_widget::init() {
    
}

uint64_t checkbox_widget::insert(vec2 size, vec3 color, bool checked, std::function<void(checkbox_widget&)> callback) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    checkbox_widget widget;

    widget.checked = checked;

    //
    
    widget.layout_mode = axiom::layout_mode::NONE;
    widget.buffer = ui_system.input_state.active_buffer;
    widget.position_mode = ui_system.input_state.active_position;
    widget.callback = callback;

    widget.color = color;

    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;

    return ui_system.insert_widget(widget);
}

capture_data checkbox_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(ui_system.window->cursor_pos, range)) return {self, z, true};
    }

    return {self, z, false};
}

}