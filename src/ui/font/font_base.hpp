#pragma once

#include <ui/ui.hpp>
#include <memory>

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

struct glyph {
    uint glyph_id;
    uint codepoint = 0xFFFFFFFF;
    uint glyph_address;

    bool visible = true;
    ivec4 bounding_box;
    int h_advance;
    int h_bearing;
};

struct glyph_phase {
    glyph_key key;

    axiom::texture_asset bitmap;

    ivec2 offset;
    ivec4 texture;
};

struct shaped_glyph {
    uint glyph;
    uint size;

    uint cluster;
    vec2 advance;
    vec2 offset;
};

struct font_handler;

struct font {
    ivec2 phase_range;

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
    
    virtual axiom::texture_asset get_bitmap(glyph_key key) = 0;
    virtual void process(std::string filepath, font_handler* handler = 0) = 0;
    virtual std::vector<shaped_glyph> shape(std::string string, uint text_size) = 0;
};

}