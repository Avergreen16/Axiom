#include <ui/widgets/text_box_widget.hpp>
#include <ui/widgets/menu_widget.hpp>
#include <ui/system.hpp>

namespace axiom {
 
void text_box_widget::handle_inputs() {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    //view_range = vec4(position, position + size);

    if(render_system.ctx->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
        //if(includes(render_system.ctx->cursor_pos, view_range)) {
            vec2 clamped_cursor = vec2(glm::clamp((float)render_system.ctx->cursor_pos.x, text.position.x + 1, text.position.x + text.size.x - 1), glm::clamp((float)render_system.ctx->cursor_pos.y, text.position.y + 1, text.position.y + text.size.y - 1));

            ui_system.cursor_anchor = clamped_cursor;

            //text.select(vec4(clamped_cursor, clamped_cursor), true);
        //}
    }

    if(render_system.ctx->char_delta.size()) {
        bool insert = false;
        //if(text.select_range == ivec2(-1)) text.string += render_system.ctx->char_delta;
        //text.time = get_time();
        //text.select_range = ivec2(text.string.size());
    }
    
    text.z = z;

    //bool inputting = text.select_range.x != -1 && text.select_range.y != -1;

    text.mesh();
    
    min_height = size.y;
    max_height = size.y;

    /*
    if(inputting || ui_system.text_selected.size() == 1 && ui_system.text_selected[0].first == self) { // input
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
    */

    if(render_system.ctx->pressed_buttons.contains(axiom::input_code::MOUSE_RIGHT) && includes(render_system.ctx->cursor_pos, vec4(position, position + size))) {
        axiom::menu_node* node = new axiom::menu_node{
            "",
            {
                axiom::menu_node("COPY", {},
                    [this]() {
                        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
                        
                        /*
                        if(text.select_range.x == text.select_range.y) {
                            glfwSetClipboardString(render_system.ctx->window_handle, text.string.c_str());
                        } else {
                            std::string str = text.retrieve();
                            glfwSetClipboardString(render_system.ctx->window_handle, str.c_str());
                        }
                        */
                    }
                ),
                axiom::menu_node("PASTE", {},
                    [this]() {
                        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
                        
                        /*
                        if(text.select_range.x == -1) {
                            text.string = glfwGetClipboardString(render_system.ctx->window_handle);
                            update = true;
                            
                            //text.dirty = true;
                            //text.remesh = true;
                        } else {
                            //paste = true;
                        }
                        */
                    }
                ),
            }
        };
        
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        ui_system.input_reset();

        ulong f = axiom::menu_widget::insert((vec2)render_system.ctx->cursor_pos + vec2(0.0f, -32.0f), 0.6f, color_editor * 0.9f, 160, 16, 32, node, {});
    }

    vec2 text_pos = position + vec2(boundary.x, boundary.y);
    text.position = round(text_pos);

    if(render_system.ctx->pressed_buttons.contains(axiom::input_code::KEY_ENTER) && !render_system.ctx->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
        callback(*this);
    }
}

void text_box_widget::mesh() {
    if(dirty) {
        dirty = false;

        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

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
        
        //
        std::vector<axiom::ui_vertex> text_vs = text.mesh();

        for(axiom::ui_vertex& v : text_vs) {
            v.pos = vec3(v.pos.xy() + text.position, z);

            v.clip_space = clip;
        }

        vertices_before.insert(vertices_before.end(), text_vs.begin(), text_vs.end());
    }
}

void text_box_widget::init() {
    axiom::ui_system* ui_system = &axiom::ecs.get_system<axiom::ui_system>();

    //
    axiom::widget_constraint c;
    c.func = [this, ui_system]() {
        text.width = size.x - boundary.x * 2.0f;
        size.y = text.size.y + boundary.y * 2.0f;
        //text.click_range = ivec4(position, position + size);
    };
    after.push_back(c);
}

ulong text_box_widget::insert(float width, vec2 boundary, std::string start, std::function<void(text_box_widget&)> callback) {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    axiom::text_box_widget widget;

    //
    
    widget.text.string = start;
    widget.text.wrap = true;
    widget.text.alignment = axiom::text_alignment::LEFT;
    widget.text.font = ui_system.font_handler.fonts["test"].get();
    widget.text.selectable = true;
    widget.text.editable = true;

    //

    widget.callback = callback;
    widget.boundary = boundary;

    widget.size = {width, widget.text.font->line_height + 2.0f * boundary.y};
    widget.min_width = widget.size.x;
    widget.max_width = widget.size.x;
    widget.min_height = widget.size.y;
    widget.max_height = widget.size.y;

    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget);
}

axiom::capture_data text_box_widget::handle_capture() {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(render_system.ctx->cursor_pos, range)) return {self, z, true, true};
    }

    return {self, z, false};
}

std::vector<axiom::text*> text_box_widget::get_text() {
    return {&text};
}

}