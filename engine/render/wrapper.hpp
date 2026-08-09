#pragma once
#define _USE_MATH_DEFINES

#include <glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <array>
#include <map>
#include <unordered_map>
#include <queue>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <set>
#include <unordered_set>
#include <deque>
#include <variant>
#include <functional>
#include <iomanip>
#include <thread>
#include <mutex>
#include <fstream>
#include <any>
#include <list>
#include <string>
#include <shared_mutex>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\orthonormalize.hpp"

#include <assets/assets.hpp>

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;

namespace axiom {
    
void set_blend();

struct vertices {
    uint vertex_buffer;
    uint vertex_array;
    uint index_buffer;

    uint num_vertices;
    uint num_indices;
    uint index_type;

    bool initialized = false;

    vertices() = default;

    vertices(const vertices& a) = delete;

    vertices& operator=(const vertices& a) = delete;

    vertices(vertices&& a);

    ~vertices();

    vertices& operator=(vertices&& a);

    void init();

    void vertex_buffer_data(void* ptr, uint num_vertices_, uint vertex_size, uint usage);

    void index_buffer_data(void* ptr, uint num_indices_, uint index_type_, uint index_size, uint usage);

    void add_vertex_attribute(uint index, uint size, uint type, uint normalized, uint stride, uint offset);

    void bind();

    void draw_vertices(uint mode);

    void draw_indices(uint mode);
};

struct storage_buffer {
    uint id;
    bool initialized = false;

    void init();

    void buffer_data(void* data, uint size_bytes, uint usage);

    void buffer_subdata(void* data, uint size_bytes, uint offset);

    void bind(uint binding);

    void delete_buffer();

    ~storage_buffer();
};

struct uniform_buffer {
    uint id;
    bool initialized = false;

    void init();

    void buffer_data(void* data, uint size_bytes, uint usage);

    void buffer_subdata(void* data, uint size_bytes, uint offset);

    void bind(uint binding);

    void delete_buffer();

    uniform_buffer() = default;
    

    uniform_buffer(const uniform_buffer& a) = delete;

    uniform_buffer& operator=(const uniform_buffer& a) = delete;

    uniform_buffer(uniform_buffer&& a);

    uniform_buffer& operator=(uniform_buffer&& a);

    ~uniform_buffer();
};

struct shader {
    uint id;

    shader() = default;

    shader(text_asset vertex_shader, text_asset fragment_shader);

    shader(text_asset vertex_shader, text_asset geometry_shader, text_asset fragment_shader);

    shader(text_asset vertex_shader, text_asset tess_ctrl_shader, text_asset tess_eval_shader, text_asset geometry_shader, text_asset fragment_shader);

    shader(text_asset compute_shader);
    
    shader(shader&& s);

    shader& operator=(shader&& s);

    bool compile(text_asset vertex_shader, text_asset fragment_shader);

    bool compile(text_asset vertex_shader, text_asset geometry_shader, text_asset fragment_shader);

    bool compile(text_asset vertex_shader, text_asset tess_ctrl_shader, text_asset tess_eval_shader, text_asset geometry_shader, text_asset fragment_shader);

    bool compile(text_asset compute_shader);

    void use();

    static void dispatch_compute(glm::uvec3 work_groups);

    ~shader();
};

struct texture_desc {
    uint formatbits;
    uint format;
    uint bits;
};

enum class texture_format {
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16,
    RG16,
    RGB16,
    RGBA16,
    RF,
    RGF,
    RGBF,
    RGBAF,

    DEPTH16,
    DEPTH32,
    DEPTHF,
};

enum class texture_attachment {
    COLOR0 = 0,
    COLOR1 = 1,
    COLOR2 = 2,
    COLOR3 = 3,
    COLOR4 = 4,
    COLOR5 = 5,
    COLOR6 = 6,
    COLOR7 = 7,
    COLOR8 = 8,
    COLOR9 = 9,
    COLOR10 = 10,
    COLOR11 = 11,
    COLOR12 = 12,
    COLOR13 = 13,
    COLOR14 = 14,
    COLOR15 = 15,
    COLOR16 = 16,
    COLOR17 = 17,
    COLOR18 = 18,
    COLOR19 = 19,
    COLOR20 = 20,
    COLOR21 = 21,
    COLOR22 = 22,
    COLOR23 = 23,
    COLOR24 = 24,
    COLOR25 = 25,
    COLOR26 = 26,
    COLOR27 = 27,
    COLOR28 = 28,
    COLOR29 = 29,
    COLOR30 = 30,
    COLOR31 = 31,

    //

    DEPTH,
    STENCIL,
    DEPTH_STENCIL
};

texture_desc get_texture_desc(texture_format f);
GLenum get_texture_attachment(axiom::texture_attachment attachment);

struct texture {
    uint id;
    uint type;
    glm::ivec3 size;
    int num_channels;
    texture_format format;

    texture(texture_asset& asset, texture_format format, int mip_levels = 0);

    texture(uint8_t* data, glm::uvec3 size, uint type, texture_format format, int mip_levels = 0);
    
    texture(glm::uvec3 size, uint type, texture_format format);

    void load(texture_asset& asset, texture_format format, int mip_levels = 0);

    bool load(glm::uvec2 size, texture_format format);

    bool load(glm::uvec3 size, texture_format format);

    void bind(int binding);

    void bind();

    void bind_image(unsigned int binding, unsigned int level, uint format, unsigned int layer = 0);
    
    texture() = default;

    texture(const texture& t) = delete;

    texture(texture&& t) noexcept;

    texture& operator=(texture&& t) noexcept;

    void delete_texture();

    ~texture();

    //
    
    texture_asset retrieve();
};

struct fb_tex_params {
    axiom::texture_format format;
    axiom::texture_attachment attachment;

    int binding = -1;
    int layers = 1;
};

struct framebuffer {
    uint id;
    std::vector<texture> textures;
    std::vector<fb_tex_params> tex_params;
    std::vector<uint> draw_buffers;
    bool initialized = false;
    glm::ivec2 size;
    uint filter = GL_NEAREST;
    
    framebuffer() = default;
    framebuffer(framebuffer&& a) noexcept;
    framebuffer& operator=(framebuffer&& a) noexcept;

    framebuffer(glm::ivec2 size_, std::vector<fb_tex_params>&& tp, uint filter = GL_NEAREST);

    void bind_texture(std::shared_ptr<texture> texture_, uint attachment, int32_t binding);

    void bind();

    void resize(glm::ivec2 new_size);

    void clear();

    ~framebuffer();
};

}