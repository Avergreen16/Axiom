#include <ui/message_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void message_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    vec4 range = ui_system.get_range(self);
}

void message_widget::mesh() {
    if(dirty) {
        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

        vertices_before.clear();
        
        vec4 view_range = ui_system.get_range(self);
        
        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        vec4 panel_range = vec4(position + vec2(0.0f, tail_size), size - vec2(0.0f, tail_size));
        
        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        
        for(ui_vertex& v : ret) {
            v.pos = vec3(panel_range.xy() + v.pos.xy() * panel_range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(color, 1.0f);
            v.data = 0x1;

            v.range = view_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // tail

        if(tail_size) {
            if(tail_settings == 1) {
                panel_range = vec4(position, vec2(tail_size));
                ret = {a, d, c};
                
                for(ui_vertex& v : ret) {
                    v.pos = vec3(panel_range.xy() + v.pos.xy() * panel_range.zw(), z);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = vec4(color, 1.0f);
                    v.data = 0x1;

                    v.range = view_range;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            } else if(tail_settings == 2) {
                panel_range = vec4(position + vec2(size.x - tail_size, 0.0f), vec2(tail_size));
                ret = {b, d, c};
                
                for(ui_vertex& v : ret) {
                    v.pos = vec3(panel_range.xy() + v.pos.xy() * panel_range.zw(), z);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = vec4(color, 1.0f);
                    v.data = 0x1;

                    v.range = view_range;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            }
        }

        //

        dirty = false;

        //text[0]->width = size.x;
        std::vector<ui_vertex> text_vertices = text[0]->mesh();
        std::vector<ui_vertex> select_vertices = text[0]->mesh_select();
        text_vertices.insert(text_vertices.end(), select_vertices.begin(), select_vertices.end());

        //std::cout << select_vertices.size() << "\n";

        for(ui_vertex& v : text_vertices) {
            v.pos.x += text[0]->position.x;
            v.pos.y += text[0]->position.y;
            v.pos.z = z;

            v.range = view_range;
        }

        vertices_before.insert(vertices_before.end(), text_vertices.begin(), text_vertices.end());
    }
}

void message_widget::get_y() {
    //text[0]->refresh();
}

void message_widget::set_str(std::string str) {
    /*
    if(text[0]->string != str) {
        text[0]->dirty = true;
        text[0]->string = str;
        dirty = true;

        get_y();
    }
    */
}

uint64_t message_widget::insert(std::string str, axiom::text_alignment alg, vec2 width, vec3 color, vec2 border, ulong timestamp, uint tail_settings) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    message_widget widget;

    //

    std::shared_ptr<axiom::text> text(new axiom::text);
    
    text->string = str;
    text->wrap = true;
    text->alignment = alg;
    text->font = ui_system.font_assets[0];

    widget.text = {text};

    widget.min_width = width.x;
    widget.max_width = width.y;

    widget.tail_settings = tail_settings;
    if(tail_settings) widget.tail_size = 8.0f;

    //

    widget.color = color;
    widget.border = border;

    widget.size = vec2(0.0f);
    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = 0.0f;
    widget.weight_height = 0.0f;
    widget.timestamp = timestamp;

    widget.position = vec2(0.0f);
    
    widget.layout_mode = axiom::layout_mode::VOID;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget);
}

void message_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        //text[0]->click_range = ivec4(position, position + size);
        if(size.x - border.x * 2.0f < text[0]->wrap_limits.x || size.x - border.x * 2.0f > text[0]->wrap_limits.y || size.x == 0.0f) {
            text[0]->size.x = size.x - border.x * 2.0f;
            text[0]->width = size.x - border.x * 2.0f;
            //text[0]->dirty = true;
            
            text[0]->mesh();

            size.y = text[0]->size.y + border.y * 2.0f + tail_size;
        }

        float f = text[0]->wrap_limits.y + border.x * 2.0f;
        if(f == FLT_MAX) {
            f = text[0]->wrap_limits.x + border.x * 2.0f;
        }
        size.x = glm::min(size.x, text[0]->wrap_limits.x + border.x * 2.0f);
        //size.x = glm::clamp(size.x, text[0]->wrap_limits.x + border.x * 2.0f, f);
        size.y = text[0]->size.y + border.y * 2.0f + tail_size;
        
        max_width = text[0]->max_width + border.x * 2.0f;
    };
    before.push_back(c);
    
    c.func = [this, ui_system]() {
        position = round(position);

        text[0]->position = position + border + vec2(0.0f, tail_size);
    };
    after.push_back(c);
}

}