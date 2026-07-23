#include <ui/panel_widget.hpp>

#include <GLFW/glfw3.h>

namespace axiom {
    
void panel_widget::init() {
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
}


void panel_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    float height = size.y;

    float new_total_scrollable = 0.0f;
    if(children.size()) {
        new_total_scrollable = ui_system.widgets[children[0]]->size.y - height + ui_system.widgets[children[0]]->buffer.y + ui_system.widgets[children[0]]->buffer.w;
    }

    if(new_total_scrollable != total_scrollable) {
        child_offset.y = child_offset.y + total_scrollable - new_total_scrollable;
        if(new_total_scrollable < 0.0f) child_offset.y = -new_total_scrollable;
        else if(child_offset.y > 0.0f) child_offset.y = 0.0f;
        total_scrollable = new_total_scrollable;
        dirty = true;
    }

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

    view_range = vec4(position, position + size);
}

void panel_widget::mesh() {
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

        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(position + v.pos.xy() * size, z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.125f, 0.125f, 0.125f, 1.0f);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

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

void panel_widget::on_place() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    float height = child_region.w - child_region.y;

    float new_total_scrollable = 0.0f;
    if(children.size()) new_total_scrollable = ui_system.widgets[children[0]]->size.y - height;

    if(new_total_scrollable != total_scrollable) {
        child_offset.y = child_offset.y + total_scrollable - new_total_scrollable;
        if(new_total_scrollable < 0.0f) child_offset.y = -new_total_scrollable;
        else if(child_offset.y > 0.0f) child_offset.y = 0.0f;
        total_scrollable = new_total_scrollable;
        dirty = true;
    }
}

uint64_t panel_widget::insert(float scroll_width, bool reserve) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    panel_widget widget;
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

capture_data panel_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size + vec2(-scroll_width * reserve, 0.0f)),
        vec4(position, position + size)
    };

    if(includes(ui_system.window->cursor_pos, ranges[0])) return {z, true, true};
    else if(includes(ui_system.window->cursor_pos, ranges[1])) return {z, true, false};

    return {z, false};
}

}