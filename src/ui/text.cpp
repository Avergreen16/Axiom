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
    line_heights.clear();

    tglyphs.clear();

    std::vector<text_glyph> line;
    std::vector<text_glyph> word;

    vec2 word_cursor = vec2(0.0f);
    vec2 cursor = vec2(0.0f, -(font->ascender) * (float(text_size) / font->base_unit));
    vec2 total_cursor = vec2(0.0f);

    line_heights.push_back((font->line_height - font->line_gap) * (float(text_size) / font->base_unit));

    vec2 range = vec2(-axiom::max_float, axiom::max_float);

    for(shaped_glyph& sglyph : sglyphs) {
        auto& glyph_struct = font->glyphs[sglyph.glyph];

        if(glyph_struct.contours.size() == 0) {
            text_glyph tglyph;
            tglyph.position = position;

            glyph_key key;
            key.glyph = sglyph.glyph;
            key.size = text_size;
            
            tglyph.key = key;
            word.push_back(tglyph);
            
            word_cursor += sglyph.advance;

            //

            if(cursor.x + word_cursor.x > width && wrap) {
                range.y = glm::min(range.y, cursor.x + word_cursor.x);

                total_cursor.x += cursor.x;

                cursor.x = 0;
                cursor.y -= font->line_height * (float(text_size) / font->base_unit);

                line_heights.push_back(font->line_height * (float(text_size) / font->base_unit));

                tglyphs.insert(tglyphs.end(), line.begin(), line.end());
                line.clear();
            } else {
                range.x = glm::max(range.x, cursor.x + word_cursor.x);
            }

            for(text_glyph& g : word) g.position += cursor;

            line.insert(line.end(), word.begin(), word.end());
            word.clear();

            cursor += word_cursor;
            word_cursor = vec2(0.0f);
        } else {
            vec2 position = word_cursor + vec2(sglyph.offset) + vec2(glyph_struct.bounding_box.xy()) * (float(text_size) / font->base_unit); /* */

            text_glyph tglyph;
            tglyph.position = position;

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
        cursor.y -= font->line_height * (float(text_size) / font->base_unit);

        tglyphs.insert(tglyphs.end(), line.begin(), line.end());
        line.clear();
        
        line_heights.push_back(font->line_height * (float(text_size) / font->base_unit));
    } else {
        range.x = glm::max(range.x, cursor.x + word_cursor.x);
    }
    
    for(text_glyph& g : word) g.position += cursor;
    line.insert(line.end(), word.begin(), word.end());
    tglyphs.insert(tglyphs.end(), line.begin(), line.end());
    cursor += word_cursor;
    total_cursor.x += cursor.x;

    float offset = 0.0f;
    for(float f : line_heights) offset += f;

    for(text_glyph& glyph : tglyphs) {
        glyph.position.y += offset;
    }
    
    wrap_limits = range;

    //
    
    size = vec2(range.x, 0.0f);
    max_width = total_cursor.x;

    for(float line : line_heights) size.y += line;
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
}
    
void measure_text(axiom::font& f, uint text_size, std::string str, text_data& data, uint width, axiom::text_alignment alignment, bool show_debug, std::vector<text_line_data>* lines) {
    
        
    /*
    data = text_data();

    float scale = float(text_size) / f.base_unit;

    //

    vec2 wrap_limits = vec2(-FLT_MAX, FLT_MAX);
    float max_width = 0;

    std::vector<text_line_data> text_lines;
    //std::vector<uint> text_line_origins;
    int word_len = 0;
    int line_len = 0;
    text_line_data word_start = {0};
    text_line_data line_start = {0};

    //
    
    float max_x = 0.0f;
    text_start.clear();

    float italic_factor = 1.0f / 3.5f;
    float bold_factor = 1.0f;

    std::u32string text = convert_string(str);

    bool accept_index = false;

    int num_escape_seq = 0;
    int i = 0;

    vec2 pos = vec2(0.0f);
    vec4 range = vec4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    vec4 color = vec4(1.0f);
    bool bold = false;
    bool italic = false;
    bool hex = false;
    float min_offset = 0.0;

    uint num_lines = 0;

    std::vector<ui_vertex> word_ret;
    vec2 word_pos = vec2(0.0f);
    
    std::vector<ui_vertex> line_ret;

    auto insert_line = [&]() {
        text_line_data s = line_start;
        line_len = 0;
        
        wrap_limits.x = glm::max(wrap_limits.x, pos.x);

        max_width += pos.x;

        //

        int line_width = pos.x;
        int offset;

        if(alignment == axiom::text_alignment::LEFT) offset = 0.0f;
        else if(alignment == axiom::text_alignment::CENTER) {
            offset = round(float(-line_width) / 2);
        }
        else if(alignment == axiom::text_alignment::RIGHT) offset = float(-line_width);

        //
        //
        
        min_offset = glm::min(min_offset, float(offset));
        for(ui_vertex& v : line_ret) {
            v.pos.x += offset;
        }
        
        max_x = glm::max(max_x, pos.x);

        pos.x = 0;
        pos.y -= round(f.line_height * scale);
        ++num_lines;

        s.offset = offset;
        text_lines.push_back(s);
    };
    
    uint end = pos.x + word_pos.x;
    
    auto insert_word = [&]() {   
        //

        uint end = pos.x + word_pos.x;

        if(end > width && pos.x != 0.0f) {
            float min_v = pos.x;
            float max_v = end;
            
            //max_x = glm::max(max_x, float(width));

            wrap_limits.x = glm::max(wrap_limits.x, min_v);
            wrap_limits.y = glm::min(wrap_limits.y, max_v);
            
            insert_line();
        } else {
            float min_v = end;
            wrap_limits.x = glm::max(wrap_limits.x, min_v + 1);
        }

        // insert word
        for(ui_vertex& v : word_ret) {
            v.pos += vec3(pos, 0.0f);
        }
        
        line_ret.insert(line_ret.end(), word_ret.begin(), word_ret.end());

        word_ret.clear();
        
        pos.x += word_pos.x;
        word_pos = vec2(0.0f);
        
        if(line_len == 0) {
            line_start = word_start;
        }
        ++line_len;
        word_len = 0;  
    };

    auto insert_char = [&](uint codepoint) {
        axiom::glyph& gd = f.glyphs.at(f.glyph_map.at(codepoint));

        float stride = gd.h_advance * scale;

        if(!gd.contours.size()) {
            if(alignment == axiom::text_alignment::LEFT) {
                if(word_len == 0) {
                    word_start.bold = bold;
                    word_start.italic = italic;
                    word_start.color = color.xyz();
                    word_start.start_index = i;
                }
                ++word_len;
                
                word_pos.x += stride;
                
                insert_word();
            } else if(alignment == axiom::text_alignment::CENTER) {
                if(word_len == 0) {
                    word_start.bold = bold;
                    word_start.italic = italic;
                    word_start.color = color.xyz();
                    word_start.start_index = i;
                }
                ++word_len;

                word_pos.x += stride;

                insert_word();
            } else if(alignment == axiom::text_alignment::RIGHT) {
                insert_word();
                
                if(word_len == 0) {
                    word_start.bold = bold;
                    word_start.italic = italic;
                    word_start.color = color.xyz();
                    word_start.start_index = i;
                }
                ++word_len;

                word_pos.x += stride;
            }
        } else {
            if(word_len == 0) {
                word_start.bold = bold;
                word_start.italic = italic;
                word_start.color = color.xyz();
                word_start.start_index = i;
            }
            ++word_len;
            
            //

            if(bold) {
                stride += bold_factor;
            }
    
            word_pos.x += stride;
        }
    };

    for(i = 0; i < text.size(); ++i) {
        uint c = text[i];

        if(c == '\n') {
            insert_word();
            insert_line();
            
            word_start.bold = bold;
            word_start.italic = italic;
            word_start.color = color.xyz();
            word_start.start_index = i;
            line_start = word_start;

            continue;
        } else {
            if(c == '\\') {
                if(i + 1 < text.size()) {
                    uint next = text[i + 1];

                    if(next == 'c') {
                        if(i + 1 + 3 < text.size()) {
                            std::string s(text.begin() + (i + 2), text.begin() + (i + 5));

                            std::size_t i0 = integers_letters.find(s[0]);
                            std::size_t i1 = integers_letters.find(s[1]);
                            std::size_t i2 = integers_letters.find(s[2]);

                            if(i0 != std::string::npos && i1 != std::string::npos && i2 != std::string::npos) {
                                color = vec4(float(i0) / 15.0f, float(i1) / 15.0f, float(i2) / 15.0f, 1.0f);
                                
                                num_escape_seq += 5;
                                if(!show_debug) {    
                                    i += 4;
                                    continue;   
                                }
                            }
                        }
                    } else if(next == 'b') {
                        bold = true;

                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    } else if(next == 'i') {
                        italic = true;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }

                    } else if(next == 'r') {
                        bold = false;
                        italic = false;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    } else if(next == 'h') {
                        hex = !hex;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    }
                }
            } 
            
            if(show_debug) {
                if(num_escape_seq > 0) {
                    color.w = 0.5f;
                    --num_escape_seq;

                    if(c == 'A') c = 0x80;
                    else if(c == 'B') c = 0x81;
                    else if(c == 'C') c = 0x82;
                    else if(c == 'D') c = 0x83;
                    else if(c == 'E') c = 0x84;
                    else if(c == 'F') c = 0x85;
                } else color.w = 1.0f;
            }

            if(hex) {
                if(c == 'A') c = 0x80;
                else if(c == 'B') c = 0x81;
                else if(c == 'C') c = 0x82;
                else if(c == 'D') c = 0x83;
                else if(c == 'E') c = 0x84;
                else if(c == 'F') c = 0x85;
            }
            
            insert_char(c);
        }
    }

    insert_word();
    insert_line();

    //
    
    float offset = (num_lines - 1) * round(f.line_height * scale);
    
    for(auto& line : text_lines) line.offset -= min_offset;

    data.size = {max_x, num_lines * round(f.line_height * scale) - round(f.line_gap * scale)};
    data.lines = line_data;
    data.wrap_limits = wrap_limits;
    data.max_width = max_width;

    if(lines != nullptr) *lines = text_lines;
    */
}

std::vector<ui_vertex> mesh_text_select(axiom::font& f, uint text_size, ivec2 selection, std::string str, text_data& data, uint width, axiom::text_alignment alignment, bool show_debug, std::vector<text_line_data>* lines) {    
    /*
    data = text_data();

    float scale = float(text_size) / f.base_unit;

    vec2 wrap_limits = vec2(-FLT_MAX, FLT_MAX);
    float max_width = 0;
    
    std::vector<text_line_data> text_lines;
    //std::vector<uint> text_line_origins;
    int word_len = 0;
    int line_len = 0;
    text_line_data word_start = {0};
    text_line_data line_start = {0};
    
    //
    
    float max_x = 0.0f;
    text_start.clear();

    float italic_factor = 1.0f / 3.5f;
    float bold_factor = 1.0f;

    std::u32string text = convert_string(str);

    bool accept_index = false;

    int num_escape_seq = 0;
    int i = 0;

    std::vector<ui_vertex> ret;
    vec2 pos = vec2(0.0f);
    vec4 range = vec4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    vec4 color = vec4(1.0f);
    bool bold = false;
    bool italic = false;
    bool hex = false;
    float min_offset = 0.0;

    uint num_lines = 0;

    std::vector<ui_vertex> word_ret;
    vec2 word_pos = vec2(0.0f);
    
    std::vector<ui_vertex> line_ret;

    
    auto insert_line = [&]() {
        text_line_data s = line_start;
        line_len = 0;
        
        wrap_limits.x = glm::max(wrap_limits.x, pos.x);

        max_width += pos.x;

        //

        int line_width = pos.x;
        int offset;

        if(alignment == axiom::text_alignment::LEFT) offset = 0.0f;
        else if(alignment == axiom::text_alignment::CENTER) {
            offset = round(float(-line_width) / 2);
        }
        else if(alignment == axiom::text_alignment::RIGHT) offset = float(-line_width);

        min_offset = glm::min(min_offset, float(offset));
        for(ui_vertex& v : line_ret) {
            v.pos.x += offset;
        }
        
        ret.insert(ret.end(), line_ret.begin(), line_ret.end());
        
        line_ret.clear();
        
        max_x = glm::max(max_x, pos.x);

        pos.x = 0;
        pos.y -= round(f.line_height * scale);
        ++num_lines;

        s.offset = offset;
        text_lines.push_back(s);
    };

    auto insert_word = [&]() {
        //

        uint end = pos.x + word_pos.x;
        
        if(end > width && pos.x != 0.0f) {
            float min_v = pos.x;
            float max_v = end;
            
            //max_x = glm::max(max_x, float(width));

            wrap_limits.x = glm::max(wrap_limits.x, min_v);
            wrap_limits.y = glm::min(wrap_limits.y, max_v);
            
            insert_line();
        } else {
            float min_v = end;
            wrap_limits.x = glm::max(wrap_limits.x, min_v + 1);
        }

        // insert word
        for(ui_vertex& v : word_ret) {
            v.pos += vec3(pos, 0.0f);
            v.pos = floor(v.pos);
        }
        
        line_ret.insert(line_ret.end(), word_ret.begin(), word_ret.end());

        word_ret.clear();
        
        pos.x += word_pos.x;
        word_pos = vec2(0.0f);  
        
        if(line_len == 0) {
            line_start = word_start;
        }
        ++line_len;
        word_len = 0;
    };

    auto insert_selection = [&](vec2 pos, vec2 size) {
        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        std::vector<ui_vertex> r = {a, b, d, a, d, c};
        for(ui_vertex& v : r) {
            v.pos = vec3(vec2(pos) + v.pos.xy() * vec2(size), 0.0f);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(1.0f, 1.0f, 1.0f, 0.35f);
            v.data = 1;
        }
        word_ret.insert(word_ret.end(), r.begin(), r.end());
    };

    auto insert_char = [&](uint codepoint) {
        //

        axiom::glyph& gd = f.glyphs.at(f.glyph_map.at(codepoint));

        float stride = gd.h_advance * scale;

        if(!gd.contours.size()) {
            ui_vertex v;
            v.pos = vec3(word_pos + vec2(stride, 0), 0.0f);
            v.data = 0xFFFFFFFF;
            word_ret.push_back(v);
            word_ret.push_back(v);
            word_ret.push_back(v);

            if(alignment == axiom::text_alignment::LEFT) {
                if(word_len == 0) {
                    word_start.bold = bold;
                    word_start.italic = italic;
                    word_start.color = color.xyz();
                    word_start.start_index = i;
                }
                ++word_len;
                
                if(i >= selection.x && i < selection.y) insert_selection(word_pos, {stride, round(f.line_height * scale)});
                word_pos.x += stride;
                
                insert_word();
            } else if(alignment == axiom::text_alignment::CENTER) {
                if(word_len == 0) {
                    word_start.bold = bold;
                    word_start.italic = italic;
                    word_start.color = color.xyz();
                    word_start.start_index = i;
                }
                ++word_len;

                if(i >= selection.x && i < selection.y) insert_selection(word_pos, {stride, round(f.line_height * scale)});
                word_pos.x += stride;

                insert_word();
            } else if(alignment == axiom::text_alignment::RIGHT) {
                insert_word();

                if(word_len == 0) {
                    word_start.bold = bold;
                    word_start.italic = italic;
                    word_start.color = color.xyz();
                    word_start.start_index = i;
                }
                ++word_len;

                if(i >= selection.x && i < selection.y) insert_selection(word_pos, {stride, round(f.line_height * scale)});
                word_pos.x += stride;
            }
        } else {
            if(word_len == 0) {
                word_start.bold = bold;
                word_start.italic = italic;
                word_start.color = color.xyz();
                word_start.start_index = i;
            }
            ++word_len;
            
            if(i >= selection.x && i < selection.y) insert_selection(word_pos, {stride + ((bold) ? bold_factor : 0.0f), round(f.line_height * scale)});

            word_pos.x += stride;
        }
    };

    for(i = 0; i < text.size(); ++i) {
        uint c = text[i];

        if(c == '\n') {
            insert_word();
            insert_line();

            word_start.bold = bold;
            word_start.italic = italic;
            word_start.color = color.xyz();
            word_start.start_index = i;
            line_start = word_start;
            
            if(i >= selection.x && i < selection.y && (text[i + 1] == '\n' || i == text.size() - 1)) {
                if(alignment == axiom::text_alignment::LEFT) insert_selection(word_pos, {6, round(f.line_height * scale)});
                else if(alignment == axiom::text_alignment::CENTER) insert_selection(word_pos - vec2(3, 0), {6, round(f.line_height * scale)});
                else if(alignment == axiom::text_alignment::RIGHT) insert_selection(word_pos - vec2(6, 0), {6, round(f.line_height * scale)});
            }

            continue;
        } else {
            if(c == '\\') {
                if(i + 1 < text.size()) {
                    uint next = text[i + 1];

                    if(next == 'c') {
                        if(i + 1 + 3 < text.size()) {
                            std::string s(text.begin() + (i + 2), text.begin() + (i + 5));

                            std::size_t i0 = integers_letters.find(s[0]);
                            std::size_t i1 = integers_letters.find(s[1]);
                            std::size_t i2 = integers_letters.find(s[2]);

                            if(i0 != std::string::npos && i1 != std::string::npos && i2 != std::string::npos) {
                                color = vec4(float(i0) / 15.0f, float(i1) / 15.0f, float(i2) / 15.0f, 1.0f);
                                
                                num_escape_seq += 5;
                                if(!show_debug) {    
                                    i += 4;
                                    continue;   
                                }
                            }
                        }
                    } else if(next == 'b') {
                        bold = true;

                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    } else if(next == 'i') {
                        italic = true;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }

                    } else if(next == 'r') {
                        bold = false;
                        italic = false;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    } else if(next == 'h') {
                        hex = !hex;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    }
                }
            } 
            
            if(show_debug) {
                if(num_escape_seq > 0) {
                    color.w = 0.5f;
                    --num_escape_seq;

                    if(c == 'A') c = 0x80;
                    else if(c == 'B') c = 0x81;
                    else if(c == 'C') c = 0x82;
                    else if(c == 'D') c = 0x83;
                    else if(c == 'E') c = 0x84;
                    else if(c == 'F') c = 0x85;
                } else color.w = 1.0f;
            }

            if(hex) {
                if(c == 'A') c = 0x80;
                else if(c == 'B') c = 0x81;
                else if(c == 'C') c = 0x82;
                else if(c == 'D') c = 0x83;
                else if(c == 'E') c = 0x84;
                else if(c == 'F') c = 0x85;
            }

            insert_char(c);
        }
    }

    insert_word();
    insert_line();

    float offset = (num_lines - 1) * round(f.line_height * scale);
    
    for(auto& line : text_lines) {
        line.offset -= min_offset;
    }

    for(ui_vertex& v : ret) {
        v.pos.y = v.pos.y + offset;
        v.pos.x -= min_offset;
    }

    for(ui_vertex& v : ret) {
        range.x = glm::min(range.x, v.pos.x);
        range.y = glm::min(range.y, v.pos.y);
        range.z = glm::max(range.z, v.pos.x);
        range.w = glm::max(range.w, v.pos.y);
    }

    data.size = {max_x, num_lines * round(f.line_height * scale) - round(f.line_gap * scale)};
    data.lines = line_data;
    data.wrap_limits = wrap_limits;
    data.max_width = max_width;

    if(lines != nullptr) *lines = text_lines;

    return ret;
    */

    return {};
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
    std::vector<ui_vertex> vs;

    for(text_glyph& tglyph : tglyphs) {
        auto tvs = create_char(*font, tglyph);

        vs.insert(vs.end(), tvs.begin(), tvs.end());
    }

    return vs;
}

std::vector<ui_vertex> text::mesh_select() {
    /*
    if(select_dirty) {
        select_dirty = false;

        if(!wrap) width = 0xFFFFFFFF;

        ivec2 abs_select = ivec2(glm::min(select_range.x, select_range.y), glm::max(select_range.x, select_range.y));
        
        text_data data;
        select_vertices = axiom::mesh_text_select(*font, text_size, abs_select, string, data, width, alignment, false, &lines);
        size = data.size;
        wrap_limits = data.wrap_limits;
        max_width = data.max_width;

        if(editable && select_range.x == select_range.y && select_range.x != -1) {
            vec2 pos = axiom::compute_cursor_pos(*font, *this, select_range.x);

            vec4 range = vec4(pos, pos + vec2(1.0f, font->line_height));
            
            ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
            ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
            ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
            ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

            std::vector<ui_vertex> ret2 = {a, b, d, a, d, c};
            vec4 color = vec4(1.0f);
            if(glm::mod(get_time() - time, 1.0) > 0.5) color = vec4(0.0f);

            for(ui_vertex& v : ret2) {
                v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = color;
                v.data = 0x1;
            }
            
            select_vertices = ret2;
        }
    }

    return select_vertices;
    */

    return {};
}

std::pair<int, bool> compute_cursor_index(axiom::font& font, axiom::text& text, vec2 cursor_pos, bool cl0, bool cl1, bool cl2) {
    /*
    float scale = float(text.text_size) / text.font->base_unit;

    int line_index = (int)text.lines.size() - glm::floor((cursor_pos.y) / round(text.font->line_height * scale)) - 1;

    if(line_index < 0 && cl0) {
        line_index = 0;
    }
    if(line_index >= (int)text.lines.size() && cl1) line_index = text.lines.size() - 1;

    if(line_index < 0 || line_index >= (int)text.lines.size()) {
        return {-1, false};
    }

    std::u32string str = axiom::convert_string(text.string);

    uint32_t line_start, line_end;
    line_start = text.lines[line_index].start_index;
    if(line_index == (text.lines.size() - 1)) line_end = str.size();
    else line_end = text.lines[line_index + 1].start_index;

    float cursor_x = cursor_pos.x;

    bool bold = false;
    bool italic = false;
    bool hex = false;

    int num_escape_seq = 0;
    bool show_debug = false;

    float start_buffer = 3.0f;

    text_line_data data = text.lines[line_index];
    bold = data.bold;
    italic = data.italic;

    float pos = 0.0f;
    float advance = 0.0f;
    
    cursor_x -= data.offset;
    
    if(pos - start_buffer < cursor_x || line_index != 0 || cl0) {
        for(int i = line_start; i < line_end; ++i) {
            uint32_t c = str[i];

            if(c == '\\') {
                if(i + 1 < str.size()) {
                    uint32_t next = str[i + 1];

                    if(next == 'c') {
                        if(i + 1 + 3 < str.size()) {
                            std::string s(str.begin() + (i + 2), str.begin() + (i + 5));

                            std::size_t i0 = integers_letters.find(s[0]);
                            std::size_t i1 = integers_letters.find(s[1]);
                            std::size_t i2 = integers_letters.find(s[2]);

                            if(i0 != std::string::npos && i1 != std::string::npos && i2 != std::string::npos) {
                                
                                num_escape_seq += 5;
                                if(!show_debug) {    
                                    i += 4;
                                    continue;   
                                }
                            }
                        }
                    } else if(next == 'b') {
                        bold = true;

                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    } else if(next == 'i') {
                        italic = true;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }

                    } else if(next == 'r') {
                        bold = false;
                        italic = false;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    } else if(next == 'h') {
                        hex = !hex;
                        
                        num_escape_seq += 2;
                        if(!show_debug) {
                            i += 1;
                            continue;
                        }
                    }
                }
            } 
            
            if(show_debug) {
                if(num_escape_seq > 0) {
                    //color.w = 0.5f;
                    --num_escape_seq;

                    if(c == 'A') c = 0x80;
                    else if(c == 'B') c = 0x81;
                    else if(c == 'C') c = 0x82;
                    else if(c == 'D') c = 0x83;
                    else if(c == 'E') c = 0x84;
                    else if(c == 'F') c = 0x85;
                }// else color.w = 1.0f;
            }

            if(hex) {
                if(c == 'A') c = 0x80;
                else if(c == 'B') c = 0x81;
                else if(c == 'C') c = 0x82;
                else if(c == 'D') c = 0x83;
                else if(c == 'E') c = 0x84;
                else if(c == 'F') c = 0x85;
            }

            //

            if(text.font->glyph_map.contains(c)) {
                auto& glyph = text.font->glyphs.at(text.font->glyph_map.at(c));

                advance = glyph.h_advance * scale;
                if(bold && glyph.contours.size()) advance += bold_factor;

                if(pos - advance * 0.5f > cursor_x && i == line_start) return {-1, false};

                if(pos + advance * 0.5f > cursor_x) {
                    return {i, false};
                }
                
                pos += advance;
            }
        }
    } else {
        return {-1, false};
    }

    //if(text.size() == 0) line_end = 0;

    if(!cl2 && !(pos + advance * 0.5f > cursor_x)) return {-1, false};

    return {line_end, false};
    */

    return {0, 0};
}

vec2 compute_cursor_pos(axiom::font& font, axiom::text& text, uint32_t index) {
    /*
    float scale = float(text.text_size) / text.font->base_unit;

    int line_index = 0;
    for(line_index = 0; line_index < text.lines.size() - 1; ++line_index) {
        uint32_t next = text.lines[line_index + 1].start_index;
        if(next > index || (text.select_line == line_index && next == index)) break;
    }

    vec2 pos;
    pos.y = (text.lines.size() - line_index - 1) * (text.font->line_height * scale);
    pos.x = 0.0f;

    if(line_index < 0) line_index = 0;
    else if(line_index >= text.lines.size()) line_index = text.lines.size() - 1;

    std::u32string str = convert_string(text.string);

    uint32_t line_start, line_end;
    line_start = text.lines[line_index].start_index;
    if(line_index == text.lines.size() - 1) line_end = str.size();
    else line_end = text.lines[line_index + 1].start_index;

    bool bold = false;
    bool italic = false;
    bool hex = false;
    uint32_t num_escape_seq = 0;
    bool show_debug = false;

    text_line_data data = text.lines[line_index];
    bold = data.bold;
    italic = data.italic;

    for(int i = line_start; i < index; ++i) {
        uint32_t c = str[i];

        if(c == '\\') {
            if(i + 1 < str.size()) {
                uint32_t next = str[i + 1];

                if(next == 'c') {
                    if(i + 1 + 3 < str.size()) {
                        std::string s(str.begin() + (i + 2), str.begin() + (i + 5));

                        std::size_t i0 = integers_letters.find(s[0]);
                        std::size_t i1 = integers_letters.find(s[1]);
                        std::size_t i2 = integers_letters.find(s[2]);

                        if(i0 != std::string::npos && i1 != std::string::npos && i2 != std::string::npos) {
                            
                            num_escape_seq += 5;
                            if(!show_debug) {    
                                i += 4;
                                continue;   
                            }
                        }
                    }
                } else if(next == 'b') {
                    bold = true;

                    num_escape_seq += 2;
                    if(!show_debug) {
                        i += 1;
                        continue;
                    }
                } else if(next == 'i') {
                    italic = true;
                    
                    num_escape_seq += 2;
                    if(!show_debug) {
                        i += 1;
                        continue;
                    }

                } else if(next == 'r') {
                    bold = false;
                    italic = false;
                    
                    num_escape_seq += 2;
                    if(!show_debug) {
                        i += 1;
                        continue;
                    }
                } else if(next == 'h') {
                    hex = !hex;
                    
                    num_escape_seq += 2;
                    if(!show_debug) {
                        i += 1;
                        continue;
                    }
                }
            }
        } 
        
        if(show_debug) {
            if(num_escape_seq > 0) {
                //color.w = 0.5f;
                --num_escape_seq;

                if(c == 'A') c = 0x80;
                else if(c == 'B') c = 0x81;
                else if(c == 'C') c = 0x82;
                else if(c == 'D') c = 0x83;
                else if(c == 'E') c = 0x84;
                else if(c == 'F') c = 0x85;
            }// else color.w = 1.0f;
        }

        if(hex) {
            if(c == 'A') c = 0x80;
            else if(c == 'B') c = 0x81;
            else if(c == 'C') c = 0x82;
            else if(c == 'D') c = 0x83;
            else if(c == 'E') c = 0x84;
            else if(c == 'F') c = 0x85;
        }

        auto glyph = text.font->glyphs.at(text.font->glyph_map.at(c));

        pos.x += glyph.h_advance * scale;
        if(bold && glyph.contours.size()) pos.x += bold_factor;
    }

    return pos;
    */
    return {0, 0};
}

ivec2 text::select(vec2 cursor, uint wrap_mode) {
    /*
    float scale = float(text_size) * font->base_unit;
    
    //if(!capture) return ivec2(-1);

    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    ivec2 prev_select = select_range;

    vec4 text_range = vec4(position, size + position);

    //

    int line = (int)lines.size() - floor(float(cursor.y - position.y + round(font->descender * scale)) / round(font->line_height * scale)) - 1;

    vec2 cursor_a = cursor - position;

    if(wrap_mode == 1) cursor_a.x = 0.0f;
    else if(wrap_mode == 2) cursor_a.x = size.x + 1;

    //

    int index = compute_cursor_index(*font, *this, cursor_a, true, false).first;

    return {index, line};
    */

    return {0, 0};
}

void text::select(vec4 cursor_range, bool anchor) {
    /*
    //if(!capture) return;

    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    ivec2 prev_select = select_range;

    vec4 text_range = vec4(position, size + position);

    //

    int line_a = (int)lines.size() - floor(float(cursor_range.y - position.y) / font->line_height) - 1;
    int line_b = (int)lines.size() - floor(float(cursor_range.w - position.y) / font->line_height) - 1;

    if(line_a == line_b) select_line = line_a;

    bool swap = false;

    if(line_a > line_b || (line_a == line_b && cursor_range.x > cursor_range.z)) {
        std::swap(line_a, line_b);
        cursor_range = {cursor_range.zw(), cursor_range.xy()};

        swap = true;
    }

    bool wrap_selection_start = false;
    bool wrap_selection_end = false;

    if(line_a < 0) wrap_selection_start = true;
    if(line_b >= lines.size()) wrap_selection_end = true;

    //

    vec2 cursor_a = cursor_range.xy() - position;
    vec2 cursor_b = cursor_range.zw() - position;

    if(wrap_selection_start) cursor_a.x = 0.0f;
    if(wrap_selection_end) cursor_b.x = size.x + 1;

    //

    int ia = compute_cursor_index(*font, *this, cursor_a, true, false).first;
    int ib = compute_cursor_index(*font, *this, cursor_b, false, true).first;

    if(ia == -1 || ib == -1) select_range = {-1, -1};
    else {
        if(anchor || select_range.x == -1) {
            if(swap) select_range = {ib, ia};
            else select_range = {ia, ib};
        } else {
            if(swap) select_range.y = ia;
            else select_range.y = ib;
        }
    }

    //

    if(prev_select != select_range) {
        select_dirty = true;
        if(select_range != ivec2(-1)) {
            if(select_range.x == select_range.y) time = get_time();

            v_select = mesh_select();
        } else {
            v_select.clear();
        }
    }
    */
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

    if(editable) {
        ivec3 prev = ivec3{select_range, select_line};

        if(select_range.x != -1 && select_range.y != -1) {
            std::string input = ui_system.window->char_delta;

            ivec2 abs_range = ivec2(glm::min(select_range.x, select_range.y), glm::max(select_range.x, select_range.y));

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_BACKSPACE) || ui_system.window->repeat_buttons.contains(axiom::input_code::KEY_BACKSPACE)) {
                if(select_range.x == select_range.y) {
                    if(select_range.x != 0) {
                        std::u32string text = axiom::convert_string(string);
                        text.erase(text.begin() + select_range.x - 1);
                        select_range -= 1;
                        
                        string = axiom::convert_string(text);
                    }
                } else {
                    std::u32string text = axiom::convert_string(string);

                    if(select_range.x != select_range.y) {
                        text.erase(text.begin() + abs_range.x, text.begin() + abs_range.y);
                    }
                    select_range = ivec2(glm::min(select_range.x, select_range.y));
                    
                    string = axiom::convert_string(text);
                }
            }

            if(input.size()) {
                std::u32string text = axiom::convert_string(string);
                std::u32string input_u32 = axiom::convert_string(input);

                if(select_range.x != select_range.y) {
                    text.erase(text.begin() + abs_range.x, text.begin() + abs_range.y);
                }
                select_range = ivec2(glm::min(select_range.x, select_range.y));
                
                text.insert(text.begin() + select_range.x, input_u32.begin(), input_u32.end());

                select_range += input_u32.size();

                string = axiom::convert_string(text);
            }

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_LEFT_ARROW) || ui_system.window->repeat_buttons.contains(axiom::input_code::KEY_LEFT_ARROW)) {
                std::u32string text = axiom::convert_string(string);

                int line = 0;
                while(true) {
                    auto L = lines[line];
                    if(L.start_index > select_range.x) {
                        line -= 1;
                        break;
                    } else if(line == lines.size() - 1) {
                        break;
                    }

                    ++line;
                }
                int lstart = lines[line].start_index;

                //

                if(select_range.x == select_range.y) {
                    if(select_range.x == lstart && select_line == line) {
                        select_line = line - 1;
                    } else {
                        select_line = line;
                        select_range -= 1;
                    }
                } else {
                    select_range = ivec2(glm::min(select_range.x, select_range.y));
                }
                
                select_range.x = glm::clamp(select_range.x, 0, (int)text.size());
                select_range.y = glm::clamp(select_range.x, 0, (int)text.size());
            }

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_RIGHT_ARROW) || ui_system.window->repeat_buttons.contains(axiom::input_code::KEY_RIGHT_ARROW)) {
                std::u32string text = axiom::convert_string(string);

                int line = 0;
                while(true) {
                    auto L = lines[line];
                    if(L.start_index > select_range.x) {
                        line -= 1;
                        break;
                    } else if(line == lines.size() - 1) {
                        break;
                    }

                    ++line;
                }
                int lstart = lines[line].start_index;
                int lend = text.size();
                if(line + 1 < lines.size()) lend = lines[line + 1].start_index;

                //

                if(select_range.x == select_range.y) {
                    if(select_range.x == lstart && select_line == line - 1) {
                        select_line = line;
                    } else {
                        select_line = line;
                        select_range += 1;
                    }
                } else {
                    select_range = ivec2(glm::max(select_range.x, select_range.y));
                }

                select_range.x = glm::clamp(select_range.x, 0, (int)text.size());
                select_range.y = glm::clamp(select_range.x, 0, (int)text.size());
            }

            if(prev != ivec3{select_range, select_line}) offset = 0;

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_UP_ARROW) || ui_system.window->repeat_buttons.contains(axiom::input_code::KEY_UP_ARROW)) {
                if(select_range.x != select_range.y) select_range = ivec2(glm::min(select_range.x, select_range.y));
                
                int line = 0;
                while(true) {
                    auto L = lines[line];
                    if(L.start_index > select_range.x || L.start_index == select_range.x && select_line == line - 1) {
                        line -= 1;
                        break;
                    } else if(line == lines.size() - 1) {
                        break;
                    }

                    ++line;
                }
                line = glm::max(0, line);

                if(line != 0) {
                    auto L0 = lines[line - 1];
                    auto L1 = lines[line];
                    int rel_pos = select_range.x - L1.start_index;
                    offset = glm::max(offset, rel_pos);

                    int new_pos = L0.start_index + offset;
                    if(new_pos > L1.start_index) {
                        new_pos = L1.start_index;
                    }

                    select_line = line - 1;
                    select_range = ivec2(new_pos);
                } else {
                    select_range = ivec2(0);
                    offset = 0;
                }
            }

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_DOWN_ARROW) || ui_system.window->repeat_buttons.contains(axiom::input_code::KEY_DOWN_ARROW)) {
                if(select_range.x != select_range.y) select_range = ivec2(glm::min(select_range.x, select_range.y));
                
                int line = 0;
                while(true) {
                    auto L = lines[line];
                    if(L.start_index > select_range.x || L.start_index == select_range.x && select_line == line - 1) {
                        line -= 1;
                        break;
                    } else if(line == lines.size() - 1) {
                        break;
                    }

                    ++line;
                }
                line = glm::max(0, line);

                std::u32string text = axiom::convert_string(string);

                if(line != lines.size() - 1) {
                    auto L0 = lines[line];
                    auto L1 = lines[line + 1];
                    int rel_pos = select_range.x - L0.start_index;
                    offset = glm::max(offset, rel_pos);

                    int new_pos = L1.start_index + offset;

                    int max_p = text.size();
                    if(line + 2 < lines.size()) max_p = lines[line + 2].start_index;
                    if(new_pos > max_p) {
                        new_pos = max_p;
                    }

                    select_line = line + 1;
                    select_range = ivec2(new_pos);
                } else {
                    select_range = ivec2(text.size());
                    offset = 0;
                }
            }

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_ENTER)) select_range = ivec2(-1);
        }
    }
}

}