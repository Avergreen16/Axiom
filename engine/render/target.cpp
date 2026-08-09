#include <render/wrapper.hpp>
#include <render/target.hpp>

namespace axiom {

void render_target::set_size(ivec2 new_size, ivec2 new_position) {
    size = new_size;
    position = new_position;
    framebuffer.resize(size);
}

void render_target::call() {
    framebuffer.bind();

    //
    
    glEnable(GL_BLEND);
    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_GEQUAL);
    //glDepthRange(0, 1);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    
    glViewport(0, 0, framebuffer.size.x, framebuffer.size.y);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(0.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //

    draw(*this);
}

render_target render_target::create(std::function<void(render_target&)> draw_func, ivec2 size, ivec2 position, std::vector<axiom::texture_format> fb_format, std::vector<axiom::texture_attachment> fb_attachment, std::vector<int> fb_binding) {
    render_target target;
    target.draw = draw_func;
    target.size = size;
    target.position = position;

    std::vector<axiom::fb_tex_params> params;
    uint counter = 0;

    for(axiom::texture_format format : fb_format) {
        axiom::fb_tex_params param;
        param.format = fb_format[counter];
        param.attachment = fb_attachment[counter];

        if(fb_binding.size()) param.binding = fb_binding[counter];
        else param.binding = counter;

        params.push_back(param);

        ++counter;
    }
    target.framebuffer = std::move(axiom::framebuffer(size, std::move(params)));

    return target;
}

}