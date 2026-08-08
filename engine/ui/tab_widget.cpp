#include <ui/tab_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void tab_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    float pos = 0.0f;
    
    uint32_t hovered = 0xFFFFFFFF;
    uint32_t i = 0;
    for(tab& tab : tabs) {
        vec4 region = vec4(position + vec2(pos, size.y - tab_height), tab.width, tab_height);

        if(includes(ui_system.window->cursor_pos, vec4(region.xy(), region.xy() + region.zw()))) {
            hovered = i;
            ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;
        }
        
        pos += tab.width + tab_sep;
        ++i;
    }

    if(ui_system.click_capture == self) {
        bool t = false;
        
        uint prev_selected = selected;

        if(hovered != 0xFFFFFFFF) {
            if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                if(selected != hovered) t = true;
                selected = hovered;
            }
        }

        if(t) {
            tabs[prev_selected].swap_out(this);

            //ui_system.erase(ui_system.get_children(self));
            //children.clear();

            uint64_t prev_cw = ui_system.input_state.current_widget;
            auto prev_pos = ui_system.input_state.active_position;
            vec4 prev_buffer = ui_system.input_state.active_buffer;
            
            ui_system.input_set(self);
            tabs[selected].swap_in(this);

            //ui_system.input_set(prev_cw);
            //ui_system.input_state.active_position = prev_pos;
            //ui_system.input_state.active_buffer = prev_buffer;
        }
    }
}

void tab_widget::mesh() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    if(dirty) {
        vec4 view_range = ui_system.get_range(self);

        vertices_before.clear();
        
        dirty = false;

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        vec3 color;
        float pos = 0.0f;
        uint i = 0;
        for(tab& tab : tabs) {
            auto& tab_text = text[i];
            std::vector<ui_vertex> text_vertices = tab_text->mesh();
            
            vec4 base_color = vec4(tab.color, 1.0f);
            if(i == selected) color = tab.color;
            //else base_color = vec4(tab.color * ((i % 2 == 0) ? 0.75f : 0.65f), 1.0f);
            else base_color = vec4(tab.color * 0.7f, 1.0f);

            /*
            std::vector<vec4> ranges = {
                vec4(0.0f, 0.0f, tab.width, tab_height),
                vec4(2.0f, 0.0f, tab.width - 4.0f, tab_height - 2.0f),
            };
            std::vector<vec4> colors = {
                base_color + 0.1f,
                base_color,
            };
            */
            std::vector<vec4> ranges = {
                vec4(0.0f, 0.0f, tab.width, tab_height),
            };
            std::vector<vec4> colors = {
                base_color,
            };

            std::vector<ui_vertex> ui_vertices = text[i]->mesh();
            vec2 text_pos = position + vec2(pos, size.y - tab_height) + (vec2(tab.width, tab_height) - text[i]->size) * 0.5f;

            for(int i = 0; i < ranges.size(); ++i) {
                std::vector<ui_vertex> ret = {a, b, d, a, d, c};
                vec4 range = ranges[i];
                vec4 color = colors[i];

                for(ui_vertex& v : ret) {
                    v.pos = vec3(position + vec2(pos, size.y - tab_height) + range.xy() + v.pos.xy() * range.zw(), z);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = color;
                    v.data = 0x1;

                    v.range = view_range;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            }
            
            std::vector<ui_vertex> vs = ui_vertices;
            for(ui_vertex& v : vs) {
                v.pos += vec3(round(text_pos), 0.0f);

                v.range = view_range;
            }
            vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());

            pos += tab.width + tab_sep;
            ++i;
        }

        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        vec4 range = vec4(0.0f, 0.0f, size.x, size.y - tab_height);
        vec4 col = vec4(color, 1.0f);

        for(ui_vertex& v : ret) {
            v.pos = vec3(position + range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = col;
            v.data = 0x1;

            v.range = view_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
    }
}

void tab_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        vec4 range = vec4(position, size - vec2(0.0f, tab_height));

        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = ui_system->widgets[children[i]];

            p0->size.x = range.z - p0->buffer.x - p0->buffer.z;

            p0->size.y = range.w - p0->buffer.y - p0->buffer.w;
            
            p0->position.x = range.x - p0->buffer.x;

            p0->position.y = range.y - p0->buffer.y;
        }
    };
    before.push_back(c);
}

uint64_t tab_widget::insert(float tab_height, float tab_sep, std::vector<tab> tabs) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    tab_widget widget;
    
    widget.buffer = ui_system.input_state.active_buffer;
    widget.layout_mode = axiom::layout_mode::VOID;
    widget.position_mode = ui_system.input_state.active_position;
    widget.tabs = tabs;
    widget.tab_height = tab_height;
    widget.tab_sep = tab_sep;

    for(tab& t : widget.tabs) {
        std::shared_ptr<axiom::text> text(new axiom::text);
        text->font = ui_system.font_assets[0];
        text->string = t.label;
        text->wrap = false;

        widget.text.push_back(text);
    }

    uint selected = widget.selected;

    uint64_t w = ui_system.insert_widget(widget);

    uint64_t prev_cw = ui_system.input_state.current_widget;
    auto prev_pos = ui_system.input_state.active_position;
    vec4 prev_buffer = ui_system.input_state.active_buffer;
    
    ui_system.input_set(w);
    ((tab_widget*)ui_system.widgets[w].get())->tabs[selected].swap_in(dynamic_cast<tab_widget*>(ui_system.widgets[w].get()));
    
    ui_system.input_set(prev_cw);
    ui_system.input_state.active_position = prev_pos;
    ui_system.input_state.active_buffer = prev_buffer;

    return w;
}

capture_data tab_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    float pos = 0.0f;
    for(tab& tab : tabs) {
        vec4 range = vec4(position + vec2(pos, size.y - tab_height), tab.width, tab_height);

        if(includes(ui_system.window->cursor_pos, vec4(range.xy(), range.xy() + range.zw()))) {
            return {self, z, true};
        }

        pos += tab.width + tab_sep;
    }

    return {self, z, false};
}

}