#include <ui/ui.hpp>
#include <utilities/utilities.hpp>

#include <numeric>

namespace axiom {

vec2 text_range;
std::vector<uint> text_start;

vec3 color_red = hsv_color(0.0f, 0.65f, 0.9f);
vec3 color_orange = hsv_color(0.375f, 0.65f, 0.9f);
vec3 color_yellow = hsv_color(0.75f, 0.65f, 0.9f);
vec3 color_green = hsv_color(2.0f, 0.65f, 0.5); 
vec3 color_blue = hsv_color(4.0f, 0.65f, 0.9f);
vec3 color_purple = hsv_color(4.5f, 0.65f, 0.9f);
vec3 color_magenta = hsv_color(5.0f, 0.65f, 0.9f);
vec3 color_rose = hsv_color(5.75f, 0.65f, 0.9f);

vec3 color_physics = color_rose;
vec3 color_editor = color_physics;
vec3 color_debug = color_physics;
vec3 color_lua = color_yellow;

bool includes(ivec2 point, ivec4 range) {
    return (point.x >= range.x && point.x < range.z && point.y >= range.y && point.y < range.w);
}

bool includes(ivec4 range_a, ivec4 range_b) {
    bool touch_x = range_a.x <= range_b.z && range_b.x <= range_a.z;
    bool touch_y = range_a.y <= range_b.w && range_b.y <= range_a.w;
    return touch_x && touch_y;
}

vec4 intersect_range(vec4 a, vec4 b) {
    return {glm::max(a.x, b.x), glm::max(a.y, b.y), glm::min(a.z, b.z), glm::min(a.w, b.w)};
}

//

// text

// window


// panel 


// debug

// column

// row


// grid

// text

// text input

void text_input_widget::handle_inputs() {
    // cursor
    if(core.key_map[GLFW_MOUSE_BUTTON_LEFT] && includes(core.cursor_pos, vec4(position, position + size))) {
        click = true;
        text_dirty = true;
        click_pos = core.cursor_pos;

        auto d = compute_cursor_index(core.cursor_pos - position, ecs.get_system<gui_system>().fonts["default mono"], text, 1, line_indices, alignment);
        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
            cursor = d.first;
            selection_anchor = d.first;
            wraparound = !d.second;
        } else {
            if(cursor != d.first && wraparound != !d.second) dirty = true;
            cursor = d.first;
            wraparound = !d.second;
        }
    }

    if(core.cursor_disabled) cursor = 0xFFFFFFFF;

    if(cursor != 0xFFFFFFFF && !core.cursor_disabled) {
        ivec2 range = ivec2(cursor, cursor);
        if(selection_anchor != 0xFFFFFFFF) {
            if(selection_anchor > cursor) range = {cursor, selection_anchor};
            else range = {selection_anchor, cursor};
        }
        std::string chars = core.char_delta;

        if(core.pressed_buttons.contains(GLFW_KEY_BACKSPACE) || core.repeat_buttons.contains(GLFW_KEY_BACKSPACE) || chars.size()) {
            if(range.x == range.y) {
                if(!chars.size() && cursor != 0) {
                    text.erase(text.begin() + (cursor - 1));
                    --cursor;
                    selection_anchor = cursor;
                    text_dirty = true;
                }
            } else {
                text.erase(text.begin() + range.x, text.begin() + range.y);
                cursor = range.x;
                selection_anchor = range.x;
                text_dirty = true;
            }
        }
        if(chars.size()) {
            text.insert(text.begin() + cursor, chars.begin(), chars.end());
            cursor += chars.size();
            selection_anchor = cursor;

            text_dirty = true;
        }

        int i = 0;
        for(i = 0; i < line_indices.size(); ++i) {
            int index = line_indices[i].start;
            if(cursor < index || (cursor == index && !wraparound)) break;
        }
        int line_start = line_indices[i - 1].start;
        int line_end = (i == line_indices.size()) ? text.size() : line_indices[i].start;

        if(core.pressed_buttons.contains(GLFW_KEY_LEFT) || core.repeat_buttons.contains(GLFW_KEY_LEFT)) {
            bool b = true;
            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) {
                b = cursor == range.x;
                if(!b) text_dirty = true;

                cursor = range.x;
            }

            if(cursor != 0 && b) {
                if(core.key_map[GLFW_KEY_LEFT_CONTROL]) {
                    gui_system& gui_system = ecs.get_system<gui_system>();
                    font_asset& f = gui_system.fonts["default mono"];
                    std::u32string str = convert_string(text);
                    
                    bool ws = true;
                    uint start_cursor = cursor;
                    while(true) {
                        if(cursor == 0) break;
                        uint current = str[cursor - 1];
                        
                        auto& g = f.at(current);
                        if(g.visible || ws) {
                            if(g.visible) ws = false;
                            
                            --cursor;
                        } else {
                            break;
                        }
                    }
                } else {
                    if(cursor == line_start && wraparound == true) {
                        wraparound = false;
                    } else {
                        if(cursor == line_start + 1) {
                            wraparound = true;
                        } 
                        --cursor;
                    }
                }

                text_dirty = true;
            }

            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) selection_anchor = cursor;
        }
        if(core.pressed_buttons.contains(GLFW_KEY_RIGHT) || core.repeat_buttons.contains(GLFW_KEY_RIGHT)) {
            bool b = true;
            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) {
                b = cursor == range.y;
                if(!b) text_dirty = true;

                cursor = range.y;
            }

            if(cursor != text.size() && b) {
                if(core.key_map[GLFW_KEY_LEFT_CONTROL]) {
                    gui_system& gui_system = ecs.get_system<gui_system>();
                    font_asset& f = gui_system.fonts["default mono"];
                    std::u32string str = convert_string(text);
                    
                    bool ws = true;
                    uint start_cursor = cursor;
                    while(true) {
                        if(cursor == text.size()) break;
                        uint current = str[cursor];
                        
                        auto& g = f.at(current);
                        if(g.visible && !ws) {
                            break;
                        } else {
                            if(!g.visible) ws = false;
                            ++cursor;
                        }
                    }
                } else {
                    if(cursor == line_end && wraparound == false) {
                        wraparound = true;
                    } else {
                        if(cursor == line_end - 1) {
                            wraparound = false;
                        } 
                        ++cursor;
                    }
                }
            
                text_dirty = true;
            }
            
            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) selection_anchor = cursor;
        }

        if(core.pressed_buttons.contains(GLFW_KEY_UP) || core.repeat_buttons.contains(GLFW_KEY_UP)) {
            bool b = true;
            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) {
                b = cursor == range.x;
                if(!b) text_dirty = true;

                cursor = range.x;
            }

            if(i > 1 && b) {
                int sep = line_start - line_indices[i - 2].start;
                int delta = cursor - line_start;

                if(sep <= delta) wraparound = false;
                cursor = line_indices[i - 2].start + min(sep, delta);
                text_dirty = true;
            } else {
                if(cursor != 0) text_dirty = true;
                cursor = 0;
            }

            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) selection_anchor = cursor;
        }
        if(core.pressed_buttons.contains(GLFW_KEY_DOWN) || core.repeat_buttons.contains(GLFW_KEY_DOWN)) {
            bool b = true;
            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) {
                b = cursor == range.y;
                if(!b) text_dirty = true;

                cursor = range.y;
            }

            if(i < line_indices.size() - 1 && b) {
                int sep = line_indices[i + 1].start - line_end;
                int delta = cursor - line_start;

                if(sep <= delta) wraparound = false;
                cursor = line_indices[i].start + min(sep, delta);
                text_dirty = true;
            } else {
                if(cursor != text.size()) text_dirty = true;
                cursor = text.size();
            }
            
            if(!core.key_map[GLFW_KEY_LEFT_SHIFT]) selection_anchor = cursor;
        }

        if(chars.size()) text_dirty = true;
    }

    if(text_dirty) {
        get_y();
        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
            if(!core.key_map[GLFW_KEY_LEFT_SHIFT] || selection_anchor == 0xFFFFFFFF) selection_anchor = cursor;
        }
    }
}

void text_input_widget::init() {
    gui_system* gui_system = &ecs.get_system<gui_system>();

    widget_constraint c;
    c.func = [this, gui_system]() {
        if(size.x < resize_range.x || size.x > resize_range.y) {
            get_y();
            text_dirty = true;
        }

        float f = resize_range.y;
        if(resize_range.y == FLT_MAX) f = resize_range.x;
        size.x = clamp(size.x, resize_range.x, f);
    };
    before.push_back(c);
}

void text_input_widget::mesh() {
    if(text_dirty) {
        text_dirty = false;

        gui_system& gui_system = ecs.get_system<gui_system>();
        font_asset& f = gui_system.fonts["default mono"];
        get_y();

        click = false;
        
        ivec2 range;
        if(selection_anchor == 0xFFFFFFFF || selection_anchor == cursor) range = {-1, -1};
        else {
            if(selection_anchor > cursor) range = {cursor, selection_anchor};
            else range = {selection_anchor, cursor};
        }
        
        text_vertices = mesh_text(f, text, text_size, size.x, range, alignment, false);
        if(cursor != 0xFFFFFFFF) cursor_pos = compute_cursor_pos(cursor, wraparound, ecs.get_system<gui_system>().fonts["default mono"], text, 1, line_indices, alignment);
    }

    if(dirty) {
        gui_system& gui_system = ecs.get_system<gui_system>();
        font_asset& f = gui_system.fonts["default mono"];

        dirty = false;

        vec4 range = gui_system.get_range(self);

        vertices_before = text_vertices;

        for(ui_vertex& v : vertices_before) {
            v.pos = vec3(round(position) + v.pos.xy(), z);
        }

        if(cursor != 0xFFFFFFFF) {
            std::vector<ui_vertex> total_ret;

            ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
            ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
            ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
            ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
            
            // panel
            std::vector<ui_vertex> ret = {a, b, d, a, d, c};
            for(ui_vertex& v : ret) {
                v.pos = vec3(position + cursor_pos + v.pos.xy() * vec2(1.0f, f.line_height), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(1.0f);
                if(mod(core.current_time, 1.0) > 0.5) v.color = vec4(0.0f);
                v.data = 1;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }
    }
}

void text_input_widget::insert_cursor() {

}

void text_input_widget::get_y() {
    gui_system& gui_system = ecs.get_system<gui_system>();
    font_asset& f = gui_system.fonts["default mono"];

    std::vector<float> ret = compute_text_bounds(f, text, text_size, size.x, true, alignment);
    line_indices = text_line_data;

    resize_range = {ret[0], ret[1]};
    size.y = ret[2];
    text_x = ret[3];

    max_width = ret[4];
}

void text_input_widget::set_str(std::string str) {
    if(text != str) {
        text_dirty = true;
        dirty = true;
        get_y();
    }
    text = str;
}

uint64_t text_input_widget::insert(std::string str, ALIGNMENT alg) {
    gui_system& gui_system = ecs.get_system<gui_system>();
    
    text_input_widget widget;

    widget.text = str;
    widget.size = vec2(0.0f);
    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = 0.0f;

    widget.position = vec2(0.0f);
    widget.alignment = alg;
    
    widget.size_mode = SM_FILL;
    widget.layout_mode = LM_VOID;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;

    return gui_system.insert_widget(widget);
}

/*
*/



void render_widget::mesh() {
    ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

    // panel
    std::vector<ui_vertex> ret = {a, b, d, a, d, c};
    for(ui_vertex& v : ret) {
        v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * size, z);
        v.tex_pos = v.tex_pos;
        v.color = vec4(1.0);//vec4(0.25f, 0.25f, 0.25f, 1.0f);
        v.data = 0x3;
    }
    vertices_before = ret;
}

void render_widget::init() {
    /*
    gui_system* gui_system = &ecs.get_system<gui_system>();

    widget_constraint c;
    c.func = [this, gui_system]() {
        position = vec2(0.0f);
        size = core.window.screen_size;
    };
    before.push_back(c);

    for(int i = 0; i < children.size(); ++i) {
        c.func = [this, gui_system, i]() {
            auto& p0 = gui_system->widgets[children[i]];

            float C0 = p0->size.y - size.y;
            p0->size.y -= C0;
            
            float C1 = p0->size.x - size.x;
            p0->size.x -= C1;
            
            float C2 = p0->position.x - position.x;
            p0->position.x -= C2;

            float C3 = p0->position.y - position.y;
            p0->position.y -= C3;
        };
        before.push_back(c);
    }
    */

    widget_constraint c;
    c.func = [this]() {

    };
    before.push_back(c);

    /*
    c.func = [this]() {
        Input_system* system = &ecs.get_system<Input_system>();
        system->view_range = {position, size};
    };
    after.push_back(c);
    */
}

uint64_t render_widget::insert() {
    gui_system& gui_system = ecs.get_system<gui_system>();

    render_widget widget;
    widget.flag = true;
    widget.target = 0;
    
    widget.buffer = gui_system.active_buffer;
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = PM_STATIC;

    return gui_system.insert_widget(widget);
}

void render_widget::handle_inputs() {
    gui_system& gui_system = ecs.get_system<gui_system>();
    Input_system& input_system = ecs.get_system<Input_system>();
    Render_system& render_system = ecs.get_system<Render_system>();

    render_system.targets[target].target_size = size;
    input_system.view_range = ivec4(position, size);

    if(gui_system.hover_capture == self && (gui_system.click_capture == NULL_WIDGET || gui_system.click_capture == self)) input_system.gui_captured = true;
    else input_system.gui_captured = false;
}

bool render_widget::handle_capture() {
    capture_data.z = z;
    capture_data.text_capture = false;

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(core.cursor_pos, range)) return true;
    }

    return false;
}

}