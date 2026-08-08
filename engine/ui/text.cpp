#include <ui/text.hpp>
#include <utilities/utilities.hpp>
#include <math/base.hpp>
#include <ui/ui_system.hpp>

namespace axiom {
    
const float italic_factor = 1.0f / 3.5f;
const float bold_factor = 1.0f;

// for text rendering data
std::vector<uint> text_line_indices;
std::vector<text_line_data> line_data;
    
std::vector<ui_vertex> create_char(glyph_data& glyph) {
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

    for(ui_vertex& v : ret) {
        v.pos = vec3(v.pos.xy() * vec2(glyph.size) + vec2(glyph.offset), 0.0f);
        v.tex_pos = vec2(glyph.pos_tex) + v.tex_pos * vec2(glyph.size);
    }

    return ret;
}

std::vector<ui_vertex> mesh_text(font_asset& f, std::string str, text_data& data, uint text_size, uint width, axiom::text_alignment alignment, bool show_debug, std::vector<text_line_data>* lines) {
    data = text_data();

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
        pos.y -= f.line_height;
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

    auto insert_selection = [&](ivec2 pos, ivec2 size) {
        if(word_len == 0) {
            word_start.bold = bold;
            word_start.italic = italic;
            word_start.color = color.xyz();
            word_start.start_index = i;
        }
        ++word_len;

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

        glyph_data& gd = f.at(codepoint);

        float stride = gd.advance;

        if(!gd.visible) {
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
            
            std::vector<ui_vertex> vs = create_char(gd);

            for(ui_vertex& v : vs) {
                v.pos += vec3(word_pos, 0.0f);
            }

            for(ui_vertex& v : vs) {
                if(italic) {
                    v.pos.x += float(v.pos.y - word_pos.y - f.line_height * 0.5f) * italic_factor;
                }

                v.color = color;
            }

            word_ret.insert(word_ret.end(), vs.begin(), vs.end());

            if(bold) {
                for(ui_vertex& v : vs) {
                    v.pos.x += bold_factor;
                }
                stride += bold_factor;
                
                word_ret.insert(word_ret.end(), vs.begin(), vs.end());
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

    float offset = (num_lines - 1) * f.line_height;
    
    for(ui_vertex& v : ret) {
        v.pos.y = v.pos.y + offset;
        v.pos.x -= min_offset;

        v.pos *= float(text_size);
    }

    for(auto& line : text_lines) line.offset -= min_offset;

    for(ui_vertex& v : ret) {
        range.x = glm::min(range.x, v.pos.x);
        range.y = glm::min(range.y, v.pos.y);
        range.z = glm::max(range.z, v.pos.x);
        range.w = glm::max(range.w, v.pos.y);
    }
    
    data.size = {max_x, num_lines * f.line_height};
    data.lines = line_data;
    data.wrap_limits = wrap_limits;
    data.max_width = max_width;

    if(lines != nullptr) *lines = text_lines;

    return ret;
}

void measure_text(font_asset& f, std::string str, text_data& data, uint text_size, uint width, axiom::text_alignment alignment, bool show_debug, std::vector<text_line_data>* lines) {
    data = text_data();

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
        pos.y -= f.line_height;
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
        glyph_data& gd = f.at(codepoint);

        float stride = gd.advance;

        if(!gd.visible) {
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
    
    float offset = (num_lines - 1) * f.line_height;
    
    for(auto& line : text_lines) line.offset -= min_offset;

    data.size = {max_x, num_lines * f.line_height};
    data.lines = line_data;
    data.wrap_limits = wrap_limits;
    data.max_width = max_width;

    if(lines != nullptr) *lines = text_lines;
}

std::vector<ui_vertex> mesh_text_select(font_asset& f, ivec2 selection, std::string str, text_data& data, uint text_size, uint width = 0xFFFFFFFF, axiom::text_alignment alignment, bool show_debug, std::vector<text_line_data>* lines) {    
    data = text_data();

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
        pos.y -= f.line_height;
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

    auto insert_selection = [&](ivec2 pos, ivec2 size) {
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

        glyph_data& gd = f.at(codepoint);

        float stride = gd.advance;

        if(!gd.visible) {
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
                
                if(i >= selection.x && i < selection.y) insert_selection(word_pos, {gd.advance, f.line_height});
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

                if(i >= selection.x && i < selection.y) insert_selection(word_pos, {gd.advance, f.line_height});
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

                if(i >= selection.x && i < selection.y) insert_selection(word_pos, {gd.advance, f.line_height});
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
            
            if(i >= selection.x && i < selection.y) insert_selection(word_pos, {gd.advance + ((bold) ? bold_factor : 0.0f), f.line_height});

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
                if(alignment == axiom::text_alignment::LEFT) insert_selection(word_pos, {6, f.line_height});
                else if(alignment == axiom::text_alignment::CENTER) insert_selection(word_pos - vec2(3, 0), {6, f.line_height});
                else if(alignment == axiom::text_alignment::RIGHT) insert_selection(word_pos - vec2(6, 0), {6, f.line_height});
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

    float offset = (num_lines - 1) * f.line_height;
    
    for(auto& line : text_lines) line.offset -= min_offset;

    for(ui_vertex& v : ret) {
        v.pos.y = v.pos.y + offset;
        v.pos.x -= min_offset;

        v.pos *= float(text_size);
    }

    for(ui_vertex& v : ret) {
        range.x = glm::min(range.x, v.pos.x);
        range.y = glm::min(range.y, v.pos.y);
        range.z = glm::max(range.z, v.pos.x);
        range.w = glm::max(range.w, v.pos.y);
    }

    data.size = {max_x, num_lines * f.line_height};
    data.lines = line_data;
    data.wrap_limits = wrap_limits;
    data.max_width = max_width;

    if(lines != nullptr) *lines = text_lines;

    return ret;
}

std::vector<ui_vertex> text::mesh() {
    if(glyph_dirty) {
        glyph_dirty = false;

        if(!wrap) width = 0xFFFFFFFF;
        
        text_data data;
        glyph_vertices = mesh_text(*font, string, data, 1, width, alignment, false, &lines);
        size = data.size;
        wrap_limits = data.wrap_limits;
        max_width = data.max_width;
    }

    return glyph_vertices;
}

std::vector<ui_vertex> text::mesh_select() {
    if(select_dirty) {
        select_dirty = false;

        if(!wrap) width = 0xFFFFFFFF;

        ivec2 abs_select = ivec2(glm::min(select_range.x, select_range.y), glm::max(select_range.x, select_range.y));
        
        text_data data;
        select_vertices = axiom::mesh_text_select(*font, abs_select, string, data, 1, width, alignment, false, &lines);
        size = data.size;
        wrap_limits = data.wrap_limits;
        max_width = data.max_width;

        if(editable && select_range.x == select_range.y && select_range.x != -1) {
            vec2 pos = axiom::compute_cursor_pos(select_range.x, *this);

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
}


void text::measure() {
    // state 
    if(state_width != width) {
        state_width = width;
        if(width <= wrap_limits.x || width >= wrap_limits.y) {
            dirty = true;
        }
    }
    if(string != state_string) {
        state_string = string;
        dirty = true;
    }

    if(state_select_line != select_line) {
        state_select_line = select_line;
        dirty = true;
    }
    if(state_select_range != select_range) {
        state_select_range = select_range;
        dirty = true;
    }

    if(dirty) {
        if(!wrap) width = 0xFFFFFFFF;
        
        text_data data;
        measure_text(*font, string, data, 1, width, alignment, false, &lines);
        size = data.size;
        wrap_limits = data.wrap_limits;
        max_width = data.max_width;

        dirty = false;
        glyph_dirty = true;
        select_dirty = true;
    }
}

std::pair<int, bool> compute_cursor_index(vec2 cursor_pos, axiom::text& text, bool cl0, bool cl1, bool cl2) {
    int line_index = (int)text.lines.size() - glm::floor(cursor_pos.y / text.font->line_height) - 1;

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

            auto glyph = text.font->at(c);

            advance = glyph.advance;
            if(bold && glyph.visible) advance += bold_factor;

            if(pos - advance * 0.5f > cursor_x && i == line_start) return {-1, false};

            if(pos + advance * 0.5f > cursor_x) {
                return {i, false};
            }
            
            pos += advance;
        }
    } else {
        return {-1, false};
    }

    //if(text.size() == 0) line_end = 0;

    if(!cl2 && !(pos + advance * 0.5f > cursor_x)) return {-1, false};

    return {line_end, false};
}

vec2 compute_cursor_pos(uint32_t index, axiom::text& text) {
    int line_index = 0;
    for(line_index = 0; line_index < text.lines.size() - 1; ++line_index) {
        uint32_t next = text.lines[line_index + 1].start_index;
        if(next > index || (text.select_line == line_index && next == index)) break;
    }

    vec2 pos;
    pos.y = (text.lines.size() - line_index - 1) * text.font->line_height;
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

        auto glyph = text.font->at(c);

        pos.x += glyph.advance;
        if(bold && glyph.visible) pos.x += bold_factor;
    }

    return pos;
}

ivec2 text::select(vec2 cursor, uint wrap_mode) {
    //if(!capture) return ivec2(-1);

    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    ivec2 prev_select = select_range;

    vec4 text_range = vec4(position, size + position);

    //

    int line = (int)lines.size() - floor(float(cursor.y - position.y) / font->line_height) - 1;

    vec2 cursor_a = cursor - position;

    if(wrap_mode == 1) cursor_a.x = 0.0f;
    else if(wrap_mode == 2) cursor_a.x = size.x + 1;

    //

    int index = compute_cursor_index(cursor_a, *this, true, false).first;

    return {index, line};
}

void text::select(vec4 cursor_range, bool anchor) {
    //if(!capture) return;

    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

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

    int ia = compute_cursor_index(cursor_a, *this, true, false).first;
    int ib = compute_cursor_index(cursor_b, *this, false, true).first;

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
}

bool text::collide(vec2 cursor) {
    int ii = -1;
    ii = compute_cursor_index(cursor - position, *this, false, false, false).first;

    return ii != -1;
}

std::string text::retrieve() {
    std::u32string u32str = convert_string(string);

    int minv = glm::min(select_range.x, select_range.y);
    int maxv = glm::max(select_range.x, select_range.y);

    std::u32string str(u32str.begin() + minv, u32str.begin() + maxv);

    return convert_string(str);
}

void text::call() {
    //

    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    if(collide(ui_system.window->cursor_pos) && includes(ui_system.window->cursor_pos, range)) {
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
        
        if(prev != ivec3{select_range, select_line}) time = get_time();
    }
}

/*
std::pair<int, bool> compute_cursor_index(vec2 cursor_pos, font_asset& f, std::string text, uint text_size, std::vector<text_line_data> text.lines, ALIGNMENT alignment, bool cl0 = true, bool cl1 = true) {
    int line_index = (int)text.lines.size() - glm::floor(cursor_pos.y / f.line_height) - 1;

    if(line_index < 0 && cl0) {
        line_index = 0;
    }
    if(line_index >= (int)text.lines.size() && cl1) line_index = text.lines.size() - 1;

    if(line_index < 0 || line_index >= (int)text.lines.size()) {
        return {-1, false};
    }

    std::u32string str = convert_string(text);

    uint line_start, line_end;
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
    
    if(pos - start_buffer < cursor_x || line_index != 0 || cl0) {
        for(int i = line_start; i < line_end; ++i) {
            uint c = str[i];

            if(c == '\\') {
                if(i + 1 < str.size()) {
                    uint next = str[i + 1];

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

            auto glyph = f.at(c);

            float advance = glyph.advance;
            if(bold && glyph.visible) advance += bold_factor;

            if(pos + advance * 0.5f > cursor_x) {
                return {i, false};
            }
            
            pos += advance;
        }
    } else {
        return {-1, false};
    }

    //if(text.size() == 0) line_end = 0;

    return {line_end, false};
}

vec2 compute_cursor_pos(uint index, bool wrap, font_asset& f, std::string text, uint text_size, std::vector<text_line_data> text.lines, ALIGNMENT alignment) {
    int line_index = 0;
    for(line_index = 0; line_index < text.lines.size() - 1; ++line_index) {
        uint next = text.lines[line_index + 1].start_index;
        if(next > index || (!wrap && next == index)) break;
    }

    vec2 pos;
    pos.y = (text.lines.size() - line_index - 1) * f.line_height;
    pos.x = 0.0f;

    if(line_index < 0) line_index = 0;
    else if(line_index >= text.lines.size()) line_index = text.lines.size() - 1;

    std::u32string str = convert_string(text);

    uint line_start, line_end;
    line_start = text.lines[line_index].start_index;
    if(line_index == text.lines.size() - 1) line_end = str.size();
    else line_end = text.lines[line_index + 1].start_index;

    bool bold = false;
    bool italic = false;
    bool hex = false;
    uint num_escape_seq = 0;
    bool show_debug = false;

    text_line_data data = text.lines[line_index];
    bold = data.bold;
    italic = data.italic;

    for(int i = line_start; i < index; ++i) {
        uint c = str[i];

        if(c == '\\') {
            if(i + 1 < str.size()) {
                uint next = str[i + 1];

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

        auto glyph = f.at(c);

        pos.x += glyph.advance;
        if(bold && glyph.visible) pos.x += bold_factor;
    }

    return pos;
}

std::vector<float> compute_text_bounds(font_asset& f, std::string text, uint text_size, uint width, bool wrap, ALIGNMENT alignment) {
    vec2 resize_range = vec2(-FLT_MAX, FLT_MAX);
    float max_x = 0.0f;
    float max_w = 0.0f;

    text_line_indices.clear();
    line_data.clear();

    std::u32string str = convert_string(text);

    //

    std::vector<ui_vertex> ret;

    bool show_debug = false;

    text_line_data word_start_data = {0, false, false, vec3(1.0f)};
    text_line_data line_start_data = {0, false, false, vec3(1.0f)};

    bool accept_index = false;

    int num_escape_seq = 0;
    int i = 0;

    vec2 pos = vec2(0.0f);
    vec4 range = vec4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    vec4 color = vec4(1.0f);
    bool bold = false;
    bool italic = false;
    bool hex = false;

    uint num_lines = 0;

    vec2 word_pos = vec2(0.0f);

    uint line_index = 0;

    //

    auto insert_line = [&]() {
        int line_width = pos.x;
        int offset;
        
        resize_range.x = glm::max(resize_range.x, pos.x);

        max_x = glm::max(pos.x, max_x);
        max_w += pos.x;

        pos.x = 0;
        pos.y -= float(f.line_height) * text_size;
        ++num_lines;

        text_line_indices.push_back(line_start_data.start);
        line_data.push_back(line_start_data);

        line_start_data = word_start_data;
    };

    auto insert_word = [&]() {
        uint end = pos.x + word_pos.x;

        if(end > width && pos.x != 0.0f && wrap) {
            float min_v = pos.x;
            float max_v = end;
            
            max_x = glm::max(max_x, float(width));

            resize_range.x = glm::max(resize_range.x, min_v);
            resize_range.y = glm::min(resize_range.y, max_v);

            insert_line();
        } else {
            float min_v = end;
            resize_range.x = glm::max(resize_range.x, min_v + 1);
        }

        pos.x += word_pos.x;
        word_pos = vec2(0.0f);

        word_start_data.start = i + 1;
        word_start_data.bold = bold;
        word_start_data.italic = italic;
        word_start_data.color = color.xyz();
    };

    auto insert_char = [&](uint c) {
        glyph_data& gd = f.at(c);

        float stride = gd.advance;

        if(!gd.visible) {
            if(alignment == axiom::text_alignment::LEFT) {
                word_pos.x += stride * text_size;

                insert_word();
            } else if(alignment == axiom::text_alignment::CENTER) {
                insert_word();

                word_pos.x += stride * text_size;

                insert_word();
            } else if(alignment == axiom::text_alignment::RIGHT) {
                insert_word();

                word_pos.x += stride * text_size;
            }
        } else {
            if(bold) {
                stride += bold_factor;
            }

            word_pos.x += stride * text_size;
        }
    };

    for(i = 0; i < str.size(); ++i) {
        uint c = str[i];

        if(c == '\n') {
            insert_word();
            insert_line();

            word_start_data.start = i + 1;
            word_start_data.bold = bold;
            word_start_data.italic = italic;
            word_start_data.color = color.xyz();
            line_start_data = word_start_data;

            continue;
        } else {
            if(c == '\\') {
                if(i + 1 < str.size()) {
                    uint next = str[i + 1];

                    if(next == 'c') {
                        if(i + 1 + 3 < str.size()) {
                            std::u32string s(str.begin() + (i + 2), str.begin() + (i + 5));

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

    float height = num_lines * f.line_height * text_size;
    float text_x = ceil(max_x);
    float max_width = max_w + 0.01f;

    return {resize_range.x, resize_range.y, height, text_x, max_width, max_x};
}

std::vector<ui_vertex> mesh_select(font_asset& f, std::string str, uint text_size, uint width, ivec2 select_range, std::vector<text_line_data> line_data, ALIGNMENT alignment, bool show_debug) {
    select_range = ivec2(glm::min(select_range.x, select_range.y), glm::max(select_range.x, select_range.y));
    
    if(select_range.x == select_range.y) {
        vec2 pos = compute_cursor_pos(select_range.x, false, f, str, 1, line_data, alignment);

        std::vector<ui_vertex> total_ret;

        ui_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        ui_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        ui_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        ui_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        // panel
        std::vector<ui_vertex> ret = {a, b, d, a, d, c};
        for(ui_vertex& v : ret) {
            v.pos = vec3(pos + v.pos.xy() * vec2(1.0f, f.line_height), 0.0f);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(1.0f);
            v.data = 1;
        }

        return ret;
    } else {
        ivec2 lines_range = {-1, -1};
        uint ii = 0;
        while(true) {
            text_line_data data = line_data[ii];

            if(data.start > select_range.x) {
                break;
            }

            ++ii;

            if(ii >= line_data.size()) break;
        }
        lines_range.x = ii - 1;
        ii = 0;
        while(true) {
            text_line_data data = line_data[ii];

            if(data.start > select_range.y) {
                break;
            }

            ++ii;
            
            if(ii >= line_data.size()) break;
        }
        lines_range.y = ii;

        //
        
        float max_x = 0.0f;
        text_start.clear();

        float italic_factor = 1.0f / 3.5f;
        float bold_factor = 1.0f;

        std::u32string text = convert_string(str);

        std::vector<uint> text_line_indices;
        std::vector<uint> text_line_origins;

        uint line_start_index = 0;
        uint word_start_index = 0;

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
            pos.y -= f.line_height;
            ++num_lines;
            
            text_line_indices.push_back(line_start_index);
            text_line_origins.push_back(offset);
        };

        auto insert_word = [&]() {
            uint end = pos.x + word_pos.x;

            if(end > width && pos.x != 0.0f) {
                insert_line();
            }
            
            if(line_ret.size() == 0) line_start_index = word_start_index;

            // insert word
            for(ui_vertex& v : word_ret) {
                v.pos += vec3(pos, 0.0f);
            }
            
            line_ret.insert(line_ret.end(), word_ret.begin(), word_ret.end());

            word_ret.clear();
            
            pos.x += word_pos.x;
            word_pos = vec2(0.0f);  
        };

        auto insert_selection = [&](ivec2 pos, ivec2 size) {
            if(word_ret.size() == 0) {
                word_start_index = i;
            }

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
            glyph_data& gd = f.at(codepoint);

            float stride = gd.advance;

            if(word_ret.size() == 0) {
                word_start_index = i;
            }

            if(!gd.visible) {
                if(alignment == axiom::text_alignment::LEFT) {
                    if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                    word_pos.x += stride;
                    
                    insert_word();
                } else if(alignment == axiom::text_alignment::CENTER) {
                    insert_word();

                    if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                    word_pos.x += stride;

                    insert_word();
                } else if(alignment == axiom::text_alignment::RIGHT) {
                    insert_word();

                    if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                    word_pos.x += stride;
                }
            } else {
                if(bold) {
                    stride += bold_factor;
                }
                
                if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance + ((bold) ? bold_factor : 0.0f), f.line_height});

                word_pos.x += stride;
            }
        };

        uint start = line_data[lines_range.x].start_index;
        uint end;
        if(lines_range.y >= line_data.size()) end = text.size();
        else end = line_data[lines_range.y].start_index;

        bold = line_data[lines_range.x].bold;
        italic = line_data[lines_range.x].italic;

        pos.y = -f.line_height * lines_range.x;

        for(i = start; i < end; ++i) {
            uint c = text[i];

            if(c == '\n') {
                if(i + 1 >= select_range.x && i + 1 < select_range.y && text[i - 1] == '\n') {
                    if(alignment == axiom::text_alignment::LEFT) insert_selection(word_pos, {6, f.line_height});
                    else if(alignment == axiom::text_alignment::CENTER) insert_selection(word_pos - vec2(3, 0), {6, f.line_height});
                    else if(alignment == axiom::text_alignment::RIGHT) insert_selection(word_pos - vec2(6, 0), {6, f.line_height});
                }

                insert_word();
                insert_line();
                word_start_index = i;
                line_start_index = i;
                
                if(i + 1 >= select_range.x && i + 1 < select_range.y && (text[i + 1] == '\n' || i == text.size() - 1)) {
                    if(alignment == axiom::text_alignment::LEFT) insert_selection(word_pos, {6, f.line_height});
                    else if(alignment == axiom::text_alignment::CENTER) insert_selection(word_pos - vec2(3, 0), {6, f.line_height});
                    else if(alignment == axiom::text_alignment::RIGHT) insert_selection(word_pos - vec2(6, 0), {6, f.line_height});
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

        float offset = (line_data.size() - 1) * f.line_height;
        
        for(ui_vertex& v : ret) {
            v.pos.y = v.pos.y + offset;
            v.pos.x -= min_offset;

            v.pos *= float(text_size);
        }

        for(ui_vertex& v : ret) {
            range.x = glm::min(range.x, v.pos.x);
            range.y = glm::min(range.y, v.pos.y);
            range.z = glm::max(range.z, v.pos.x);
            range.w = glm::max(range.w, v.pos.y);
        }

        text_range = {max_x, num_lines * f.line_height};
        text_start = text_line_indices;
        
        return ret;
    }
}

std::vector<ui_vertex> mesh_text(font_asset& f, std::string str, uint text_size, uint width, ivec2 select_range, ALIGNMENT alignment, bool show_debug) {
    float max_x = 0.0f;
    text_start.clear();

    float italic_factor = 1.0f / 3.5f;
    float bold_factor = 1.0f;

    std::u32string text = convert_string(str);

    std::vector<uint> text_line_indices;
    std::vector<uint> text_line_origins;

    uint line_start_index = 0;
    uint word_start_index = 0;

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
        pos.y -= f.line_height;
        ++num_lines;
        
        text_line_indices.push_back(line_start_index);
        text_line_origins.push_back(offset);
    };

    auto insert_word = [&]() {
        uint end = pos.x + word_pos.x;

        if(end > width && pos.x != 0.0f) {
            insert_line();
        }
        
        if(line_ret.size() == 0) line_start_index = word_start_index;

        // insert word
        for(ui_vertex& v : word_ret) {
            v.pos += vec3(pos, 0.0f);
        }
        
        line_ret.insert(line_ret.end(), word_ret.begin(), word_ret.end());

        word_ret.clear();
        
        pos.x += word_pos.x;
        word_pos = vec2(0.0f);  
    };

    auto insert_selection = [&](ivec2 pos, ivec2 size) {
        if(word_ret.size() == 0) {
            word_start_index = i;
        }

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
        glyph_data& gd = f.at(codepoint);

        float stride = gd.advance;

        if(word_ret.size() == 0) {
            word_start_index = i;
        }

        if(!gd.visible) {
            ui_vertex v;
            v.pos = vec3(word_pos + vec2(stride, 0), 0.0f);
            v.data = 0xFFFFFFFF;
            word_ret.push_back(v);
            word_ret.push_back(v);
            word_ret.push_back(v);

            if(alignment == axiom::text_alignment::LEFT) {
                if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                word_pos.x += stride;
                
                insert_word();
            } else if(alignment == axiom::text_alignment::CENTER) {
                insert_word();

                if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                word_pos.x += stride;

                insert_word();
            } else if(alignment == axiom::text_alignment::RIGHT) {
                insert_word();

                if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                word_pos.x += stride;
            }
        } else {
            std::vector<ui_vertex> vs = create_char(gd);

            for(ui_vertex& v : vs) {
                v.pos += vec3(word_pos, 0.0f);
            }

            for(ui_vertex& v : vs) {
                if(italic) {
                    v.pos.x += float(v.pos.y - word_pos.y - f.line_height * 0.5f) * italic_factor;
                }

                v.color = color;
            }

            word_ret.insert(word_ret.end(), vs.begin(), vs.end());

            if(bold) {
                for(ui_vertex& v : vs) {
                    v.pos.x += bold_factor;
                }
                stride += bold_factor;
                
                word_ret.insert(word_ret.end(), vs.begin(), vs.end());
            }
            
            if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance + ((bold) ? bold_factor : 0.0f), f.line_height});

            word_pos.x += stride;
        }
    };

    for(i = 0; i < text.size(); ++i) {
        uint c = text[i];

        if(c == '\n') {
            insert_word();
            insert_line();
            word_start_index = i;
            line_start_index = i;
            
            if(i >= select_range.x && i < select_range.y && (text[i + 1] == '\n' || i == text.size() - 1)) {
                if(alignment == axiom::text_alignment::LEFT) insert_selection(word_pos, {6, f.line_height});
                else if(alignment == axiom::text_alignment::CENTER) insert_selection(word_pos - vec2(3, 0), {6, f.line_height});
                else if(alignment == axiom::text_alignment::RIGHT) insert_selection(word_pos - vec2(6, 0), {6, f.line_height});
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

    float offset = (num_lines - 1) * f.line_height;
    
    for(ui_vertex& v : ret) {
        v.pos.y = v.pos.y + offset;
        v.pos.x -= min_offset;

        v.pos *= float(text_size);
    }

    for(ui_vertex& v : ret) {
        range.x = glm::min(range.x, v.pos.x);
        range.y = glm::min(range.y, v.pos.y);
        range.z = glm::max(range.z, v.pos.x);
        range.w = glm::max(range.w, v.pos.y);
    }

    text_range = {max_x, num_lines * f.line_height};
    text_start = text_line_indices;

    return ret;
}

void text::mesh(font_asset& font) {
    if(wrap) width = size.x;
    else width = 0xFFFFFFFF;

    if(select_range.x != -1) vertices_select = mesh_select(font, string, 1, width, ivec2(glm::min(select_range.x, select_range.y), glm::max(select_range.x, select_range.y)), line_data, alignment, false);

    vertices = mesh_text(font, string, 1, width, {-1, -1}, alignment);
}

std::vector<ui_vertex> text::get_vertices() {
    std::vector<ui_vertex> ret;// = vertices_select;
    
    /*
    if(select_range.x == select_range.y) {
        double freq = 1.0;
        if((fmod(core.current_time - start_cursor, freq) / freq) > 0.5) {
            for(auto& vertex : ret) vertex.color.w = 0.0f;
        }
    }
    */
    
    /*

    ret.insert(ret.end(), vertices.begin(), vertices.end());
    
    for(ui_vertex& v : ret) {
        v.pos = vec3(ceil(position) + v.pos.xy(), z);
    }

    return ret;
}

void text::refresh() {
    if(wrap) width = size.x;
    else width = 0xFFFFFFFF;

    std::vector<float> ret = compute_text_bounds(*font, string, 1, width, wrap, alignment);

    resize_range = {ret[0], ret[1]};
    size.y = ret[2];
    size.x = glm::min(size.x, ret[5]);
    //size.x = width;
    //text_x = ret[3];
    
    max_width = ret[4];

    this->line_data = line_data;
}

/*
void text::select(vec4 cursor_range) {
    ivec2 prev_select = select_range;

    vec4 abs_cursor_range = vec4(glm::min(cursor_range.x, cursor_range.z), min(cursor_range.y, cursor_range.w), glm::max(cursor_range.x, cursor_range.z), glm::max(cursor_range.y, cursor_range.w));

    vec4 text_range = vec4(position, size + position);

    if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT) && editable) {
        if((includes(core.cursor_pos, (ivec4)text_range) || (click_range.x != click_range.z && click_range.y != click_range.w && includes(core.cursor_pos, click_range)))) {
            focused = true;
        } else if(focused) {
            focused = false;
        }
    }

    if(gui_system.isolate_selection && focused || !gui_system.isolate_selection) {
        //if(includes((ivec4)abs_cursor_range, (ivec4)text_range) || (click_range.x != click_range.z && click_range.y != click_range.w && includes((ivec4)abs_cursor_range, click_range))) {
            int line_a = line_data.size() - floor(float(cursor_range.y - position.y) / font.line_height) - 1;
            int line_b = line_data.size() - floor(float(cursor_range.w - position.y) / font.line_height) - 1;

            bool wrap_selection_start = false;
            bool wrap_selection_end = false;
            if(line_b < 0 && line_a >= 0 || line_a < 0 && line_b >= 0) wrap_selection_start = true;
            if(line_b > (int)line_data.size() - 1 && line_a < (int)line_data.size() || line_a > (int)line_data.size() - 1 && line_b < (int)line_data.size()) wrap_selection_end = true;

            if(line_a == line_b || line_a < line_b) {
                vec2 cursor_a = cursor_range.xy() - position;
                vec2 cursor_b = cursor_range.zw() - position;

                if(line_a == line_b && cursor_a.x > cursor_b.x) {
                    if(wrap_selection_start) cursor_b.x = 0.0f;
                    if(wrap_selection_end) cursor_a.x = size.x + 1;
                } else {
                    if(wrap_selection_start) cursor_a.x = 0.0f;
                    if(wrap_selection_end) cursor_b.x = size.x + 1;
                }

                int ia = compute_cursor_index(cursor_a, font, string, 1, line_data, alignment, true, false).first;
                int ib = compute_cursor_index(cursor_b, font, string, 1, line_data, alignment, false, true).first;

                if(ia == -1 || ib == -1) select_range = {-1, -1};
                else select_range = {ia, ib};
            } else if(line_a > line_b) {
                vec2 cursor_a = cursor_range.zw() - position;
                vec2 cursor_b = cursor_range.xy() - position;
                
                if(wrap_selection_start) cursor_a.x = 0.0f;
                if(wrap_selection_end) cursor_b.x = size.x + 1;

                int ia = compute_cursor_index(cursor_a, font, string, 1, line_data, alignment, true, false).first;
                int ib = compute_cursor_index(cursor_b, font, string, 1, line_data, alignment, false, true).first;

                if(ia == -1 || ib == -1) select_range = {-1, -1};
                else select_range = {ib, ia};
            }
        //} else {
        //    select_range = vec2(-1);
        //}
    } else {
        select_range = vec2(-1);
    }

    if(prev_select != select_range) {
        if(select_range != ivec2(-1)) {
            if(select_range.x == select_range.y) start_cursor = core.current_time;

            vertices_select = mesh_select(font, string, 1, width, select_range, line_data, alignment, false);
        } else {
            vertices_select.clear();
        }
    }
}
*/

/*
std::string text::retrieve() {
    std::u32string u32str = convert_string(string);

    int minv = glm::min(select_range.x, select_range.y);
    int maxv = glm::max(select_range.x, select_range.y);

    std::u32string str(u32str.begin() + minv, u32str.begin() + maxv);

    return convert_string(str);
}
*/

}