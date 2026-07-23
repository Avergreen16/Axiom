#include <ui/button_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void button_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    pressed = false;

    if(includes(ui_system.window->cursor_pos, vec4(position, position + size)) && ui_system.hover_capture == self && (ui_system.click_capture == NULL_WIDGET || ui_system.click_capture == self)) {
        hovered = true;
        if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
            pressed = true;
            held = true;
        }
    } else {
        hovered = false;
    }

    if(!ui_system.window->input_map[axiom::input_code::MOUSE_LEFT]) held = false;

    if(hovered) ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;
    
    callback(*this);
}

void button_widget::mesh() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    if(dirty) {
        vec4 view_range = ui_system.get_range(self);

        std::vector<ui_vertex> text_vertices = text[0]->mesh();

        dirty = false;

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        vec4 base_color = vec4(color, 1.0f);
        if(hovered) base_color = vec4(color + 0.25f, 1.0f);
        if(held) base_color = vec4(color * 0.75f, 1.0f);

        // panel
        float border = 3.0f;
        std::vector<vec4> ranges = {
            vec4(0.0f, 0.0f, size.x, size.y),
        };
        std::vector<vec4> colors = {
            vec4(base_color.xyz(), 1.0f),
        };
        vec2 text_pos = position + (size - text[0]->size) * 0.5f;
        
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

        if(icon.x != 0.0f) {
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

        std::vector<ui_vertex> vs = text_vertices;
        for(ui_vertex& v : vs) {
            v.pos += vec3(round(text_pos), z);
            
            v.range = view_range;
        }
        vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
    }
}

void button_widget::init() {
    
}

uint64_t button_widget::insert(vec2 size, vec3 color, std::string str, std::function<void(button_widget&)> callback) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    button_widget widget;

    std::shared_ptr<axiom::text> text(new axiom::text);
    text->string = str;
    text->wrap = false;
    text->font = ui_system.font_assets[0];

    widget.text = {text};

    //
    
    widget.layout_mode = axiom::layout_mode::VOID;
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

capture_data button_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(ui_system.window->cursor_pos, range)) return {z, true};
    }

    return {z, false};
}

}