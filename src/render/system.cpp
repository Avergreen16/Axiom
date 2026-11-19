#include <render/system.hpp>
#include <render/mesh/mesh.hpp>
#include <include/wrapper.hpp>

namespace axiom {

render_system::render_system(axiom::context* ctx_) {
    ctx = ctx_;
}

void render_system::process_inputs() {
    /*
    if(ctx->pressed_buttons.contains(axiom::input_code::KEY_F6)) { // screenshot
        ivec2 size = ctx->size;

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
    */
}

void render_system::call() {
    bind_default_framebuffer();
    set_viewport({0, 0, ctx->size.x, ctx->size.y});
    set_clear_color({0.0f, 0.0f, 0.0f, 1.0f});

    clear_framebuffer(true, true, false);

    for(auto& target : targets) target->call();

    //
    
    process_inputs();
}

#ifdef AXIOM_TAKE_SCREENSHOT
void render_system::take_screenshot(std::string filepath) {
    ivec2 size = ctx->size;

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
#endif

void render_init(axiom::context* ctx) {
    render_system system(ctx);
    ecs.register_system<render_system>(std::move(system));

    {
        axiom::signature sig = axiom::update_signature<axiom::transform3d>();
        axiom::update_signature<axiom::color_mesh3d>(sig);
        axiom::collector collector = axiom::collector(sig);
        axiom::ecs.create_collector("color_mesh", collector);
    }
    
    {
        axiom::signature sig = axiom::update_signature<axiom::transform3d>();
        axiom::update_signature<axiom::texture_mesh3d>(sig);
        axiom::collector collector = axiom::collector(sig);
        axiom::ecs.create_collector("texture_mesh", collector);
    }
    
    {
        axiom::signature sig = axiom::update_signature<axiom::transform3d>();
        axiom::update_signature<axiom::texture_range_mesh3d>(sig);
        axiom::collector collector = axiom::collector(sig);
        axiom::ecs.create_collector("texture_range_mesh", collector);
    }

    ctx->create_shaders();
}

axiom::shader& get_shader(std::string id) {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();

    return render_system.shaders[id];
}

axiom::texture& get_texture(std::string id) {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();

    return render_system.textures[id];
}
    
}