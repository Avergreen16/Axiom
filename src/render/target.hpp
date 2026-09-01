#pragma once

#include <render/wrapper.hpp>

namespace axiom {

struct render_target {
    // attributes
    ivec2 size;
    ivec2 position;
    
    // resources
    framebuffer framebuffer;
    std::function<void(render_target&)> draw;

    void set_size(ivec2 new_size, ivec2 new_position);
    void call();

    //

    render_target() = default;

    render_target(render_target&&) = default;

    render_target& operator=(render_target&&) = default;

    ~render_target() = default;

    //
    
    static render_target create(std::function<void(render_target&)> draw_func, ivec2 size, ivec2 position, std::vector<texture_format> fb_format, std::vector<texture_attachment> fb_attachment, std::vector<int> fb_binding = {});
};

}