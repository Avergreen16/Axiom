#include <render/target.hpp>
#include <render/system.hpp>
#include <render/shadow/shadow.hpp>

namespace axiom {

void render_target::set_size(ivec2 new_size) {
    if(size != new_size && new_size.x != 0 && new_size.y != 0) {
        size = new_size;
        framebuffer.resize(size);
    }
}

void render_target::call() {
    framebuffer.bind();

    //
    enable_blend();

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    set_blend_func_separate(axiom::blend_func::SRC_ALPHA, axiom::blend_func::ONE_MINUS_SRC_ALPHA, axiom::blend_func::SRC_ALPHA, axiom::blend_func::ONE);

    set_viewport({0, 0, framebuffer.size.x, framebuffer.size.y});

    set_clear_color(vec4(0.0f, 0.0f, 0.0f, 1.0f));
    set_clear_depth(0.0f);

    clear_framebuffer(true, true, false);

    //

    draw(*this);

    /*
    if(shadow != nullptr) {
        shadow->call();
    }
    */
}

render_target* render_target::create(std::function<void(render_target&)> draw_func, ivec2 size, std::vector<texture_format> fb_format, std::vector<texture_attachment> fb_attachment, std::vector<int> fb_binding) {
    axiom::render_system& rs = ecs.get_system<axiom::render_system>();

    rs.targets.emplace_back(std::unique_ptr<axiom::render_target>(new axiom::render_target));

    render_target* target = rs.targets.back().get();

    target->draw = draw_func;
    target->size = size;

    std::vector<fb_tex_params> params;
    uint counter = 0;

    for(texture_format format : fb_format) {
        fb_tex_params param;
        param.format = fb_format[counter];
        param.attachment = fb_attachment[counter];

        if(fb_binding.size()) param.binding = fb_binding[counter];
        else param.binding = counter;

        params.push_back(param);

        ++counter;
    }
    target->framebuffer = std::move(axiom::framebuffer(size, std::move(params)));

    return target;
}

}