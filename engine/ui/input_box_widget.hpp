#pragma once

#include <ui/widget_base.hpp>
#include <ui/menu_widget.hpp>
#include <ui/text.hpp>

#include <sstream>

namespace axiom {

template<typename type>
struct input_box_widget : widget {
    type value;
    std::function<void(input_box_widget<type>&)> callback;

    bool update = false;
    bool input = false;

    void handle_inputs();
    void mesh();
    void init();
    axiom::capture_data handle_capture();
    
    static ulong insert(vec2 size, type value, std::function<void(input_box_widget<type>&)> callback = [](input_box_widget<type>& self) {});
};

template<typename type>
void input_box_widget<type>::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    view_range = vec4(position, position + size);
    
    text[0]->z = z;

    input = text[0]->select_range.x != -1 && text[0]->select_range.y != -1;

    if(input || ui_system.text_selected.size() == 1 && ui_system.text_selected[0].first == self) { // input
        update = true;
    } else {
        if(update) {
            std::stringstream stream;
            stream << text[0]->string;

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

            if(new_str != text[0]->string || new_str == "") {
                text[0]->string = new_str;
                //text[0]->dirty = true;
            }
        }
        
        //auto a = compute_text_bounds(ui_system.fonts["default mono"], text[0]->string, 1, 0xFFFFFFFF, false, ALIGNMENT_LEFT);
        //text[0]->size = {a[0], a[2]};
    }

    if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_RIGHT) && includes(ui_system.window->cursor_pos, vec4(position, position + size))) {
        std::shared_ptr<axiom::menu_node> node(new axiom::menu_node{
            "",
            {
                axiom::menu_node("COPY", {},
                    [this]() {
                        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

                        if(text[0]->select_range.x == text[0]->select_range.y) {
                            glfwSetClipboardString(ui_system.window->window_handle, text[0]->string.c_str());
                        } else {
                            std::string str = text[0]->retrieve();
                            glfwSetClipboardString(ui_system.window->window_handle, str.c_str());
                        }
                    }
                ),
                axiom::menu_node("PASTE", {},
                    [this]() {
                        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
                        
                        if(text[0]->select_range.x == -1) {
                            text[0]->string = glfwGetClipboardString(ui_system.window->window_handle);
                            update = true;
                            
                            //text[0]->dirty = true;
                            //text[0]->remesh = true;
                        } else {
                            paste = true;
                        }
                    }
                ),
            }
        });
        
        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
        ui_system.input_reset();

        ulong f = axiom::menu_widget::insert(ui_system.window->cursor_pos + vec2(0.0f, -32.0f), 0.6f, color_editor * 0.9f, 160, 16, 32, node, {});
    }

    vec2 range = {glm::max(2.0f, text[0]->font->line_height * 0.5f), size.x - 4};
    if(text[0]->select_range.y != -1) {
        std::cout << text[0]->select_range.x << " " << text[0]->select_range.y << "\n";

        vec2 cursor_pos = axiom::compute_cursor_pos(text[0]->select_range.y, *text[0].get());
        vec2 rel_pos = text[0]->position - position;

        float pos_x = rel_pos.x;

        float cpos = cursor_pos.x + rel_pos.x;
        if(cpos < range.x) {
            pos_x -= cpos - range.x;
        } else if(cpos > range.y) {
            pos_x -= cpos - range.y;
        }

        if(range.y - range.x < text[0]->size.y) pos_x = 2.0f;
        pos_x = glm::min(2.0f, pos_x);

        vec2 text_pos = position + vec2(pos_x, (size.y - text[0]->size.y) * 0.5f);
        text[0]->position = round(text_pos);
    } else {
        vec2 text_pos = position + vec2(2.0f, (size.y - text[0]->size.y) * 0.5f);
        text[0]->position = round(text_pos);
    }
}

template<typename type>
void input_box_widget<type>::mesh() {
    if(dirty) {
        dirty = false;

        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

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

        vec4 view_range = ui_system.get_range(self, true);

        //
        std::vector<axiom::ui_vertex> text_vs = text[0]->mesh();
        std::vector<axiom::ui_vertex> text_select = text[0]->mesh_select();
        text_vs.insert(text_vs.end(), text_select.begin(), text_select.end());

        for(axiom::ui_vertex& v : text_vs) {
            v.pos = vec3(v.pos.xy() + text[0]->position, z);
            v.range = view_range;
        }

        vertices_before.insert(vertices_before.end(), text_vs.begin(), text_vs.end());
    }
}

template<typename type>
void input_box_widget<type>::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    //
    axiom::widget_constraint c;
    c.func = [this, ui_system]() {
        //text[0]->click_range = ivec4(position, position + size);
    };
    after.push_back(c);
}

template<typename type>
ulong input_box_widget<type>::insert(vec2 size, type value, std::function<void(input_box_widget<type>&)> callback) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    axiom::input_box_widget<type> widget;

    //
    
    std::shared_ptr<axiom::text> text(new axiom::text);
    
    text->string = "";
    text->wrap = false;
    text->alignment = axiom::text_alignment::LEFT;
    text->font = ui_system.font_assets[0];
    text->selectable = true;
    text->editable = true;

    widget.text = {text};

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