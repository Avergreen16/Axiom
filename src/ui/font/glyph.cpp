#include <ui/font/glyph.hpp>

namespace axiom {

void font_handler::initialize() {
    auto error = FT_Init_FreeType(&library);

    if(error) std::cout << "ERROR: initializing freetype failed" << std::endl;
}

void font_handler::load_face(std::string filepath, std::string name) {
    fonts[name] = font();

    auto error = FT_New_Face(library, filepath.c_str(), 0, &fonts[name].ft_font);

    if(error == FT_Err_Unknown_File_Format) std::cout << "ERROR: unknown font file format" << std::endl;
    else if(error) std::cout << "ERROR: reading font file failed\nfilepath: " << filepath << std::endl;

    fonts[name].hb_font = hb_ft_font_create_referenced(fonts[name].ft_font);
}

std::vector<shaped_glyph> font_handler::shape_text(std::string str) {
    std::cout << str.size() << "\n";

    hb_font_set_scale(fonts["test"].hb_font, 16 * 64, 16 * 64);

    hb_buffer_t* buffer = hb_buffer_create();

    hb_buffer_add_utf8(
        buffer,
        str.data(),
        static_cast<int>(str.size()),
        0,
        static_cast<int>(str.size())
    );

    hb_buffer_guess_segment_properties(buffer);

    //

    hb_feature_t features[2];

    hb_feature_from_string("liga=0", -1, &features[0]);

    hb_shape(fonts["test"].hb_font, buffer, features, 2);

    unsigned int count;

    hb_glyph_info_t* infos =
        hb_buffer_get_glyph_infos(buffer, &count);

    hb_glyph_position_t* positions =
        hb_buffer_get_glyph_positions(buffer, &count);

    //

    std::vector<shaped_glyph> result;
    result.reserve(count);

    for (unsigned int i = 0; i < count; ++i) {
        result.push_back(shaped_glyph{
            .key = {infos[i].codepoint, 16},
            .cluster   = infos[i].cluster,

            .advance = {positions[i].x_advance, positions[i].y_advance},
            .offset = {positions[i].x_offset, positions[i].y_offset},
        });
    }

    //////

    static const uint num_phases = 4;

    ivec2 cursor = ivec2(0);
    for(shaped_glyph& sglyph : result) {
        sglyph.key.phase_x = (floor(float(cursor.x % 64) / (64 / num_phases)) + 0.5) * (64 / num_phases);
        sglyph.key.phase_y = (floor(float(cursor.y % 64) / (64 / num_phases)) + 0.5) * (64 / num_phases);
        sglyph.key.size = 16;

        touch_glyph(sglyph.key);
        auto& glyph = glyphs[sglyph.key];
        cursor += sglyph.advance;
    }

    return result;
}

void font_handler::touch_glyph(glyph_key key) {
    if(!glyphs.contains(key)) {
        auto& face = fonts["test"].ft_font;

        auto error = FT_Set_Pixel_Sizes(
            face,   // handle to face object 
            0,           // pixel_width           
            key.size     // pixel_height          
        );
        
        FT_Vector origin;
        origin.x = 0;
        origin.y = 0;
        FT_Set_Transform(face, nullptr, &origin);
        
        FT_Load_Glyph(
            face,
            key.glyph,       
            FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP
        );

        FT_Glyph ft_glyph;
        FT_Get_Glyph(face->glyph, &ft_glyph);

        FT_BBox bbox;
        FT_Glyph_Get_CBox(ft_glyph, FT_GLYPH_BBOX_SUBPIXELS, &bbox);
        
        //

        origin.x = key.phase_x * 16 - bbox.xMin;
        origin.y = key.phase_y - bbox.yMin;
        FT_Set_Transform(face, nullptr, &origin);

        //auto glyph_index = FT_Get_Char_Index(face, codepoint);

        FT_Load_Glyph(
            face,
            key.glyph,       
            FT_LOAD_DEFAULT
        );

        //
    
        FT_Render_Glyph( 
            face->glyph,
            FT_RENDER_MODE_NORMAL 
        );

        std::vector<byte> bitmap(face->glyph->bitmap.buffer, face->glyph->bitmap.buffer + (face->glyph->bitmap.width * face->glyph->bitmap.rows));

        //

        axiom::glyph glyph;
        //glyph.advance = ivec2(face->glyph->advance.x, face->glyph->advance.y);
        glyph.bitmap_size = ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        glyph.bitmap_offset = ivec2(face->glyph->bitmap_left * 64 + bbox.xMin, face->glyph->bitmap_top * 64 - bbox.yMin - face->glyph->bitmap.rows * 64);

        glyph.bitmap = bitmap;

        glyphs.emplace(key, glyph);
    }

    active_glyphs.emplace(key);
}

void font_handler::touch_texture() {
    static std::set<glyph_key, glyph_key_less> prev_glyphs;

    if(prev_glyphs != active_glyphs) {
        prev_glyphs = active_glyphs;

        std::vector<glyph_key> to_delete;
        for(auto& [key, glyph] : glyphs) {
            if(!prev_glyphs.contains(key)) to_delete.push_back(key);
        }
        for(auto& key : to_delete) glyphs.erase(key);

        //
        
        std::vector<byte> bytes;
        ivec2 img_size = ivec2(0);

        uint width = 1024;
        std::vector<uint> heights(1024, 0);
        uint cursor = 0;

        for(auto& [key, glyph] : glyphs) {
            if(cursor + glyph.bitmap_size.x > width) cursor = 0;

            uint height = 0;
            for(int i = 0; i < glyph.bitmap_size.x; ++i) {
                uint h = heights[i + cursor];
                height = glm::max(height, h);
            }
            
            for(int i = 0; i < glyph.bitmap_size.x; ++i) {
                heights[i + cursor] = height + glyph.bitmap_size.y;
            }

            uint new_height = glm::max(img_size.y, int(height + glyph.bitmap_size.y));

            bytes.resize(width * new_height * 4, 0x00);

            vec2 dst_pos = vec2(cursor, height);

            ivec4 atlas = ivec4(dst_pos, dst_pos + vec2(glyph.bitmap_size));

            glyph.texture = atlas;

            //

            for(int j = 0; j < glyph.bitmap_size.x * glyph.bitmap_size.y; ++j) {
                ivec2 src_pos = {j % glyph.bitmap_size.x, j / glyph.bitmap_size.x};
                ivec2 ndst_pos = ivec2(dst_pos) + src_pos;

                src_pos.y = glyph.bitmap_size.y - src_pos.y - 1;

                int src_i = src_pos.y * glyph.bitmap_size.x + src_pos.x;
                int dst_i = ndst_pos.y * width + ndst_pos.x;

                bytes[dst_i * 4] = 0xFF;
                bytes[dst_i * 4 + 1] = 0xFF;
                bytes[dst_i * 4 + 2] = 0xFF;
                bytes[dst_i * 4 + 3] = glyph.bitmap[src_i];
            }

            cursor += glyph.bitmap_size.x;

            img_size.x = glm::max(img_size.x, (int)cursor);
            img_size.y = glm::max(img_size.y, (int)new_height);
        }

        axiom::texture_asset asset = axiom::texture_asset::load(bytes, ivec2(1024, img_size.y), 4);

        texture.load(asset, axiom::texture_format::RGBA8);
    }
}

font::~font() {
    hb_font_destroy(hb_font);
}

}