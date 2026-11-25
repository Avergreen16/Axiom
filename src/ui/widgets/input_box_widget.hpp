#pragma once

#include <ui/widget_base.hpp>
#include <ui/text.hpp>
#include <ui/system.hpp>
#include <include/render.hpp>
#include <ui/widgets/menu_widget.hpp>

#include <sstream>

namespace axiom {

template<typename type>
struct input_box_widget : widget {
    type value;
    std::function<void(input_box_widget<type>&)> callback;

    axiom::text text;

    bool update = false;
    bool input = false;

    void handle_inputs();
    void mesh();
    void init();
    axiom::capture_data handle_capture();
    
    std::vector<axiom::text*> get_text();
    
    static ulong insert(vec2 size, type value, std::function<void(input_box_widget<type>&)> callback = [](input_box_widget<type>& self) {});
};

template<typename type>
void input_box_widget<type>::handle_inputs() {
    /*
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
    vec4 view_range = vec4(position, position + size);
    
    text.z = z;

    input = text.select_range.x != -1 && text.select_range.y != -1;

    if(input || ui_system.text_selected.size() == 1 && ui_system.text_selected[0].first == self) { // input
        update = true;
    } else {
        if(update) {
            std::stringstream stream;
            stream << text.string;

            type prev = value;

            if(!(stream >> value)) {
                value = prev;
            }

            callback(*this);

            update = false;
        } else {
            callback(*this);

            std::stringstream stream;
            stream << value;

            std::string new_str = stream.str();

            if(new_str != text.string || new_str == "") {
                text.string = new_str;
                //text.dirty = true;
            }
        }
        
        //auto a = compute_text_bounds(ui_system.fonts["default mono"], text.string, 1, 0xFFFFFFFF, false, ALIGNMENT_LEFT);
        //text.size = {a[0], a[2]};
    }

    if(render_system.ctx->pressed_buttons.contains(axiom::input_code::MOUSE_RIGHT) && includes(render_system.ctx->cursor_pos, vec4(position, position + size))) {
        std::shared_ptr<axiom::menu_node> node(new axiom::menu_node{
            "",
            {
                axiom::menu_node("COPY", {},
                    [this]() {
                        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

                        if(text.select_range.x == text.select_range.y) {
                            //glfwSetClipboardString(render_system.ctx->window_handle, text.string.c_str());
                        } else {
                            std::string str = text.retrieve();
                            //glfwSetClipboardString(render_system.ctx->window_handle, str.c_str());
                        }
                    }
                ),
                axiom::menu_node("PASTE", {},
                    [this]() {
                        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
                        
                        if(text.select_range.x == -1) {
                            //text.string = glfwGetClipboardString(render_system.ctx->window_handle);
                            //update = true;
                            
                            //text.dirty = true;
                            //text.remesh = true;
                        } else {
                            //paste = true;
                        }
                    }
                ),
            }
        });
        
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        ui_system.input_reset();

        ulong f = axiom::menu_widget::insert(vec2(render_system.ctx->cursor_pos) + vec2(0.0f, -32.0f), 0.6f, color_editor * 0.9f, 160, 16, 32, node.get(), {});
    }

    vec2 range = {glm::max(2.0f, text.font->line_height * 0.5f), size.x - 4};
    if(text.select_range.y != -1) {
        std::cout << text.select_range.x << " " << text.select_range.y << "\n";

        vec2 cursor_pos = axiom::compute_cursor_pos(text.select_range.y, *text[0].get());
        vec2 rel_pos = text.position - position;

        float pos_x = rel_pos.x;

        float cpos = cursor_pos.x + rel_pos.x;
        if(cpos < range.x) {
            pos_x -= cpos - range.x;
        } else if(cpos > range.y) {
            pos_x -= cpos - range.y;
        }

        if(range.y - range.x < text.size.y) pos_x = 2.0f;
        pos_x = glm::min(2.0f, pos_x);

        vec2 text_pos = position + vec2(pos_x, (size.y - text.size.y) * 0.5f);
        text.position = round(text_pos);
    } else {
        vec2 text_pos = position + vec2(2.0f, (size.y - text.size.y) * 0.5f);
        text.position = round(text_pos);
    }
    */
}

template<typename type>
void input_box_widget<type>::mesh() {
    if(dirty) {
        dirty = false;

        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

        vertices_before.clear();

        axiom::ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        axiom::ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        axiom::ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        axiom::ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        std::vector<axiom::ui_vertex> ret = {a, b, d, a, d, c};

        vec4 range = vec4(position, size);
        
        for(axiom::ui_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0625f, 0.0625f, 0.0625f, 1.0f);
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        //
        std::vector<axiom::ui_vertex> text_vs = text.vertices;
        std::vector<axiom::ui_vertex> text_select = text.select_vertices;
        text_vs.insert(text_vs.end(), text_select.begin(), text_select.end());

        for(axiom::ui_vertex& v : text_vs) {
            v.pos = vec3(v.pos.xy() + text.position, z);
            v.clip_space = clip;
        }

        vertices_before.insert(vertices_before.end(), text_vs.begin(), text_vs.end());
    }
}

template<typename type>
void input_box_widget<type>::init() {
    axiom::ui_system* ui_system = &axiom::ecs.get_system<axiom::ui_system>();

    //
    axiom::widget_constraint c;
    c.func = [this, ui_system]() {
        //text.click_range = ivec4(position, position + size);
    };
    after.push_back(c);
}

template<typename type>
ulong input_box_widget<type>::insert(vec2 size, type value, std::function<void(input_box_widget<type>&)> callback) {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    axiom::input_box_widget<type> widget;

    //
    
    widget.text.string = "";
    widget.text.wrap = false;
    widget.text.alignment = axiom::text_alignment::LEFT;
    widget.text.font = ui_system.font_handler.fonts["test"].get();
    widget.text.selectable = true;
    widget.text.editable = true;

    //

    widget.value = value;
    widget.callback = callback;

    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;

    widget.layout_mode = axiom::layout_mode::NONE;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget);
}

template<typename type>
axiom::capture_data input_box_widget<type>::handle_capture() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(render_system.ctx->cursor_pos, range)) return {z, true};
    }

    return {z, false};
}

template<typename type>
std::vector<axiom::text*> input_box_widget<type>::get_text() {
    return {&text};
}

}