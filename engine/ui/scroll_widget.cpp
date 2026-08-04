#include <ui/scroll_widget.hpp>

#include <GLFW/glfw3.h>

namespace axiom {
    
void scroll_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        float scrollable = 0.0f;
        if(children.size()) {
            scrollable = ui_system->widgets[children[0]]->size.y + ui_system->widgets[children[0]]->buffer.y + ui_system->widgets[children[0]]->buffer.w - size.y;
        }

        scroll_pos = glm::clamp(scroll_pos, -scrollable, 0.0f);

        //
        
        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = ui_system->widgets[children[i]];

            float scrollbar = 0.0f;
            if(reserve) scrollbar = scroll_width;

            p0->size.x = size.x - scrollbar - (p0->buffer.x + p0->buffer.z);

            p0->size.y = size.y - (p0->buffer.y + p0->buffer.w);
            
            p0->position.x = position.x + p0->buffer.x;

            p0->position.y = position.y + p0->buffer.y - scroll_pos;
        }
    };
    before.push_back(c);

    c.func = [this, ui_system]() {
        // update total scrollable

        float new_total_scrollable = 0.0f;
        if(children.size()) {
            new_total_scrollable = ui_system->widgets[children[0]]->size.y - size.y + ui_system->widgets[children[0]]->buffer.y + ui_system->widgets[children[0]]->buffer.w;
        }

        if(new_total_scrollable != total_scrollable) {
            total_scrollable = new_total_scrollable;
            dirty = true;
        }

        //

        float new_child_height = ui_system->widgets[children[0]]->size.y;
        float new_self_height = size.y;

        bool o = false;

        float delta_scroll = 0.0f;

        if(state_child_height != new_child_height || state_self_height != new_self_height) {
            state_child_height = new_child_height;
            state_self_height = new_self_height;

            auto& child = ui_system->widgets[children[0]];

            if(anchor_widget == NULL_WIDGET || std::find(child->children.begin(), child->children.end(), anchor_widget) == child->children.end()) {
                anchor_widget = NULL_WIDGET;
                anchor_mode = 2;
                
                //std::cout << "NOT FOUND\n";
            }

            float new_scroll = scroll_pos;
            
            if(anchor_widget != NULL_WIDGET) {

                auto& parent_widget = ui_system->widgets[children[0]];

                uint32_t index = 0;
                bool found = false;
                while(true) {
                    if(index >= parent_widget->children.size()) break;
                    if(parent_widget->children[index] == anchor_widget) {
                        found = true;
                        break;
                    }

                    ++index;
                }

                if(!found) {
                    std::cout << "x";
                    int index = 0;
                    while(true) {
                        if(prev_order[index] == anchor_widget) break;
                        ++index;
                    }

                    int down_index = index - 1;
                    bool found = false;
                    while(true) {
                        if(down_index < 0) break;
                        if(ui_system->widgets.contains(prev_order[down_index])) {
                            found = true;
                            break;
                        }

                        --down_index;
                    }

                    if(found) {
                        anchor_widget = prev_order[down_index];
                        anchor_frac = 0.0f;
                    } else {
                        int up_index = index - 1;
                        while(true) {
                            if(up_index >= prev_order.size()) break;
                            if(ui_system->widgets.contains(prev_order[up_index])) {
                                found = true;
                                break;
                            }

                            ++up_index;
                        }

                        if(found) {
                            anchor_widget = prev_order[up_index];
                            anchor_frac = 1.0f;
                        }
                    }
                }

                index = 0;
                while(true) {
                    if(parent_widget->children[index] == anchor_widget) break;
                    ++index;
                }
                
                auto& widget = ui_system->widgets[parent_widget->children[index]];

                float start;
                float end = widget->position.y + scroll_pos - (position.y + size.y);

                if(index != 0) {
                    auto& widget_prev = ui_system->widgets[ui_system->widgets[children[0]]->children[index - 1]];
                    start = widget_prev->position.y + scroll_pos - (position.y + size.y);
                } else start = widget->position.y + widget->size.y + scroll_pos - (position.y + size.y);

                float n = end + (start - end) * anchor_frac;//* (1.0f - anchor_frac); // TOP float new_scroll = -widget->size.y * (1.0f - anchor_frac);
                float p = scroll_pos;
                new_scroll = n + size.y;
                
                //std::cout << start << " " << end << " " << new_scroll << " | " << widget->position.y << " " << ui_system->widgets[children[0]]->position.y << " " << ui_system->widgets[children[0]]->size.y << " " << ui_system->widgets[children[0]]->max_width << " " << ui_system->widgets[children[0]]->min_width << " " << ui_system->widgets[children[0]]->max_height << " " << ui_system->widgets[children[0]]->min_height << " " << scroll_pos << "\n";

                //std::cout << index << " " << start << " " << end << " " << new_scroll << "\n";
            }
            if(anchor_mode == 1) {
                new_scroll = 0.0f;
            } else if(anchor_mode == 2) {
                new_scroll = FLT_MAX * -0.5f;
            } 

            delta_scroll = scroll_pos - new_scroll;
            scroll_pos = new_scroll;
            
            float min_scroll = -total_scrollable;
            float max_scroll = 0.0f;
            scroll_pos = glm::clamp(scroll_pos, min_scroll, max_scroll);
        }

        float pos = 0.0f;
        float target = position.y - delta_scroll; // TOP float target = position.y + size.y;

        float prev = 0.0f;
        uint32_t index = 0;
        
        //
    
        while(true) {
            if(index >= ui_system->widgets[children[0]]->children.size()) {
                anchor_mode = 2;
                anchor_widget = ui_system->widgets[children[0]]->children.back();
                anchor_frac = 0.0f;

                break;
            }

            //

            auto& widget = ui_system->widgets[ui_system->widgets[children[0]]->children[index]];

            float end = widget->position.y;

            if(index == 0) prev = widget->position.y + widget->size.y;

            if(target - end > 0.0f) {
                float frac = (target - end) / (prev - end);

                anchor_frac = frac;

                ulong prev2 = anchor_widget;

                anchor_widget = ui_system->widgets[children[0]]->children[index];
                anchor_mode = 0;
                
                break;
            } else {
                prev = end;
            }

            ++index;
        }
        if(scroll_pos == 0.0f) {
            anchor_mode = 1;
        }

        //

        prev_order.clear();
        prev_order.reserve(ui_system->widgets[children[0]]->children.size());
        for(auto child : ui_system->widgets[children[0]]->children) {
            prev_order.push_back(child);
        }
    };
    after.push_back(c);
}


void scroll_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    float height = size.y;

    int scroll_speed = 60;

    float min_scroll = -total_scrollable;
    float max_scroll = 0.0f;
    scroll_pos = glm::clamp(scroll_pos, min_scroll, max_scroll);
    if(max_scroll > min_scroll) {
        if(ui_system.window->scroll_delta && includes(ui_system.window->cursor_pos, vec4(position, position + size))) {
            dirty = true;

            float scroll_speed = 60.0f;
            scroll_pos += ui_system.window->scroll_delta * scroll_speed;
            scroll_pos = glm::clamp(scroll_pos, min_scroll, max_scroll);

            anchor_mode = 0;
        }
    }

    if(reserve) {
        bool scrollbar = false;
        float scrollbar_height;
        float scrollbar_pos;

        if(total_scrollable > 0.0f) {
            scrollbar = true;

            float visible_height = size.y;
            float total_height = total_scrollable + visible_height;

            scrollbar_height = visible_height / total_height;
            scrollbar_pos = (scroll_pos) / total_height;

            scrollbar_height *= visible_height;
            scrollbar_pos *= visible_height;

            scrollbar_pos = (size.y - scrollbar_height + scrollbar_pos);
        }

        if(scrollbar) {
            vec2 ssize = vec2(scroll_width, scrollbar_height);
            vec2 spos = vec2(position.x + size.x - scroll_width, position.y + scrollbar_pos);

            if(includes(ui_system.window->cursor_pos, {spos, spos + ssize}) && ui_system.hover_capture == self) {
                ui_system.cursor.cursor_mode = ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_T; 
                
                if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    capture_scroll = true;
                    scroll_anchor = ui_system.window->cursor_pos.y - position.y - scrollbar_pos;
                }
            }
        }

        if(ui_system.click_capture == self && capture_scroll) {
            float rel_pos = ui_system.window->cursor_pos.y - position.y - scroll_anchor;

            scroll_pos = -(1.0f - rel_pos / (size.y - scrollbar_height)) * total_scrollable;
            scroll_pos = glm::clamp(scroll_pos, min_scroll, max_scroll);

            ui_system.cursor.cursor_mode = ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_T;
        }

        if(!ui_system.window->input_map[axiom::input_code::MOUSE_LEFT]) capture_scroll = false;
    }

    //view_range = vec4(position, position + size);
}

void scroll_widget::mesh() {
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

        /*
        {
            ret = {a, b, d, a, d, c};
            vec4 area = vec4(position, size);

            for(ui_vertex& v : ret) {
                v.pos = vec3(area.xy() + v.pos.xy() * area.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(1.0f, 0.0f, 0.0f, 0.5f);
                v.data = 1;
            }
            total_ret.insert(total_ret.end(), ret.begin(), ret.end());
        }
        */

        if(reserve) {
            ret = {a, b, d, a, d, c};
            vec4 area = vec4(position.x + size.x - scroll_width, position.y, scroll_width, size.y);

            for(ui_vertex& v : ret) {
                v.pos = vec3(area.xy() + v.pos.xy() * area.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(0.0625f, 0.0625f, 0.0625f, 1.0f);
                v.data = 1;
            }
            total_ret.insert(total_ret.end(), ret.begin(), ret.end());
        }

        vertices_before = total_ret;
        total_ret.clear();

        if(total_scrollable > 0.0f) {
            scrollbar = true;

            float visible_height = size.y;
            float total_height = total_scrollable + visible_height;

            scrollbar_height = visible_height / total_height;
            scrollbar_pos = (-scroll_pos) / total_height;

            scrollbar_height *= visible_height;
            scrollbar_pos *= visible_height;
        }
        
        if(scrollbar) {
            vec2 sb_size = vec2(scroll_width, scrollbar_height);
            vec2 sb_pos = vec2(position.x + size.x - scroll_width, position.y + ((size.y - scrollbar_height) - scrollbar_pos));

            ret = {a, b, d, a, d, c};
            for(ui_vertex& v : ret) {
                v.pos = vec3(sb_pos + v.pos.xy() * sb_size, z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(1.0f, 1.0f, 1.0f, 0.25f);
                v.data = 1;
            }
            total_ret.insert(total_ret.end(), ret.begin(), ret.end());
        }

        vertices_after = total_ret;
    }
}

uint64_t scroll_widget::insert(float scroll_width, bool reserve) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    scroll_widget widget;
    widget.position_mode = axiom::position_mode::BOTTOM_LEFT;
    widget.layout_mode = axiom::layout_mode::VOID;

    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = FLT_MAX;
    widget.buffer = ui_system.input_state.active_buffer;
    
    widget.scroll_width = scroll_width;
    widget.reserve = reserve;

    widget.flag = true;

    widget.size = vec2(0.0f);
    widget.position = vec2(0.0f);

    return ui_system.insert_widget(widget, true);
}

capture_data scroll_widget::handle_capture() {
    if(reserve) {
        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

        std::vector<vec4> ranges = {
            vec4(position + vec2(size.x - scroll_width, 0.0f), position + size),
        };

        if(includes(ui_system.window->cursor_pos, ranges[0])) return {z, true, false, true};
    }

    return {z, false};
}

}