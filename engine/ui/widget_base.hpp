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

#include <ui/ui.hpp>
#include <ui/text.hpp>

namespace axiom {
    
enum class layout_mode {
    VOID, ROW, COLUMN, GRID
};
enum class position_mode {
    STATIC, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, TOP_CENTER, BOTTOM_CENTER, CENTER_LEFT, CENTER_RIGHT, CENTER, VOID
};

struct widget_constraint {
    std::function<void()> func;
};

struct capture_data {
    ulong key = NULL_WIDGET;
    float z;
    bool capture = false;
    bool text_capture = false;
    bool overwrite = false;
};

struct widget {
    ulong self;
    bool flag = false;

    vec2 position = vec2(0.0f);
    vec2 size = vec2(0.0f);
    vec2 next_position;
    vec2 next_size;
    vec4 buffer = vec4(0.0f);
    
    float z = 0.0f;
    
    vec4 child_region = vec4(0.0f);
    vec2 child_offset = vec2(0.0f);
    vec4 range;
    std::vector<ui_vertex> vertices_before;
    std::vector<ui_vertex> vertices_after;

    std::function<float(std::unique_ptr<widget>&)> get_height = [](std::unique_ptr<widget>& w) {
        return w->size.y;
    };

    //

    axiom::layout_mode layout_mode = axiom::layout_mode::VOID;
    axiom::position_mode position_mode = axiom::position_mode::STATIC;

    //

    vec2 rel_position = vec2(0.0f);
    float min_width = 0.0f;
    float max_width = 0.0f;
    float weight_width = 1.0f;
    float min_height = 0.0f;
    float max_height = 0.0f;
    float weight_height = 1.0f;

    vec4 available_space;
    bool dirty = true;

    ulong parent = NULL_WIDGET;
    std::vector<ulong> children;

    vec4 view_range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);

    //std::vector<text> texts;

    std::vector<std::shared_ptr<axiom::text>> text;

    std::vector<widget_constraint> before;
    std::vector<widget_constraint> after;

    virtual void handle_inputs() {};
    virtual void mesh() {};
    virtual void get_y() {};
    virtual void init() {};
    
    virtual void on_measure() {};
    virtual void on_transform() {};
    virtual void on_place() {};
    virtual void on_delete() {};

    virtual capture_data handle_capture() {
        return {0.0f, false, false};
    };
};

}