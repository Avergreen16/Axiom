#include <ui/widgets/menu_widget.hpp>
#include <ui/system.hpp>

namespace axiom {

void menu_widget::handle_inputs() {
    axiom::ui_system &ui_system = axiom::ecs.get_system<axiom::ui_system>();

    menu_node *rootn = root.get();
    for(uint p : path) {
        rootn = &rootn->children[p];
    }

    float height = glm::min(drop_unit_height * rootn->children.size(), drop_height);

    //

    float content_height = drop_unit_height * rootn->children.size();
    float panel_height = glm::min(content_height, drop_height);

    float offset = 0.0f;

    vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);
    range = {range.xy(), range.xy() + range.zw()};

    if(includes(ui_system.window->cursor_pos, range) && ui_system.hover_capture == self && (ui_system.click_capture == NULL_WIDGET || ui_system.click_capture == self)) {
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

        float origin = position.y + (panel_height + offset) + scroll;

        uint rel = -floor((ui_system.window->cursor_pos.y - origin) / drop_unit_height + 1);

        if(hovered != rel) dirty = true;
        hovered = rel;

        if(hovered != index && index != 0xFFFFFFFF) {
            timer += axiom::ecs.delta_time;
        } else {
            timer = 0.0;
        }

        if(timer > 0.5 && children.size()) {
            ui_system.delete_buffer.push_back(children[0]);
            children.clear();
            index = 0xFFFFFFFF;
        }

        if(rootn->children[hovered].children.size()) {
            // if((widget.pressed || (pressed_siblings && widget.hovered)) && w == NULL_WIDGET) {
            if(children.size() && index != hovered) {
                ui_system.delete_buffer.push_back(children[0]);
                children.clear();
            }

            ui_system.position(axiom::position_mode::BOTTOM_LEFT);

            if(children.size() == 0) {
                vec2 pos = vec2(position.x + button_width, origin - drop_unit_height * (hovered));

                auto new_path = path;
                new_path.push_back(hovered);

                ui_system.input_set(self);
                int max_size = 64;

                vec3 child_color = color;
                //if(color == child_color) child_color *= 0.9f;

                menu_widget::insert(pos + vec2(0.0f, -glm::min(float(rootn->children[hovered].children.size()) * drop_unit_height, (float)max_size)), z, child_color, 160, 16, max_size, root, new_path);
                index = hovered;
            }

            /*
            if(w != NULL_WIDGET) {
                widget.pressed = true;
            }

            bool hover_self = widget.hovered || w != NULL_WIDGET && includes(ui_system.window->cursor_pos, vec4(ui_system.widgets[w]->position, ui_system.widgets[w]->position + ui_system.widgets[w]->size));

            if(!hover_self && ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) || hovered_siblings) {
                widget.children.clear();
                ecs.get_system<ui_system>().delete_buffer.push_back(w);
                w = NULL_WIDGET;
            }
            */
        }

        if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
            clicked = hovered;
            if(!rootn->children[clicked].children.size()) {
                ui_system.delete_buffer.push_back(self);
            }
            
            rootn->children[clicked].callback();
        }
    } else {
        if(hovered != 0xFFFFFFFF)
            dirty = true;
        hovered = 0xFFFFFFFF;
    }

    {   
        if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
            std::vector<uint64_t> parent_children;

            uint64_t current = self;
            while (true) {
                parent_children.push_back(current);

                auto &w = ui_system.widgets[current];
                if(w->children.size()) {
                    current = w->children[0];
                }
                else
                    break;
            }

            current = self;
            while (true) {
                parent_children.push_back(current);

                auto &w = ui_system.widgets[current];
                if(w->parent != NULL_WIDGET) {
                    if(menu_widget* menu = dynamic_cast<menu_widget*>(ui_system.widgets[w->parent].get()))
                    {
                        current = w->parent;
                    }
                    else
                        break;
                }
                else
                    break;
            }

            bool erase = true;
            for(uint64_t w : parent_children) {
                auto &ww = ui_system.widgets[w];
                menu_widget* menu = dynamic_cast<menu_widget*>(ww.get());

                if(menu->hovered != 0xFFFFFFFF) {
                    axiom::menu_node* rnode = menu->root.get();
                    for(uint32_t path : menu->path) {
                        rnode = &rnode->children[path];
                    }

                    if(rnode->children[menu->hovered].children.size() != 0) {
                        erase = false;
                        break;
                    }
                }
            }

            if(erase) {
                ui_system.delete_buffer.push_back(self);
            }
        }
    }

    // callback(*this);
}

void menu_widget::mesh() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    if(hovered != 0xFFFFFFFF) ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;

    menu_node *rootn = root.get();
    for(uint p : path) {
        rootn = &rootn->children[p];
    }

    ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

    std::vector<std::pair<std::vector<ui_vertex>, vec2>> label_vertices;
    label_vertices.reserve(rootn->children.size());

    z = 0.5f;

    if(dirty) {

        axiom::ui_system &ui_system = axiom::ecs.get_system<axiom::ui_system>();

        vertices_before.clear();

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        //
        float content_height = drop_unit_height * rootn->children.size();
        float panel_height = glm::min(drop_unit_height * rootn->children.size(), drop_height);

        float offset = 0.0f;

        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);

        vec4 col = vec4(color * 0.875f, 1.0f);

        vec4 panel_range = {range.xy(), range.xy() + range.zw()};

        for(ui_vertex &v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = col;
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // hovered

        if(hovered != 0xFFFFFFFF) {
            std::vector<ui_vertex> ret = {a, b, d, a, d, c};

            uint h = hovered;

            vec4 range = vec4(position.x, position.y + panel_height + offset - (drop_unit_height * (float(h) + 1.0f)) + scroll, size.x, drop_unit_height);
            vec4 col = vec4(color, 1.0f);

            for(ui_vertex &v : ret) {
                v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = col;
                v.data = 0x1;
                v.range = panel_range;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }

        // all options
        float pos = scroll - (panel_height + offset);
        for(int i = 0; i < rootn->children.size(); ++i) {
            
            std::vector<ui_vertex> vs = text[i]->mesh();

            vec2 origin = position + vec2(4.0f, panel_height + scroll - (drop_unit_height * float(i + 1.0f)) + 2.0f);
            origin = round(origin);

            text[i]->position = origin;

            for(auto &vertex : vs) {
                vertex.pos += vec3(origin, z);
                vertex.range = panel_range;
            }
            vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());

            if(rootn->children[i].children.size()) {
                std::vector<ui_vertex> vs = {a, b, d, a, d, c};

                vec2 pos = position + (vec2(size.x - drop_unit_height, panel_height + scroll - (drop_unit_height * float(i + 1.0f))) + (drop_unit_height - 8.0f) * 0.5f);

                for(ui_vertex &v : vs) {
                    
                    v.pos = vec3(pos + v.pos.xy() * 8.0f, z);
                    v.tex_pos = vec2(26, 48) + v.tex_pos * 8.0f;
                    v.data = 0x1;
                    v.range = panel_range;
                }

                vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
            }
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

            for(ui_vertex &v : ret) {
                v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(1.0f, 1.0f, 1.0f, 0.5f);
                v.data = 0x1;
                v.range = panel_range;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }
    }
}

void menu_widget::init() {
}

uint64_t menu_widget::insert(vec2 position, float z, vec3 color, float w, float h, float h2, std::shared_ptr<menu_node> root, std::vector<uint> path) {
    axiom::ui_system &ui_system = axiom::ecs.get_system<axiom::ui_system>();

    menu_widget widget;

    menu_node *rootn = root.get();
    for(uint p : path) {
        rootn = &rootn->children[p];
    }

    for(menu_node& child : rootn->children) {
        std::shared_ptr<axiom::text> text(new axiom::text);
        text->font = ui_system.font_assets[0];
        text->string = child.label;
        text->wrap = false;

        widget.text.push_back(text);
    }

    vec2 size = vec2(w, glm::min(h * rootn->children.size(), h2));

    if(ui_system.input_state.active_position == axiom::position_mode::TOP_LEFT) {
        position.y -= size.y;
    } else if(ui_system.input_state.active_position == axiom::position_mode::BOTTOM_RIGHT) {
        position.x -= size.x;
    } else if(ui_system.input_state.active_position == axiom::position_mode::TOP_RIGHT) {
        position.x -= size.x;
        position.y -= size.y;
    }
    
    widget.position = position;
    widget.z = z;

    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;
    widget.size = size;

    widget.drop_height = h2;
    widget.drop_unit_height = h;
    widget.button_width = w;
    widget.color = color;

    widget.root = root;
    widget.path = path;

    return ui_system.insert_widget(widget);
}

// capture data

capture_data menu_widget::handle_capture() {
    axiom::ui_system &ui_system = axiom::ecs.get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(ui_system.window->cursor_pos, range)) return {self, z, true};
    }

    return {self, z, false};
}

}