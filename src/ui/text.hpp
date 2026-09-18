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

std::vector<ui_vertex> create_char(ttf_font& font, ttf_glyph& glyph, uint size);
std::vector<ui_vertex> mesh_text(ttf_font& f, uint size, std::string text, text_data& data, uint width = 0xFFFFFFFF, axiom::text_alignment alignment = axiom::text_alignment::LEFT, bool show_debug = false, std::vector<text_line_data>* lines = nullptr);
std::vector<ui_vertex> mesh_text_select(ttf_font& f, uint size, ivec2 selection, std::string text, text_data& data, uint width = 0xFFFFFFFF, axiom::text_alignment alignment = axiom::text_alignment::LEFT, bool show_debug = false, std::vector<text_line_data>* lines = nullptr);

//std::vector<float> compute_text_bounds(ttf_font& f, std::string text, uint text_size, uint width, bool wrap, ALIGNMENT alignment);

class text {
    public:
    
    double time = 0.0;
    uint clip = 0xFFFFFFFF;
    ///vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);

    ttf_font* font;
    uint text_size = 14;
    
    std::string string;
    axiom::text_alignment alignment = axiom::text_alignment::LEFT;

    bool wrap = false;
    uint32_t width = 0xFFFFFFFF;
    vec2 wrap_limits;
    float max_width;
    float z = 0.0f;

    ulong parent = NULL_WIDGET;

    bool selectable = true;
    bool editable = false;

    vec2 position = vec2(0.0f);
    vec2 size = vec2(0.0f);

    std::vector<text_line_data> lines;

    int offset = 0;
    int select_line = 0;
    ivec2 select_range = ivec2(-1);

    // vertices
    std::vector<ui_vertex> glyph_vertices;
    std::vector<ui_vertex> select_vertices;

    // state

    bool dirty = false;
    bool glyph_dirty = false;
    bool select_dirty = false;
    
    float state_width = 0.0f;
    std::string state_string = "";
    int state_select_line = 0;
    ivec2 state_select_range = ivec2(-1);

    //

    std::vector<ui_vertex> v_select;
    
    std::vector<ui_vertex> mesh();
    std::vector<ui_vertex> mesh_select();
    void measure();

    void select(vec4 cursor_range, bool anchor = false);
    ivec2 select(vec2 cursor, uint wrap_mode);

    bool collide(vec2 pos);
    void call();

    std::string retrieve();

    private:
};

std::pair<int, bool> compute_cursor_index(axiom::ttf_font& font, axiom::text& text, vec2 cursor_pos, bool cl0 = true, bool cl1 = true, bool cl2 = true);
vec2 compute_cursor_pos(axiom::ttf_font& font, axiom::text& text, uint32_t index);

/*
struct text {
    ttf_font* font;

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
    void mesh(ttf_font& font);
    //void select(vec4 cursor_range);
    std::string retrieve();

    void call();
};
*/

}