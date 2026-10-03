#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;
using ulong = uint64_t;

#include <string>

#include <include/core.hpp>
#include <ui/ui.hpp>
#include <ui/font/ttf.hpp>

namespace axiom {

enum class text_alignment {
    LEFT,
    RIGHT,
    CENTER   
};

struct text_line_data {
    uint start_index;
    bool bold = false;
    bool italic = false;
    vec3 color = vec3(1.0f);
    float offset = 0.0f;
};

struct text_data {
    std::vector<text_line_data> lines;
    vec2 size;
    vec2 wrap_limits;
    float max_width;
};

// text mesh data

extern vec2 text_range;
extern std::vector<uint> text_start;
extern std::vector<uint> text_line_indices;
extern std::vector<text_line_data> line_data;

//std::vector<float> compute_text_bounds(axiom::font& f, std::string text, uint text_size, uint width, bool wrap, ALIGNMENT alignment);

struct text_glyph {
    glyph_key key;
    vec2 position;
    vec4 selection;
};

struct text {
    axiom::font* font;
    uint text_size = 14;

    std::string string;
    axiom::text_alignment alignment = axiom::text_alignment::LEFT;
    std::vector<std::tuple<float, int>> lines;
    std::vector<shaped_glyph> sglyphs;
    std::vector<text_glyph> tglyphs;
    vec2 position = vec2(0.0f);
    vec2 size = vec2(0.0f);
    float z = 0.0f;

    //

    uint32_t width = 0xFFFFFFFF;
    vec2 wrap_limits;
    float max_width;
    bool selectable = true;
    bool editable = false;
    bool wrap = false;

    //
    
    uint clip = 0xFFFFFFFF;
    ulong parent = NULL_WIDGET;

    //

    std::vector<ui_vertex> vertices;
    std::vector<ui_vertex> select_vertices;
    bool dirty = false;

    //

    void shape();
    void measure();
    void touch();
    
    std::vector<ui_vertex> mesh();

    bool collide(vec2 pos);
    void call();

    std::string retrieve();
};

bool select(axiom::text& text, vec2 anchor, vec2 pos, ivec2& range);
int cursor_index(axiom::text& text, vec2 pos);
vec2 cursor_pos(axiom::text& text, uint index);
std::vector<ui_vertex> mesh_selection(axiom::text& text, ivec2 range);

/*
struct text {
    axiom::font* font;

    std::string string;
    ALIGNMENT alignment = ALIGNMENT_LEFT;

    bool wrap = true;
    bool selectable = true;
    bool editable = false;

    bool dirty = false;
    bool remesh = false;

    vec2 position = vec2(0.0f);
    vec2 size = vec2(0.0f);
    uint width = 0xFFFFFFFF;
    float z = 0.0f;

    vec2 resize_range = vec2(0.0f); // range that the text has to go over/under to change its wrapping
    float max_width = 0.0f; // width of the text if no wrapping -> everything is on one line

    ivec2 select_range = ivec2(-1);
    int anchor = -1;

    ivec4 click_range = ivec4(-1);
    double start_cursor = 0.0;

    bool focused = false;

    std::vector<text_line_data> line_data;
    std::vector<ui_vertex> vertices;
    std::vector<ui_vertex> vertices_select;

    std::vector<ui_vertex> get_vertices();
    void refresh();
    void mesh(axiom::font& font);
    //void select(vec4 cursor_range);
    std::string retrieve();

    void call();
};
*/

}