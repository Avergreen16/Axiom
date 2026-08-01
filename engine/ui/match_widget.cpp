#include <ui/match_widget.hpp>
#include <ui/ui_system.hpp>
#include <ui/text_widget.hpp>

namespace axiom {

ulong match_widget::insert(vec4 color, bool border, vec4 buf) {
    axiom::ui_system &ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    match_widget widget;
    
    //widget.z = z;
    widget.border = border;
    widget.color = color;
    widget.buf = buf;

    widget.min_width = 0.0f;
    widget.max_width = 0.0f;
    widget.min_height = 0.0f;
    widget.max_height = 0.0f;
    widget.size = vec2(0.0f);

    return ui_system.insert_widget(widget);
}

void match_widget::mesh() {
    if(dirty) {
        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

        auto& parent_widget = ui_system.widgets[parent];

        vec4 range = vec4(parent_widget->position, parent_widget->size);

        if(border) range += vec4(-parent_widget->buffer.x, -parent_widget->buffer.y, parent_widget->buffer.x + parent_widget->buffer.z, parent_widget->buffer.y + parent_widget->buffer.w);
        else range += vec4(-buf.x, -buf.y, buf.x, buf.y);

        vec4 view_range = ui_system.get_range(self);

        vertices_before.clear();
        
        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        
        for(ui_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = color;
            v.data = 0x1;

            v.range = view_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
    }
}

void match_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        //
    };
    before.push_back(c);
}

}