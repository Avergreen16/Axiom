#include <ui/drop_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void drop_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    float height = glm::min(drop_unit_height * (options.size() - 1), drop_height);

    if(ui_system.hover_capture == self && (ui_system.click_capture == NULL_WIDGET || ui_system.click_capture == self)) {
        if(drop_down) {
            float content_height = drop_unit_height * (options.size() - 1);
            float panel_height = glm::min(content_height, drop_height);
            
            float offset = -panel_height;
            if(drop_direction) offset = size.y;
            
            vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);
            range = {range.xy(), range.xy() + range.zw()};
            
            if(includes(ui_system.window->cursor_pos, range)) {
                if(content_height > panel_height) {
                    float scroll_region = content_height - panel_height;

                    if(ui_system.window->scroll_delta) {
                        float scroll_speed = 20.0f;
                        scroll -= ui_system.window->scroll_delta * scroll_speed;
                        
                        scroll = glm::clamp(scroll, 0.0f, scroll_region);
                        dirty = true;
                    }
                }

                //
                
                float origin = position.y + (panel_height + offset) - drop_unit_height + scroll;

                uint rel = -floor((ui_system.window->cursor_pos.y - origin) / drop_unit_height);

                if(hovered != rel) dirty = true;
                hovered = rel;
                
                if(hovered >= selected) ++hovered;
            } else {
                if(hovered != 0xFFFFFFFF) dirty = true;
                hovered = 0xFFFFFFFF;
            }
        }
    }

    bool drop_hover = false;
    
    vec4 range = vec4(position.x + size.x - button_width, position.y, button_width, size.y);
    range = {range.xy(), range.xy() + range.zw()};

    if(includes(ui_system.window->cursor_pos, range)) {
        drop_hover = true;
        ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;
    }
    
    if(ui_system.click_capture == self) {
        if(drop_down) {
            if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                if(hovered != 0xFFFFFFFF) {
                    selected = hovered;

                    text[0]->string = options[selected].label;
                    toggle_drop();
                }
            }
        }

        if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && drop_hover) {
            if(drop_down == false) {
                if(position.y - height < 0.0f) drop_direction = true;
                else drop_direction = false;

                toggle_drop();
                dirty = true;
            } else {
                toggle_drop();
                dirty = true;
            }
        }
    }

    if(ui_system.click_capture != NULL_WIDGET && ui_system.click_capture != self && drop_down) {
        toggle_drop();
    }
    
    callback(*this);
}

void drop_widget::toggle_drop() {
    scroll = 0.0f;
    
    drop_down = !drop_down;

    if(drop_down) {
        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

        uint32_t i = 0;
        for(drop_option& option : options) {
            if(i != selected) {
                std::shared_ptr<axiom::text> t(new axiom::text);
                t->string = option.label;
                t->wrap = false;
                t->font = ui_system.font_assets[0];

                ui_system.text.push_back(t);
                text.push_back(t);
            }

            ++i;
        }
    } else {
        text.resize(1);
    }
}

void drop_widget::mesh() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    /*
    for(drop_option& option : options) {
        if(option.dirty) {
            option.dirty = false;

            font_asset& f = ui_system.fonts["default mono"];

            option.vertices = mesh_text(f, option.label, 1, 0xFFFFFFFF, {-1, -1}, ALIGNMENT_CENTER, false);
            option.size = text_range;
        }
    }
    */

    if(dirty) {
        vertices_before.clear();
        
        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        std::vector<ui_vertex> ret = {a, b, d, a, d, c};

        vec4 range = vec4(position, size);
        vec4 col = vec4(color, 1.0f);

        for(ui_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = col;
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // current option text

        drop_option option = options[selected];
        
        std::vector<ui_vertex> vs = text[0]->mesh();

        vec2 origin = position + vec2(4.0f, (size.y - text[0]->size.y) * 0.5f);
        origin = round(origin);

        text[0]->position = origin;
        for(auto& vertex : vs) {
            vertex.pos += vec3(origin, z);
        }
        vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());

        // panel for button

        ret = {a, b, d, a, d, c};

        range = vec4(position.x + size.x - button_width, position.y, button_width, size.y);
        col = vec4(color + 0.25f, 1.0f);

        for(ui_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = col;
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // button

        ret = {a, b, d, a, d, c};

        vec4 tex_range = vec4(26.0f, 56.0f, 8.0f, 8.0f);

        vec2 rel_pos = (vec2(button_width, size.y) - vec2(8.0f, 8.0f)) * 0.5f;

        range = vec4(position.x + size.x - button_width + rel_pos.x, position.y + rel_pos.y, 8.0f, 8.0f);
        col = vec4(color + 0.25f, 1.0f);

        for(ui_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = tex_range.xy() + v.tex_pos * tex_range.zw();
            v.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        if(drop_down) {
            float content_height = drop_unit_height * (options.size() - 1);
            float panel_height = glm::min(drop_unit_height * (options.size() - 1), drop_height);

            float offset = -panel_height;
            if(drop_direction) offset = size.y;

            std::vector<ui_vertex> ret = {a, b, d, a, d, c};
            vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);
            

            vec4 col = vec4(color * 0.875f, 1.0f);

            vec4 panel_range = {range.xy(), range.xy() + range.zw()};

            for(ui_vertex& v : ret) {
                v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z + 0.001f);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = col;
                v.data = 0x1;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

            // hovered

            if(hovered != 0xFFFFFFFF) {
                std::vector<ui_vertex> ret = {a, b, d, a, d, c};

                uint h = hovered;
                if(h >= selected) --h;

                vec4 range = vec4(position.x, position.y + (panel_height + offset) - (drop_unit_height * (float(h) + 1.0f)) + scroll, size.x, drop_unit_height);
                vec4 col = vec4(color, 1.0f);

                for(ui_vertex& v : ret) {
                    v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z + 0.001f);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = col;
                    v.data = 0x1;
                    v.range = panel_range;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            }

            // all options
            float pos = -drop_unit_height + scroll + (panel_height + offset);

            int j = 0;
            for(int i = 0; i < options.size(); ++i) {
                if(i == selected) continue;
                
                std::vector<ui_vertex> vs = text[j + 1]->mesh();
                text[j + 1]->position = origin;

                drop_option option = options[i];
                vec2 origin = position + vec2(4.0f, (size.y - text[j + 1]->size.y) * 0.5f + pos);
                pos -= drop_unit_height;
                origin = round(origin);

                for(auto& vertex : vs) {
                    vertex.pos += vec3(origin, z + 0.001f);
                    vertex.range = panel_range;
                }
                vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());

                ++j;
            }

            if(content_height > panel_height) {
                float scroll_width = 2.0f;

                float scroll_region = content_height - panel_height;

                float scroll_height = panel_height / content_height * panel_height;
                float scroll_pos = (1.0f - scroll / scroll_region) * (panel_height - scroll_height);

                vec4 scrollbar = vec4(position.x + size.x - scroll_width, position.y + offset + scroll_pos, scroll_width, scroll_height);

                //

                ret = {a, b, d, a, d, c};

                range = scrollbar;

                for(ui_vertex& v : ret) {
                    v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z + 0.001f);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = vec4(1.0f, 1.0f, 1.0f, 0.5f);
                    v.data = 0x1;
                    v.range = panel_range;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            }
        }
    }
}

void drop_widget::init() {

}

uint64_t drop_widget::insert(vec2 size, vec3 color, float w, float h, float h2, std::vector<std::string> options, uint selected, std::function<void(drop_widget&)> callback) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    drop_widget widget;
    
    std::shared_ptr<axiom::text> text(new axiom::text);
    text->string = options[selected];
    text->wrap = false;
    text->font = ui_system.font_assets[0];

    widget.text = {text};

    //

    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;
    widget.color = color;

    widget.layout_mode = axiom::layout_mode::VOID;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    widget.drop_height = h2;
    widget.drop_unit_height = h;
    widget.button_width = w;
    widget.selected = selected;

    widget.callback = callback;

    widget.z = 0.25f;

    //

    std::vector<drop_option> option_structs;
    for(std::string o : options) {
        drop_option option;
        option.label = o;
        
        option_structs.push_back(option);
    }

    widget.options = option_structs;

    return ui_system.insert_widget(widget);
}

capture_data drop_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(ui_system.window->cursor_pos, range)) return {z, true};
    }
    
    if(drop_down) {
        float height = glm::min(drop_height, drop_unit_height * (options.size() - 1));
        ranges = {
            vec4(position + vec2(0.0f, -height), vec2(position.x + size.x, position.y))
        };
        
        for(vec4 range : ranges) {
            if(includes(ui_system.window->cursor_pos, range)) return {z + 0.001f, true};
        }
    }

    return {z, false};
}

}