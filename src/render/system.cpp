#include <render/system.hpp>
#include <graphicsh.hpp>
#include <render/mesh/mesh.hpp>

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
        ivec2 size = win->size;

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

    for(auto& target : targets) target.call();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win->size.x, win->size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        if(win->target != nullptr) {
            vertices.init();

            std::vector<axiom::texture_vertex3d> vs = {
                axiom::texture_vertex3d(vec3(-1.0f, -1.0f, 0.5f), vec2(0.0f, 0.0f), vec4(1.0f), vec3(0.0f)),
                axiom::texture_vertex3d(vec3(1.0f, -1.0f, 0.5f), vec2(win->size.x, 0.0f), vec4(1.0f), vec3(0.0f)),
                axiom::texture_vertex3d(vec3(-1.0f, 1.0f, 0.5f), vec2(0.0f, win->size.y), vec4(1.0f), vec3(0.0f)),
                axiom::texture_vertex3d(vec3(1.0f, 1.0f, 0.5f), vec2(win->size.x, win->size.y), vec4(1.0f), vec3(0.0f)),
            };
            vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

            shaders["texture3d"].use();
            win->target->framebuffer.textures[0].bind(0);

            mat4 model_matrix = glm::identity<mat4>();
            mat4 view_matrix = glm::identity<mat4>();
            mat4 proj_matrix = glm::identity<mat4>();

            vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(axiom::texture_vertex3d), GL_STREAM_DRAW);

            vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), 0);
            vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 3);
            vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 5);
            vertices.add_vertex_attribute(3, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 9);

            vertices.bind();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glUniformMatrix4fv(0, 1, false, &model_matrix[0][0]);
            glUniformMatrix4fv(1, 1, false, &view_matrix[0][0]);
            glUniformMatrix4fv(2, 1, false, &proj_matrix[0][0]);
            glUniform1f(3, 0.0f);

            vertices.draw_vertices_triangles();
        }
    }

    //

    glfwSwapBuffers(win->window_handle);
}

void render_system::take_screenshot(std::string filepath) {
    ivec2 size = win->size;

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
}

axiom::shader& get_shader(std::string id) {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();

    return render_system.shaders[id];
}

axiom::texture& get_texture(std::string id) {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();

    return render_system.textures[id];
}

axiom::vertices& get_vertices() {
    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();

    return render_system.vertices;
}
    
}