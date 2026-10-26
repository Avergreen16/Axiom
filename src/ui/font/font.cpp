#include <ui/font/font.hpp>

namespace axiom {

void font_handler::init() {
    auto error = FT_Init_FreeType( &library );
    if(error) std::cout << "ERROR: freetype failed to initialize\n";
}

void font_handler::process(std::string filepath, std::string label) {
    if(std::string(filepath.end() - 4, filepath.end()) == ".bdf") {
        fonts.emplace(label, std::unique_ptr<axiom::bdf_font>(new axiom::bdf_font()));
        axiom::bdf_font* font = reinterpret_cast<axiom::bdf_font*>(fonts[label].get());

        font->process(filepath, this);
    } else if(std::string(filepath.end() - 4, filepath.end()) == ".ttf") {
        fonts.emplace(label, std::unique_ptr<axiom::ttf_font>(new axiom::ttf_font()));
        axiom::ttf_font* font = reinterpret_cast<axiom::ttf_font*>(fonts[label].get());

        font->process(filepath, this);
    }
}

void font_handler::touch_phase(glyph_key key) {
    if(!fonts["test"]->phases.contains(key)) {
        axiom::glyph_phase phase;
        
        fonts["test"]->phases.emplace(key, phase);

        auto bitmap = fonts["test"]->get_bitmap(key);
        fonts["test"]->phases[key].bitmap = bitmap;
    }

    fonts["test"]->active_phases.emplace(key);
}

void font_handler::touch_texture() {
    static std::set<glyph_key, glyph_key_less> prev_phases;

    if(prev_phases != fonts["test"]->active_phases) {
        std::vector<glyph_key> to_erase;
        for(const glyph_key& prev_key : prev_phases) {
            if(!fonts["test"]->active_phases.contains(prev_key)) to_erase.push_back(prev_key);
        }
        for(glyph_key key : to_erase) fonts["test"]->phases.erase(key);

        prev_phases = fonts["test"]->active_phases;

        //
            
        std::vector<byte> bytes;
        ivec2 img_size = ivec2(0);

        uint width = 1024;
        std::vector<uint> heights(1024, 0);
        uint cursor = 0;

        uint ctr = 0;
        for(auto& key : prev_phases) {
            auto& phase = fonts["test"]->phases[key];
            
            if(phase.bitmap.size.x) {
                axiom::texture_asset& asset = phase.bitmap;

                if(cursor + asset.size.x > width) cursor = 0;

                uint height = 0;
                for(int i = 0; i < asset.size.x; ++i) {
                    uint h = heights[i + cursor];
                    height = glm::max(height, h);
                }
                
                for(int i = 0; i < asset.size.x; ++i) {
                    heights[i + cursor] = height + asset.size.y;
                }

                uint new_height = glm::max(img_size.y, int(height + asset.size.y) + 1);

                bytes.resize(width * new_height * 4, 0x00);

                vec2 dst_pos = vec2(cursor, height);

                ivec4 atlas = ivec4(dst_pos, dst_pos + vec2(asset.size));
                phase.texture = atlas;

                for(int j = 0; j < asset.size.x * asset.size.y; ++j) {
                    ivec2 src_pos = {j % asset.size.x, j / asset.size.x};
                    ivec2 ndst_pos = ivec2(dst_pos) + src_pos;

                    int src_i = src_pos.y * asset.size.x + src_pos.x;
                    int dst_i = ndst_pos.y * width + ndst_pos.x;

                    bytes[dst_i * 4] = asset.data[src_i * 4];
                    bytes[dst_i * 4 + 1] = asset.data[src_i * 4 + 1];
                    bytes[dst_i * 4 + 2] = asset.data[src_i * 4 + 2];
                    bytes[dst_i * 4 + 3] = asset.data[src_i * 4 + 3];
                }

                cursor += asset.size.x;

                img_size.x = glm::max(img_size.x, (int)cursor);
                img_size.y = glm::max(img_size.y, (int)new_height);
            } else phase.texture.z = 0.0;
        }
        
        axiom::texture_asset asset = axiom::texture_asset::load(bytes, {width, img_size.y}, 4);

        texture.load(asset, axiom::texture_format::RGBA8, 0);
    }

    fonts["test"]->active_phases.clear();
}

}