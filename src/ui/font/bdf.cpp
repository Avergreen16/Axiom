#include <ui/font/bdf.hpp>

namespace axiom {

enum class bdf_region{BDF_NULL, BDF_HEADER, BDF_GLYPH};

void bdf_font::process(std::string filepath, font_handler* handler) {
    bool bitmap = false;

    phase_range = {1, 1};

    font_asset font;

    text_asset text = text_asset::load(filepath);

    ivec2 size;
    ivec2 offset;
    int line_ascent = 0;
    int line_descent = 0;

    std::stringstream ss(text.data);

    bdf_region region = axiom::bdf_region::BDF_NULL;

    uint encoding;
    glyph_data glyph;
    std::vector<uint> keys;

    while(true) {
        std::string line;
        std::getline(ss, line);

        std::stringstream ss_line(line);
        std::string name;
        ss_line >> name;

        if(region == axiom::bdf_region::BDF_HEADER) {
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
        } else if(region == axiom::bdf_region::BDF_GLYPH) {
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
                region = axiom::bdf_region::BDF_NULL;
                bitmap = false;
                collect_bits = false;

                keys.push_back(encoding);
                font.glyph_map.emplace(encoding, glyph);
            }

            if(collect_bits) {
                for(int i = 0; i < name.size() / 2; ++i) {
                    int ii = name.size() / 2 - i - 1;

                    std::string bit(name.begin() + ii * 2, name.begin() + (ii + 1) * 2);

                    byte b = std::stoi(bit, nullptr, 16);

                    glyph.bitmap.push_back(b);
                }

                byte accum = 0x0;
                for(byte& b : glyph.bitmap) {
                    accum |= b;
                }
                if(accum == 0x0) glyph.visible = false;
                else glyph.visible = true;
            }
        }

        if(name == "STARTFONT") region = axiom::bdf_region::BDF_HEADER;
        else if(name == "ENDFONT") break;
        else if(name == "STARTCHAR") region = axiom::bdf_region::BDF_GLYPH;
    }

    //

    ascender = line_ascent;
    descender = -line_descent;
    line_height = ascender - descender;
    line_gap = line_height - (ascender - descender);
    base_unit = 1.0f;
    
    for(auto& [id, glyph] : font.glyph_map) {
        axiom::glyph g;
        g.bounding_box = {glyph.offset.x, glyph.offset.y, glyph.size.x, glyph.size.y};
        g.h_advance = glyph.advance;
        g.glyph_id = id;
        g.h_bearing = glyph.offset.x;
        g.codepoint = id;
        g.glyph_address = id;
        g.visible = glyph.visible;

        bdf_glyph bg;
        bg.bitmap_size = glyph.size;
        bg.bitmap = glyph.bitmap;

        bdf_glyphs.emplace(id, bg);

        glyphs.emplace(id, g);
    }
}

axiom::texture_asset bdf_font::get_bitmap(glyph_key key) {
    bdf_glyph& g = bdf_glyphs[key.glyph];
    auto& phase = phases[key];

    phase.offset = {0, 0};

    //

    uint pitch = ceil(float(g.bitmap_size.x) / 8) * 8;
    uint o = pitch - g.bitmap_size.x;

    std::vector<uint8_t> colors;
    for(int y = 0; y < g.bitmap_size.y; ++y) {
        for(int x = 0; x < g.bitmap_size.x; ++x) {
            int i = g.bitmap_size.x - x - 1 + o + (g.bitmap_size.y - y - 1) * pitch;

            uint8_t color = (g.bitmap[i / 8] >> (i % 8)) & 0x1;
            color *= 0xFF;

            colors.push_back(color);
            colors.push_back(color);
            colors.push_back(color);
            colors.push_back(0xFF);
        }
    }

    ivec2 size = g.bitmap_size;

    return axiom::texture_asset::load(colors, size, 4);
}

std::vector<shaped_glyph> bdf_font::shape(std::string string, uint text_size) {
    auto s = axiom::convert_string(string);

    unsigned int count = s.size();

    std::vector<shaped_glyph> sglyphs;

    for (unsigned int i = 0; i < count; ++i) {
        uint32_t c = s[i];
        if(!glyphs.contains(c)) c = 0;

        auto& g = glyphs[c];
        auto& bdf_glyph = bdf_glyphs[g.glyph_id];

        uint32_t glyph_id = g.glyph_id;

        float x_advance = g.h_advance * (float)text_size;
        float y_advance = 0.0f;

        float x_offset = 0.0f;
        float y_offset = 0.0f;//g.bounding_box.y;

        shaped_glyph sglyph;
        sglyph.glyph = glyph_id;
        sglyph.cluster = i;
        sglyph.advance = {x_advance, y_advance};
        sglyph.offset = {x_offset, y_offset};

        sglyphs.push_back(sglyph);
    }

    return sglyphs;
}

}