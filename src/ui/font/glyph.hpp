#pragma once

#include <include/core.hpp>
#include <include/render.hpp>

namespace axiom {


struct font {
    FT_Face ft_font;
    hb_font_t* hb_font;

    ~font();
};

struct shaped_glyph {
    glyph_key key;

    uint cluster;
    vec2 advance;
    vec2 offset;
};

struct font_handler {
    FT_Library library;
    std::map<std::string, font> fonts;

    std::set<glyph_key, glyph_key_less> active_glyphs;
    std::map<glyph_key, glyph, glyph_key_less> glyphs;
    axiom::texture texture;

    void initialize();
    void load_face(std::string filepath, std::string name);

    void touch_glyph(glyph_key key);
    void touch_texture();

    std::vector<shaped_glyph> shape_text(std::string str);
};

}