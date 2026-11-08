#pragma once

#include <ui/ui.hpp>
#include <memory>

#include <ui/font/font_base.hpp>
#include <ui/font/bdf.hpp>

namespace axiom {
    
struct font_handler {
    axiom::texture texture;

    std::map<std::string, std::unique_ptr<font>> fonts;

    void init();
    void process(std::string filepath, std::string label);

    void touch_phase(glyph_key key);
    void touch_texture();
};

}