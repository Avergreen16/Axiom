#include <ui/text_box_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {
 
void text_box_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    view_range = vec4(position, position + size);

    if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
        if(includes(ui_system.window->cursor_pos, view_range)) {
            vec2 clamped_cursor = vec2(glm::clamp(ui_system.window->cursor_pos.x, text[0]->position.x + 1, text[0]->position.x + text[0]->size.x - 1), glm::clamp(ui_system.window->cursor_pos.y, text[0]->position.y + 1, text[0]->position.y + text[0]->size.y - 1));

            ui_system.cursor_anchor = clamped_cursor;

            text[0]->select(vec4(clamped_cursor, clamped_cursor), true);
        }
    }

    if(ui_system.window->char_delta.size()) {
        bool insert = false;
        if(text[0]->select_range == ivec2(-1)) text[0]->string += ui_system.window->char_delta;
        text[0]->time = get_time();
        text[0]->select_range = ivec2(text[0]->string.size());
    }
    
    text[0]->z = z;

    bool inputting = text[0]->select_range.x != -1 && text[0]->select_range.y != -1;

    text[0]->mesh();
    
    min_height = size.y;
    max_height = size.y;

    /*
    if(inputting || ui_system.text_selected.size() == 1 && ui_system.text_selected[0].first == self) { // input
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
    */

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
                            //paste = true;
                        }
                    }
                ),
            }
        });
        
        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
        ui_system.input_reset();

        ulong f = axiom::menu_widget::insert(ui_system.window->cursor_pos + vec2(0.0f, -32.0f), 0.6f, color_editor * 0.9f, 160, 16, 32, node, {});
    }

    vec2 text_pos = position + vec2(boundary.x, boundary.y);
    text[0]->position = round(text_pos);

    if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_ENTER) && !ui_system.window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
        callback(*this);
    }
}

void text_box_widget::mesh() {
    if(dirty) {
        dirty = false;

        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

        vertices_before.clear();

        axiom::ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        axiom::ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        axiom::ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        axiom::ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        //

        std::vector<axiom::ui_vertex> ret = {a, b, d, a, d, c};

        vec4 range = vec4(position, size);
        
        for(axiom::ui_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(1.0f);
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        //

        ret = {a, b, d, a, d, c};

        range = vec4(position + 1.0f, size - 2.0f);
        
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

void text_box_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    //
    axiom::widget_constraint c;
    c.func = [this, ui_system]() {
        text[0]->width = size.x - boundary.x * 2.0f;
        size.y = text[0]->size.y + boundary.y * 2.0f;
        //text[0]->click_range = ivec4(position, position + size);
    };
    after.push_back(c);
}

ulong text_box_widget::insert(float width, vec2 boundary, std::string start, std::function<void(text_box_widget&)> callback) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    axiom::text_box_widget widget;

    //
    
    std::shared_ptr<axiom::text> text(new axiom::text);
    
    text->string = start;
    text->wrap = true;
    text->alignment = axiom::text_alignment::LEFT;
    text->font = ui_system.font_assets[0];
    text->selectable = true;
    text->editable = true;

    widget.text = {text};

    //

    widget.callback = callback;
    widget.boundary = boundary;

    widget.size = {width, widget.text[0]->font->line_height + 2.0f * boundary.y};
    widget.min_width = widget.size.x;
    widget.max_width = widget.size.x;
    widget.min_height = widget.size.y;
    widget.max_height = widget.size.y;

    widget.layout_mode = axiom::layout_mode::NONE;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget);
}

axiom::capture_data text_box_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(ui_system.window->cursor_pos, range)) return {self, z, true, true};
    }

    return {self, z, false};
}

}