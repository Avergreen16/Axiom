#pragma once

#include <ui/ui.hpp>

namespace axiom {

struct ttf_point {
    vec2 point;
    bool curve = false;
};

struct ttf_bezier {
    uint a;
    uint b;
    uint c;
};

struct ttf_contour {
    std::vector<ttf_point> points;
    std::vector<ttf_bezier> beziers;
};

struct ttf_glyph {
    uint glyph_id;
    uint codepoint = 0xFFFFFFFF;
    uint glyph_address;

    ivec4 bounding_box;
    
    std::vector<ivec4> atlas;
    std::vector<float> frac;

    int h_advance;
    int h_bearing;

    std::vector<ttf_contour> contours;
    std::vector<byte> bytecode_glyf;
};

struct ttf_font {
    std::map<uint, ttf_glyph> glyphs;
    std::map<uint, uint> glyph_map; // first is unicode codepoint, second is glyph ID

    ivec4 bounding_box;
    uint base_unit;

    int ascender;
    int descender;
    int line_gap;
    int line_height;

    axiom::texture texture;

    std::vector<byte> bytecode_fpgm;
    std::vector<byte> bytecode_prep;
};

ttf_font process_ttf(std::string filepath);

}