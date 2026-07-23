#pragma once;

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\orthonormalize.hpp"

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
#include <map>
#include <memory>

#include <ecs/ecs.hpp>
#include <assets/assets.hpp>

namespace axiom {

extern vec3 color_physics;
extern vec3 color_editor;
extern vec3 color_debug;
extern vec3 color_lua;

bool includes(ivec2 point, ivec4 range);
bool includes(ivec4 range_a, ivec4 range_b);

vec4 intersect_range(vec4 a, vec4 b);

const ulong NULL_WIDGET = 0xFFFFFFFFFFFFFFFF;
const uint NULL_OPERATION = 0xFFFFFFFF;

struct ui_vertex {
    vec3 pos;
    vec2 tex_pos;
    vec4 color = vec4(1.0f);
    vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
    uint data = 0;
};

// 

/*

//

struct text_input_widget : widget {
    std::string text;
    uint text_size = 1;
    float text_width = 0.0f;
    float text_x = 0.0f;
    ALIGNMENT alignment = ALIGNMENT_LEFT;
    vec2 resize_range = vec2(0.0f, 0.0f);
    bool text_dirty = true;

    uint cursor = 0xFFFFFFFF;
    uint selection_anchor = 0xFFFFFFFF;
    bool wraparound = false;
    vec2 cursor_pos;
    vec2 click_pos;
    bool click = false;
    std::vector<text_line_data> line_indices;

    std::vector<ui_vertex> text_vertices;

    void handle_inputs();
    void mesh();
    void get_y();
    void insert_cursor();
    void set_str(std::string str);
    void init();

    static ulong insert(std::string str, ALIGNMENT alg);
};

// capture data
*/

}

//#include "ui.tpp"
