#include <ui/slider_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void slider_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    if(ui_system.hover_capture == self && ui_system.click_capture == NULL_WIDGET || ui_system.click_capture == self) {
        ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_L;
        hovered = true;
    } else hovered = false;
    
    if(ui_system.click_capture == self) {
        pressed = true;

        float p = ui_system.window->cursor_pos.x - position.x - slider_width * 0.5f;

        float frac = p / (size.x - slider_width);
        frac = glm::clamp(frac, 0.0f, 1.0f);

        float value = frac * (range.y - range.x);
        if(step != 0.0f) value = round(value / step) * step;
        value += range.x;

        current_value = value;
    } else pressed = false;
    
    callback(*this);
}

void slider_widget::mesh() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    if(dirty) {
        dirty = false;

        std::vector<ui_vertex> text_vertices = text[0]->mesh();

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        float slider_pos = (size.x - slider_width) * ((current_value - range.x) / (range.y - range.x));
        
        vec4 base_color = vec4(color, 1.0f);
        if(hovered || pressed) base_color = vec4(color + 0.25f, 1.0f);
        // panel
        float border = 3.0f;
        std::vector<vec4> ranges = {
            vec4(0.0f, border, size.x, size.y - border * 2.0f),
            vec4(slider_pos, 0.0f, slider_width, size.y),
        };
        std::vector<vec4> colors = {
            vec4(0.0f, 0.0f, 0.0f, 0.5f),
            base_color,
        };

        //
        
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
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }

        vec2 text_pos = position + (size - text[0]->size) * 0.5f;

        std::vector<ui_vertex> vs = text_vertices;
        for(ui_vertex& v : vs) {
            v.pos += vec3(round(text_pos), 0.0f);
        }
        vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
    }
}

void slider_widget::init() {
    //ui_system* ui_system = &ecs.get_system<ui_system>();
}

uint64_t slider_widget::insert(vec2 size, float slider_width, vec3 color, vec2 range, float step, float start, std::string str, std::function<void(slider_widget&)> callback)  {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    slider_widget widget;

    std::shared_ptr<axiom::text> text(new axiom::text);
    text->string = str;
    text->wrap = false;
    text->font = ui_system.font_assets[0];

    widget.text = {text};

    //
    
    widget.layout_mode = axiom::layout_mode::NONE;
    widget.buffer = ui_system.input_state.active_buffer;
    widget.position_mode = ui_system.input_state.active_position;
    widget.callback = callback;
    widget.label = str;
    widget.color = color;

    widget.range = range;
    widget.step = step;
    widget.current_value = start;
    widget.slider_width = slider_width;

    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;

    return ui_system.insert_widget(widget);
}

capture_data slider_widget::handle_capture() {
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