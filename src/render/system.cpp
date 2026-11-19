#include <render/system.hpp>
#include <render/mesh/mesh.hpp>
#include <include/wrapper.hpp>

namespace axiom {

render_system::render_system(axiom::context* ctx_) {
    ctx = ctx_;

    axiom::text_asset vert;
    axiom::text_asset geom;
    axiom::text_asset frag;
    axiom::texture_asset texasset;

    /*
    vert = axiom::text_asset::load(resource_root + "/shaders/ui.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/ui.frag");
    shaders.emplace("ui", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/ui_composite.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/ui_composite.frag");
    shaders.emplace("ui_composite", std::move(axiom::shader(vert, frag)));

    vert = axiom::text_asset::load(resource_root + "/shaders/grid2d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/grid2d.frag");
    shaders.emplace("grid2d", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/grid3d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/grid3d.frag");
    shaders.emplace("grid3d", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/color2d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/color2d.frag");
    shaders.emplace("color2d", std::move(axiom::shader(vert, frag)));
    
    vert = axiom::text_asset::load(resource_root + "/shaders/color3d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/color3d.frag");
    shaders.emplace("color3d", std::move(axiom::shader(vert, frag)));

    vert = axiom::text_asset::load(resource_root + "/shaders/colortex3d.vert");
    geom = axiom::text_asset::load(resource_root + "/shaders/colortex3d.geom");
    frag = axiom::text_asset::load(resource_root + "/shaders/colortex3d.frag");
    shaders.emplace("colortex3d", std::move(axiom::shader(vert, geom, frag)));

    vert = axiom::text_asset::load(resource_root + "/shaders/texture2d.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/texture2d.frag");
    shaders.emplace("texture2d", std::move(axiom::shader(vert, frag)));

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
    
    vert = axiom::text_asset::load(resource_root + "/shaders/glyph.vert");
    frag = axiom::text_asset::load(resource_root + "/shaders/glyph.frag");
    shaders.emplace("glyph", std::move(axiom::shader(vert, frag)));

    //

    texasset = axiom::texture_asset::load(resource_root + "/textures/ui.png");
    textures.emplace("ui", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));

    texasset = axiom::texture_asset::load(resource_root + "/textures/tilesheet.png");
    textures.emplace("tilesheet", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));
    
    texasset = axiom::texture_asset::load(resource_root + "/textures/test.png");
    textures.emplace("test", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));
    */
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
    static axiom::vertices vertices;

    //enable_blend();
    //enable_depth_test();

    //set_depth_func(axiom::depth_func::GREATER_EQUAL); //glDepthFunc(GL_GEQUAL);
    //set_clear_depth(0.0f); //glClearDepth(0.0f);
    //set_depth_range(0.0f, 1.0f); //glDepthRange(0, 1);
    //set_clip_control(); //glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    //disable_depth_clamp();
    //set_blend_func_separate(axiom::blend_func::SRC_ALPHA, axiom::blend_func::ONE_MINUS_SRC_ALPHA, axiom::blend_func::SRC_ALPHA, axiom::blend_func::ONE);
    
    //


    bind_default_framebuffer();
    set_viewport({0, 0, ctx->size.x, ctx->size.y});
    set_clear_color({0.0f, 0.0f, 0.0f, 1.0f});

    clear_framebuffer(true, true, false);

    for(auto& target : targets) target->call();
    draw_func();

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