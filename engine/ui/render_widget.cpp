#include <ui/render_widget.hpp>
#include <ui/ui_system.hpp>
#include <render/target.hpp>

namespace axiom {
    
void render_widget::mesh() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

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

        position = floor(position);
        size = floor(size);

        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(position + v.pos.xy() * size, z);
            v.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
            v.data = 0x80000000 + ui_system->target;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        vertices_before = total_ret;
    }

    //std::cout << position.x << " " << position.y << " " << size.x << " " << size.y << " " << target->size.x << " " << target->size.y << "\n";

    ++ui_system->target;
}

void render_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    //
}

void render_widget::handle_inputs() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    if(target->size != ivec2(size) || target->position != ivec2(position)) target->set_size(ivec2(size), ivec2(position));
    target->call();

    ui_system->target_textures.push_back(&target->framebuffer.textures[texture]);
}

axiom::capture_data render_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    if(includes(ui_system.window->cursor_pos, ranges[0])) return {z, true, false};

    return {z, false};
}

ulong render_widget::insert(axiom::render_target* target, uint texture) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    render_widget widget;
    widget.position_mode = axiom::position_mode::BOTTOM_LEFT;
    widget.layout_mode = axiom::layout_mode::VOID;
    widget.target = target;
    widget.texture = texture;

    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = FLT_MAX;
    widget.buffer = ui_system.input_state.active_buffer;
    widget.texture = texture;

    widget.flag = true;

    widget.size = vec2(0.0f);
    widget.position = vec2(0.0f);

    return ui_system.insert_widget(widget);
}

}