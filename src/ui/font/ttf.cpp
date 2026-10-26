#include <ui/font/ttf.hpp>
#include <ui/font/font.hpp>
#include <hb-ot.h>

#include <ui/font/font_base.hpp>

namespace axiom {

void ttf_font::process(std::string filepath, font_handler* handler) {
    phase_range = {4, 4};

    auto error = FT_New_Face(handler->library, filepath.c_str(), 0, &ft_font);
    if (error == FT_Err_Unknown_File_Format) {
        std::cout << "ERROR: font file is of an unknown format\n" << "font filepath: " << filepath << std::endl;
    }

    hb_font = hb_ft_font_create_referenced(ft_font);

    ascender = ft_font->ascender;
    descender = ft_font->descender;

    line_height = ft_font->height;
    base_unit = ft_font->units_per_EM;
    
    line_gap = line_height - (ascender - descender);

    for(uint id = 0; id < ft_font->num_glyphs; ++id) {
        FT_Load_Glyph(ft_font, id, FT_LOAD_NO_SCALE);

        const FT_Glyph_Metrics& metrics = ft_font->glyph->metrics;

        axiom::glyph g;
        g.bounding_box = {metrics.horiBearingX, metrics.horiBearingY - metrics.height, metrics.width, metrics.height};
        g.h_advance = metrics.horiAdvance;
        g.glyph_id = id;

        glyphs.emplace(id, g);
    }
}

axiom::texture_asset ttf_font::get_bitmap(glyph_key key) {
    FT_Set_Pixel_Sizes(ft_font, 0, key.size);
    auto& glyph = glyphs[key.glyph];
    auto& phase = phases[key];

    FT_Vector delta = {(-glyph.bounding_box.x * (float(key.size) / base_unit) * 64 + key.phase_x * round(64.0f / phase_range.x)), -glyph.bounding_box.y * (float(key.size) / base_unit) * 64 + key.phase_y * round(64.0f / phase_range.y)}; // 0.25 pixels
    FT_Set_Transform(ft_font, nullptr, &delta);

    FT_Load_Glyph(ft_font, key.glyph, FT_LOAD_DEFAULT);
    auto error = FT_Render_Glyph(ft_font->glyph, FT_RENDER_MODE_LCD);
    if (error) {
        printf("FT_Render_Glyph failed: %d\n", error);
        return;
    }
    
    int left = ft_font->glyph->bitmap_left;
    int top  = ft_font->glyph->bitmap_top;

    //

    const FT_Bitmap& bitmap = ft_font->glyph->bitmap;

    unsigned int width = bitmap.width / 3;
    unsigned int height = bitmap.rows;

    const unsigned char* pixels = bitmap.buffer;

    phase.offset = {left, top - (int)height};

    //

    std::vector<uint8_t> colors;
    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            int i = x * 3 + (height - y - 1) * bitmap.pitch;

            uint8_t color = pixels[i];

            colors.push_back(pixels[i]);
            colors.push_back(pixels[i + 1]);
            colors.push_back(pixels[i + 2]);
            colors.push_back(0xFF);
        }
    }

    ivec2 size = {width, height};

    return axiom::texture_asset::load(colors, size, 4);
}

std::vector<shaped_glyph> ttf_font::shape(std::string string, uint text_size) {
    FT_Set_Pixel_Sizes(ft_font, 0, text_size);
    hb_ft_font_changed(hb_font);

    hb_buffer_t* buffer = hb_buffer_create();

    hb_buffer_add_utf8(
        buffer,
        string.c_str(),
        -1,
        0,
        -1
    );

    //

    hb_buffer_guess_segment_properties(buffer);

    hb_shape(hb_font, buffer, nullptr, 0);

    unsigned int count;

    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &count);
    hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &count);

    std::vector<shaped_glyph> sglyphs;

    for (unsigned int i = 0; i < count; ++i) {
        uint32_t glyph_id = infos[i].codepoint;

        float x_advance = positions[i].x_advance / 64.0f;
        float y_advance = positions[i].y_advance / 64.0f;

        float x_offset = positions[i].x_offset / 64.0f;
        float y_offset = positions[i].y_offset / 64.0f;

        shaped_glyph sglyph;
        sglyph.glyph = glyph_id;
        sglyph.cluster = infos[i].cluster;
        sglyph.advance = {x_advance, y_advance};
        sglyph.offset = {x_offset, y_offset};

        sglyphs.push_back(sglyph);
    }

    return sglyphs;
}

}