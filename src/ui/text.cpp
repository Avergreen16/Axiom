#include <ui/text.hpp>
#include <ui/system.hpp>

namespace axiom {
    
const float italic_factor = 1.0f / 3.5f;
const float bold_factor = 1.0f;

// for text rendering data
std::vector<uint> text_line_indices;
std::vector<text_line_data> line_data;

void text::shape() {
    sglyphs.clear();

    hb_buffer_t* buffer = hb_buffer_create();

    hb_buffer_add_utf8(
        buffer,
        string.c_str(),
        -1,
        0,
        -1
    );

    hb_font_set_scale(font->hb_font, text_size * 64, text_size * 64);

    hb_buffer_guess_segment_properties(buffer);

    hb_shape(font->hb_font, buffer, nullptr, 0);

    unsigned int count;

    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &count);
    hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &count);

    for (unsigned int i = 0; i < count; ++i) {
        uint32_t glyph_id = infos[i].codepoint;

        float x_advance = positions[i].x_advance / 64.0f;
        float y_advance = positions[i].y_advance / 64.0f;

        float x_offset = positions[i].x_offset / 64.0f;
        float y_offset = positions[i].y_offset / 64.0f;

        shaped_glyph sglyph;
        sglyph.glyph = glyph_id;
        sglyph.cluster = infos[i].cluster;
        sglyph.advance = {x_advance, y_advance};
        sglyph.offset = {x_offset, y_offset};

        sglyphs.push_back(sglyph);
    }
}

void text::measure() {
    lines.clear();

    tglyphs.clear();

    std::vector<text_glyph> line;
    std::vector<text_glyph> word;

    vec2 word_cursor = vec2(0.0f);
    vec2 cursor = vec2(0.0f, round(-font->ascender * (float(text_size) / font->base_unit)));
    vec2 total_cursor = vec2(0.0f);

    uint line_start = 0;
    float line_height = round((font->line_height - font->line_gap) * (float(text_size) / font->base_unit));

    //

    vec2 range = vec2(-axiom::max_float, axiom::max_float);

    for(shaped_glyph& sglyph : sglyphs) {
        auto& glyph_struct = font->glyphs[sglyph.glyph];

        if(glyph_struct.contours.size() == 0) {
            text_glyph tglyph;
            tglyph.position = position;
            tglyph.selection = vec4(word_cursor + vec2(0.0f, round(font->descender * (float(text_size) / font->base_unit))), sglyph.advance.x, line_height + sglyph.advance.y);

            glyph_key key;
            key.glyph = sglyph.glyph;
            key.size = text_size;
            
            tglyph.key = key;
            word.push_back(tglyph);
            
            word_cursor += sglyph.advance;

            //

            if(cursor.x + word_cursor.x > width && wrap && cursor.x != 0.0f) {
                range.y = glm::min(range.y, cursor.x + word_cursor.x);

                total_cursor.x += cursor.x;

                cursor.x = 0;
                cursor.y -= round(font->line_height * (float(text_size) / font->base_unit));

                lines.push_back({line_height, line_start});

                tglyphs.insert(tglyphs.end(), line.begin(), line.end());
                
                line.clear();

                //

                line_height = round(font->line_height * (float(text_size) / font->base_unit));
                line_start = tglyphs.size();
            } else {
                range.x = glm::max(range.x, cursor.x + word_cursor.x);
            }

            for(text_glyph& g : word) {
                g.position += cursor;
                g.selection.x += cursor.x;
                g.selection.y += cursor.y;
            }

            line.insert(line.end(), word.begin(), word.end());
            word.clear();

            cursor += word_cursor;
            word_cursor = vec2(0.0f);
        } else {
            vec2 position = word_cursor + vec2(sglyph.offset) + vec2(glyph_struct.bounding_box.xy()) * (float(text_size) / font->base_unit); /* */

            text_glyph tglyph;
            tglyph.position = position;
            tglyph.selection = vec4(word_cursor + vec2(0.0f, round(font->descender * (float(text_size) / font->base_unit))), sglyph.advance.x, line_height + sglyph.advance.y);

            glyph_key key;
            key.glyph = sglyph.glyph;
            key.size = text_size;

            //
            
            tglyph.key = key;

            word.push_back(tglyph);
            
            word_cursor += sglyph.advance;
        }
    }
    
    if(cursor.x + word_cursor.x > width && wrap) {
        range.y = glm::min(range.y, cursor.x + word_cursor.x);

        cursor.x = 0;
        cursor.y -= round(font->line_height * (float(text_size) / font->base_unit));

        tglyphs.insert(tglyphs.end(), line.begin(), line.end());
        line.clear();

        lines.push_back({line_height, line_start});

        line_height = round(font->line_height * (float(text_size) / font->base_unit));
        line_start = tglyphs.size();
    } else {
        range.x = glm::max(range.x, cursor.x + word_cursor.x);
    }
    
    for(text_glyph& g : word) {
        g.position += cursor;
        g.selection.x += cursor.x;
        g.selection.y += cursor.y;
    }
    line.insert(line.end(), word.begin(), word.end());
    tglyphs.insert(tglyphs.end(), line.begin(), line.end());
    cursor += word_cursor;
    total_cursor.x += cursor.x;
    
    lines.push_back({line_height, line_start});
    
    size = vec2(range.x, 0.0f);
    max_width = total_cursor.x;

    float offset = 0.0f;
    for(auto& f : lines) {
        offset += std::get<0>(f);
        size.y += std::get<0>(f);
    }

    for(text_glyph& glyph : tglyphs) {
        glyph.position.y += offset;
        glyph.selection.y += offset;
    }
    
    wrap_limits = range;
}

void text::touch() {
    for(text_glyph& tglyph : tglyphs) {
        vec2 position = tglyph.position;
        
        tglyph.position = glm::floor(position);

        //

        tglyph.key.phase_x = glm::floor((position.x - tglyph.position.x) * 4);
        tglyph.key.phase_y = glm::floor((position.y - tglyph.position.y) * 4);

        axiom::ecs.get_system<axiom::ui_system>().font_handler.touch_phase(tglyph.key);
    }

    //if(lines.size() > 1) std::cout << size << "\n";
}

std::vector<ui_vertex> create_char(axiom::font& font, axiom::text_glyph& glyph) {
    std::vector<ui_vertex> ret;

    ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

    ret.push_back(a);
    ret.push_back(b);
    ret.push_back(d);
    ret.push_back(a);
    ret.push_back(d);
    ret.push_back(c);

    auto& phase = font.phases[glyph.key];
    if(phase.texture.z == 0) return {};

    for(ui_vertex& v : ret) {
        vec2 pos = glyph.position;

        ivec2 size = phase.texture.zw() - phase.texture.xy();

        v.pos = vec3(v.pos.xy() * vec2(size) + vec2(pos), 0.0f);
        v.tex_pos = vec2(phase.texture.xy()) + v.tex_pos * vec2(phase.texture.zw() - phase.texture.xy());
    }

    return ret;
}

std::vector<ui_vertex> text::mesh() {
    std::vector<ui_vertex> vs = select_vertices;

    for(text_glyph& tglyph : tglyphs) {
        auto tvs = create_char(*font, tglyph);

        vs.insert(vs.end(), tvs.begin(), tvs.end());
    }

    return vs;
}

bool text::collide(vec2 cursor) {
    /*
    int ii = -1;
    ii = compute_cursor_index(*font, *this, cursor - position, false, false, false).first;

    return ii != -1;
    */

    return false;
}

std::string text::retrieve() {
    /*
    std::u32string u32str = convert_string(string);

    int minv = glm::min(select_range.x, select_range.y);
    int maxv = glm::max(select_range.x, select_range.y);

    std::u32string str(u32str.begin() + minv, u32str.begin() + maxv);

    return convert_string(str);
    */

    return "";
}

void text::call() {
    //

    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    text_size = 12;

    if(collide(ui_system.window->cursor_pos) && ui_system.cursor_clip(clip, ui_system.window->cursor_pos)) {
        ulong current = parent;
        bool hover = false;
        while(true) {
            if(current == ui_system.hover_capture) {
                hover = true;
                break;
            } else if(current == NULL_WIDGET) break;
            current = ui_system.widgets[current]->parent;
        }

        if(hover) ui_system.cursor.cursor_mode = axiom::cursor_mode::TEXT;
    }
}

bool select(axiom::text& text, vec2 anchor, vec2 pos, ivec2& range) {
    int anchor_index = cursor_index(text, anchor);
    int pos_index = cursor_index(text, pos);

    if(anchor_index > pos_index) {
        range = {pos_index, anchor_index};
    } else range = {anchor_index, pos_index};

    return !(range.x < 0 && range.y < 0) && !(range.x > (int)text.sglyphs.size() && range.y > (int)text.sglyphs.size());
}

int cursor_index(axiom::text& text, vec2 pos) {
    vec2 rel = pos - text.position;
    float line_height = round(text.font->line_height * (float(text.text_size) / text.font->base_unit));

    int line = int(text.lines.size()) - 1 - floor(rel.y / line_height);

    if(line < 0) return -1;
    if(line >= text.lines.size()) return text.tglyphs.size() + 1;

    ivec2 range;
    range.x = std::get<1>(text.lines[line]);

    if(text.lines.size() == line + 1) range.y = text.sglyphs.size();
    else range.y = std::get<1>(text.lines[line + 1]);

    float cursor = 0.0;

    int i;
    for(i = range.x; i < range.y; ++i) {
        shaped_glyph& glyph = text.sglyphs[i];

        if(cursor > rel.x) {
            float diff = rel.x - cursor;
            if(diff < -glyph.advance.x * 0.5) return i - 1;
            return i;
        }
        
        cursor += glyph.advance.x;
    }

    return i;
}

vec2 cursor_pos(axiom::text& text, uint index) {
    int line;
    vec2 pos = vec2(0.0f);

    for(line = 0; line < text.lines.size(); ++line) {
        if(std::get<1>(text.lines[line]) > index) {
            break;
        }
        pos.y -= std::get<0>(text.lines[line]);
    }
    --line;

    pos.y += text.size.y;

    //

    ivec2 range;
    range.x = std::get<1>(text.lines[line]);
    if(text.lines.size() == line + 1) range.y = text.sglyphs.size();
    else range.y = std::get<1>(text.lines[line + 1]);

    std::cout << range.x << " " << range.y << "\n";

    for(int i = range.x; i < index; ++i) {
        shaped_glyph& glyph = text.sglyphs[i];

        pos.x += glyph.advance.x;
    }

    return pos + text.position;
}

std::vector<ui_vertex> mesh_selection(axiom::text& text, ivec2 range) {
    std::vector<ui_vertex> vertices;

    uint clip = text.clip;

    for(int i = glm::max(range.x, 0); i < glm::min(range.y, (int)text.tglyphs.size()); ++i) {
        vec4 quad = text.tglyphs[i].selection;
    
        std::vector<ui_vertex> vs = {
            axiom::ui_vertex{
                vec3(quad.xy(), text.z),
                vec2(1.0f, 63.0f),
                vec4(0.35f, 0.35f, 1.0f, 0.5f),
                0x1,
                clip
            },
            
            axiom::ui_vertex{
                vec3(quad.xy() + vec2(1.0f, 0.0f) * quad.zw(), text.z),
                vec2(1.0f, 63.0f),
                vec4(0.35f, 0.35f, 1.0f, 0.5f),
                0x1,
                clip
            },
            
            axiom::ui_vertex{
                vec3(quad.xy() + vec2(0.0f, 1.0f) * quad.zw(), text.z),
                vec2(1.0f, 63.0f),
                vec4(0.35f, 0.35f, 1.0f, 0.5f),
                0x1,
                clip
            },
            
            axiom::ui_vertex{
                vec3(quad.xy() + vec2(1.0f, 1.0f) * quad.zw(), text.z),
                vec2(1.0f, 63.0f),
                vec4(0.35f, 0.35f, 1.0f, 0.5f),
                0x1,
                clip
            }
        };

        vs = {
            vs[0],
            vs[1],
            vs[3],
            vs[0],
            vs[3],
            vs[2],
        };

        vertices.insert(vertices.end(), vs.begin(), vs.end());
    }
    
    return vertices;
}

}