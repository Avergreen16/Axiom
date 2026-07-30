#pragma once

#include <render/wrapper.hpp>

namespace axiom {

struct render_target {
    // attributes
    ivec2 size;
    
    // resources
    axiom::framebuffer framebuffer;
    std::function<void(render_target&)> draw;

    void set_size(ivec2 new_size);
    void call();

    //

    render_target() = default;

    render_target(render_target&&) = default;

    render_target& operator=(render_target&&) = default;

    ~render_target() = default;

    //
    
    static render_target create(std::function<void(render_target&)> draw_func, ivec2 size, std::vector<axiom::texture_format> fb_format, std::vector<axiom::texture_attachment> fb_attachment, std::vector<int> fb_binding = {});
};

}