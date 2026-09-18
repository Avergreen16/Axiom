#pragma once

#include <include/core.hpp>
#include <render/wrapper.hpp>
#include <render/target.hpp>

namespace axiom {

struct shadow_renderer {
    uint num_cascades = 5;
    float cascade_factor = 8.0f;
    float base_pixel_size = 1.0f / 16.0f;
    uint texture_size = 2048;

    uint camera_entity;
    render_target* target;

    std::vector<framebuffer> framebuffers;
    std::function<void(framebuffer&, transform3d&, mat4, mat4)> render_func;

    static void create(uint num_cascades, float cascade_factor, float base_pixel_size, uint texture_size, uint camera, render_target* target, std::function<void(framebuffer&, transform3d&, mat4, mat4)> render_func);
    void call();
};

}