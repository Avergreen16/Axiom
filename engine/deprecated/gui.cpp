#include "gui.hpp"
#include "render.hpp"
#include "input.hpp"
#include "physics.hpp"

#include <numeric>

#include "stb_image.h"
#include "stb_image_write.h"

float italic_factor = 1.0f / 3.5f;
float bold_factor = 1.0f;

vec2 text_range;
std::vector<uint32_t> text_start;

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
    return {max(a.x, b.x), max(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}

Glyph_data& Font::at(uint32_t key) {
    if(glyph_map.find(key) != glyph_map.end()) return glyph_map[key];
    return empty_data;
}

enum bdf_region{BDF_NULL, BDF_HEADER, BDF_GLYPH};
bool bitmap = false;

void Font::init(std::string filepath) {
    std::string text = get_text_from_file(filepath);

    ivec2 size;
    ivec2 offset;
    int line_ascent = 0;
    int line_descent = 0;

    std::stringstream ss(text);

    bdf_region region = BDF_NULL;

    uint32_t encoding;
    Glyph_data glyph;
    std::vector<uint32_t> keys;

    while(true) {
        std::string line;
        std::getline(ss, line);

        std::stringstream ss_line(line);
        std::string name;
        ss_line >> name;

        if(region == BDF_HEADER) {
            if(name == "FONTBOUNDINGBOX") {
                int xsize, ysize, xoffset, yoffset;
                ss_line >> xsize >> ysize >> xoffset >> yoffset;

                size = {xsize, ysize};
                offset = {xoffset, yoffset};
            } else if(name == "FONT_ASCENT") {
                ss_line >> line_ascent;
            } else if(name == "FONT_DESCENT") {
                ss_line >> line_descent;
            }
        } else if(region == BDF_GLYPH) {
            bool collect_bits = bitmap;

            if(name == "ENCODING") {
                int number;
                ss_line >> number;

                encoding = number;
            } else if(name == "DWIDTH") {
                int x, y;
                ss_line >> x >> y;

                glyph.advance = x;
            } else if(name == "BBX") {
                int xsize, ysize, xoffset, yoffset;
                ss_line >> xsize >> ysize >> xoffset >> yoffset;

                glyph.size = {xsize, ysize};
                glyph.offset = {xoffset, yoffset};
            } else if(name == "BITMAP") {
                bitmap = true;
                glyph.bitmap.clear();
            } else if(name == "ENDCHAR") {
                region = BDF_NULL;
                bitmap = false;
                collect_bits = false;

                keys.push_back(encoding);
                glyph_map.emplace(encoding, glyph);
            }

            if(collect_bits) {
                for(int i = 0; i < name.size() / 2; ++i) {
                    std::string bit(name.begin() + i * 2, name.begin() + (i + 1) * 2);

                    uint8_t b = from_base(bit, 16);

                    glyph.bitmap.push_back(b);
                }
            }
        }

        if(name == "STARTFONT") region = BDF_HEADER;
        else if(name == "ENDFONT") break;
        else if(name == "STARTCHAR") region = BDF_GLYPH;
    }

    std::vector<bool> pixels;
    ivec2 tex_size;
    uint32_t width = 1024;
    uint32_t num_per_row = width / size.x;
    uint32_t height = ceil(float(glyph_map.size()) / num_per_row) * size.y;
    tex_size = ivec2(width, height);

    pixels.resize(tex_size.x * tex_size.y, 0);

    uint32_t pos = 0;
    for(uint32_t i : keys) {
        Glyph_data& glyph = glyph_map[i];
        glyph.offset.y += line_descent;

        ivec2 origin = ivec2(pos % num_per_row, pos / num_per_row) * size;
        origin = origin + (glyph.offset - ivec2(0, line_descent) - offset);

        glyph.pos_tex = origin;

        uint32_t byte_row = ceil(float(glyph.size.x) / 8);

        bool visible = false;

        for(int y = 0; y < glyph.size.y; ++y) {
            for(int x = 0; x < byte_row; ++x) {
                int index = y * byte_row + x;
                uint8_t byte = glyph.bitmap[index];

                if(byte != 0x0) visible = true;

                for(int xx = x * 8; xx < min((x + 1) * 8, glyph.size.x); ++xx) {
                    int bit = 7 - (xx - x * 8);

                    bool b = (byte >> bit) & 0x1;

                    ivec2 bit_pos = origin + ivec2(xx, glyph.size.y - 1 - y);
                    int bit_index = bit_pos.y * tex_size.x + bit_pos.x;

                    pixels[bit_index] = b;
                }
            }
        }

        glyph.visible = visible;

        ++pos;
    }

    std::vector<uint8_t> texture(tex_size.x * tex_size.y * 4, 0x0);

    for(int i = 0; i < tex_size.x * tex_size.y; ++i) {
        if(pixels[i]) {
            texture[i * 4] = 0xFF;
            texture[i * 4 + 1] = 0xFF;
            texture[i * 4 + 2] = 0xFF;
            texture[i * 4 + 3] = 0xFF;
        }
    }

    core.textures.emplace("text_texture", std::make_shared<Texture>(Texture(texture.data(), ivec3(tex_size, 1), GL_TEXTURE_2D, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE})));
    
    line_height = line_ascent + line_descent;
}

Font::Font(std::string filepath) {
    init(filepath);
}

std::vector<UI_vertex> create_char(Glyph_data& glyph) {
    std::vector<UI_vertex> ret;

    UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

    ret.push_back(a);
    ret.push_back(b);
    ret.push_back(d);
    ret.push_back(a);
    ret.push_back(d);
    ret.push_back(c);

    for(UI_vertex& v : ret) {
        v.pos = vec3(v.pos.xy() * vec2(glyph.size) + vec2(glyph.offset), 0.0f);
        v.tex_pos = vec2(glyph.pos_tex) + v.tex_pos * vec2(glyph.size);
    }

    return ret;
}

std::pair<int, bool> compute_cursor_index(vec2 cursor_pos, Font& f, std::string text, uint32_t text_size, std::vector<Text_Line_Data> line_indices, ALIGNMENT alignment, bool cl0 = true, bool cl1 = true) {
    int line_index = (int)line_indices.size() - glm::floor(cursor_pos.y / f.line_height) - 1;

    if(line_index < 0 && cl0) {
        line_index = 0;
    }
    if(line_index >= (int)line_indices.size() && cl1) line_index = line_indices.size() - 1;

    if(line_index < 0 || line_index >= (int)line_indices.size()) {
        return {-1, false};
    }

    std::u32string str = convert_string(text);

    uint32_t line_start, line_end;
    line_start = line_indices[line_index].start;
    if(line_index == (line_indices.size() - 1)) line_end = str.size();
    else line_end = line_indices[line_index + 1].start;

    float cursor_x = cursor_pos.x;

    bool bold = false;
    bool italic = false;
    bool hex = false;

    int num_escape_seq = 0;
    bool show_debug = false;

    float start_buffer = 3.0f;

    Text_Line_Data data = line_indices[line_index];
    bold = data.bold;
    italic = data.italic;

    float pos = 0.0f;
    
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

vec2 compute_cursor_pos(uint32_t index, bool wrap, Font& f, std::string text, uint32_t text_size, std::vector<Text_Line_Data> line_indices, ALIGNMENT alignment) {
    int line_index = 0;
    for(line_index = 0; line_index < line_indices.size() - 1; ++line_index) {
        uint32_t next = line_indices[line_index + 1].start;
        if(next > index || (!wrap && next == index)) break;
    }

    vec2 pos;
    pos.y = (line_indices.size() - line_index - 1) * f.line_height;
    pos.x = 0.0f;

    if(line_index < 0) line_index = 0;
    else if(line_index >= line_indices.size()) line_index = line_indices.size() - 1;

    std::u32string str = convert_string(text);

    uint32_t line_start, line_end;
    line_start = line_indices[line_index].start;
    if(line_index == line_indices.size() - 1) line_end = str.size();
    else line_end = line_indices[line_index + 1].start;

    bool bold = false;
    bool italic = false;
    bool hex = false;
    uint32_t num_escape_seq = 0;
    bool show_debug = false;

    Text_Line_Data data = line_indices[line_index];
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

        auto glyph = f.at(c);

        pos.x += glyph.advance;
        if(bold && glyph.visible) pos.x += bold_factor;
    }

    return pos;
}

std::vector<uint32_t> text_line_indices;
std::vector<Text_Line_Data> text_line_data;
std::vector<float> compute_text_bounds(Font& f, std::string text, uint32_t text_size, uint32_t width, bool wrap, ALIGNMENT alignment) {
    vec2 resize_range = vec2(-FLT_MAX, FLT_MAX);
    float max_x = 0.0f;
    float max_w = 0.0f;

    text_line_indices.clear();
    text_line_data.clear();

    std::u32string str = convert_string(text);

    //

    std::vector<UI_vertex> ret;

    bool show_debug = false;

    Text_Line_Data word_start_data = {0, false, false, vec3(1.0f)};
    Text_Line_Data line_start_data = {0, false, false, vec3(1.0f)};

    bool accept_index = false;

    int num_escape_seq = 0;
    int i = 0;

    vec2 pos = vec2(0.0f);
    vec4 range = vec4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    vec4 color = vec4(1.0f);
    bool bold = false;
    bool italic = false;
    bool hex = false;

    uint32_t num_lines = 0;

    vec2 word_pos = vec2(0.0f);

    uint32_t line_index = 0;

    //

    auto insert_line = [&]() {
        int line_width = pos.x;
        int offset;
        
        resize_range.x = max(resize_range.x, pos.x);

        max_x = max(pos.x, max_x);
        max_w += pos.x;

        pos.x = 0;
        pos.y -= float(f.line_height) * text_size;
        ++num_lines;

        text_line_indices.push_back(line_start_data.start);
        text_line_data.push_back(line_start_data);

        line_start_data = word_start_data;
    };

    auto insert_word = [&]() {
        uint32_t end = pos.x + word_pos.x;

        if(end > width && pos.x != 0.0f && wrap) {
            float min_v = pos.x;
            float max_v = end;
            
            max_x = max(max_x, float(width));

            resize_range.x = max(resize_range.x, min_v);
            resize_range.y = min(resize_range.y, max_v);

            insert_line();
        } else {
            float min_v = end;
            resize_range.x = max(resize_range.x, min_v + 1);
        }

        pos.x += word_pos.x;
        word_pos = vec2(0.0f);

        word_start_data.start = i + 1;
        word_start_data.bold = bold;
        word_start_data.italic = italic;
        word_start_data.color = color.xyz();
    };

    auto insert_char = [&](uint32_t c) {
        Glyph_data& gd = f.at(c);

        float stride = gd.advance;

        if(!gd.visible) {
            if(alignment == ALIGNMENT_LEFT) {
                word_pos.x += stride * text_size;

                insert_word();
            } else if(alignment == ALIGNMENT_CENTER) {
                insert_word();

                word_pos.x += stride * text_size;

                insert_word();
            } else if(alignment == ALIGNMENT_RIGHT) {
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
        uint32_t c = str[i];

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
                    uint32_t next = str[i + 1];

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

std::vector<UI_vertex> mesh_select(Font& f, std::string str, uint32_t text_size, uint32_t width, ivec2 select_range, std::vector<Text_Line_Data> line_data, ALIGNMENT alignment, bool show_debug) {
    select_range = ivec2(min(select_range.x, select_range.y), max(select_range.x, select_range.y));
    
    if(select_range.x == select_range.y) {
        vec2 pos = compute_cursor_pos(select_range.x, false, f, str, 1, line_data, alignment);

        std::vector<UI_vertex> total_ret;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        // panel
        std::vector<UI_vertex> ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(pos + v.pos.xy() * vec2(1.0f, f.line_height), 0.0f);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(1.0f);
            v.data = 1;
        }

        return ret;
    } else {
        ivec2 lines_range = {-1, -1};
        uint32_t ii = 0;
        while(true) {
            Text_Line_Data data = line_data[ii];

            if(data.start > select_range.x) {
                break;
            }

            ++ii;

            if(ii >= line_data.size()) break;
        }
        lines_range.x = ii - 1;
        ii = 0;
        while(true) {
            Text_Line_Data data = line_data[ii];

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

        std::vector<uint32_t> text_line_indices;
        std::vector<uint32_t> text_line_origins;

        uint32_t line_start_index = 0;
        uint32_t word_start_index = 0;

        bool accept_index = false;

        int num_escape_seq = 0;
        int i = 0;

        std::vector<UI_vertex> ret;
        vec2 pos = vec2(0.0f);
        vec4 range = vec4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

        vec4 color = vec4(1.0f);
        bool bold = false;
        bool italic = false;
        bool hex = false;
        float min_offset = 0.0;

        uint32_t num_lines = 0;

        std::vector<UI_vertex> word_ret;
        vec2 word_pos = vec2(0.0f);
        
        std::vector<UI_vertex> line_ret;

        auto insert_line = [&]() {
            int line_width = pos.x;
            int offset;

            if(alignment == ALIGNMENT_LEFT) offset = 0.0f;
            else if(alignment == ALIGNMENT_CENTER) {
                offset = round(float(-line_width) / 2);
            }
            else if(alignment == ALIGNMENT_RIGHT) offset = float(-line_width);

            min_offset = min(min_offset, float(offset));
            for(UI_vertex& v : line_ret) {
                v.pos.x += offset;
            }
            
            ret.insert(ret.end(), line_ret.begin(), line_ret.end());
            
            line_ret.clear();

            max_x = max(max_x, pos.x);

            pos.x = 0;
            pos.y -= f.line_height;
            ++num_lines;
            
            text_line_indices.push_back(line_start_index);
            text_line_origins.push_back(offset);
        };

        auto insert_word = [&]() {
            uint32_t end = pos.x + word_pos.x;

            if(end > width && pos.x != 0.0f) {
                insert_line();
            }
            
            if(line_ret.size() == 0) line_start_index = word_start_index;

            // insert word
            for(UI_vertex& v : word_ret) {
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

            UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
            UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
            UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
            UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

            std::vector<UI_vertex> r = {a, b, d, a, d, c};
            for(UI_vertex& v : r) {
                v.pos = vec3(vec2(pos) + v.pos.xy() * vec2(size), 0.0f);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(1.0f, 1.0f, 1.0f, 0.35f);
                v.data = 1;
            }
            word_ret.insert(word_ret.end(), r.begin(), r.end());
        };

        auto insert_char = [&](uint32_t codepoint) {
            Glyph_data& gd = f.at(codepoint);

            float stride = gd.advance;

            if(word_ret.size() == 0) {
                word_start_index = i;
            }

            if(!gd.visible) {
                if(alignment == ALIGNMENT_LEFT) {
                    if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                    word_pos.x += stride;
                    
                    insert_word();
                } else if(alignment == ALIGNMENT_CENTER) {
                    insert_word();

                    if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                    word_pos.x += stride;

                    insert_word();
                } else if(alignment == ALIGNMENT_RIGHT) {
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

        uint32_t start = line_data[lines_range.x].start;
        uint32_t end;
        if(lines_range.y >= line_data.size()) end = text.size();
        else end = line_data[lines_range.y].start;

        bold = line_data[lines_range.x].bold;
        italic = line_data[lines_range.x].italic;

        pos.y = -f.line_height * lines_range.x;

        for(i = start; i < end; ++i) {
            uint32_t c = text[i];

            if(c == '\n') {
                if(i + 1 >= select_range.x && i + 1 < select_range.y && text[i - 1] == '\n') {
                    if(alignment == ALIGNMENT_LEFT) insert_selection(word_pos, {6, f.line_height});
                    else if(alignment == ALIGNMENT_CENTER) insert_selection(word_pos - vec2(3, 0), {6, f.line_height});
                    else if(alignment == ALIGNMENT_RIGHT) insert_selection(word_pos - vec2(6, 0), {6, f.line_height});
                }

                insert_word();
                insert_line();
                word_start_index = i;
                line_start_index = i;
                
                if(i + 1 >= select_range.x && i + 1 < select_range.y && (text[i + 1] == '\n' || i == text.size() - 1)) {
                    if(alignment == ALIGNMENT_LEFT) insert_selection(word_pos, {6, f.line_height});
                    else if(alignment == ALIGNMENT_CENTER) insert_selection(word_pos - vec2(3, 0), {6, f.line_height});
                    else if(alignment == ALIGNMENT_RIGHT) insert_selection(word_pos - vec2(6, 0), {6, f.line_height});
                }

                continue;
            } else {
                if(c == '\\') {
                    if(i + 1 < text.size()) {
                        uint32_t next = text[i + 1];

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
        
        for(UI_vertex& v : ret) {
            v.pos.y = v.pos.y + offset;
            v.pos.x -= min_offset;

            v.pos *= float(text_size);
        }

        for(UI_vertex& v : ret) {
            range.x = min(range.x, v.pos.x);
            range.y = min(range.y, v.pos.y);
            range.z = max(range.z, v.pos.x);
            range.w = max(range.w, v.pos.y);
        }

        text_range = {max_x, num_lines * f.line_height};
        text_start = text_line_indices;
        
        return ret;
    }
}

std::vector<UI_vertex> mesh_text(Font& f, std::string str, uint32_t text_size, uint32_t width, ivec2 select_range, ALIGNMENT alignment, bool show_debug) {
    float max_x = 0.0f;
    text_start.clear();

    float italic_factor = 1.0f / 3.5f;
    float bold_factor = 1.0f;

    std::u32string text = convert_string(str);

    std::vector<uint32_t> text_line_indices;
    std::vector<uint32_t> text_line_origins;

    uint32_t line_start_index = 0;
    uint32_t word_start_index = 0;

    bool accept_index = false;

    int num_escape_seq = 0;
    int i = 0;

    std::vector<UI_vertex> ret;
    vec2 pos = vec2(0.0f);
    vec4 range = vec4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    vec4 color = vec4(1.0f);
    bool bold = false;
    bool italic = false;
    bool hex = false;
    float min_offset = 0.0;

    uint32_t num_lines = 0;

    std::vector<UI_vertex> word_ret;
    vec2 word_pos = vec2(0.0f);
    
    std::vector<UI_vertex> line_ret;

    auto insert_line = [&]() {
        int line_width = pos.x;
        int offset;

        if(alignment == ALIGNMENT_LEFT) offset = 0.0f;
        else if(alignment == ALIGNMENT_CENTER) {
            offset = round(float(-line_width) / 2);
        }
        else if(alignment == ALIGNMENT_RIGHT) offset = float(-line_width);

        min_offset = min(min_offset, float(offset));
        for(UI_vertex& v : line_ret) {
            v.pos.x += offset;
        }
        
        ret.insert(ret.end(), line_ret.begin(), line_ret.end());
        
        line_ret.clear();

        max_x = max(max_x, pos.x);

        pos.x = 0;
        pos.y -= f.line_height;
        ++num_lines;
        
        text_line_indices.push_back(line_start_index);
        text_line_origins.push_back(offset);
    };

    auto insert_word = [&]() {
        uint32_t end = pos.x + word_pos.x;

        if(end > width && pos.x != 0.0f) {
            insert_line();
        }
        
        if(line_ret.size() == 0) line_start_index = word_start_index;

        // insert word
        for(UI_vertex& v : word_ret) {
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

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        std::vector<UI_vertex> r = {a, b, d, a, d, c};
        for(UI_vertex& v : r) {
            v.pos = vec3(vec2(pos) + v.pos.xy() * vec2(size), 0.0f);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(1.0f, 1.0f, 1.0f, 0.35f);
            v.data = 1;
        }
        word_ret.insert(word_ret.end(), r.begin(), r.end());
    };

    auto insert_char = [&](uint32_t codepoint) {
        Glyph_data& gd = f.at(codepoint);

        float stride = gd.advance;

        if(word_ret.size() == 0) {
            word_start_index = i;
        }

        if(!gd.visible) {
            UI_vertex v;
            v.pos = vec3(word_pos + vec2(stride, 0), 0.0f);
            v.data = 0xFFFFFFFF;
            word_ret.push_back(v);
            word_ret.push_back(v);
            word_ret.push_back(v);

            if(alignment == ALIGNMENT_LEFT) {
                if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                word_pos.x += stride;
                
                insert_word();
            } else if(alignment == ALIGNMENT_CENTER) {
                insert_word();

                if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                word_pos.x += stride;

                insert_word();
            } else if(alignment == ALIGNMENT_RIGHT) {
                insert_word();

                if(i >= select_range.x && i < select_range.y) insert_selection(word_pos, {gd.advance, f.line_height});
                word_pos.x += stride;
            }
        } else {
            std::vector<UI_vertex> vs = create_char(gd);

            for(UI_vertex& v : vs) {
                v.pos += vec3(word_pos, 0.0f);
            }

            for(UI_vertex& v : vs) {
                if(italic) {
                    v.pos.x += float(v.pos.y - word_pos.y - f.line_height * 0.5f) * italic_factor;
                }

                v.color = color;
            }

            word_ret.insert(word_ret.end(), vs.begin(), vs.end());

            if(bold) {
                for(UI_vertex& v : vs) {
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
        uint32_t c = text[i];

        if(c == '\n') {
            insert_word();
            insert_line();
            word_start_index = i;
            line_start_index = i;
            
            if(i >= select_range.x && i < select_range.y && (text[i + 1] == '\n' || i == text.size() - 1)) {
                if(alignment == ALIGNMENT_LEFT) insert_selection(word_pos, {6, f.line_height});
                else if(alignment == ALIGNMENT_CENTER) insert_selection(word_pos - vec2(3, 0), {6, f.line_height});
                else if(alignment == ALIGNMENT_RIGHT) insert_selection(word_pos - vec2(6, 0), {6, f.line_height});
            }

            continue;
        } else {
            if(c == '\\') {
                if(i + 1 < text.size()) {
                    uint32_t next = text[i + 1];

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
    
    for(UI_vertex& v : ret) {
        v.pos.y = v.pos.y + offset;
        v.pos.x -= min_offset;

        v.pos *= float(text_size);
    }

    for(UI_vertex& v : ret) {
        range.x = min(range.x, v.pos.x);
        range.y = min(range.y, v.pos.y);
        range.z = max(range.z, v.pos.x);
        range.w = max(range.w, v.pos.y);
    }

    text_range = {max_x, num_lines * f.line_height};
    text_start = text_line_indices;

    return ret;
}

void GUI_system::init() {
    fonts.emplace("default mono", Font("resources/fonts/axiom_default.bdf"));
}

void GUI_system::step() {
    current_widget = widgets[current_widget]->parent;
}

void GUI_system::position(POSITION_MODE mode) {
    active_position = mode;
}
void GUI_system::buffer(vec4 buffer) {
    active_buffer = buffer;
}

void GUI_system::make_dirty(uint64_t root) {
    std::vector<uint64_t> path;
    std::vector<uint64_t> child_ids;

    //

    path = {root};
    child_ids = {0};
    while(true) {
        if (path.size() == 0) break;

        auto& widget = widgets[path.back()];

        vec2 children_size = vec2(0.0f);

        if (widget->children.size() <= child_ids.back()) {
            widget->dirty = true;

            // go up
            path.pop_back();
            child_ids.pop_back();
        } else {
            path.push_back(widget->children[child_ids.back()]);

            ++child_ids.back();
            child_ids.push_back(0);
        }
    }
}

void GUI_system::solve_constraints() {
    std::vector<uint64_t> roots;
    for(auto& [key, widget] : widgets) {
        if(widget->parent == NULL_WIDGET) roots.push_back(key);
    }

    for(uint64_t root : roots) {
        std::vector<uint64_t> path = {root};
        std::vector<uint64_t> child_ids = {0};
        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];
            if(child_ids.back() == 0) {
                for(auto& f : widget->before) f.func();
            }

            if (widget->children.size() <= child_ids.back()) {
                for(auto& f : widget->after) f.func();

                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }
    }
    
    for(auto& [key, widget] : widgets) widget->dirty = true;
}

void GUI_system::call() {
    handle_capture();

    copy = false;
    paste = false;
    copy_strings.clear();
    if(core.pressed_buttons.contains(GLFW_KEY_V) && core.key_map[GLFW_KEY_LEFT_CONTROL]) paste = true;
    if(core.pressed_buttons.contains(GLFW_KEY_C) && core.key_map[GLFW_KEY_LEFT_CONTROL] && !paste) copy = true;

    vertices.clear();
    cursor_mode = CURSOR_CLICK;

    //do_layout();

    std::vector<vec4> sizes;
    sizes.reserve(widgets.size());
    for(auto& [key, widget] : widgets) sizes.push_back(vec4(widget->position, widget->size));

    float tolerance = 0.1f;

    for(int i = 0; i < 4; ++i) {
        solve_constraints();
        
        /*
        for(auto& [key, widget] : widgets) {
            for(Text& text : widget->texts) text.call();
        }*/
        
        std::vector<vec4> new_sizes;
        new_sizes.reserve(widgets.size());
        for(auto& [key, widget] : widgets) new_sizes.push_back(vec4(widget->position, widget->size));

        bool finish = true;
        for(int i = 0; i < new_sizes.size(); ++i) {
            vec4 diff = sizes[i] - new_sizes[i];
            float d = abs(diff.x) + abs(diff.y) + abs(diff.z) + abs(diff.w);
            if(d > tolerance) finish = false;
        }

        if(finish) break;

        sizes = new_sizes;
    }

    // mesh

    for(auto& [key, widget] : widgets) {
        for(Text& text : widget->texts) {
            if(text.dirty) {
                text.dirty = false;

                text.refresh();
                text.mesh();
            }
        }
    }

    std::vector<uint64_t> roots;
    for(auto& [key, widget] : widgets) if(widget->parent == NULL_WIDGET) roots.push_back(key);

    for(uint64_t root : roots) {
        std::vector<uint64_t> path = {root};
        std::vector<uint64_t> child_ids = {0};

        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];

            vec2 children_size = vec2(0.0f);

            if(child_ids.back() == 0) {
                widget->mesh();
                vertices.insert(vertices.end(), widget->vertices_before.begin(), widget->vertices_before.end());
            }

            if (widget->children.size() <= child_ids.back()) {
                vertices.insert(vertices.end(), widget->vertices_after.begin(), widget->vertices_after.end());

                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }
    }

    for(uint64_t root : roots) {
        std::vector<uint64_t> path = {root};
        std::vector<uint64_t> child_ids = {0};
        //

        std::vector<uint64_t> widget_ids;

        path = {root};
        child_ids = {0};
        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];
            if(child_ids.back() == 0) widget_ids.push_back(path.back());

            if (widget->children.size() <= child_ids.back()) {
                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }

        for(uint64_t i : widget_ids) {
            if(widgets.contains(i)) {
                auto& widget = widgets[i];
                widget->handle_inputs();
            }
        }
    }

    for(uint64_t k : delete_buffer) widgets.erase(k);
    delete_buffer.clear();

    
    text_selected.clear();
    for(auto& [key, widget] : widgets) {
        uint32_t i = 0;
        for(Text& text : widget->texts) {
            if(text.select_range.x != -1 || text.select_range.y != -1) text_selected.push_back({widget->self, i});
            ++i;
            
            if(text.select_range.x != -1) {
                if(copy) {
                    Copy_String str;
                    str.str = text.retrieve();
                    str.y = text.position.y + text.size.y;

                    copy_strings.push_back(str);
                }
            }
        }
    }

    if(copy) {
        std::sort(copy_strings.begin(), copy_strings.end(), 
            [](const Copy_String& a, const Copy_String& b) {
                return a.y > b.y;
            }
        );

        std::string full_copy;
        int i = 0;
        for(auto& cs : copy_strings) {
            full_copy += cs.str;
            if(i != copy_strings.size() - 1) full_copy += "\n";
            ++i;
        }

        if(full_copy.size()) glfwSetClipboardString(core.window.window, full_copy.c_str());
    }

    if(text_selected.size() == 1) {
        auto& p = text_selected[0];

        auto& widget = widgets[p.first];
        Text& text = widget->texts[p.second];

        if(text.editable && text.focused) {
            std::string input = core.char_delta;
            if(paste) {
                const char* text = glfwGetClipboardString(core.window.window);
                if(text) {
                    input = text;
                }
            }

            auto wstring = convert_string(text.string);

            if(input.size()) {
                int minv = min(text.select_range.x, text.select_range.y);
                int maxv = max(text.select_range.x, text.select_range.y);
                
                if(text.select_range.x != text.select_range.y) {
                    text.string.erase(text.string.begin() + minv, text.string.begin() + maxv);
                    text.select_range.x = minv;
                    text.select_range.y = minv;
                }

                text.string.insert(text.string.begin() + minv, input.begin(), input.end());
                text.select_range += input.size();

                text.dirty = true;
            }

            if(core.pressed_buttons.contains(GLFW_KEY_BACKSPACE) || core.repeat_buttons.contains(GLFW_KEY_BACKSPACE)) {
                int minv = min(text.select_range.x, text.select_range.y);
                int maxv = max(text.select_range.x, text.select_range.y);

                if(text.select_range.x != text.select_range.y) {
                    text.string.erase(text.string.begin() + minv, text.string.begin() + maxv);
                    text.select_range.x = minv;
                    text.select_range.y = minv;

                    text.dirty = true;
                    text.start_cursor = core.current_time;
                } else {
                    if(text.select_range.x > 0) {
                        text.string.erase(text.string.begin() + (text.select_range.x - 1), text.string.begin() + text.select_range.x);
                        text.select_range -= 1;

                        text.dirty = true;
                        text.start_cursor = core.current_time;
                    }
                }
            }

            if(core.pressed_buttons.contains(GLFW_KEY_LEFT) || core.repeat_buttons.contains(GLFW_KEY_LEFT)) {
                if(core.key_map[GLFW_KEY_LEFT_SHIFT] || core.key_map[GLFW_KEY_RIGHT_SHIFT]) {
                    if(text.select_range.y > 0) {
                        --text.select_range.y;

                        text.dirty = true;
                        text.start_cursor = core.current_time;
                    }
                } else {
                    if(text.select_range.x != text.select_range.y) {
                        text.select_range.y = text.select_range.x;

                        text.dirty = true;
                        text.start_cursor = core.current_time;
                    } else if(text.select_range.x > 0) {
                        text.select_range -= 1.0f;

                        text.dirty = true;
                        text.start_cursor = core.current_time;
                    }
                }
            }
            if(core.pressed_buttons.contains(GLFW_KEY_RIGHT) || core.repeat_buttons.contains(GLFW_KEY_RIGHT)) {
                if(core.key_map[GLFW_KEY_LEFT_SHIFT] || core.key_map[GLFW_KEY_RIGHT_SHIFT]) {
                    if(text.select_range.y < wstring.size()) {
                        ++text.select_range.y;

                        text.dirty = true;
                        text.start_cursor = core.current_time;
                    }
                } else {
                    if(text.select_range.x != text.select_range.y) {
                        text.select_range.x = text.select_range.y;

                        text.dirty = true;
                        text.start_cursor = core.current_time;
                    } else if(text.select_range.x < wstring.size()) {
                        text.select_range += 1;

                        text.dirty = true;
                        text.start_cursor = core.current_time;
                    }
                }
            }
        }
    }
    
    if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
        cursor_anchor = core.cursor_pos;
    }
    
    if(text_capture != NULL_WIDGET) {
        std::set<uint64_t> c;
        uint32_t n_focused = 0;
        
        std::vector<uint64_t> path = {text_capture};
        std::vector<uint64_t> child_ids = {0};
        //

        cursor_pos = core.cursor_pos;
        
        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];

            if(child_ids.back() == 0) {
                c.insert(path.back());
            }

            if (widget->children.size() <= child_ids.back()) {
                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }

        for(auto& [key, widget] : widgets) {
            if(c.contains(key)) {
                for(Text& text : widget->texts) {
                    text.select(vec4(cursor_anchor, cursor_pos));
                    if(text.focused) ++n_focused;
                }
            } else {
                for(Text& text : widget->texts) {
                    text.select(vec4(-1));
                }
            }
        }
        
        if(n_focused) isolate_selection = true;
        else isolate_selection = false;
    }
    
    
    if(core.pressed_buttons.contains(GLFW_KEY_ENTER) || core.pressed_buttons.contains(GLFW_KEY_ESCAPE)) {
        for(auto& [key, widget] : widgets) {
            for(Text& text : widget->texts) {
                text.select(vec4(-1));
            }
        }
    }
}

//

vec4 GUI_system::get_range(uint64_t v) {
    uint64_t current = v;
    vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);

    while(true) {
        uint64_t parent = widgets[current]->parent;

        if(parent == NULL_WIDGET) break;

        vec4 r = widgets[parent]->child_region;
        //r.x = floor(r.x);
        //r.y = floor(r.y);
        //r.z = ceil(r.z);
        //r.w = ceil(r.w);
        
        range = intersect_range(range, r);
        current = parent;
    }

    return range;
}

// text

void Text::mesh() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Font& f = gui_system.fonts["default mono"];

    if(wrap) width = size.x;
    else width = 0xFFFFFFFF;

    if(select_range.x != -1) vertices_select = mesh_select(f, string, 1, width, ivec2(min(select_range.x, select_range.y), max(select_range.x, select_range.y)), line_data, alignment, false);

    vertices = mesh_text(f, string, 1, width, {-1, -1}, alignment);
}

// window

uint64_t Window_Widget::insert(std::string label, vec2 size, vec2 position, vec3 color) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Window_Widget widget;
    widget.size = size;
    widget.position = position;
    widget.label = label;
    widget.header_color = color;
    
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = PM_STATIC;

    return gui_system.insert_widget(widget, true);
}

void Window_Widget::handle_inputs() {
    bool close_window = false;

    GUI_system& gui_system = ecs.get_system<GUI_system>();

    float text_scale = 1;
    std::string label = "WINDOW";
    bool scrollbar = false;

    vec4 range = vec4(size.x - header + position.x, size.y + position.y, header, header);
    range.z += range.x;
    range.w += range.y;

    if(includes(core.cursor_pos, range)) {
        hover_close = true;
    } else hover_close = false;

    if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT) && hover_close) {
        auto children = gui_system.get_children(self);
        children.push_back(self);

        gui_system.delete_buffer.insert(gui_system.delete_buffer.end(), children.begin(), children.end());
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
    //ivec4 hover_range = {position + vec2(0, -header - size.y), position + vec2(size.x, -header)};

    buffer_range = ivec4(-resize_border, -resize_border, 0.0f, resize_border);
    range_left += buffer_range;
    buffer_range = ivec4(0.0f, -resize_border, resize_border, resize_border);
    range_right += buffer_range;
    buffer_range = ivec4(-resize_border, 0.0f, resize_border, resize_border);
    range_top += buffer_range;
    buffer_range = ivec4(-resize_border, -resize_border, resize_border, 0.0f);
    range_bottom += buffer_range;

    vec2 min_size = vec2(192, 192);

    auto resize_left = [&]() {
        position.x += core.cursor_delta.x;
        size.x -= core.cursor_delta.x;

        float delta_max = (core.cursor_pos.x) - position.x;
        position.x += delta_max;
        size.x -= delta_max;

        float delta_min = max(0.0f, min_size.x - size.x);
        size.x += delta_min;
        position.x -= delta_min;
    };

    auto resize_right = [&]() {
        size.x += core.cursor_delta.x;

        float delta_max = (position.x + size.x) - (core.cursor_pos.x);
        size.x -= delta_max;

        float delta_min = max(0.0f, min_size.x - size.x);
        size.x += delta_min;
    };

    auto resize_top = [&]() {
        float delta_min;
        size.y += core.cursor_delta.y;

        float delta_max = (position.y + size.y + header) - (core.cursor_pos.y);
        size.y -= delta_max;

        delta_min = max(0.0f, min_size.y - size.y);
        size.y += delta_min; 
    };

    auto resize_bottom = [&]() {
        float delta_min;
        position.y += core.cursor_delta.y;
        size.y -= core.cursor_delta.y;
        
        float delta_max = (core.cursor_pos.y) - position.y;
        position.y += delta_max;
        size.y -= delta_max;

        delta_min = max(0.0f, min_size.y - size.y);
        position.y -= delta_min;  
        size.y += delta_min;
    };

    if(gui_system.click_capture == self) {
        switch(operation) {
            case 0:
                position += core.cursor_delta;
                dirty = true;
                break;
            case 1:
                resize_left();
                gui_system.cursor_mode = CURSOR_DRAG_L;
                dirty = true;

                break;
            case 2:
                resize_right();
                gui_system.cursor_mode = CURSOR_DRAG_R;
                dirty = true;

                break;
            case 3:
                resize_bottom();
                gui_system.cursor_mode = CURSOR_DRAG_B;
                dirty = true;

                break;
            case 4:
                resize_left();
                resize_bottom();
                gui_system.cursor_mode = CURSOR_DRAG_BL;
                dirty = true;

                break;
            case 5:
                resize_right();
                resize_bottom();
                gui_system.cursor_mode = CURSOR_DRAG_BR;
                dirty = true;

                break;
            case 6:
                resize_top();
                gui_system.cursor_mode = CURSOR_DRAG_T;
                dirty = true;

                break;
            case 7:
                resize_left();
                resize_top();
                gui_system.cursor_mode = CURSOR_DRAG_TL;
                dirty = true;

                break;
            case 8:
                resize_right();
                resize_top();
                gui_system.cursor_mode = CURSOR_DRAG_TR;
                dirty = true;

                break;
        }
    }

    if(gui_system.click_capture != self) operation = 0;

    if(gui_system.click_capture == NULL_WIDGET && gui_system.hover_capture == self || (operation == NULL_OPERATION && gui_system.click_capture == self)) {
        uint32_t op = NULL_OPERATION;

        bool left_cont = includes(core.cursor_pos, range_left);
        bool right_cont = includes(core.cursor_pos, range_right);
        bool top_cont = includes(core.cursor_pos, range_top);
        bool bottom_cont = includes(core.cursor_pos, range_bottom);
        
        if(left_cont && bottom_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_BL;
            op = 4;
        } else if(right_cont && bottom_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_BR;
            op = 5;
        } else if(left_cont && top_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_TL;
            op = 7;
        } else if(right_cont && top_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_TR;
            op = 8;
        } else if(left_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_L;
            op = 1;
        } else if(right_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_R;
            op = 2;
        } else if(bottom_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_B;
            op = 3;
        } else if(top_cont) {
            gui_system.cursor_mode = CURSOR_DRAG_T;
            op = 6;
        }
        
        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
            if(op != NULL_OPERATION) {
                operation = op;
            } else {
                if(includes(core.cursor_pos, range_move)) {
                    operation = 0;
                } else {
                    operation = NULL_OPERATION;
                }
                
            }
        }
    }
}

void Window_Widget::mesh() {
    if(dirty) {
        GUI_system& gui_system = ecs.get_system<GUI_system>();

        float text_scale = 1;
        bool scrollbar = false;
        float shadow_width = 6;

        //

        vec4 header_range = vec4(position + vec2(0.0f, size.y), position + vec2(size.x, size.y + header));

        std::vector<UI_vertex> total_ret;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        // panel
        std::vector<UI_vertex> ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * size, z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.25f, 0.25f, 0.25f, 1.0f);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // header
        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0, size.y) + v.pos.xy() * vec2(size.x, header), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(header_color, 1.0f);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // label
        ret = mesh_text(gui_system.fonts["default mono"], label, 1);

        for(UI_vertex& v : ret) {
            float s = floor(header * 0.5f - 11.0f * float(text_scale) * 0.5f);
            v.pos = vec3(position + v.pos.xy() * float(text_scale) + vec2(s, -s - 11.0f * float(text_scale) + size.y + header), z);
            v.range = header_range;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        vec4 range = vec4(59, 9, 64, 14);

        // close button
        
        vec3 hover_color = clamp(header_color + 0.15f, 0.0f, 1.0f);
        vec3 col;
        
        // close
        vec4 r = vec4(size.x - header + position.x, size.y + position.y, header, header);
        vec2 nsize = vec2(10);
        vec4 texture_range = vec4(14, 54, 10, 10);
        
        col = hover_close ? hover_color : header_color;
        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);
            //v.range = header_range;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
            //v.range = header_range;
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
        range = {position + vec2(-shadow_width, 0.0), position + vec2(0.0, size.y + header)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // top left
        ret = {a, b, c, b, d, c};
        ret[0].color.w = 0.0f;
        ret[2].color.w = 0.0f;
        ret[4].color.w = 0.0f;
        ret[5].color.w = 0.0f;
        range = {position + vec2(-shadow_width, size.y + header), position + vec2(0.0f, shadow_width + size.y + header)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());
        
        // bottom left
        ret = {a, b, d, a, d, c};
        ret[0].color.w = 0.0f;
        ret[1].color.w = 0.0f;
        ret[3].color.w = 0.0f;
        ret[5].color.w = 0.0f;
        range = {position + vec2(-shadow_width, -shadow_width), position + vec2(0.0f, 0.0f)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // right
        ret = {a, b, d, a, d, c};
        ret[1].color.w = 0.0f;
        ret[2].color.w = 0.0f;
        ret[4].color.w = 0.0f;
        range = {position + vec2(size.x, 0.0f), position + vec2(size.x + shadow_width, size.y + header)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // top right
        ret = {a, b, d, a, d, c};
        ret[1].color.w = 0.0f;
        ret[2].color.w = 0.0f;
        ret[4].color.w = 0.0f;
        ret[5].color.w = 0.0f;
        range = {position + vec2(size.x, size.y + header), position + vec2(size.x + shadow_width, shadow_width + size.y + header)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());
        

        //bottom right
        ret = {a, b, c, b, d, c};
        ret[0].color.w = 0.0f;
        ret[1].color.w = 0.0f;
        ret[3].color.w = 0.0f;
        ret[4].color.w = 0.0f;
        range = {position + vec2(size.x, -shadow_width), position + vec2(size.x + shadow_width, 0.0f)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // top
        ret = {a, b, d, a, d, c};
        ret[2].color.w = 0.0f;
        ret[4].color.w = 0.0f;
        ret[5].color.w = 0.0f;
        range = {position + vec2(0.0f, size.y + header), position + vec2(size.x, shadow_width + size.y + header)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        // bottom
        ret = {a, b, d, a, d, c};
        ret[0].color.w = 0.0f;
        ret[1].color.w = 0.0f;
        ret[3].color.w = 0.0f;
        range = {position + vec2(0.0f, -shadow_width), position + vec2(size.x, 0.0f)};
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * (range.zw() - range.xy()), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, v.color.w * shadow_w);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());
        
        //

        vertices_before = total_ret;

        dirty = false;
    }
}

// panel 

void Panel_Widget::handle_inputs() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    float height = size.y;

    float new_total_scrollable = 0.0f;
    if(children.size()) {
        new_total_scrollable = gui_system.widgets[children[0]]->size.y - height + gui_system.widgets[children[0]]->buffer.y + gui_system.widgets[children[0]]->buffer.w;
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
    scroll_pos = clamp(scroll_pos, min_scroll, max_scroll);
    if(max_scroll > min_scroll) {
        if(core.scroll_delta && includes(core.cursor_pos, vec4(position, position + size))) {
            dirty = true;

            float scroll_speed = 60.0f;
            scroll_pos += core.scroll_delta * scroll_speed;
            scroll_pos = clamp(scroll_pos, min_scroll, max_scroll);
        }
    }

    if(reserve) {
        bool scrollbar = false;
        float scrollbar_height;
        float scrollbar_pos;

        if(total_scrollable > 0.0f) {
            scrollbar = true;

            float visible_height = child_region.w - child_region.y;
            float total_height = total_scrollable + visible_height;

            scrollbar_height = visible_height / total_height;
            scrollbar_pos = (-child_offset.y) / total_height;

            scrollbar_height *= visible_height;
            scrollbar_pos *= visible_height;
        }

        if(scrollbar) {
            vec2 size = vec2(scroll_width, scrollbar_height);
            vec2 pos = vec2(child_region.z - scroll_width, child_region.y + scrollbar_pos);

            if(includes(core.cursor_pos, {pos, pos + size}) && core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                capture_scroll = true;

                scroll_anchor = core.cursor_pos.y - position.y - scrollbar_pos;
            }
        }

        if(gui_system.click_capture == self && capture_scroll) {
            float rel_pos = core.cursor_pos.y - position.y - scroll_anchor;

            child_offset.y = -(rel_pos / (size.y - scrollbar_height)) * total_scrollable;
            child_offset.y = clamp(child_offset.y, min_scroll, max_scroll);
        }

        if(!core.key_map[GLFW_MOUSE_BUTTON_LEFT]) capture_scroll = false;
    }
}

void Panel_Widget::mesh() {
    if(dirty) {
        dirty = false;
        
        std::vector<UI_vertex> ret;
        std::vector<UI_vertex> total_ret;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        bool scrollbar = false;
        float scrollbar_height;
        float scrollbar_pos;

        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(position + v.pos.xy() * size, z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.125f, 0.125f, 0.125f, 1.0f);
            v.data = 1;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        if(reserve) {
            ret = {a, b, d, a, d, c};
            vec4 area = vec4(position.x + size.x - scroll_width, position.y, scroll_width, size.y);

            for(UI_vertex& v : ret) {
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
            for(UI_vertex& v : ret) {
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

void Panel_Widget::on_place() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    float height = child_region.w - child_region.y;

    float new_total_scrollable = 0.0f;
    if(children.size()) new_total_scrollable = gui_system.widgets[children[0]]->size.y - height;

    if(new_total_scrollable != total_scrollable) {
        child_offset.y = child_offset.y + total_scrollable - new_total_scrollable;
        if(new_total_scrollable < 0.0f) child_offset.y = -new_total_scrollable;
        else if(child_offset.y > 0.0f) child_offset.y = 0.0f;
        total_scrollable = new_total_scrollable;
        dirty = true;
    }
}

void Panel_Widget::on_transform() {
    vec4 new_region = vec4(position, position + size);
    if(reserve) new_region.z -= scroll_width;
    //new_region.x = floor(new_region.x);
    //new_region.y = floor(new_region.y);
    //new_region.z = ceil(new_region.z);
    //new_region.y = ceil(new_region.y);
    if(child_region != new_region) {
        GUI_system& gui_system = ecs.get_system<GUI_system>();
        gui_system.make_dirty(self);
    }
    child_region = new_region;
}

uint64_t Panel_Widget::insert(float scroll_width, bool reserve) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Panel_Widget widget;
    widget.position_mode = PM_BOTTOM_LEFT;
    widget.layout_mode = LM_VOID;
    widget.size_mode = SM_FILL;

    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = FLT_MAX;
    widget.buffer = gui_system.active_buffer;
    
    widget.scroll_width = scroll_width;
    widget.reserve = reserve;

    widget.flag = true;

    widget.size = vec2(0.0f);
    widget.position = vec2(0.0f);

    return gui_system.insert_widget(widget, true);
}

// debug

void Debug_Widget::mesh() {
    if(dirty) {
        GUI_system& gui_system = ecs.get_system<GUI_system>();

        auto& pw = gui_system.widgets[parent];

        std::vector<UI_vertex> total_ret;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        vec4 range = gui_system.get_range(self);

        // panel
        std::vector<UI_vertex> ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * size, z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(color, 1.0f);
            v.data = 1;

            //v.range = range;
        }
        total_ret.insert(total_ret.end(), ret.begin(), ret.end());

        vertices_before = total_ret;
        dirty = false;
    }
}

uint64_t Debug_Widget::insert(vec2 size, float max_width, vec3 color) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Debug_Widget widget;
    widget.size = size;
    widget.color = color;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;

    widget.min_width = size.x;
    widget.max_width = max_width;
    widget.min_height = size.y;
    widget.max_height = size.y;

    if(widget.max_width > widget.min_width) widget.size_mode = SM_FILL;

    return gui_system.insert_widget(widget);
}

// column

uint64_t Column_Widget::insert(bool fill) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Column_Widget widget;
    widget.size_mode = SM_SURROUND;
    widget.layout_mode = LM_COLUMN;
    widget.buffer = gui_system.active_buffer;
    widget.position_mode = gui_system.active_position;
    widget.fill = fill;

    return gui_system.insert_widget(widget, true);
}

// row

uint64_t Row_Widget::insert(bool fill) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Row_Widget widget;
    widget.size_mode = SM_SURROUND;
    widget.layout_mode = LM_ROW;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;
    widget.fill = fill;

    widget.max_width = FLT_MAX;
    widget.min_width = 0.0f;

    return gui_system.insert_widget(widget, true);
}

// grid

uint64_t Grid_Widget::insert(uint32_t num_columns) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Grid_Widget widget;
    widget.size_mode = SM_SURROUND;
    widget.layout_mode = LM_GRID;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;

    widget.num_columns = num_columns;
 
    return gui_system.insert_widget(widget, true);
}

void Grid_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    //
    Widget_Constraint c;
    c.func = [this, gui_system]() {
        int num_rows = ceil(float(children.size()) / num_columns);

        rows.resize(num_rows, 0.0f);
        row_buffers.resize(num_rows - 1, buffer.x);
        columns.resize(num_columns, 0.0f);
        column_buffers.resize(num_columns - 1, buffer.y);
    };
    before.push_back(c);

    // constrain ratios
    c.func = [this, gui_system]() {
        for(int i = 0; i < 16; ++i) {
            // distribute space
            float available_space = size.x;

            std::vector<float> total(num_columns, 0.0f);
            std::vector<float> space(num_columns, 0.0f);
            //float total = 0.0f;

            float prev = 0.0f;
            
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];

                

                int column = i % num_columns;

                if(column != num_columns - 1) {
                    space[column] = max(space[column], c0->size.x + buffer.x);
                } else {
                    space[column] = max(space[column], c0->size.x);
                }

                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width) total[column] = max(total[column], c0->weight_width);
            }

            float total_accum = 0.0f;
            for(float t : total) total_accum += t;
            for(float f : space) available_space -= f;
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width) c0->size.x = clamp(c0->size.x + available_space * c0->weight_width / total_accum, c0->min_width, c0->max_width);
            }

            // constrain x
            
            float max_x = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];

                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width && total_accum) {
                    float x = (c0->size.x - c0->min_width) / (c0->weight_width / total_accum);
                    max_x = max(max_x, x);
                }
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width && total_accum) {
                    c0->size.x = clamp(c0->min_width + max_x * c0->weight_width / total_accum, c0->min_width, c0->max_width);
                }
            }
        }
    };
    before.push_back(c);

    // column and row derivation
    c.func = [this, gui_system]() {
        float column_target = 0.0f;
        std::fill(rows.begin(), rows.end(), 0.0f);
        std::fill(columns.begin(), columns.end(), 0.0f);

        int num_rows = ceil(float(children.size()) / num_columns);

        float prev = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];
            
            uint32_t row = floor(float(i) / num_columns);
            uint32_t column = i % num_columns;

            rows[row] = max(rows[row], c0->size.y);
            columns[column] = max(columns[column], c0->size.x);
        }

        for(int i = 0; i < num_columns - 1; ++i) {
            column_buffers[i] = buffer.x;
        }
        for(int i = 0; i < num_rows - 1; ++i) {
            row_buffers[i] = buffer.y;
        }

        float size_x = 0.0f;
        for(float f : columns) size_x += f;
        for(float f : column_buffers) size_x += f;
    };
    before.push_back(c);

    // target and weights for x and y
    c.func = [this, gui_system]() {
        std::vector<float> target_min = std::vector<float>(num_columns, 0.0f);
        std::vector<float> target_max = std::vector<float>(num_columns, 0.0f);
        std::vector<float> target_weight = std::vector<float>(num_columns, 0.0f);
        
        int num_rows = ceil(float(children.size()) / num_columns);

        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];

            ivec2 index = {i % num_columns, num_rows - floor(float(i) / num_columns) - 1};

            float buf = 0.0f;
            if(index.x != num_columns - 1) buf = buffer.x;

            if(c0->min_width != c0->max_width) {
                target_min[index.x] = max(target_min[index.x], c0->min_width + buf);
                target_max[index.x] = max(target_max[index.x], c0->max_width + buf);
                target_weight[index.x] = max(target_weight[index.x], c0->weight_width);
            } else {
                target_min[index.x] = max(target_min[index.x], c0->size.x + buf);
                target_max[index.x] = max(target_max[index.x], c0->size.x + buf);
            }
        }

        min_width = std::accumulate(target_min.begin(), target_min.end(), 0.0f);
        max_width = std::accumulate(target_max.begin(), target_max.end(), 0.0f);
        weight_width = std::accumulate(target_weight.begin(), target_weight.end(), 0.0f);

        //

        target_min = std::vector<float>(num_rows, 0.0f);
        target_max = std::vector<float>(num_rows, 0.0f);
        target_weight = std::vector<float>(num_rows, 0.0f);

        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];

            ivec2 index = {i % num_columns, num_rows - floor(float(i) / num_columns) - 1};
            
            float buf = 0.0f;
            if(index.y != num_rows - 1) buf = buffer.y;

            if(c0->min_height != c0->max_height) {
                target_min[index.y] = max(target_min[index.y], c0->min_height + buf);
                target_max[index.y] = max(target_max[index.y], c0->max_height + buf);
                target_weight[index.y] = max(target_weight[index.y], c0->weight_height);
            } else {
                target_min[index.y] = max(target_min[index.y], c0->size.y + buf);
                target_max[index.y] = max(target_max[index.y], c0->size.y + buf);
            }
        }

        min_height = std::accumulate(target_min.begin(), target_min.end(), 0.0f);
        max_height = std::accumulate(target_max.begin(), target_max.end(), 0.0f);
        weight_height = std::accumulate(target_weight.begin(), target_weight.end(), 0.0f);
    };
    before.push_back(c);

    // position self
    c.func = [this, gui_system]() {
        vec4 area = {position, size};
        vec2 true_size = vec2(0.0f, 0.0f);

        for(int i = 0; i < columns.size(); ++i) {
            true_size.x += columns[i];
            if(i != columns.size() - 1) true_size.x += column_buffers[i];
        }
        for(int i = 0; i < rows.size(); ++i) {
            true_size.y += rows[i];
            if(i != rows.size() - 1) true_size.y += row_buffers[i];
        }

        switch(position_mode) {
            case PM_BOTTOM_LEFT: {
                // do nothing
                break;
            }
            case PM_BOTTOM_CENTER: {
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_BOTTOM_RIGHT: {
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case PM_CENTER_LEFT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                break;
            }
            case PM_CENTER: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_CENTER_RIGHT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case PM_TOP_LEFT: {
                position.y = position.y + (size.y - true_size.y);
                break;
            }
            case PM_TOP_CENTER: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_TOP_RIGHT: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x);
                break;
            }
        }

        size = true_size;
    };
    before.push_back(c);

    // positioning
    c.func = [this, gui_system]() {
        vec2 pos = position;
        int num_rows = ceil(float(children.size()) / num_columns);

        for(int i = 0; i < children.size(); ++i) {
            ivec2 index = {i % num_columns, floor(float(i) / num_columns)};
            int ii = index.x + (num_rows - index.y - 1) * num_columns;

            if(index.x == 0 && index.y != 0) {
                pos.x = position.x;

                pos.y += rows[index.y];
                pos.y += row_buffers[index.y - 1];
            }
            
            vec4 range = vec4(pos, columns[index.x], rows[index.y]);

            pos.x += columns[index.x];
            if(index.x != column_buffers.size()) pos.x += column_buffers[index.x];

            if(ii < children.size()) { 
                auto& c0 = gui_system->widgets[children[ii]];
            
                // positioning

                switch(c0->position_mode) {
                    case PM_BOTTOM_LEFT: {
                        c0->position.x = range.x;
                        c0->position.y = range.y;
                        break;
                    }
                    case PM_BOTTOM_CENTER: {
                        c0->position.x = position.x + (range.z - c0->size.x) * 0.5f;
                        c0->position.y = range.y;
                        break;
                    }
                    case PM_BOTTOM_RIGHT: {
                        c0->position.x = range.x + (range.z - c0->size.x);
                        c0->position.y = range.y;
                        break;
                    }
                    case PM_CENTER_LEFT: {
                        c0->position.x = range.x;
                        c0->position.y = range.y + (range.w - c0->size.y) * 0.5f;
                        break;
                    }
                    case PM_CENTER: {
                        c0->position.x = position.x + (range.z - c0->size.x) * 0.5f;
                        c0->position.y = range.y + (range.w - c0->size.y) * 0.5f;
                        break;
                    }
                    case PM_CENTER_RIGHT: {
                        c0->position.x = range.x + (range.z - c0->size.x);
                        c0->position.y = range.y + (range.w - c0->size.y) * 0.5f;
                        break;
                    }
                    case PM_TOP_LEFT: {
                        c0->position.x = range.x;
                        c0->position.y = range.y + (range.w - c0->size.y);
                        break;
                    }
                    case PM_TOP_CENTER: {
                        c0->position.x = position.x + (range.z - c0->size.x) * 0.5f;
                        c0->position.y = range.y + (range.w - c0->size.y);
                        break;
                    }
                    case PM_TOP_RIGHT: {
                        c0->position.x = range.x + (range.z - c0->size.x);
                        c0->position.y = range.y + (range.w - c0->size.y);
                        break;
                    }
                }
            }
        }
    };
    before.push_back(c);
}

// text

std::vector<UI_vertex> Text::get_vertices() {
    std::vector<UI_vertex> ret = vertices_select;
    
    if(select_range.x == select_range.y) {
        double freq = 1.0;
        if((fmod(core.current_time - start_cursor, freq) / freq) > 0.5) {
            for(auto& vertex : ret) vertex.color.w = 0.0f;
        }
    }
    
    ret.insert(ret.end(), vertices.begin(), vertices.end());
    
    for(UI_vertex& v : ret) {
        v.pos = vec3(ceil(position) + v.pos.xy(), z);
    }

    return ret;
}

void Text::refresh() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Font& f = gui_system.fonts["default mono"];
    //return {resize_range.x, resize_range.y, height, text_x, max_width};
    
    if(wrap) width = size.x;
    else width = 0xFFFFFFFF;

    std::vector<float> ret = compute_text_bounds(f, string, 1, width, wrap, alignment);

    resize_range = {ret[0], ret[1]};
    size.y = ret[2];
    size.x = min(size.x, ret[5]);
    //size.x = width;
    //text_x = ret[3];
    
    max_width = ret[4];

    line_data = text_line_data;
}

void Text::select(vec4 cursor_range) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Font& font = gui_system.fonts["default mono"];

    ivec2 prev_select = select_range;

    vec4 abs_cursor_range = vec4(min(cursor_range.x, cursor_range.z), min(cursor_range.y, cursor_range.w), max(cursor_range.x, cursor_range.z), max(cursor_range.y, cursor_range.w));

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

std::string Text::retrieve() {
    std::u32string u32str = convert_string(string);

    int minv = min(select_range.x, select_range.y);
    int maxv = max(select_range.x, select_range.y);

    std::u32string str(u32str.begin() + minv, u32str.begin() + maxv);
    

    return convert_string(str);
}

void Text_Widget::handle_inputs() {
    texts[0].z = z;

    std::string str = texts[0].string;
    str = callback(str);

    set_str(str);
}

void Text_Widget::mesh() {
    if(dirty) {
        dirty = false;

        GUI_system& gui_system = ecs.get_system<GUI_system>();

        vec4 range = gui_system.get_range(self);

        vertices_before = texts[0].get_vertices();
    }
}

void Text_Widget::get_y() {
    texts[0].refresh();
}

void Text_Widget::set_str(std::string str) {
    if(texts[0].string != str) {
        texts[0].dirty = true;
        texts[0].string = str;
        dirty = true;

        get_y();
    }
}

uint64_t Text_Widget::insert(std::string str, ALIGNMENT alg, bool wrap, std::function<std::string(std::string)> callback) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    
    Text_Widget widget;

    Text text;

    widget.size = vec2(0.0f);
    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = 0.0f;
    widget.weight_height = 0.0f;
    widget.callback = std::move(callback);

    widget.position = vec2(0.0f);
    
    text.string = str;
    text.wrap = wrap;
    text.alignment = alg;

    widget.texts = {text};
    
    widget.size_mode = SM_FILL;
    widget.layout_mode = LM_VOID;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;

    return gui_system.insert_widget(widget);
}

// text input

void Text_Input_Widget::handle_inputs() {
    // cursor
    if(core.key_map[GLFW_MOUSE_BUTTON_LEFT] && includes(core.cursor_pos, vec4(position, position + size))) {
        click = true;
        text_dirty = true;
        click_pos = core.cursor_pos;

        auto d = compute_cursor_index(core.cursor_pos - position, ecs.get_system<GUI_system>().fonts["default mono"], text, 1, line_indices, alignment);
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
                    GUI_system& gui_system = ecs.get_system<GUI_system>();
                    Font& f = gui_system.fonts["default mono"];
                    std::u32string str = convert_string(text);
                    
                    bool ws = true;
                    uint32_t start_cursor = cursor;
                    while(true) {
                        if(cursor == 0) break;
                        uint32_t current = str[cursor - 1];
                        
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
                    GUI_system& gui_system = ecs.get_system<GUI_system>();
                    Font& f = gui_system.fonts["default mono"];
                    std::u32string str = convert_string(text);
                    
                    bool ws = true;
                    uint32_t start_cursor = cursor;
                    while(true) {
                        if(cursor == text.size()) break;
                        uint32_t current = str[cursor];
                        
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

void Text_Input_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;
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

void Text_Input_Widget::mesh() {
    if(text_dirty) {
        text_dirty = false;

        GUI_system& gui_system = ecs.get_system<GUI_system>();
        Font& f = gui_system.fonts["default mono"];
        get_y();

        click = false;
        
        ivec2 range;
        if(selection_anchor == 0xFFFFFFFF || selection_anchor == cursor) range = {-1, -1};
        else {
            if(selection_anchor > cursor) range = {cursor, selection_anchor};
            else range = {selection_anchor, cursor};
        }
        
        text_vertices = mesh_text(f, text, text_size, size.x, range, alignment, false);
        if(cursor != 0xFFFFFFFF) cursor_pos = compute_cursor_pos(cursor, wraparound, ecs.get_system<GUI_system>().fonts["default mono"], text, 1, line_indices, alignment);
    }

    if(dirty) {
        GUI_system& gui_system = ecs.get_system<GUI_system>();
        Font& f = gui_system.fonts["default mono"];

        dirty = false;

        vec4 range = gui_system.get_range(self);

        vertices_before = text_vertices;

        for(UI_vertex& v : vertices_before) {
            v.pos = vec3(round(position) + v.pos.xy(), z);
        }

        if(cursor != 0xFFFFFFFF) {
            std::vector<UI_vertex> total_ret;

            UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
            UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
            UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
            UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
            
            // panel
            std::vector<UI_vertex> ret = {a, b, d, a, d, c};
            for(UI_vertex& v : ret) {
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

void Text_Input_Widget::insert_cursor() {

}

void Text_Input_Widget::get_y() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Font& f = gui_system.fonts["default mono"];

    std::vector<float> ret = compute_text_bounds(f, text, text_size, size.x, true, alignment);
    line_indices = text_line_data;

    resize_range = {ret[0], ret[1]};
    size.y = ret[2];
    text_x = ret[3];

    max_width = ret[4];
}

void Text_Input_Widget::set_str(std::string str) {
    if(text != str) {
        text_dirty = true;
        dirty = true;
        get_y();
    }
    text = str;
}

uint64_t Text_Input_Widget::insert(std::string str, ALIGNMENT alg) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    
    Text_Input_Widget widget;

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

uint64_t Split_Widget::insert(LAYOUT_MODE layout, std::vector<Panel_Constraint> constraints_) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Split_Widget widget;
    widget.size_mode = SM_SURROUND;
    widget.layout_mode = layout;
    widget.position_mode = PM_BOTTOM_LEFT;
    widget.sep = vec2(1.0f);
    widget.constraints = constraints_;
    
    return gui_system.insert_widget(widget, true);
}

void Split_Widget::handle_inputs() {
    if(core.window.minimized) {
        return;
    }

    process_size();

    GUI_system& gui_system = ecs.get_system<GUI_system>();

    if(gui_system.hover_capture == self || gui_system.click_capture == self) {
        if(layout_mode == LM_ROW) {
            gui_system.cursor_mode = CURSOR_DRAG_L;
        } else if(layout_mode == LM_COLUMN) {
            gui_system.cursor_mode = CURSOR_DRAG_B;
        }
    }
    
    float buffer = 4.0f;

    vec4 buffer_range;
    if(layout_mode == LM_ROW) buffer_range = ivec4(-buffer, 0.0f, buffer, 0.0f);
    else if(layout_mode == LM_COLUMN) buffer_range = ivec4(0.0f, -buffer, 0.0f, buffer);

    if(gui_system.click_capture != self) operation = 0;

    if(gui_system.click_capture == self) {
        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
            for(int i = 0; i < children.size() - 1; ++i) {
                int i0 = i;
                int i1 = i + 1;


                if(layout_mode == LM_ROW) {
                    auto& w0 = gui_system.widgets[children[i0]];
                    auto& w1 = gui_system.widgets[children[i1]];
                    
                    vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x, w1->position.y + w1->size.y);

                    range += buffer_range;

                    if(includes(core.cursor_pos, range)) {
                        gui_system.cursor_mode = CURSOR_DRAG_L;

                        operation = i;
                    }
                } else if(layout_mode == LM_COLUMN) {
                    auto& w0 = gui_system.widgets[children[i1]];
                    auto& w1 = gui_system.widgets[children[i0]];
                    
                    vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x + w1->size.x, w1->position.y);

                    range += buffer_range;

                    if(includes(core.cursor_pos, range)) {
                        gui_system.cursor_mode = CURSOR_DRAG_B;
                        
                        operation = i;
                    }
                }
            }
            float mw = 20.0f;
        }

        //
        
        if(layout_mode == LM_ROW) {
            gui_system.cursor_mode = CURSOR_DRAG_L;

            int i = operation;

            Panel_Constraint& c0 = constraints[i];
            Panel_Constraint& c1 = constraints[i + 1];

            auto& w0 = gui_system.widgets[children[i]];
            auto& w1 = gui_system.widgets[children[i + 1]];

            float s = size.x;
            float borders = (children.size() - 1) * sep.x;
            s -= borders;
            
            float p = position.x;
            float c = core.cursor_pos.x;
            
            c = clamp(c, w0->position.x + w0->min_width, w1->position.x + w1->size.x - w1->min_width - sep.x);

            float r = (c - w0->position.x);
            float new_ratio = r / s;

            float difference = c0.value - new_ratio;
            c0.value -= difference;
            c1.value += difference;
        } else if(layout_mode == LM_COLUMN) {
            gui_system.cursor_mode = CURSOR_DRAG_B;

            int i = operation;

            Panel_Constraint& c0 = constraints[i + 1];
            Panel_Constraint& c1 = constraints[i];

            auto& w0 = gui_system.widgets[children[i + 1]];
            auto& w1 = gui_system.widgets[children[i]];

            float s = size.y;
            float borders = (children.size() - 1) * sep.y;
            s -= borders;
            
            float p = position.y;
            float c = core.cursor_pos.y;
            
            c = clamp(c, w0->position.y + w0->min_height, w1->position.y + w1->size.y - w1->min_height - sep.y);

            float r = (c - w0->position.y);
            float new_ratio = r / s;

            float difference = c0.value - new_ratio;
            c0.value -= difference;
            c1.value += difference;
        }
    }

    // handle inputs
    recalibrate();
}

void Split_Widget::process_size() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    min_width = 0;
    min_height = 0;

    if(layout_mode == LM_ROW) {
        for(int i = 0; i < children.size(); ++i) {
            auto& child = gui_system.widgets[children[i]];
            
            if(child->flag) {
                child->min_width = 20;
                child->min_height = 20;
            }

            if(layout_mode == LM_COLUMN) {
                min_width = child->min_width;
                min_height += child->min_height + ((i == 0) ? 0 : sep.y);
            } else if(layout_mode == LM_ROW) {
                min_width += child->min_width + ((i == 0) ? 0 : sep.x);
                min_height = child->min_height;
            }
        }
        
        if(prev_size != size.x && prev_size >= 0.0f) {
            float prev_asize = prev_size - (children.size() - 1) * sep.x;
            float new_asize = size.x - (children.size() - 1) * sep.x;
             
            float change = 0.0f;
            std::set<uint32_t> locked;

            for(int i = 0; i < children.size(); ++i) {
                Panel_Constraint& constraint = constraints[i];

                if(constraint.panel_mode == PANEL_MODE_SIZE) {
                    float new_frac = constraint.value * prev_asize / new_asize;
                    
                    float delta = new_frac - constraint.value;
                    
                    constraint.value += delta;

                    change += delta;

                    locked.insert(i);
                }
            }

            int num = children.size() - locked.size();

            if(num != 0) {
                for(int i = 0; i < children.size(); ++i) {
                    if(locked.find(i) == locked.end()) {
                        Panel_Constraint& constraint = constraints[i];
                        constraint.value -= change / num;
                    }
                }
            }
        }

        prev_size = size.x;
    } else if(layout_mode == LM_COLUMN) {
        for(int i = 0; i < children.size(); ++i) {
            auto& child = gui_system.widgets[children[children.size() - i - 1]];
            
            if(child->flag) {
                child->min_width = 20;
                child->min_height = 20;
            }

            if(layout_mode == LM_COLUMN) {
                min_width = child->min_width;
                min_height += child->min_height + ((i == 0) ? 0 : sep.y);
            } else if(layout_mode == LM_ROW) {
                min_width += child->min_width + ((i == 0) ? 0 : sep.x);
                min_height = child->min_height;
            }
        }

        if(prev_size != size.y && prev_size >= 0.0f) {
            float prev_asize = prev_size - (children.size() - 1) * sep.y;
            float new_asize = size.y - (children.size() - 1) * sep.y;
             
            float change = 0.0f;
            std::set<uint32_t> locked;

            for(int i = 0; i < children.size(); ++i) {
                Panel_Constraint& constraint = constraints[children.size() - i - 1];

                if(constraint.panel_mode == PANEL_MODE_SIZE) {
                    float new_frac = constraint.value * prev_asize / new_asize;
                    
                    float delta = new_frac - constraint.value;

                    constraint.value += delta;

                    change += delta;

                    locked.insert(i);
                }
            }

            int num = children.size() - locked.size();

            if(num != 0) {
                for(int i = 0; i < children.size(); ++i) {
                    if(locked.find(i) == locked.end()) {
                        Panel_Constraint& constraint = constraints[children.size() - i - 1];
                        constraint.value -= change / num;
                    }
                }
            }
        }
        
        prev_size = size.y;
    }
}

void Split_Widget::recalibrate() {
    float mw = 20.0f;
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    if(layout_mode == LM_ROW) {
        float total_area = size.x;

        float borders = (children.size() - 1) * sep.x;
        total_area -= borders;
        
        float change = 1.0f;
        std::set<uint32_t> locked;

        while(change != 0.0f) {    
            change = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& child = gui_system.widgets[children[i]];
                Panel_Constraint& constraint = constraints[i];
                
                float new_size = max(constraint.value, child->min_width / total_area);
                if(constraint.value <= child->min_width / total_area) {
                    float diff = new_size - constraint.value;
                    constraint.value += diff;
                    locked.insert(i);

                    change += diff;
                }
            }

            int num = children.size() - locked.size();

            if(num == 0) break;
            
            for(int i = 0; i < children.size(); ++i) {
                if(locked.find(i) == locked.end()) {
                    Panel_Constraint& constraint = constraints[i];
                    constraint.value -= change / num;
                }
            }
        }

        //

        float total = 0.0f;

        for(Panel_Constraint& constraint : constraints) {
            total += constraint.value;
        }
        for(Panel_Constraint& constraint : constraints) constraint.value /= total;
    } else if(layout_mode == LM_COLUMN) {
        float total_area = size.y;

        float borders = (children.size() - 1) * sep.y;
        total_area -= borders;

        float change = 1.0f;
        std::set<uint32_t> locked;

        while(change != 0.0f) {    
            change = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& child = gui_system.widgets[children[children.size() - i - 1]];
                Panel_Constraint& constraint = constraints[children.size() - i - 1];
                
                float new_size = max(constraint.value, child->min_height / total_area);
                if(constraint.value <= child->min_height / total_area) {
                    float diff = new_size - constraint.value;
                    constraint.value += diff;
                    locked.insert(i);

                    change += diff;
                }
            }

            int num = children.size() - locked.size();

            if(num == 0) break;
            
            for(int i = 0; i < children.size(); ++i) {
                if(locked.find(i) == locked.end()) {
                    Panel_Constraint& constraint = constraints[children.size() - i - 1];
                    constraint.value -= change / num;
                }
            }
        }
        
        //

        float total = 0.0f;

        for(Panel_Constraint& constraint : constraints) {
            total += constraint.value;
        }
        for(Panel_Constraint& constraint : constraints) constraint.value /= total;
    }
}

void Column_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    //
    Widget_Constraint c;
    c.func = [this, gui_system]() {
        rows.resize(children.size(), 0.0f);
        row_buffers.resize(children.size() - 1, 0.0f);
        column = 0.0f;
    };
    before.push_back(c);

    // constrain ratios
    c.func = [this, gui_system]() {
        for(int i = 0; i < 16; ++i) {
            // distribute space
            float available_space = size.y;
            float total = 0.0f;

            float prev = 0.0f;
            
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];

                available_space -= c0->size.y;
                if(i != children.size() - 1) {
                    float max_buffer = max(c0->buffer.y, prev);
                    available_space -= max_buffer;
                }
                prev = c0->buffer.w;

                if(c0->min_height != c0->max_height) total += c0->weight_height;
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];
                if(c0->min_height != c0->max_height) c0->size.y = clamp(c0->size.y + available_space * c0->weight_height / total, c0->min_height, c0->max_height);
            }

            // constrain x
            
            float max_x = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];

                if(c0->min_height != c0->max_height) {
                    float x = (c0->size.y - c0->min_height) / (c0->weight_height / total);
                    max_x = max(max_x, x);
                }
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];
                if(c0->min_height != c0->max_height) {
                    c0->size.y = clamp(c0->min_height + max_x * c0->weight_height / total, c0->min_height, c0->max_height);
                }
            }
        }
    };
    before.push_back(c);

    // distribute x
    c.func = [this, gui_system]() {
        float available_space = size.x;
        
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];

            if(c0->weight_width != 0.0f) {
                float C = c0->size.x - clamp(available_space, c0->min_width, c0->max_width);
                c0->size.x -= C;
            }
        }
    };
    before.push_back(c);

    // column and row derivation
    c.func = [this, gui_system]() {
        float column_target = 0.0f;

        float prev = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];

            float C0 = rows[i] - c0->size.y;
            rows[i] -= C0;

            if(i != children.size() - 1) {
                float max_buffer = max(c0->buffer.y, prev);
                row_buffers[i] = max_buffer;
            }
            prev = c0->buffer.w;

            column_target = max(column_target, c0->size.x);
        }
        
        if(fill) {
            column = size.x;
        } else {
            column = column_target;
        }
    };
    after.push_back(c);

    // target and weights for x and y
    c.func = [this, gui_system]() {
        float target_min = 0.0f;
        float target_max = 0.0f;
        float target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];

            if(c0->min_height != c0->max_height) {
                target_min += c0->min_height;
                target_max += c0->max_height;
                target_weight += c0->weight_height;
            } else {
                target_min += c0->size.y;
                target_max += c0->size.y;
            }
        }

        min_height = target_min;
        max_height = target_max;
        weight_height = target_weight;

        //

        target_min = 0.0f;
        target_max = 0.0f;
        target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];

            if(c0->min_width != c0->max_width) {
                target_min = max(target_min, c0->min_width);
                target_max = max(target_max, c0->max_width);
                target_weight = max(target_weight, c0->weight_width);
            } else {
                target_min = max(target_min, c0->size.x);
                target_max = max(target_max, c0->size.x);
            }
        }

        min_width = target_min;
        max_width = target_max;
        weight_width = target_weight;
    };
    after.push_back(c);

    // position self
    c.func = [this, gui_system]() {
        vec4 area = {position, size};
        vec2 true_size = vec2(column, 0.0f);

        for(int i = 0; i < rows.size(); ++i) {
            true_size.y += rows[i];
            if(i != rows.size() - 1) true_size.y += row_buffers[i];
        }

        switch(position_mode) {
            case PM_BOTTOM_LEFT: {
                // do nothing
                break;
            }
            case PM_BOTTOM_CENTER: {
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_BOTTOM_RIGHT: {
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case PM_CENTER_LEFT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                break;
            }
            case PM_CENTER: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_CENTER_RIGHT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case PM_TOP_LEFT: {
                position.y = position.y + (size.y - true_size.y);
                break;
            }
            case PM_TOP_CENTER: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_TOP_RIGHT: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x);
                break;
            }
        }

        size = true_size;
    };
    after.push_back(c);

    // positioning
    c.func = [this, gui_system]() {
        float y_pos = position.y;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[(int)children.size() - i - 1]];

            // y positioning
            c0->position.y = y_pos;
            
            y_pos += rows[i];
            if(i != children.size() - 1) y_pos += row_buffers[i];

            // x positioning
            switch(c0->position_mode) {
                case PM_BOTTOM_LEFT:
                case PM_CENTER_LEFT:
                case PM_TOP_LEFT: {
                    c0->position.x = position.x;

                    break;
                }
                case PM_BOTTOM_CENTER:
                case PM_CENTER:
                case PM_TOP_CENTER: {
                    c0->position.x = position.x + (column - c0->size.x) * 0.5f;

                    break;
                }
                case PM_BOTTOM_RIGHT:
                case PM_CENTER_RIGHT:
                case PM_TOP_RIGHT: {
                    c0->position.x = position.x + (column - c0->size.x);

                    break;
                }
            }
        }
    };
    after.push_back(c);
}


void Row_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    columns.resize(children.size(), 0.0f);
    row = 0.0f;

    //
    Widget_Constraint c;
    c.func = [this, gui_system]() {
        columns.resize(children.size(), 0.0f);
        column_buffers.resize(children.size() - 1, 0.0f);
        row = 0.0f;
    };
    before.push_back(c);

    // constrain ratios
    c.func = [this, gui_system]() {
        for(int i = 0; i < 16; ++i) {
            // distribute space
            float available_space = size.x;
            float total = 0.0f;

            float prev = 0.0f;
            
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];

                available_space -= c0->size.x;
                if(i != children.size() - 1) {
                    float max_buffer = max(c0->buffer.x, prev);
                    available_space -= max_buffer;
                }
                prev = c0->buffer.z;

                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width) total += c0->weight_width;
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width) c0->size.x = clamp(c0->size.x + available_space * c0->weight_width / total, c0->min_width, c0->max_width);
            }

            // constrain x
            
            float max_x = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];

                if(c0->min_width != c0->max_width) {
                    float x = (c0->size.x - c0->min_width) / (c0->weight_width / total);
                    max_x = max(max_x, x);
                }
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = gui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width) {
                    c0->size.x = clamp(c0->min_width + max_x * c0->weight_width / total, c0->min_width, c0->max_width);
                }
            }
        }
    };
    before.push_back(c);

    // distribute y
    c.func = [this, gui_system]() {
        float available_space = size.y;
        
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];

            if(c0->weight_height != 0.0f) {
                c0->size.y = clamp(available_space, c0->min_height, c0->max_height);
            }
        }
    };
    before.push_back(c);

    // column and row derivation
    c.func = [this, gui_system]() {
        float row_target = 0.0f;

        float prev = 0.0f;

        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];

            columns[i] = c0->size.x;

            if(i != children.size() - 1) column_buffers[i] = max(c0->buffer.x, prev);
            prev = c0->buffer.z;

            row_target = max(row_target, c0->size.y);
        
        }

        if(fill) {
            row = size.y;
        } else {
            row = row_target;
        }
    };
    before.push_back(c);

    // position self
    c.func = [this, gui_system]() {
        vec4 area = {position, size};
        vec2 true_size = vec2(0.0f, row);

        for(int i = 0; i < columns.size(); ++i) {
            true_size.x += columns[i];
            
            if(i != children.size() - 1) true_size.x += column_buffers[i];
        }

        switch(position_mode) {
            case PM_BOTTOM_LEFT: {
                // do nothing
                break;
            }
            case PM_BOTTOM_CENTER: {
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_BOTTOM_RIGHT: {
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case PM_CENTER_LEFT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                break;
            }
            case PM_CENTER: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_CENTER_RIGHT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case PM_TOP_LEFT: {
                position.y = position.y + (size.y - true_size.y);
                break;
            }
            case PM_TOP_CENTER: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case PM_TOP_RIGHT: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x);
                break;
            }
        }
    };
    before.push_back(c);

    // target and weights for x and y
    c.func = [this, gui_system]() {
        float target_min = 0.0f;
        float target_max = 0.0f;
        float target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];

            if(c0->min_width != c0->max_width) {
                target_min += c0->min_width;
                target_max += c0->max_width;
                target_weight += c0->weight_width;
            } else {
                target_min += c0->size.x;
                target_max += c0->size.x;
            }
        }

        min_width = max(min_width, target_min);
        max_width = min(max_width, target_max);
        
        if(weight_width == 2.0f || flag) {
            flag = true;
        }
        weight_width = target_weight;


        //

        target_min = 0.0f;
        target_max = 0.0f;
        target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];

            if(c0->min_height != c0->max_height && c0->weight_height != 0) {
                target_min = max(target_min, c0->min_height);
                target_max = max(target_max, c0->max_height);
                target_weight = max(target_weight, c0->weight_height);
            } else {
                target_min = max(target_min, c0->size.y);
                target_max = max(target_max, c0->size.y);
            }
        }

        min_height = target_min;
        max_height = target_max;
        weight_height = target_weight;

        if(weight_height == 0.0f) size.y = min_height;
    };
    before.push_back(c);

    // positioning
    c.func = [this, gui_system]() {
        float x_pos = position.x;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = gui_system->widgets[children[i]];

            // y positioning
            c0->position.x = x_pos;
            
            x_pos += columns[i];
            if(i != children.size() - 1) x_pos += column_buffers[i];

            // y positioning
            // x positioning
            switch(c0->position_mode) {
                case PM_BOTTOM_LEFT:
                case PM_BOTTOM_CENTER:
                case PM_BOTTOM_RIGHT: {
                    c0->position.y = position.y;

                    break;
                }
                case PM_CENTER_LEFT:
                case PM_CENTER:
                case PM_CENTER_RIGHT: {
                    c0->position.y = position.y + (row - c0->size.y) * 0.5f;

                    break;
                }

                case PM_TOP_LEFT:
                case PM_TOP_CENTER:
                case PM_TOP_RIGHT: {
                    c0->position.y = position.y + (row - c0->size.y);

                    break;
                }
            }
            
        }
    };
    before.push_back(c);
}

/*
*/

void Split_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;

    c.func = [this, gui_system]() {
        process_size();

        float s = 0.0f;
        if(layout_mode == LM_COLUMN) s = size.y - sep.y * (constraints.size() - 1);
        else if(layout_mode == LM_ROW) s = size.x - sep.x * (constraints.size() - 1);

        for(int i = 0; i < 16; ++i) {
            float change = 0.0f;
            uint32_t num = constraints.size();
            float total = 0.0f;

            for(int i = 0; i < constraints.size(); ++i) {
                Panel_Constraint& panel_constraint = constraints[i];
                auto& p0 = gui_system->widgets[children[i]];

                float minimum;
                if(layout_mode == LM_COLUMN) minimum = p0->min_height;
                else if(layout_mode == LM_ROW) minimum = p0->min_width;
                    
                float v = panel_constraint.value * s;
                if(v <= minimum) {
                    --num;
                }
                float c = (v - max(v, minimum)) / s;
                change += c;
                panel_constraint.value -= c;
                total += panel_constraint.value;
            }

            
            for(int i = 0; i < constraints.size(); ++i) {
                Panel_Constraint& panel_constraint = constraints[i];
                panel_constraint.value /= total;
            }
        }
    };
    before.push_back(c);

    if(layout_mode == LM_COLUMN) {
        c.func = [this, gui_system]() {
            for(int i = 0; i < 16; ++i) {
                for(int j = children.size() - 1; j >= 0; --j) {
                    int jj = j;

                    auto& p0 = gui_system->widgets[children[jj]];
                    Panel_Constraint& panel_constraint = constraints[jj];

                    if(jj == children.size() - 1) {
                        float C = p0->position.y - position.y;
                        p0->position.y -= C;
                    } else {
                        auto& pp = gui_system->widgets[children[jj + 1]];

                        float C = p0->position.y - (pp->position.y + pp->size.y + sep.y);
                        p0->position.y -= C;
                    }

                    float C0 = p0->size.y - (size.y - sep.y * (children.size() - 1)) * panel_constraint.value;
                    p0->size.y -= C0;
                    
                    float C1 = p0->size.x - size.x;
                    p0->size.x -= C1;

                    float C = p0->position.x - position.x;
                    p0->position.x -= C;
                }
            }
        };
        before.push_back(c);
    } else if(layout_mode == LM_ROW) {
        c.func = [this, gui_system]() {
            for(int i = 0; i < 16; ++i) {
                for(int j = 0; j < children.size(); ++j) {
                    auto& p0 = gui_system->widgets[children[j]];
                    Panel_Constraint& panel_constraint = constraints[j];

                    if(j == 0) {
                        float C = p0->position.x - position.x;
                        p0->position.x -= C;
                    } else {
                        auto& pp = gui_system->widgets[children[j - 1]];

                        float C = p0->position.x - (pp->position.x + pp->size.x + sep.x);
                        p0->position.x -= C;
                    }
                    
                    float C = p0->position.y - position.y;
                    p0->position.y -= C;
                    
                    float C0 = p0->size.x - (size.x - sep.x * (children.size() - 1)) * panel_constraint.value;
                    p0->size.x -= C0;
                    
                    float C1 = p0->size.y - size.y;
                    p0->size.y -= C1;
                }
            }
        };
        before.push_back(c);
    }
}

void Window_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;
    c.func = [this, gui_system]() {
        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = gui_system->widgets[children[i]];

            p0->size.y = size.y;
            p0->size.x = size.x;
            
            p0->position.x = position.x;
            p0->position.y = position.y;
        }
    };
    before.push_back(c);
}

void Text_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;
    c.func = [this, gui_system]() {
        texts[0].click_range = ivec4(position, position + size);
        if(size.x < texts[0].resize_range.x || size.x > texts[0].resize_range.y || size.x == 0.0f) {
            texts[0].size.x = size.x;
            texts[0].dirty = true;
            
            get_y();

            size.y = texts[0].size.y;
        }

        float f = texts[0].resize_range.y;
        if(f == FLT_MAX) {
            f = texts[0].resize_range.x;
        }
        size.x = clamp(size.x, texts[0].resize_range.x, f);
        size.y = texts[0].size.y;
        
        max_width = texts[0].max_width;
    };

    before.push_back(c);
    
    c.func = [this, gui_system]() {
        position = round(position);

        texts[0].position = position;
    };
    after.push_back(c);
}

void Screen_Widget::handle_inputs() {
    vec4 region;

    if(glfwGetWindowMonitor(core.window.window) != nullptr) fullscreen = true;
    else fullscreen = false;

    auto& gui_system = ecs.get_system<GUI_system>();

    if(gui_system.hover_capture == self && (gui_system.click_capture == NULL_WIDGET || gui_system.click_capture == self)) {
        region = vec4(size.x - header, size.y - header, header, header);
        region.z += region.x;
        region.w += region.y;
        if(includes(core.cursor_pos, region)) {
            hover_close = true;
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                // close
                glfwSetWindowShouldClose(core.window.window, GLFW_TRUE);
            }
        } else hover_close = false;

        region = vec4(size.x - header * 2.0f, size.y - header, header, header);
        region.z += region.x;
        region.w += region.y;
        if(includes(core.cursor_pos, region)) {
            hover_maximize = true;
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                // maximize/demaximize
                if(fullscreen) {
                    glfwSetWindowMonitor(core.window.window, nullptr,  core.window.prev_pos.x, core.window.prev_pos.y, core.window.prev_pos.z, core.window.prev_pos.w, 0 );
                    fullscreen = false;
                    core.window.fullscreen = false;
                    glfwRestoreWindow(core.window.window);
                } else if(glfwGetWindowAttrib(core.window.window, GLFW_MAXIMIZED)) {
                    glfwRestoreWindow(core.window.window);
                } else { 
                    glfwMaximizeWindow(core.window.window);
                }
            }
        } else hover_maximize = false;

        region = vec4(size.x - header * 3.0f, size.y - header, header, header);
        region.z += region.x;
        region.w += region.y;
        if(includes(core.cursor_pos, region)) {
            hover_minimize = true;
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                // minimize
                if(fullscreen) {
                    glfwSetWindowMonitor(core.window.window, nullptr,  core.window.prev_pos.x, core.window.prev_pos.y, core.window.prev_pos.z, core.window.prev_pos.w, 0 );
                    fullscreen = false;
                    core.window.fullscreen = false;
                }
                glfwIconifyWindow(core.window.window);
            }
        } else hover_minimize = false;
    }
}

void Screen_Widget::mesh() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();
    float text_scale = 1.0f;
    vec3 color = color_editor;
    vec3 background_color = vec3(0.5f);

    vertices_before.clear();

    UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
    std::vector<UI_vertex> ret;

    if(fullscreen || true) {
        // title bar
        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
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
        for(UI_vertex& v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
            //v.range = header_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // text
        /*
        ret = mesh_text(gui_system->fonts["default mono"], label, 1);
        vec2 origin = vec2(header, size.y - header);
        float height = gui_system->fonts["default mono"].line_height;
        origin += (header - height) * 0.5f;
        
        for(UI_vertex& v : ret) {
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
        for(UI_vertex& v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);
            //v.range = header_range;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
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
        if(glfwGetWindowAttrib(core.window.window, GLFW_MAXIMIZED) || fullscreen) texture_range = vec4(14, 34, 10, 10);
        
        col = hover_maximize ? hover_color : color;
        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
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
        for(UI_vertex& v : ret) {
            v.pos = vec3(r.xy() + v.pos.xy() * r.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.data = 1;
            v.color = vec4(col, 1.0f);
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3((r.xy() + (r.zw() - nsize) * 0.5f) + v.pos.xy() * nsize, z);
            v.tex_pos = v.tex_pos * texture_range.zw() + texture_range.xy();
            v.data = 1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        
        // panel
        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * vec2(size.x, size.y - header), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(background_color, 1.0f);
            v.data = 1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
    } else {
        // panel
        ret = {a, b, d, a, d, c};
        for(UI_vertex& v : ret) {
            v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * vec2(size.x, size.y), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(background_color, 1.0f);
            v.data = 1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
    }
}

void Screen_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;
    c.func = [this, gui_system]() {
        position = vec2(0.0f);
        if(!core.window.minimized) size = core.window.screen_size;
    };
    before.push_back(c);

    c.func = [this, gui_system]() {
        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = gui_system->widgets[children[i]];

            if(fullscreen || true) p0->size.y = size.y - header;
            else p0->size.y = size.y;
            
            p0->size.x = size.x;
            
            p0->position.x = position.x;

            p0->position.y = position.y;
        }
    };
    before.push_back(c);
}

uint64_t Screen_Widget::insert() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Screen_Widget widget;
    widget.flag = true;
    widget.label = name;
    
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = PM_STATIC;

    return gui_system.insert_widget(widget, true);
}


void Render_Widget::mesh() {
    UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

    // panel
    std::vector<UI_vertex> ret = {a, b, d, a, d, c};
    for(UI_vertex& v : ret) {
        v.pos = vec3(position + vec2(0.0f, 0.0f) + v.pos.xy() * size, z);
        v.tex_pos = v.tex_pos;
        v.color = vec4(1.0);//vec4(0.25f, 0.25f, 0.25f, 1.0f);
        v.data = 0x3;
    }
    vertices_before = ret;
}

void Render_Widget::init() {
    /*
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;
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

    Widget_Constraint c;
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

uint64_t Render_Widget::insert() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Render_Widget widget;
    widget.flag = true;
    widget.target = 0;
    
    widget.buffer = gui_system.active_buffer;
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = PM_STATIC;

    return gui_system.insert_widget(widget);
}

void Render_Widget::handle_inputs() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Input_system& input_system = ecs.get_system<Input_system>();
    Render_system& render_system = ecs.get_system<Render_system>();

    render_system.targets[target].target_size = size;
    input_system.view_range = ivec4(position, size);

    if(gui_system.hover_capture == self && (gui_system.click_capture == NULL_WIDGET || gui_system.click_capture == self)) input_system.gui_captured = true;
    else input_system.gui_captured = false;
}

void Panel_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;
    c.func = [this, gui_system]() {
        float scrollable = 0.0f;
        if(children.size()) {
            scrollable = gui_system->widgets[children[0]]->size.y + gui_system->widgets[children[0]]->buffer.y + gui_system->widgets[children[0]]->buffer.w - size.y;
        }

        scroll_pos = clamp(scroll_pos, -scrollable, 0.0f);

        //
        
        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = gui_system->widgets[children[i]];

            p0->size.x = size.x - (p0->buffer.x + p0->buffer.z);

            p0->size.y = size.y - (p0->buffer.y + p0->buffer.w);
            
            p0->position.x = position.x + p0->buffer.x;

            p0->position.y = position.y + p0->buffer.y - scroll_pos;
        }
    };
    before.push_back(c);
}

void Button_Widget::handle_inputs() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    pressed = false;

    if(includes(core.cursor_pos, vec4(position, position + size)) && gui_system.hover_capture == self && (gui_system.click_capture == NULL_WIDGET || gui_system.click_capture == self)) {
        hovered = true;
        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
            pressed = true;
            held = true;
        }
    } else {
        hovered = false;
    }

    if(!core.key_map[GLFW_MOUSE_BUTTON_LEFT]) held = false;
    
    callback(*this);
}

/*
float border = 2.0f;
std::vector<vec4> ranges = {
    vec4(0.0f, 0.0f, border, border),
    vec4(border, 0.0f, size.x - border * 2.0f, border),
    vec4(size.x - border, 0.0f, border, border),
    vec4(0.0f, border, border, size.y - border * 2.0f),
    vec4(border, border, size.x - border * 2.0f, size.y - border * 2.0f),
    vec4(size.x - border, border, border, size.y - border * 2.0f),
    vec4(0.0f, size.y - border, border, border),
    vec4(border, size.y - border, size.x - border * 2.0f, border),
    vec4(size.x - border, size.y - border, border, border),
};
std::vector<vec4> colors = {
    base_color,
    base_color * 0.75f,
    base_color * 0.75f,
    base_color * 1.33f,
    base_color,
    base_color * 0.75f,
    base_color * 1.33f,
    base_color * 1.33f,
    base_color,
};

if(pressed) {
    colors = {
        base_color,
        base_color * 1.33f,
        base_color * 1.33f,
        base_color * 0.75f,
        base_color,
        base_color * 1.33f,
        base_color * 0.75f,
        base_color * 0.75f,
        base_color,
    };
}
*/

void Button_Widget::mesh() {
    if(text_dirty) {
        text_dirty = false;

        GUI_system& gui_system = ecs.get_system<GUI_system>();
        Font& f = gui_system.fonts["default mono"];

        text_vertices = mesh_text(f, label, 1, 1000000.0f, {-1, -1}, ALIGNMENT_CENTER, false);
        text_size = text_range;
    }

    if(dirty) {
        dirty = false;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        vec4 base_color = vec4(color, 1.0f);
        if(hovered) base_color = vec4(color + 0.25f, 1.0f);
        if(held) base_color = vec4(color * 0.75f, 1.0f);

        // panel
        float border = 3.0f;
        std::vector<vec4> ranges = {
            vec4(0.0f, 0.0f, size.x, size.y),
        };
        std::vector<vec4> colors = {
            vec4(base_color.xyz(), 1.0f),
        };
        vec2 text_pos = position + (size - text_size) * 0.5f;
        
        vertices_before.clear();

        for(int i = 0; i < ranges.size(); ++i) {
            std::vector<UI_vertex> ret = {a, b, d, a, d, c};
            vec4 range = ranges[i];
            vec4 color = colors[i];

            for(UI_vertex& v : ret) {
                v.pos = vec3(position + range.xy() + v.pos.xy() * range.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = color;
                v.data = 0x1;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }

        if(icon.x != 0.0f) {
            std::vector<UI_vertex> ret = {a, b, d, a, d, c};
            vec2 pos = position + (size - icon_size) * 0.5f;
            pos = round(pos);

            for(UI_vertex& v : ret) {
                v.pos = vec3(pos + v.pos.xy() * icon_size, z);
                v.tex_pos = v.tex_pos * icon.zw() + icon.xy();
                v.color = vec4(1.0f);
                v.data = 0x1;
            }

            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }

        std::vector<UI_vertex> vs = text_vertices;
        for(UI_vertex& v : vs) {
            v.pos += vec3(round(text_pos), 0.0f);
        }
        vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
    }
}

void Button_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();
}

uint64_t Button_Widget::insert(vec2 size, vec3 color, std::string str, std::function<void(Button_Widget&)> callback) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Button_Widget widget;
    
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.buffer = gui_system.active_buffer;
    widget.position_mode = gui_system.active_position;
    widget.callback = callback;
    widget.label = str;
    widget.color = color;

    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;

    return gui_system.insert_widget(widget);
}

void Slider_Widget::handle_inputs() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    
    if(gui_system.hover_capture == self && gui_system.click_capture == NULL_WIDGET || gui_system.click_capture == self) {
        gui_system.cursor_mode = CURSOR_DRAG_L;
        hovered = true;
    } else hovered = false;
    
    if(gui_system.click_capture == self) {
        pressed = true;

        float p = core.cursor_pos.x - position.x - slider_width * 0.5f;

        float frac = p / (size.x - slider_width);
        frac = clamp(frac, 0.0f, 1.0f);

        float value = frac * (range.y - range.x);
        if(step != 0.0f) value = round(value / step) * step;
        value += range.x;

        current_value = value;
    }
    
    callback(*this);
}

void Slider_Widget::mesh() {
    if(text_dirty) {
        text_dirty = false;

        GUI_system& gui_system = ecs.get_system<GUI_system>();
        Font& f = gui_system.fonts["default mono"];

        text_vertices = mesh_text(f, label, 1, 0xFFFFFFFF, {-1, -1}, ALIGNMENT_CENTER, false);
        text_size = text_range;
    }

    if(dirty) {
        dirty = false;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        float slider_pos = (size.x - slider_width) * ((current_value - range.x) / (range.y - range.x));
        
        vec4 base_color = vec4(color, 1.0f);
        if(hovered || pressed) base_color = vec4(color + 0.25f, 1.0f);
        // panel
        float border = 3.0f;
        std::vector<vec4> ranges = {
            vec4(0.0f, border, size.x, size.y - border * 2.0f),
            vec4(slider_pos, 0.0f, slider_width, size.y),
        };
        std::vector<vec4> colors = {
            vec4(0.0f, 0.0f, 0.0f, 0.5f),
            base_color,
        };
        vec2 text_pos = position + (size - text_size) * 0.5f;

        //
        
        vertices_before.clear();

        for(int i = 0; i < ranges.size(); ++i) {
            std::vector<UI_vertex> ret = {a, b, d, a, d, c};
            vec4 range = ranges[i];
            vec4 color = colors[i];

            for(UI_vertex& v : ret) {
                v.pos = vec3(position + range.xy() + v.pos.xy() * range.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = color;
                v.data = 0x1;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }

        std::vector<UI_vertex> vs = text_vertices;
        for(UI_vertex& v : vs) {
            v.pos += vec3(round(text_pos), 0.0f);
        }
        vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
    }
}

void Slider_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();
}

uint64_t Slider_Widget::insert(vec2 size, float slider_width, vec3 color, vec2 range, float step, float start, std::string str, std::function<void(Slider_Widget&)> callback)  {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Slider_Widget widget;
    
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.buffer = gui_system.active_buffer;
    widget.position_mode = gui_system.active_position;
    widget.callback = callback;
    widget.label = str;
    widget.color = color;

    widget.range = range;
    widget.step = step;
    widget.current_value = start;
    widget.slider_width = slider_width;

    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;

    return gui_system.insert_widget(widget);
}

void Tab_Widget::handle_inputs() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    if(gui_system.click_capture == self) {
        bool t = false;

        float pos = 0.0f;
        uint32_t i = 0;
        for(Tab& tab : tabs) {
            vec4 region = vec4(position + vec2(pos, size.y - tab_height), tab.width, tab_height);

            if(includes(core.cursor_pos, vec4(region.xy(), region.xy() + region.zw()))) {
                //hovered = true;
                if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                    if(selected != i) t = true;
                    selected = i;
                }
            } else {
                //hovered = false;
            }

            pos += tab.width + tab_sep;
            ++i;
        }

        if(t) {
            gui_system.erase(gui_system.get_children(self));
            children.clear();
            gui_system.current_widget = self;
            tabs[selected].swap();
        }
    }
}

void Tab_Widget::mesh() {
    for(Tab& tab : tabs) {
        if(tab.text_dirty) {
            tab.text_dirty = false;

            GUI_system& gui_system = ecs.get_system<GUI_system>();
            Font& f = gui_system.fonts["default mono"];

            tab.text_vertices = mesh_text(f, tab.label, 1, 0xFFFFFFFF, {-1, -1}, ALIGNMENT_CENTER, false);
            tab.text_size = text_range;
        }
    }

    if(dirty) {
        vertices_before.clear();

        GUI_system& gui_system = ecs.get_system<GUI_system>();
        
        dirty = false;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        
        vec3 color;
        float pos = 0.0f;
        uint32_t i = 0;
        for(Tab& tab : tabs) {
            vec4 base_color = vec4(tab.color, 1.0f);
            if(i == selected) color = tab.color;
            else base_color = vec4(tab.color * ((i % 2 == 0) ? 0.75f : 0.65f), 1.0f);
        
            std::vector<vec4> ranges = {
                vec4(0.0f, 0.0f, tab.width, tab_height),
            };
            std::vector<vec4> colors = {
                base_color,
            };
            vec2 text_pos = position + vec2(pos, size.y - tab_height) + (vec2(tab.width, tab_height) - tab.text_size) * 0.5f;

            for(int i = 0; i < ranges.size(); ++i) {
                std::vector<UI_vertex> ret = {a, b, d, a, d, c};
                vec4 range = ranges[i];
                vec4 color = colors[i];

                for(UI_vertex& v : ret) {
                    v.pos = vec3(position + vec2(pos, size.y - tab_height) + range.xy() + v.pos.xy() * range.zw(), z);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = color;
                    v.data = 0x1;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            }
            
            std::vector<UI_vertex> vs = tab.text_vertices;
            for(UI_vertex& v : vs) {
                v.pos += vec3(round(text_pos), 0.0f);
            }
            vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());

            pos += tab.width + tab_sep;
            ++i;
        }

        std::vector<UI_vertex> ret = {a, b, d, a, d, c};
        vec4 range = vec4(0.0f, 0.0f, size.x, size.y - tab_height);
        vec4 col = vec4(color, 1.0f);

        for(UI_vertex& v : ret) {
            v.pos = vec3(position + range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = col;
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
    }
}

void Tab_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    Widget_Constraint c;
    c.func = [this, gui_system]() {
        vec4 range = vec4(position, size - vec2(0.0f, tab_height));

        for(int i = 0; i < children.size(); ++i) {
            auto& p0 = gui_system->widgets[children[i]];

            p0->size.x = range.z - p0->buffer.x - p0->buffer.z;

            p0->size.y = range.w - p0->buffer.y - p0->buffer.w;
            
            p0->position.x = range.x - p0->buffer.x;

            p0->position.y = range.y - p0->buffer.y;
        }
    };
    before.push_back(c);
}

uint64_t Tab_Widget::insert(float tab_height, float tab_sep, std::vector<Tab> tabs) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Tab_Widget widget;
    
    widget.buffer = gui_system.active_buffer;
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = gui_system.active_position;
    widget.tabs = tabs;
    widget.tab_height = tab_height;
    widget.tab_sep = tab_sep;

    uint32_t selected = widget.selected;

    uint64_t w = gui_system.insert_widget(widget);

    uint64_t prev_cw = gui_system.current_widget;
    auto prev_pos = gui_system.active_position;
    vec4 prev_buffer = gui_system.active_buffer;
    
    gui_system.current_widget = w;
    ((Tab_Widget*)gui_system.widgets[w].get())->tabs[selected].swap();
    gui_system.current_widget = prev_cw;
    gui_system.active_position = prev_pos;
    gui_system.active_buffer= prev_buffer;

    return w;
}

std::vector<uint64_t> GUI_system::get_children(uint64_t root) {
    std::vector<uint64_t> keys;
    
    std::vector<uint64_t> path;
    std::vector<uint64_t> child_ids;

    path = {root};
    child_ids = {0};
    while(true) {
        if (path.size() == 0) break;

        auto& widget = widgets[path.back()];

        vec2 children_size = vec2(0.0f);

        if (widget->children.size() <= child_ids.back()) {
            if(widget->self != root) keys.push_back(widget->self);

            // go up
            path.pop_back();
            child_ids.pop_back();
        } else {
            path.push_back(widget->children[child_ids.back()]);

            ++child_ids.back();
            child_ids.push_back(0);
        }
    }

    return keys;
}

void GUI_system::erase(std::vector<uint64_t> ws) {
    for(uint64_t key : ws) {
        widgets.erase(key);
    }
}

void Drop_Widget::handle_inputs() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    float height = min(drop_unit_height * (options.size() - 1), drop_height);

    if(gui_system.hover_capture == self && (gui_system.click_capture == NULL_WIDGET || gui_system.click_capture == self)) {
        if(drop_down) {
            float content_height = drop_unit_height * (options.size() - 1);
            float panel_height = min(content_height, drop_height);
            
            float offset = -panel_height;
            if(drop_direction) offset = size.y;
            
            vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);
            range = {range.xy(), range.xy() + range.zw()};
            
            if(includes(core.cursor_pos, range)) {
                if(content_height > panel_height) {
                    float scroll_region = content_height - panel_height;

                    if(core.scroll_delta) {
                        float scroll_speed = 20.0f;
                        scroll -= core.scroll_delta * scroll_speed;
                        
                        scroll = clamp(scroll, 0.0f, scroll_region);
                        dirty = true;
                    }
                }

                //
                
                float origin = position.y + (panel_height + offset) - drop_unit_height + scroll;

                uint32_t rel = -floor((core.cursor_pos.y - origin) / drop_unit_height);

                if(hovered != rel) dirty = true;
                hovered = rel;
                
                if(hovered >= selected) ++hovered;
            } else {
                if(hovered != 0xFFFFFFFF) dirty = true;
                hovered = 0xFFFFFFFF;
            }
        }
    }
    
    if(gui_system.click_capture == self) {
        if(drop_down) {
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                if(hovered != 0xFFFFFFFF) {
                    selected = hovered;
                    drop_down = false;
                }
            }
        } else {
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                vec4 range = vec4(position.x + size.x - button_width, position.y, button_width, size.y);
                range = {range.xy(), range.xy() + range.zw()};

                if(includes(core.cursor_pos, range)) {
                    if(position.y - height < 0.0f) drop_direction = true;
                    else drop_direction = false;

                    drop_down = true;
                    dirty = true;
                }
            }
        }
    }

    if(gui_system.click_capture != NULL_WIDGET && gui_system.click_capture != self && drop_down) {
        drop_down = false;
    }
    
    callback(*this);
}

void Drop_Widget::mesh() {
    for(Drop_Option& option : options) {
        if(option.dirty) {
            option.dirty = false;

            GUI_system& gui_system = ecs.get_system<GUI_system>();
            Font& f = gui_system.fonts["default mono"];

            option.vertices = mesh_text(f, option.label, 1, 0xFFFFFFFF, {-1, -1}, ALIGNMENT_CENTER, false);
            option.size = text_range;
        }
    }

    if(dirty) {
        GUI_system& gui_system = ecs.get_system<GUI_system>();
    
        vertices_before.clear();
        
        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        std::vector<UI_vertex> ret = {a, b, d, a, d, c};

        vec4 range = vec4(position, size);
        vec4 col = vec4(color, 1.0f);

        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = col;
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // current option text

        Drop_Option option = options[selected];
        vec2 origin = position + vec2(4.0f, (size.y - option.size.y) * 0.5f);
        origin = floor(origin);

        std::vector<UI_vertex> vs = option.vertices;
        for(auto& vertex : vs) {
            vertex.pos += vec3(origin, z);
        }
        vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());

        // panel for button

        ret = {a, b, d, a, d, c};

        range = vec4(position.x + size.x - button_width, position.y, button_width, size.y);
        col = vec4(color + 0.25f, 1.0f);

        for(UI_vertex& v : ret) {
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

        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = tex_range.xy() + v.tex_pos * tex_range.zw();
            v.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        if(drop_down) {
            float content_height = drop_unit_height * (options.size() - 1);
            float panel_height = min(drop_unit_height * (options.size() - 1), drop_height);

            float offset = -panel_height;
            if(drop_direction) offset = size.y;

            std::vector<UI_vertex> ret = {a, b, d, a, d, c};
            vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);
            

            vec4 col = vec4(color * 0.875f, 1.0f);

            vec4 panel_range = {range.xy(), range.xy() + range.zw()};

            for(UI_vertex& v : ret) {
                v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z + 0.25f);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = col;
                v.data = 0x1;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

            // hovered

            if(hovered != 0xFFFFFFFF) {
                std::vector<UI_vertex> ret = {a, b, d, a, d, c};

                uint32_t h = hovered;
                if(h >= selected) --h;

                vec4 range = vec4(position.x, position.y + (panel_height + offset) - (drop_unit_height * (float(h) + 1.0f)) + scroll, size.x, drop_unit_height);
                vec4 col = vec4(color, 1.0f);

                for(UI_vertex& v : ret) {
                    v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z + 0.25f);
                    v.tex_pos = vec2(1.0f, 63.0f);
                    v.color = col;
                    v.data = 0x1;
                    v.range = panel_range;
                }
                vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
            }

            // all options
            float pos = -drop_unit_height + scroll + (panel_height + offset);
            for(int i = 0; i < options.size(); ++i) {
                if(i == selected) continue;

                Drop_Option option = options[i];
                vec2 origin = position + vec2(4.0f, (size.y - option.size.y) * 0.5f + pos);
                pos -= drop_unit_height;
                origin = floor(origin);

                std::vector<UI_vertex> vs = option.vertices;
                for(auto& vertex : vs) {
                    vertex.pos += vec3(origin, z + 0.25f);
                    vertex.range = panel_range;
                }
                vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
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

                for(UI_vertex& v : ret) {
                    v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z + 0.25f);
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

void Drop_Widget::init() {

}

uint64_t Drop_Widget::insert(vec2 size, vec3 color, float w, float h, float h2, std::vector<std::string> options, uint32_t selected, std::function<void(Drop_Widget&)> callback) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Drop_Widget widget;
    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;
    widget.color = color;

    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;

    widget.drop_height = h2;
    widget.drop_unit_height = h;
    widget.button_width = w;
    widget.selected = selected;

    widget.callback = callback;

    widget.z = 0.25f;

    //

    std::vector<Drop_Option> option_structs;
    for(std::string o : options) {
        Drop_Option option;
        option.label = o;
        
        option_structs.push_back(option);
    }

    widget.options = option_structs;

    return gui_system.insert_widget(widget);
}


uint64_t Spacer_Widget::insert(vec2 min_size, vec2 max_size, bool visual) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Spacer_Widget widget;
    widget.min_width = min_size.x;
    widget.max_width = max_size.x;
    widget.min_height = min_size.y;
    widget.max_height = max_size.y;
    widget.weight_width = 0.001f;
    widget.weight_height = 0.001f;

    widget.visual = visual;

    if(widget.min_width == widget.max_width) widget.size.x = widget.min_width;
    if(widget.min_height == widget.max_height) widget.size.y = widget.min_height;
    
    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;

    return gui_system.insert_widget(widget);
}

void Spacer_Widget::mesh() {
    if(dirty) {
        vertices_before.clear();
        dirty = false;

        if(visual) {
            UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
            UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
            UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
            UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
            std::vector<UI_vertex> ret = {a, b, d, a, d, c};

            vec4 range = vec4(position.x, position.y + size.y * 0.5f, size.x, 1.0f);
            range = round(range);
            
            for(UI_vertex& v : ret) {
                v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
                v.tex_pos = vec2(1.0f, 63.0f);
                v.color = vec4(1.0f, 1.0f, 1.0f, 0.5f);
                v.data = 0x1;
            }
            vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());
        }
    }
}

void GUI_system::set_attrib(vec2 min_size, vec2 max_size, vec2 weights) {
    auto& widget = widgets[last_widget];

    if(min_size.x >= 0.0f) widget->min_width = min_size.x;
    if(max_size.x >= 0.0f) widget->max_width = max_size.x;

    if(min_size.y >= 0.0f) widget->min_height = min_size.y;
    if(max_size.y >= 0.0f) widget->max_height = max_size.y;

    if(weights.x >= 0.0f) widget->weight_width = weights.x;
    if(weights.y >= 0.0f) widget->weight_height = weights.y;

    if(widget->min_width == widget->max_width) widget->size.x = widget->min_width;
    if(widget->min_height == widget->max_height) widget->size.y = widget->min_height;
}

void Relative_Widget::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    //
    Widget_Constraint c;
    c.func = [this, gui_system]() {
        callback(*this);
    };
    after.push_back(c);
}

uint64_t Relative_Widget::insert(std::function<void(Relative_Widget&)> callback = [](Relative_Widget& self) {}) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Relative_Widget widget;
    widget.callback = callback;
    widget.weight_width = 0.0f;
    widget.weight_height = 0.0f;

    return gui_system.insert_widget(widget, true);
}

void Menu_Widget::handle_inputs() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Menu_Node* rootn = root.get();
    for(uint32_t p : path) {
        rootn = &rootn->children[p];
    }

    float height = min(drop_unit_height * rootn->children.size(), drop_height);

    //

    float content_height = drop_unit_height * rootn->children.size();
    float panel_height = min(content_height, drop_height);
    
    float offset = 0.0f;
    
    vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);
    range = {range.xy(), range.xy() + range.zw()};
    
    if(includes(core.cursor_pos, range) && gui_system.hover_capture == self && (gui_system.click_capture == NULL_WIDGET || gui_system.click_capture == self)) {
        if(content_height > panel_height) {
            float scroll_region = content_height - panel_height;

            if(core.scroll_delta) {
                float scroll_speed = 20.0f;
                scroll -= core.scroll_delta * scroll_speed;
                
                scroll = clamp(scroll, 0.0f, scroll_region);
                dirty = true;
            }
        }

        //
        
        float origin = position.y + (panel_height + offset) + scroll;

        uint32_t rel = -floor((core.cursor_pos.y - origin) / drop_unit_height + 1);

        if(hovered != rel) dirty = true;
        hovered = rel;

        if(hovered != index && index != 0xFFFFFFFF) {
            timer += core.delta_time;
        } else {
            timer = 0.0;
        }

        if(timer > 0.5 && children.size()) {
            gui_system.delete_buffer.push_back(children[0]);
            children.clear();
            index = 0xFFFFFFFF;
        }

        if(rootn->children[hovered].children.size()) {
            //if((widget.pressed || (pressed_siblings && widget.hovered)) && w == NULL_WIDGET) {
            if(children.size() && index != hovered) {
                gui_system.delete_buffer.push_back(children[0]);
                children.clear();
            }

            if(children.size() == 0) {
                vec2 pos = vec2(position.x + button_width, origin - drop_unit_height * (hovered));

                auto new_path = path;
                new_path.push_back(hovered);

                ecs.get_system<GUI_system>().current_widget = self;
                int max_size = 64;

                vec3 child_color = color_editor;
                if(color == child_color) child_color *= 0.9f;

                Menu_Widget::insert(pos + vec2(0.0f, -min(float(rootn->children[hovered].children.size()) * drop_unit_height, (float)max_size)), z, child_color, 160, 16, max_size, root, new_path);
                index = hovered;
            }

            /*
            if(w != NULL_WIDGET) {
                widget.pressed = true;
            }

            bool hover_self = widget.hovered || w != NULL_WIDGET && includes(core.cursor_pos, vec4(gui_system.widgets[w]->position, gui_system.widgets[w]->position + gui_system.widgets[w]->size));
            
            if(!hover_self && core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT) || hovered_siblings) {
                widget.children.clear();
                ecs.get_system<GUI_system>().delete_buffer.push_back(w);
                w = NULL_WIDGET;
            }
            */
        }

        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
            clicked = hovered;
            if(!rootn->children[clicked].children.size()) {
                gui_system.delete_buffer.push_back(self);
            }

            rootn->children[clicked].callback();
        }
    } else {
        if(hovered != 0xFFFFFFFF) dirty = true;
        hovered = 0xFFFFFFFF;

        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
            std::vector<uint64_t> parent_children;

            uint64_t current = self;
            while(true) {
                parent_children.push_back(current);

                auto& w = gui_system.widgets[current];
                if(w->children.size()) {
                    current = w->children[0];
                } else break;
            }

            current = self;
            while(true) {
                parent_children.push_back(current);

                auto& w = gui_system.widgets[current];
                if(w->parent != NULL_WIDGET) {
                    if(Menu_Widget* menu = dynamic_cast<Menu_Widget*>(gui_system.widgets[w->parent].get())) {
                        current = w->parent;
                    } else break;
                } else break;
            }

            bool erase = true;
            for(uint64_t w : parent_children) {
                auto& ww = gui_system.widgets[w];
                Menu_Widget* menu = dynamic_cast<Menu_Widget*>(ww.get());

                if(menu->hovered != 0xFFFFFFFF) {
                    erase = false;
                    break;
                }
            }

            if(erase) {
                gui_system.delete_buffer.push_back(self);
            }
        }
    }
    
    //callback(*this);
}

void Menu_Widget::mesh() {
    Menu_Node* rootn = root.get();
    for(uint32_t p : path) {
        rootn = &rootn->children[p];
    }
        
    UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
    UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
    UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
    UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

    std::vector<std::pair<std::vector<UI_vertex>, vec2>> label_vertices;
    label_vertices.reserve(rootn->children.size());

    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Font& f = gui_system.fonts["default mono"];
    for(auto& child : rootn->children) {
        std::pair<std::vector<UI_vertex>, vec2> p;

        p.first = mesh_text(f, child.label, 1, 0xFFFFFFFF, {-1, -1}, ALIGNMENT_CENTER, false);
        p.second = text_range;

        label_vertices.push_back(p);
    }

    z = 0.5f;

    if(dirty) {

        GUI_system& gui_system = ecs.get_system<GUI_system>();
    
        vertices_before.clear();
        
        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};

        //
        float content_height = drop_unit_height * rootn->children.size();
        float panel_height = min(drop_unit_height * rootn->children.size(), drop_height);

        float offset = 0.0f;

        std::vector<UI_vertex> ret = {a, b, d, a, d, c};
        vec4 range = vec4(position.x, position.y + offset, size.x, panel_height);
        

        vec4 col = vec4(color * 0.875f, 1.0f);

        vec4 panel_range = {range.xy(), range.xy() + range.zw()};

        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = col;
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        // hovered

        if(hovered != 0xFFFFFFFF) {
            std::vector<UI_vertex> ret = {a, b, d, a, d, c};

            uint32_t h = hovered;

            vec4 range = vec4(position.x, position.y + panel_height + offset - (drop_unit_height * (float(h) + 1.0f)) + scroll, size.x, drop_unit_height);
            vec4 col = vec4(color, 1.0f);

            for(UI_vertex& v : ret) {
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
            vec2 origin = position + vec2(4.0f, panel_height + scroll - (drop_unit_height * float(i + 1.0f)) + 2.0f);
            origin = round(origin);

            std::vector<UI_vertex> vs = label_vertices[i].first;
            for(auto& vertex : vs) {
                vertex.pos += vec3(origin, z);
                vertex.range = panel_range;
            }
            vertices_before.insert(vertices_before.end(), vs.begin(), vs.end());
            
            if(rootn->children[i].children.size()) {
                std::vector<UI_vertex> vs = {a, b, d, a, d, c};
                vec2 pos = position + (vec2(size.x - drop_unit_height, panel_height + scroll - (drop_unit_height * float(i + 1.0f))) + (drop_unit_height - 8.0f) * 0.5f);

                for(UI_vertex& v : vs) {
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

            for(UI_vertex& v : ret) {
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

void Menu_Widget::init() {

}

uint64_t Menu_Widget::insert(vec2 position, float z, vec3 color, float w, float h, float h2, std::shared_ptr<Menu_Node> root, std::vector<uint32_t> path) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    Menu_Widget widget;

    widget.position = position;
    widget.z = z;
    
    Menu_Node* rootn = root.get();
    for(uint32_t p : path) {
        rootn = &rootn->children[p];
    }

    vec2 size = vec2(w, min(h * rootn->children.size(), h2));
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
    
    return gui_system.insert_widget(widget);
}

// capture data
Capture_Data capture_data;

bool Window_Widget::handle_capture() {
    capture_data.z = z;
    capture_data.text_capture = false;

    std::vector<vec4> ranges = {
        vec4(position - (float)resize_border, position + vec2(size.x, size.y + header) + (float)resize_border)
    };

    for(vec4 range : ranges) {
        if(includes(core.cursor_pos, range)) return true;
    }

    return false;
}

bool Drop_Widget::handle_capture() {
    capture_data.z = z;
    capture_data.text_capture = false;

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(core.cursor_pos, range)) return true;
    }
    
    if(drop_down) {
        float height = min(drop_height, drop_unit_height * (options.size() - 1));
        ranges = {
            vec4(position + vec2(0.0f, -height), vec2(position.x + size.x, position.y))
        };
        capture_data.z = z + 0.001f;
        
        for(vec4 range : ranges) {
            if(includes(core.cursor_pos, range)) return true;
        }
    }

    return false;
}

bool Panel_Widget::handle_capture() {
    capture_data.z = z;
    capture_data.text_capture = true;

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(core.cursor_pos, range)) return true;
    }

    return false;
}

bool Screen_Widget::handle_capture() {
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

bool Menu_Widget::handle_capture() {
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

bool Render_Widget::handle_capture() {
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

bool Button_Widget::handle_capture() {
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

bool Split_Widget::handle_capture() {
    capture_data.z = z + 0.001f;
    capture_data.text_capture = false;

    GUI_system& gui_system = ecs.get_system<GUI_system>();
    
    float buffer = 4.0f;
    
    vec4 buffer_range;
    if(layout_mode == LM_ROW) buffer_range = ivec4(-buffer, 0.0f, buffer, 0.0f);
    else if(layout_mode == LM_COLUMN) buffer_range = ivec4(0.0f, -buffer, 0.0f, buffer);

    for(int i = 0; i < children.size() - 1; ++i) {
        int i0 = i;
        int i1 = i + 1;


        if(layout_mode == LM_ROW) {
            auto& w0 = gui_system.widgets[children[i0]];
            auto& w1 = gui_system.widgets[children[i1]];
            
            vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x, w1->position.y + w1->size.y);

            range += buffer_range;

            if(includes(core.cursor_pos, range)) {
                return true;
            }
        } else if(layout_mode == LM_COLUMN) {
            auto& w0 = gui_system.widgets[children[i1]];
            auto& w1 = gui_system.widgets[children[i0]];
            
            vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x + w1->size.x, w1->position.y);

            range += buffer_range;

            if(includes(core.cursor_pos, range)) {
                return true;
            }
        }
    }

    return false;
}

bool Slider_Widget::handle_capture() {
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

bool Tab_Widget::handle_capture() {
    capture_data.z = z;
    capture_data.text_capture = false;

    float pos = 0.0f;
    for(Tab& tab : tabs) {
        vec4 range = vec4(position + vec2(pos, size.y - tab_height), tab.width, tab_height);

        if(includes(core.cursor_pos, vec4(range.xy(), range.xy() + range.zw()))) {
            return true;
        }

        pos += tab.width + tab_sep;
    }

    return false;
}

void GUI_system::handle_capture() {
    Capture_Data capture;
    capture.z = 0.0f;
    Capture_Data capture_text;
    capture_text.z = 0.0f;

    uint64_t w = NULL_WIDGET;
    uint64_t wt = NULL_WIDGET;

    std::vector<uint64_t> roots;
    for(auto& [key, widget] : widgets) {
        bool is_capture = widget->handle_capture();

        if(is_capture) {
            if(capture_data.z >= capture.z) {
                w = key;
                capture = capture_data;
            }

            if(capture_data.text_capture) {
                if(capture_data.z >= capture_text.z) {
                    wt = key;
                    capture_text = capture_data;
                }
            }
        }
    }
    
    hover_capture = w;

    if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
        click_capture = hover_capture;
        text_capture = wt;
    }
    if(!core.key_map[GLFW_MOUSE_BUTTON_LEFT]) {
        click_capture = NULL_WIDGET;
        text_capture = NULL_WIDGET;
    }
}