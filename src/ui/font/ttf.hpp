#pragma once

#include <ui/ui.hpp>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H
#include <hb.h>
#include <hb-ft.h>

#include <ui/font/font.hpp>

namespace axiom {

struct ttf_font : font {
    hb_font_t* hb_font;
    FT_Face ft_font;

    axiom::texture_asset get_bitmap(glyph_key key);
    void process(std::string filepath, font_handler* handler);
    std::vector<shaped_glyph> shape(std::string string, uint text_size);
};

}