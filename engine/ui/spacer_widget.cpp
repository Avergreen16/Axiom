#include <ui/spacer_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

ulong spacer_widget::insert(vec2 min_size, vec2 max_size, bool visual, vec4 color, bool step) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    spacer_widget widget;
    widget.min_width = min_size.x;
    widget.max_width = max_size.x;
    widget.min_height = min_size.y;
    widget.max_height = max_size.y;
    widget.weight_width = 0.001f;
    widget.weight_height = 0.001f;
    widget.size = vec2(0.0f);

    widget.visual = visual;
    widget.color = color;

    if(widget.min_width == widget.max_width) widget.size.x = widget.min_width;
    if(widget.min_height == widget.max_height) widget.size.y = widget.min_height;
    
    widget.layout_mode = axiom::layout_mode::NONE;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget, step);
}

void spacer_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;

    // target and weights for x and y
    c.func = [this, ui_system]() {
        if(children.size()) {
            float target_min = 0.0f;
            float target_max = 0.0f;
            float target_weight = 0.0f;
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

                if(c0->min_width != c0->max_width) {
                    target_min = glm::max(target_min, c0->min_width + c0->buffer.x + c0->buffer.z);
                    target_max = glm::max(target_max, c0->max_width + c0->buffer.x + c0->buffer.z);
                    target_weight = glm::max(target_weight, c0->weight_width);
                } else {
                    target_min = glm::max(target_min, c0->size.x + c0->buffer.x + c0->buffer.z);
                    target_max = glm::max(target_max, c0->size.x + c0->buffer.x + c0->buffer.z);
                    target_weight = glm::max(target_weight, 1.0f);
                }
            }

            min_width = target_min;
            max_width = target_max;
            weight_width = target_weight;

            //

            target_min = 0.0f;
            target_max = 0.0f;
            target_weight = 0.0f;
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

                if(c0->min_width != c0->max_width) {
                    target_min = glm::max(target_min, c0->min_height + c0->buffer.y + c0->buffer.w);
                    target_max = glm::max(target_max, c0->max_height + c0->buffer.y + c0->buffer.w);
                    target_weight = glm::max(target_weight, c0->weight_height);
                } else {
                    target_min = glm::max(target_min, c0->size.y + c0->buffer.y + c0->buffer.w);
                    target_max = glm::max(target_max, c0->size.y + c0->buffer.y + c0->buffer.w);
                    target_weight = glm::max(target_weight, 1.0f);
                }
            }

            min_height = target_min;
            max_height = target_max;
            weight_height = target_weight;
        }
    };
    after.push_back(c);

    c.func = [this, ui_system]() {
        float y_pos = position.y;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

            c0->position = position + c0->buffer.xy();
            c0->size = size - c0->buffer.xy() - c0->buffer.zw();
        }

    };
    after.push_back(c);
}

void spacer_widget::mesh() {
    if(dirty) {
        vertices_before.clear();
        dirty = false;

        if(visual) {
            if(color.w != 0.0f) {
                ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
                ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
                ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
                ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
                std::vector<ui_vertex> ret = {a, b, d, a, d, c};

                vec4 range = vec4(position, size);
                range = round(range);
                
                for(ui_vertex& v : ret) {
                    v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = vec4(1.0f, 1.0f, 1.0f, 0.5f);
                    v.data = 0x1;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            } else {
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

}