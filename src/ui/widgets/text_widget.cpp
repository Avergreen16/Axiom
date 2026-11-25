#include <ui/widgets/text_widget.hpp>
#include <ui/system.hpp>

namespace axiom {

void text_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
    //vec4 range = ui_system.get_range(self);

    std::string str = text.string;
    str = callback(str);

    text.clip = clip;
    text.string = str;
}

void text_widget::mesh() {
    if(dirty) {
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

        dirty = false;

        //text.width = size.x;
        std::vector<ui_vertex> text_vertices = text.mesh();

        //std::cout << select_vertices.size() << "\n";

        //if(abs(text.position.y - position.y) > 1.0f) std::cout << "ERROR: TEXT MISALIGNED (text_widget.cpp) -> " << text.position.y - position.y << " (" << text.string << ")\n"; 

        for(ui_vertex& v : text_vertices) {
            v.pos.x += text.position.x;
            v.pos.y += text.position.y;
            v.pos.z = z;

            v.clip_space = clip;
        }

        vertices_before = text_vertices;

        //

        /*
        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        vec4 panel_range = vec4(position, size);
        
        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        
        for(ui_vertex& v : ret) {
            v.pos = vec3(panel_range.xy() + v.pos.xy() * panel_range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(1.0f, 0.0f, 0.0f, 0.5f);
            v.data = 0x1;

            v.range = view_range;
        }
        vertices_before.insert(vertices_before.begin(), ret.begin(), ret.end());
        */
    }
}

void text_widget::get_y() {
    //text.refresh();
}

void text_widget::set_str(std::string str) {
    /*
    if(text.string != str) {
        text.dirty = true;
        text.string = str;
        dirty = true;

        get_y();
    }
    */
}

uint64_t text_widget::insert(std::string str, axiom::text_alignment alg, bool wrap, std::function<std::string(std::string)> callback) {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
    
    text_widget widget;

    //
    
    widget.text.string = str;
    widget.text.wrap = wrap;
    widget.text.alignment = alg;
    widget.text.font = ui_system.font_handler.fonts["test"].get();

    //

    widget.size = vec2(0.0f);
    widget.min_width = 0.0f;
    widget.max_width = 0.0f;
    widget.min_height = 0.0f;
    widget.max_height = 0.0f;
    widget.weight_height = 0.0f;
    widget.callback = std::move(callback);

    widget.position = vec2(0.0f);
    
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget);
}

void text_widget::init() {
    axiom::ui_system* ui_system = &axiom::ecs.get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        //text.click_range = ivec4(position, position + size);

        if(size.x <= text.wrap_limits.x || size.x >= text.wrap_limits.y || size.x == 0.0f) {
            text.size.x = size.x;
            text.width = size.x;
            text.dirty = true;

            text.measure();

            size.y = text.size.y;
        }

        float f = text.wrap_limits.y;
        if(f == FLT_MAX) {
            f = text.wrap_limits.x;
        }
        size.x = glm::clamp(size.x, text.wrap_limits.x, f);
        size.y = text.size.y;
        
        max_width = text.max_width;
        if(!text.wrap) min_width = text.size.x;
        else min_width = 0.0f;
    };

    before.push_back(c);
    
    c.func = [this, ui_system]() {
        float offset = 0.0f;
        if(text.alignment == axiom::text_alignment::RIGHT) offset = (size.x - text.size.x);

        position = round(position + vec2(offset, 0.0f));

        text.position = position;
    };
    after.push_back(c);
}

axiom::capture_data text_widget::handle_capture() {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    if(ui_system.cursor_clip(clip, render_system.ctx->cursor_pos) && includes(render_system.ctx->cursor_pos, ivec4(position, position + size))) {
        return {parent, z, true, true};
    }

    return {self, z, false};
}

std::vector<axiom::text*> text_widget::get_text() {
    return {&text};
}

}