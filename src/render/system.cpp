#include <render/system.hpp>
#include <graphicsh.hpp>

namespace axiom {

render_system::render_system(axiom::window* win_) {
    win = win_;

    axiom::text_asset vert;
    axiom::text_asset frag;
    axiom::texture_asset texasset;

    vert = axiom::text_asset::load(resource_root + "/shaders/ui.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/ui.frag");
    shaders.emplace("ui", std::move(axiom::shader(vert, frag)));

    vert = axiom::text_asset::load(resource_root + "/shaders/grid.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/grid.frag");
    shaders.emplace("grid", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/grid3d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/grid3d.frag");
    shaders.emplace("grid3d", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/color.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/color.frag");
    shaders.emplace("color", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/color3d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/color3d.frag");
    shaders.emplace("color3d", std::move(axiom::shader(vert, frag)));

    vert = axiom::text_asset::load(resource_root + "/shaders/texture3d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/texture3d.frag");
    shaders.emplace("texture3d", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/shadow.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/shadow.frag");
    shaders.emplace("shadow", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/test.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/test.frag");
    shaders.emplace("test", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/texture_range3d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/texture_range3d.frag");
    shaders.emplace("texture_range3d", std::move(axiom::shader(vert, frag)));

    //

    texasset = axiom::texture_asset::load(resource_root + "/textures/ui.png");
    textures.emplace("ui", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));

    texasset = axiom::texture_asset::load(resource_root + "/textures/tilesheet.png");
    textures.emplace("tilesheet", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));
    
    texasset = axiom::texture_asset::load(resource_root + "/textures/test.png");
    textures.emplace("test", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));

    vertices.init();
}

void render_system::process_inputs() {
    if(win->pressed_buttons.contains(axiom::input_code::KEY_F6)) { // screenshot
        ivec2 size = win->viewport_size;

        std::vector<byte> pixels(size.x * size.y * 4);

        glReadPixels(
            0, 0,
            size.x, size.y,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels.data()
        );

        axiom::texture_asset asset = axiom::texture_asset::load(pixels, size, 4);

        ulong timestamp = axiom::get_timestamp();
        asset.save(output_root + "/screenshot" + std::to_string(timestamp) + ".png");
    }

    if(win->pressed_buttons.contains(axiom::input_code::KEY_F11)) { // fullscreen
        if(win->is_fullscreen()) win->restore();
        else {
            win->make_fullscreen();
        }
    }
}

void render_system::call() {
    process_inputs();

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glClearDepth(0.0f);
    glDepthRange(0, 1);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glDisable(GL_DEPTH_CLAMP);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    
    //

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win->viewport_size.x, win->viewport_size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //

    glfwSwapBuffers(win->window_handle);
}

void render_system::take_screenshot(std::string filepath) {
    ivec2 size = win->viewport_size;

    std::vector<byte> pixels(size.x * size.y * 4);

    glReadPixels(
        0, 0,
        size.x, size.y,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );

    axiom::texture_asset asset = axiom::texture_asset::load(pixels, size, 4);

    ulong timestamp = axiom::get_timestamp();
    asset.save(filepath);
}

void render_init(axiom::window* window) {
    render_system system(window);
    ecs.register_system<render_system>(std::move(system));
}
    
}