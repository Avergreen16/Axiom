#include <iostream>

#include "assets/assets.hpp"
#include "utilities/utilities.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

namespace axiom {

texture_asset texture_asset::load(std::string path) {
    texture_asset asset;

    int width;
    int height;
    int num_channels;

    stbi_set_flip_vertically_on_load(true);

    byte* bytes = stbi_load(path.data(), &width, &height, &num_channels, 0);

    if(!bytes) {
        std::cout << "ERROR: Texture failed to load.\n";
        std::cout << path << "\n";
    }

    asset.size = {width, height};
    asset.num_channels = num_channels;
    asset.data = std::vector<byte>(bytes, bytes + (width * height * num_channels));

    asset.filepath = path;

    stbi_image_free(bytes);

    return asset;
}

texture_asset texture_asset::load(std::vector<byte> data, uvec2 size, uint num_channels) {
    texture_asset asset;

    asset.size = size;
    asset.num_channels = num_channels;
    asset.data = data;

    asset.filepath = "";

    return asset;
}

void texture_asset::save(std::string path) {
    stbi_flip_vertically_on_write(true);
    stbi_write_png(path.c_str(), size.x, size.y, num_channels, data.data(), size.x * num_channels);
}

text_asset text_asset::load(std::string path) {
    text_asset asset;
    
    asset.data = axiom::get_text_from_file(path);
    asset.filepath = path;

    return asset;
}


glyph_data& font_asset::at(uint key) {
    if(glyph_map.find(key) != glyph_map.end()) return glyph_map[key];
    return empty_data;
}

enum bdf_region{BDF_NULL, BDF_HEADER, BDF_GLYPH};
bool bitmap = false;

font_asset font_asset::load(std::string path) {
    font_asset font;

    text_asset text = text_asset::load(path);

    ivec2 size;
    ivec2 offset;
    int line_ascent = 0;
    int line_descent = 0;

    std::stringstream ss(text.data);

    bdf_region region = BDF_NULL;

    uint encoding;
    glyph_data glyph;
    std::vector<uint> keys;

    while(true) {
        std::string line;
        std::getline(ss, line);

        std::stringstream ss_line(line);
        std::string name;
        ss_line >> name;

        if(region == BDF_HEADER) {
            if(name == "FONTBOUNDINGBOX") {
                int xsize, ysize, xoffset, yoffset;
                ss_line >> xsize >> ysize >> xoffset >> yoffset;

                size = {xsize, ysize};
                offset = {xoffset, yoffset};
            } else if(name == "FONT_ASCENT") {
                ss_line >> line_ascent;
            } else if(name == "FONT_DESCENT") {
                ss_line >> line_descent;
            }
        } else if(region == BDF_GLYPH) {
            bool collect_bits = bitmap;

            if(name == "ENCODING") {
                int number;
                ss_line >> number;

                encoding = number;
            } else if(name == "DWIDTH") {
                int x, y;
                ss_line >> x >> y;

                glyph.advance = x;
            } else if(name == "BBX") {
                int xsize, ysize, xoffset, yoffset;
                ss_line >> xsize >> ysize >> xoffset >> yoffset;

                glyph.size = {xsize, ysize};
                glyph.offset = {xoffset, yoffset};
            } else if(name == "BITMAP") {
                bitmap = true;
                glyph.bitmap.clear();
            } else if(name == "ENDCHAR") {
                region = BDF_NULL;
                bitmap = false;
                collect_bits = false;

                keys.push_back(encoding);
                font.glyph_map.emplace(encoding, glyph);
            }

            if(collect_bits) {
                for(int i = 0; i < name.size() / 2; ++i) {
                    std::string bit(name.begin() + i * 2, name.begin() + (i + 1) * 2);

                    byte b = std::stoi(bit, nullptr, 16);

                    glyph.bitmap.push_back(b);
                }
            }
        }

        if(name == "STARTFONT") region = BDF_HEADER;
        else if(name == "ENDFONT") break;
        else if(name == "STARTCHAR") region = BDF_GLYPH;
    }

    std::vector<bool> pixels;
    ivec2 tex_size;
    uint width = 1024;
    uint num_per_row = width / size.x;
    uint height = ceil(float(font.glyph_map.size()) / num_per_row) * size.y;
    tex_size = ivec2(width, height);

    pixels.resize(tex_size.x * tex_size.y, 0);

    uint pos = 0;
    for(uint i : keys) {
        glyph_data& glyph = font.glyph_map[i];
        glyph.offset.y += line_descent;

        ivec2 origin = ivec2(pos % num_per_row, pos / num_per_row) * size;
        origin = origin + (glyph.offset - ivec2(0, line_descent) - offset);

        glyph.pos_tex = origin;

        uint byte_row = ceil(float(glyph.size.x) / 8);

        bool visible = false;

        for(int y = 0; y < glyph.size.y; ++y) {
            for(int x = 0; x < byte_row; ++x) {
                int index = y * byte_row + x;
                byte byte = glyph.bitmap[index];

                if(byte != 0x0) visible = true;

                for(int xx = x * 8; xx < glm::min((x + 1) * 8, glyph.size.x); ++xx) {
                    int bit = 7 - (xx - x * 8);

                    bool b = (byte >> bit) & 0x1;

                    ivec2 bit_pos = origin + ivec2(xx, glyph.size.y - 1 - y);
                    int bit_index = bit_pos.y * tex_size.x + bit_pos.x;

                    pixels[bit_index] = b;
                }
            }
        }

        glyph.visible = visible;

        ++pos;
    }

    std::vector<byte> texture_data(tex_size.x * tex_size.y * 4, 0x0);

    for(int i = 0; i < tex_size.x * tex_size.y; ++i) {
        if(pixels[i]) {
            texture_data[i * 4] = 0xFF;
            texture_data[i * 4 + 1] = 0xFF;
            texture_data[i * 4 + 2] = 0xFF;
            texture_data[i * 4 + 3] = 0xFF;
        }
    }

    font.texture = texture_asset::load(texture_data, tex_size, 4);

    font.line_height = line_ascent + line_descent;

    return font;
}

}