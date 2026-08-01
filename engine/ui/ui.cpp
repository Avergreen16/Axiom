#include <ui/ui.hpp>
#include <utilities/utilities.hpp>

#include <numeric>

namespace axiom {

vec2 text_range;
std::vector<uint> text_start;

vec3 color_red = hsv_color(0.0f, 0.675f, 0.9f);
vec3 color_orange = hsv_color(0.375f, 0.675f, 0.9f);
vec3 color_yellow = hsv_color(1.0f, 0.675f, 0.9f);
vec3 color_green = hsv_color(2.0f, 0.675f, 0.5f); 
vec3 color_cyan = hsv_color(3.0f, 0.675f, 0.9f);
vec3 color_blue = hsv_color(4.0f, 0.675f, 0.9f);
vec3 color_purple = hsv_color(4.5f, 0.675f, 0.9f);
vec3 color_magenta = hsv_color(5.0f, 0.675f, 0.9f);
vec3 color_rose = hsv_color(5.75f, 0.675f, 0.9f);

vec3 color_physics = color_rose;
vec3 color_editor = color_physics;
vec3 color_debug = color_physics;
vec3 color_lua = color_yellow;

bool includes(ivec2 point, ivec4 range) {
    return (point.x >= range.x && point.x < range.z && point.y >= range.y && point.y < range.w);
}

bool includes(ivec4 range_a, ivec4 range_b) {
    bool touch_x = range_a.x <= range_b.z && range_b.x <= range_a.z;
    bool touch_y = range_a.y <= range_b.w && range_b.y <= range_a.w;
    return touch_x && touch_y;
}

vec4 intersect_range(vec4 a, vec4 b) {
    return {glm::max(a.x, b.x), glm::max(a.y, b.y), glm::min(a.z, b.z), glm::min(a.w, b.w)};
}

}