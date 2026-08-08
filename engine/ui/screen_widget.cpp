#include <ui/screen_widget.hpp>
#include <ui/ui_system.hpp>

#include <GLFW/glfw3.h>

namespace axiom {

void screen_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    vec4 region;

    if(ui_system.window->is_fullscreen()) fullscreen = true;
    else fullscreen = false;

    if(ui_system.hover_capture == self && (ui_system.click_capture == NULL_WIDGET || ui_system.click_capture == self)) {
        region = vec4(size.x - header, size.y - header, header, header);
        region.z += region.x;
        region.w += region.y;
        if(includes(ui_system.window->cursor_pos, region)) {
            hover_close = true;
            ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                // close
                glfwSetWindowShouldClose(ui_system.window->window_handle, GLFW_TRUE);
            }
        } else hover_close = false;

        region = vec4(size.x - header * 2.0f, size.y - header, header, header);
        region.z += region.x;
        region.w += region.y;
        if(includes(ui_system.window->cursor_pos, region)) {
            hover_maximize = true;
            ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                // maximize/demaximize
                if(ui_system.window->is_fullscreen()) {
                    ui_system.window->make_windowed();
                } else if(ui_system.window->is_maximized()) {
                    ui_system.window->make_windowed();
                } else { 
                    ui_system.window->make_maximized();
                }
            }
        } else hover_maximize = false;

        region = vec4(size.x - header * 3.0f, size.y - header, header, header);
        region.z += region.x;
        region.w += region.y;
        if(includes(ui_system.window->cursor_pos, region)) {
            hover_minimize = true;
            ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;
            
            if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                // minimize
                if(ui_system.window->is_fullscreen()) {
                    ui_system.window->make_windowed();
                }
                ui_system.window->make_minimized();
            }
        } else hover_minimize = false;
    } else {
        hover_close = false;
        hover_maximize = false;
        hover_minimize = false;
    }
}

void screen_widget::mesh() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    float text_scale = 1.0f;
    vec3 background_color = vec3(0.5f);

    vertices_before.clear();

    ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
    std::vector<ui_vertex> ret;

    if(fullscreen || true) {
        // title bar
        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, size.y - header) + v.pos.xy() * vec2(size.x, header), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(color, 1.0f);
            v.data = 1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // icons
        
        // axiom icon
        vec4 r = vec4(0.0f, size.y - header, header, header);
        vec2 nsize = vec2(16);
        vec4 texture_range = vec4(32, 16, 16, 16);

        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
            //v.range = header_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // text
        /*
        ret = mesh_text(gui_system.fonts["default mono"], label, 1);
        vec2 origin = vec2(header, size.y - header);
        float height = gui_system.fonts["default mono"].line_height;
        origin += (header - height) * 0.5f;
        
        for(ui_vertex& v : ret) {
            v.pos = vec3(round(origin) + v.pos.xy() * float(text_scale), z);
            v.data = 0;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        */

        vec4 range = vec4(59, 9, 64, 14);

        // icons
        
        vec3 hover_color = clamp(color + 0.15f, 0.0f, 1.0f);
        vec3 col;
        
        // close
        r = vec4(size.x - header, size.y - header, header, header);
        nsize = vec2(10);
        texture_range = vec4(14, 54, 10, 10);
        
        col = hover_close ? hover_color : color;
        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);
            //v.range = header_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
            //v.range = header_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // maximize
        r = vec4(size.x - header * 2.0f, size.y - header, header, header);
        nsize = vec2(10);
        texture_range = vec4(14, 24, 10, 10);
        if(glfwGetWindowAttrib(ui_system.window->window_handle, GLFW_MAXIMIZED) || fullscreen) texture_range = vec4(14, 34, 10, 10);
        
        col = hover_maximize ? hover_color : color;
        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
            //v.range = header_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // minimize
        r = vec4(size.x - header * 3.0f, size.y - header, header, header);
        nsize = vec2(10);
        texture_range = vec4(14, 44, 10, 10);

        col = hover_minimize ? hover_color : color;
        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        
        // panel
        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * vec2(size.x, size.y - header), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(background_color, 1.0f);
            v.data = 1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
    } else {
        // panel
        ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * vec2(size.x, size.y), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(background_color, 1.0f);
            v.data = 1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
    }
}

void screen_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        position = vec2(0.0f);
        if(!ui_system->window->is_minimized()) size = ui_system->window->screen_size;
    };
    before.push_back(c);

    c.func = [this, ui_system]() {
        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = ui_system->widgets[children[i]];

            if(fullscreen || true) p0->size.y = size.y - header;
            else p0->size.y = size.y;
            
            p0->size.x = size.x;
            
            p0->position.x = position.x;

            p0->position.y = position.y;
        }
    };
    before.push_back(c);
}

ulong screen_widget::insert(std::string name, vec3 color) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    screen_widget widget;
    widget.flag = true;
    widget.label = name;
    widget.color = color;
    
    widget.layout_mode = axiom::layout_mode::VOID;
    widget.position_mode = axiom::position_mode::STATIC;

    return ui_system.insert_widget(widget, true);
}

capture_data screen_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(ui_system.window->cursor_pos, range)) return {self, z, true};
    }

    return {self, z, false};
}

}