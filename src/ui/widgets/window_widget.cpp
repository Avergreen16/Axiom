#include <ui/widgets/window_widget.hpp>
#include <ui/system.hpp>

namespace axiom {

ulong window_widget::insert(std::string label, ivec2 size, ivec2 position, vec3 color, std::function<void()> on_close) {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    window_widget widget;
    widget.size = size;
    widget.position = position;
    widget.label = label;
    widget.header_color = color;
    
    widget.attachments.resize(1);
    widget.has_clip = true;
    widget.attachments[0].has_clip = true;

    widget.layout_mode = axiom::layout_mode::NONE;
    widget.position_mode = axiom::position_mode::STATIC;

    widget.on_close = on_close;

    //

    std::shared_ptr<axiom::text> label_text(new axiom::text);
    
    label_text->string = label;
    label_text->font = &ui_system.fonts[0];
    label_text->position = position + ivec2(0.0f, size.y) + int((float(widget.header) - label_text->font->line_height * (float(label_text->text_size) / label_text->font->base_unit)) * 0.5f);

    widget.text.push_back(label_text);

    //

    return ui_system.insert_widget(widget, true);
}

void window_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    bool close_window = false;

    float text_scale = 1;
    std::string label = "WINDOW";
    bool scrollbar = false;

    vec4 range = vec4(size.x - header + position.x, size.y + position.y, header, header);
    range.z += range.x;
    range.w += range.y;

    if(includes(ui_system.window->cursor_pos, range)) {
        hover_close = true;
    } else hover_close = false;

    if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && hover_close) {
        auto children = ui_system.get_children(self);
        children.push_back(self);

        ui_system.delete_buffer.insert(ui_system.delete_buffer.end(), children.begin(), children.end());
    }

    //

    ivec4 buffer_range = ivec4(-resize_border, -resize_border, resize_border, resize_border);

    ivec4 range_move = {position + vec2(0.0f, size.y), position + vec2(size.x, size.y + header)};
    range_move += buffer_range;

    buffer_range = ivec4(-resize_border, -resize_border, resize_border, resize_border);

    ivec4 range_left = {position + vec2(0, 0), position + vec2(0, size.y + header)};
    ivec4 range_right = {position + vec2(size.x, 0), position + vec2(size.x, size.y + header)};
    ivec4 range_top = {position + vec2(0, size.y + header), position + vec2(size.x, size.y + header)};
    ivec4 range_bottom = {position + vec2(0, 0), position + vec2(size.x, 0)};
    // ivec4 hover_range = {position + vec2(0, -header - size.y), position + vec2(size.x, -header)};

    buffer_range = ivec4(-resize_border, -resize_border, 0.0f, resize_border);
    range_left += buffer_range;
    buffer_range = ivec4(0.0f, -resize_border, resize_border, resize_border);
    range_right += buffer_range;
    buffer_range = ivec4(-resize_border, 0.0f, resize_border, resize_border);
    range_top += buffer_range;
    buffer_range = ivec4(-resize_border, -resize_border, resize_border, 0.0f);
    range_bottom += buffer_range;

    vec2 min_size = vec2(16, 16);

    auto resize_left = [&]() {
        position.x += ui_system.window->cursor_delta.x;
        size.x -= ui_system.window->cursor_delta.x;

        float delta_max = (ui_system.window->cursor_pos.x) - position.x;
        position.x += delta_max;
        size.x -= delta_max;

        float delta_min = glm::max(0.0f, min_size.x - size.x);
        size.x += delta_min;
        position.x -= delta_min;
    };

    auto resize_right = [&]() {
        size.x += ui_system.window->cursor_delta.x;

        float delta_max = (position.x + size.x) - (ui_system.window->cursor_pos.x);
        size.x -= delta_max;

        float delta_min = glm::max(0.0f, min_size.x - size.x);
        size.x += delta_min;
    };

    auto resize_top = [&]() {
        float delta_min;
        size.y += ui_system.window->cursor_delta.y;

        float delta_max = (position.y + size.y + header) - (ui_system.window->cursor_pos.y);
        size.y -= delta_max;

        delta_min = glm::max(0.0f, min_size.y - size.y);
        size.y += delta_min;
    };

    auto resize_bottom = [&]() {
        float delta_min;
        position.y += ui_system.window->cursor_delta.y;
        size.y -= ui_system.window->cursor_delta.y;

        float delta_max = (ui_system.window->cursor_pos.y) - position.y;
        position.y += delta_max;
        size.y -= delta_max;

        delta_min = glm::max(0.0f, min_size.y - size.y);
        position.y -= delta_min;
        size.y += delta_min;
    };

    if(ui_system.click_capture == self) {
        switch (operation) {
        case 0:
            position += ui_system.window->cursor_delta;
            dirty = true;
            break;
        case 1:
            resize_left();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_L;
            dirty = true;

            break;
        case 2:
            resize_right();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_R;
            dirty = true;

            break;
        case 3:
            resize_bottom();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_B;
            dirty = true;

            break;
        case 4:
            resize_left();
            resize_bottom();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_BL;
            dirty = true;

            break;
        case 5:
            resize_right();
            resize_bottom();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_BR;
            dirty = true;

            break;
        case 6:
            resize_top();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_T;
            dirty = true;

            break;
        case 7:
            resize_left();
            resize_top();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_TL;
            dirty = true;

            break;
        case 8:
            resize_right();
            resize_top();
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_TR;
            dirty = true;

            break;
        }
    } else {
        operation = NULL_OPERATION;
    }

    if(operation == NULL_OPERATION && (ui_system.hover_capture == self || ui_system.click_capture == self)) {
        uint op = NULL_OPERATION;

        bool left_cont = includes(ui_system.window->cursor_pos, range_left);
        bool right_cont = includes(ui_system.window->cursor_pos, range_right);
        bool top_cont = includes(ui_system.window->cursor_pos, range_top);
        bool bottom_cont = includes(ui_system.window->cursor_pos, range_bottom);

        if(left_cont && bottom_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_BL;
            op = 4;
        } else if(right_cont && bottom_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_BR;
            op = 5;
        } else if(left_cont && top_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_TL;
            op = 7;
        } else if(right_cont && top_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_TR;
            op = 8;
        } else if(left_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_L;
            op = 1;
        } else if(right_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_R;
            op = 2;
        } else if(bottom_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_B;
            op = 3;
        } else if(top_cont) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_T;
            op = 6;
        }

        if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
            if(op != NULL_OPERATION) {
                operation = op;
            }
            else {
                if(includes(ui_system.window->cursor_pos, range_move)) {
                    operation = 0;
                }
                else {
                    operation = NULL_OPERATION;
                }
            }
        }
    }

    if(hover_close) ui_system.cursor.cursor_mode = axiom::cursor_mode::CLICK;
}

void window_widget::mesh() {
    if(dirty) {
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

        float radius = 6.0f;

        ui_system.clip_spaces[clip].range = vec4(position, position + vec2(size.x, size.y + header));
        ui_system.clip_spaces[clip].radius = radius;
        
        ui_system.clip_spaces[attachments[0].clip].range = {position, position + size};


        float text_scale = 1;
        bool scrollbar = false;
        float shadow_width = 6;

        //

        vec4 range;

        vec4 header_range = vec4(position + vec2(0.0f, size.y), position + vec2(size.x, size.y + header));

        std::vector<ui_vertex> total_ret;

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        // panel
        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        for(ui_vertex &v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * size, z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.25f, 0.25f, 0.25f, 1.0f);
            v.data = 1;

            v.clip_space = clip;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // header
        ret = {a, b, d, a, d, c};
        for(ui_vertex &v : ret) {
            v.pos = vec3(position + vec2(0.0, size.y) + v.pos.xy() * vec2(size.x, header), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(header_color, 1.0f);
            v.data = 1;
            
            v.clip_space = clip;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // label

        ret = text[0]->mesh();

        for(ui_vertex& v : ret) {
            float s = floor(header * 0.5f - 11.0f * float(text_scale) * 0.5f);
            v.pos = vec3(v.pos.xy() + text[0]->position, z);
            //v.range = intersect_range(view_range, header_range);
            v.clip_space = clip;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        //

        // close button

        vec3 hover_color = clamp(header_color + 0.15f, 0.0f, 1.0f);
        vec3 col;

        // close
        vec4 r = vec4(size.x - header + position.x, size.y + position.y, header, header);
        vec2 nsize = vec2(10);
        vec4 texture_range = vec4(14, 54, 10, 10);

        col = hover_close ? hover_color : header_color;
        ret = {a, b, d, a, d, c};
        for(ui_vertex &v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);

            v.clip_space = clip;
            //v.range = intersect_range(view_range, header_range);
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(ui_vertex &v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
            
            v.clip_space = clip;
            //v.range = intersect_range(view_range, header_range);
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        //
        // shadow
        //

        float shadow_w = 0.25f;

        // left
        ret = {a, b, d, a, d, c};
        ret[0].color.w = 0.0f;
        ret[3].color.w = 0.0f;
        ret[5].color.w = 0.0f;
        range = {position + vec2(-shadow_width, radius), position + vec2(0.0, size.y + header - radius)};
        for(ui_vertex &v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;

            //v.clip_space = clip;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // right
        ret = {a, b, d, a, d, c};
        ret[1].color.w = 0.0f;
        ret[2].color.w = 0.0f;
        ret[4].color.w = 0.0f;
        range = {position + vec2(size.x, radius), position + vec2(size.x + shadow_width, size.y + header - radius)};
        for(ui_vertex &v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;

            //v.clip_space = clip;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // top
        ret = {a, b, d, a, d, c};
        ret[2].color.w = 0.0f;
        ret[4].color.w = 0.0f;
        ret[5].color.w = 0.0f;
        range = {position + vec2(radius, size.y + header), position + vec2(size.x - radius, shadow_width + size.y + header)};
        for(ui_vertex &v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;

            //v.clip_space = clip;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // bottom
        ret = {a, b, d, a, d, c};
        ret[0].color.w = 0.0f;
        ret[1].color.w = 0.0f;
        ret[3].color.w = 0.0f;
        range = {position + vec2(radius, -shadow_width), position + vec2(size.x - radius, 0.0f)};
        for(ui_vertex &v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;

            //v.clip_space = clip;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // curves

        std::vector<ui_vertex> outer_vertices;
        std::vector<ui_vertex> inner_vertices;

        for(int i = 0; i < 6; ++i) {
            float frac = float(i) / (5.0f);

            vec2 pos = vec2(cos((1.0f - frac) * axiom::pi * 0.5f), sin((1.0f - frac) * axiom::pi * 0.5f));

            ui_vertex vv;
            vv.tex_pos = vec2(1.0f, 63.0f);
            vv.data = 1;

            vv.pos = vec3(pos * radius, z);
            vv.color = vec4(0.0f, 0.0f, 0.0f, shadow_w);

            inner_vertices.push_back(vv);

            vv.pos = vec3(pos * (radius + shadow_width), z);
            vv.color = vec4(0.0f, 0.0f, 0.0f, 0.0f);

            outer_vertices.push_back(vv);
        }

        std::vector<mat2> orientations = {
            mat2(-1.0f, 0.0f, 0.0f, -1.0f),
            mat2(0.0f, -1.0f, 1.0f, 0.0f),
            mat2(0.0f, 1.0f, -1.0f, 0.0f),
            mat2(1.0f, 0.0f, 0.0f, 1.0f),
        };
        std::vector<vec2> positions = {
            vec2(radius, radius),
            vec2(size.x - radius, radius),
            vec2(radius, size.y + header - radius),
            vec2(size.x - radius, size.y + header - radius),
        };

        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 5; ++j) {
                int a = j;
                int b = j + 1;

                ui_vertex va = inner_vertices[a];
                ui_vertex vb = inner_vertices[b];
                ui_vertex vc = outer_vertices[a];
                ui_vertex vd = outer_vertices[b];

                va.pos = vec3(position + orientations[i] * va.pos.xy() + positions[i], va.pos.z);
                vb.pos = vec3(position + orientations[i] * vb.pos.xy() + positions[i], vb.pos.z);
                vc.pos = vec3(position + orientations[i] * vc.pos.xy() + positions[i], vc.pos.z);
                vd.pos = vec3(position + orientations[i] * vd.pos.xy() + positions[i], vd.pos.z);
                
                total_ret.push_back(va);
                total_ret.push_back(vb);
                total_ret.push_back(vd);
                total_ret.push_back(va);
                total_ret.push_back(vd);
                total_ret.push_back(vc);
            }
        }

        vertices_before = total_ret;

        dirty = false;
    }
}

void window_widget::init() {
    axiom::ui_system* ui_system = &axiom::ecs.get_system<axiom::ui_system>();
    
    widget_constraint c;
    c.func = [this, ui_system]() {
        attachments[0].region = {position, size};

        for(int i = 0; i < children.size(); ++i) {
            auto &p0 = ui_system->widgets[children[i]];
            auto attachment = attachments[child_attachments[i]];

            p0->size.x = attachment.region.z;
            p0->size.y = attachment.region.w;

            p0->position.x = attachment.region.x;
            p0->position.y = attachment.region.y;
        }
    };
    before.push_back(c);
    
    c.func = [this]() {
        float a = (float(header) - text[0]->size.y) * 0.5f;

        text[0]->position = position + vec2(0.0f, size.y) + vec2(a + 2.0f, a);
        text[0]->position = round(text[0]->position);
    };
    after.push_back(c);
}

capture_data window_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    std::vector<vec4> ranges = {
        vec4(position - (float)resize_border, position + vec2(size.x, size.y + header) + (float)resize_border)};

    for(vec4 range : ranges) {
        if(includes(ui_system.window->cursor_pos, range))
            return {self, z, true, false, true};
    }

    return {self, z, false};
}

void window_widget::on_delete() {
    on_close();
}

}