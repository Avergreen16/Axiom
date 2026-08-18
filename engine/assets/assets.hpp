#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <sstream>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;
using byte = uint8_t;

namespace axiom {

class texture_asset {
    public:

    std::vector<byte> data;
    uvec2 size;
    uint num_channels;

    std::string filepath;

    void save(std::string path);

    static texture_asset load(std::string path);
    static texture_asset load(std::vector<byte> data, uvec2 size, uint num_channels);
};

class text_asset {
    public:

    std::string data;
    std::string filepath;

    static text_asset load(std::string path);
};


struct glyph_data {
    std::vector<byte> bitmap;
    bool visible = true;

    ivec2 size;
    ivec2 offset;
    int advance;
    
    ivec2 pos_tex;
};

struct font_asset {
    int line_height;
    glyph_data empty_data = {{}, false, {0, 0}, {0, 0}, 0, {0, 0}};
    std::unordered_map<uint, glyph_data> glyph_map;
    texture_asset texture;

    std::string filepath;

    font_asset() = default;
    font_asset(const font_asset& f) = default;
    font_asset(font_asset&& f) = default;
    font_asset& operator=(font_asset& f) = default;
    font_asset& operator=(font_asset&& f) = default;
    
    glyph_data& at(uint key);

    static font_asset load(std::string path);
};

}