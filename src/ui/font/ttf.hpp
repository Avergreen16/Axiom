#pragma once

#include <ui/ui.hpp>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include <hb.h>
#include <hb-ft.h>

namespace axiom {
    
struct glyph_key {
    uint glyph;
    uint size;
    uint16_t phase_x;
    uint16_t phase_y;
};

struct glyph_key_less {
    bool operator()(const glyph_key& a, const glyph_key& b) const;
};

bool operator==(const glyph_key& a, const glyph_key& b);

//

struct contour_point {
    vec2 point;
    bool curve = false;
};

struct contour_bezier {
    uint a;
    uint b;
    uint c;
};

struct contour {
    std::vector<contour_point> points;
    std::vector<contour_bezier> beziers;
};

struct glyph {
    uint glyph_id;
    uint codepoint = 0xFFFFFFFF;
    uint glyph_address;

    ivec4 bounding_box;
    bool compound = false;

    int h_advance;
    int h_bearing;

    std::vector<contour> contours;
    std::vector<byte> bytecode_glyf;
};

struct glyph_phase {
    glyph_key key;

    axiom::texture_asset bitmap;

    ivec4 texture;
};

struct shaped_glyph {
    uint glyph;
    uint size;

    uint cluster;
    vec2 advance;
    vec2 offset;
};

struct font {
    hb_font_t* hb_font;
    
    std::map<uint, glyph> glyphs;
    std::map<uint, uint> glyph_map; // first is unicode codepoint, second is glyph ID

    std::set<glyph_key, glyph_key_less> active_phases;
    std::map<glyph_key, glyph_phase, glyph_key_less> phases;
    
    ivec4 bounding_box;
    uint base_unit;

    int ascender;
    int descender;
    int line_gap;
    int line_height;
    
    axiom::texture_asset get_bitmap(glyph_key key);
};


struct font_handler {
    std::map<std::string, font> fonts;

    axiom::texture texture;

    void process_ttf(std::string filepath, std::string label);

    void touch_phase(glyph_key key);
    void touch_texture();
};


}