#include <ui/panel_widget.hpp>

#include <GLFW/glfw3.h>

namespace axiom {
    
void panel_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        float scrollable = 0.0f;
        if(children.size()) {
            scrollable = ui_system->widgets[children[0]]->size.y + ui_system->widgets[children[0]]->buffer.y + ui_system->widgets[children[0]]->buffer.w - size.y;
        }

        //
        
        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = ui_system->widgets[children[i]];

            p0->size.x = size.x - (p0->buffer.x + p0->buffer.z);

            p0->size.y = size.y - (p0->buffer.y + p0->buffer.w);
            
            p0->position.x = position.x + p0->buffer.x;

            p0->position.y = position.y + p0->buffer.y;
        }
        
        view_range = vec4(position, position + size);
    };
    before.push_back(c);
    
    c.func = [this, ui_system]() {
        view_range = vec4(position, position + size);
    };
    after.push_back(c);
}


void panel_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    float height = size.y;

    float new_total_scrollable = 0.0f;
    if(children.size()) {
        new_total_scrollable = ui_system.widgets[children[0]]->size.y - height + ui_system.widgets[children[0]]->buffer.y + ui_system.widgets[children[0]]->buffer.w;
    }
}

void panel_widget::mesh() {
    if(dirty) {
        dirty = false;
        
        std::vector<ui_vertex> ret;
        std::vector<ui_vertex> total_ret;

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        bool scrollbar = false;
        float scrollbar_height;
        float scrollbar_pos;

        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(position + v.pos.xy() * size, z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.125f, 0.125f, 0.125f, 1.0f);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        vertices_before = total_ret;
    }
}

uint64_t panel_widget::insert() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    panel_widget widget;
    widget.position_mode = axiom::position_mode::BOTTOM_LEFT;
    widget.layout_mode = axiom::layout_mode::NONE;

    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = FLT_MAX;
    widget.buffer = ui_system.input_state.active_buffer;

    widget.flag = true;

    widget.size = vec2(0.0f);
    widget.position = vec2(0.0f);

    return ui_system.insert_widget(widget, true);
}

capture_data panel_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    if(includes(ui_system.window->cursor_pos, ranges[0])) return {self, z, true, true};

    return {self, z, false};
}

}