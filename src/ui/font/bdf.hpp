#pragma once

#include <ui/ui.hpp>
#include <ui/font/font_base.hpp>

namespace axiom {

struct bdf_glyph {
    ivec2 bitmap_size;
    std::vector<byte> bitmap;
};
    
struct bdf_font : font {
    std::map<uint, bdf_glyph> bdf_glyphs;

    axiom::texture_asset get_bitmap(glyph_key key);
    void process(std::string filepath, font_handler* handler);
    std::vector<shaped_glyph> shape(std::string string, uint text_size);
};

}