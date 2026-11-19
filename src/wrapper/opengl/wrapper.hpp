#pragma once
#define _USE_MATH_DEFINES

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
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

#include <include/core.hpp>

#include <glad.h>

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using dvec2 = glm::vec<2, double>;
using dvec3 = glm::vec<3, double>;
using dvec4 = glm::vec<4, double>;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;

namespace axiom {
    
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

enum class blend_func {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_AMPHA,
    SRC_ALPHA_SATURATE,
    SRC1_COLOR,
    ONE_MINUS_SRC1_COLOR,
    SRC1_APHA,
    ONE_MINUS_SRC1_ALPHA
};

enum class depth_func {
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOTEQUAL,
    GREATER_EQUAL,
    ALWAYS
};

enum class usage_type {
    STATIC_DRAW,
    DYNAMIC_DRAW,
    STREAM_DRAW
};

enum class var_type {
    BYTE, 
    UNSIGNED_BYTE, 
    SHORT, 
    USHORT, 
    INT, 
    UINT,

    HALF_FLOAT, 
    FLOAT, 
    DOUBLE, 
    COLOR_PACKED_INT, // 2 alpha, 10 red, 10 green, 10 blue
    COLOR_PACKED_UINT // 2 alpha, 10 red, 10 green, 10 blue
};

void clear_framebuffer(bool clear_color, bool clear_depth, bool clear_stencil);
void bind_default_framebuffer();

void enable_blend();
void disable_blend();
void enable_depth_test();
void disable_depth_test();
void enable_depth_clamp();
void disable_depth_clamp();

void set_clear_color(vec4 color);
void set_clear_depth(float depth);
void set_viewport(ivec4 range);
void set_depth_func(axiom::depth_func depth_func);
void set_clear_depth(float depth);
void set_depth_range(float near, float far);
void set_clip_control();
void set_blend_func(blend_func src_func, blend_func dst_func);
void set_blend_func_separate(blend_func src_func_color, blend_func dst_func_color, blend_func src_func_alpha, blend_func dst_func_alpha);

struct framebuffer;

void blit(ivec4 src, ivec4 dst, framebuffer* srcf, framebuffer* dstf, axiom::texture_attachment src_attachment, axiom::texture_attachment dst_attachment, bool color, bool depth, bool stencil);

void draw_points(uint start, uint num);
void draw_lines(uint start, uint num);
void draw_triangles(uint start, uint num);

//

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

    void vertex_buffer_data(void* ptr, uint num_vertices_, uint vertex_size, axiom::usage_type usage);
    void index_buffer_data(void* ptr, uint num_indices_, axiom::var_type index_type_, uint index_size, axiom::usage_type usage);
    void add_vertex_attribute(uint index, uint size, axiom::var_type type, uint normalized, uint stride, uint offset);

    void bind();

    void draw_vertices_points();
    void draw_vertices_lines();
    void draw_vertices_triangles();

    void draw_indices_points();
    void draw_indices_lines();
    void draw_indices_triangles();
};

struct storage_buffer {
    uint id;
    bool initialized = false;

    void init();

    void buffer_data(void* data, uint size_bytes, axiom::usage_type usage);

    void buffer_subdata(void* data, uint size_bytes, uint offset);

    void bind(uint binding);

    void delete_buffer();

    ~storage_buffer();
};

struct uniform_buffer {
    uint id;
    bool initialized = false;

    void init();

    void buffer_data(void* data, uint size_bytes, axiom::usage_type usage);

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

texture_desc get_texture_desc(texture_format f);
uint get_texture_attachment(texture_attachment attachment);

struct texture {
    uint id = 0xFFFFFFFF;
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
    texture_format format;
    texture_attachment attachment;

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

    uint depth_texture;
    
    framebuffer() = default;
    framebuffer(framebuffer&& a) noexcept;
    framebuffer& operator=(framebuffer&& a) noexcept;

    framebuffer(glm::ivec2 size_, std::vector<fb_tex_params> tp, uint filter = GL_NEAREST);

    void bind_texture(std::shared_ptr<texture> texture_, uint attachment, int32_t binding);

    void bind();

    void resize(glm::ivec2 new_size);

    void clear(vec4 color = vec4(0.0f, 0.0f, 0.0f, 1.0f), float depth = 0.0f);

    ~framebuffer();
};

void blit(ivec4 src, ivec4 dst, framebuffer* srcf, framebuffer* dstf, axiom::texture_attachment src_attachment, axiom::texture_attachment dst_attachment, bool color, bool depth, bool stencil);

void push_uniform(uint index, int value);
void push_uniform(uint index, uint value);
void push_uniform(uint index, float value);
void push_uniform(uint index, double value);

//
void push_uniform(uint index, ivec2 value);
void push_uniform(uint index, uvec2 value);
void push_uniform(uint index, vec2 value);
void push_uniform(uint index, dvec2 value);

//
void push_uniform(uint index, ivec3 value);
void push_uniform(uint index, uvec3 value);
void push_uniform(uint index, vec3 value);
void push_uniform(uint index, dvec3 value);

//
void push_uniform(uint index, ivec4 value);
void push_uniform(uint index, uvec4 value);
void push_uniform(uint index, vec4 value);
void push_uniform(uint index, dvec4 value);

// pointers
void push_uniform(uint index, int* value);
void push_uniform(uint index, uint* value);
void push_uniform(uint index, float* value);
void push_uniform(uint index, double* value);

//
void push_uniform(uint index, ivec2* value);
void push_uniform(uint index, uvec2* value);
void push_uniform(uint index, vec2* value);
void push_uniform(uint index, dvec2* value);

//
void push_uniform(uint index, ivec3* value);
void push_uniform(uint index, uvec3* value);
void push_uniform(uint index, vec3* value);
void push_uniform(uint index, dvec3* value);

//
void push_uniform(uint index, ivec4* value);
void push_uniform(uint index, uvec4* value);
void push_uniform(uint index, vec4* value);
void push_uniform(uint index, dvec4* value);

//
void push_uniform(uint index, mat2* value);
void push_uniform(uint index, glm::mat2x3* value);
void push_uniform(uint index, glm::mat2x4* value);
void push_uniform(uint index, glm::mat3x2* value);
void push_uniform(uint index, mat3* value);
void push_uniform(uint index, glm::mat3x4* value);
void push_uniform(uint index, glm::mat4x2* value);
void push_uniform(uint index, glm::mat4x3* value);
void push_uniform(uint index, mat4* value);

// pointer packs
void push_uniform(uint index, uint count, int* value);
void push_uniform(uint index, uint count, uint* value);
void push_uniform(uint index, uint count, float* value);
void push_uniform(uint index, uint count, double* value);

//
void push_uniform(uint index, uint count, ivec2* value);
void push_uniform(uint index, uint count, uvec2* value);
void push_uniform(uint index, uint count, vec2* value);
void push_uniform(uint index, uint count, dvec2* value);

//
void push_uniform(uint index, uint count, ivec3* value);
void push_uniform(uint index, uint count, uvec3* value);
void push_uniform(uint index, uint count, vec3* value);
void push_uniform(uint index, uint count, dvec3* value);

//
void push_uniform(uint index, uint count, ivec4* value);
void push_uniform(uint index, uint count, uvec4* value);
void push_uniform(uint index, uint count, vec4* value);
void push_uniform(uint index, uint count, dvec4* value);

//
void push_uniform(uint index, uint count, mat2* value);
void push_uniform(uint index, uint count, glm::mat2x3* value);
void push_uniform(uint index, uint count, glm::mat2x4* value);
void push_uniform(uint index, uint count, glm::mat3x2* value);
void push_uniform(uint index, uint count, mat3* value);
void push_uniform(uint index, uint count, glm::mat3x4* value);
void push_uniform(uint index, uint count, glm::mat4x2* value);
void push_uniform(uint index, uint count, glm::mat4x3* value);
void push_uniform(uint index, uint count, mat4* value);

}