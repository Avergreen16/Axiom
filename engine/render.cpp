#include "render.hpp"
#include "gui.hpp"
#include "input.hpp"
#include "billboard.hpp"
#include "utility.hpp"
#include "voxel.hpp"

float ellipsoid_height(vec3 pos, vec3 radii) {
    return sqrt(1.0f / (pos.x * pos.x / (radii.x * radii.x) + pos.y * pos.y / (radii.y * radii.y) + pos.z * pos.z / (radii.z * radii.z)));
}


std::function<void(Render_Target&)> target_func = [](Render_Target& render_target) {
    static ivec2 prev_size = ivec2(0);

    Render_system& render_system = ecs.get_system<Render_system>();
    Physics_system& ps = ecs.get_system<Physics_system>();
    Input_system& input_system = ecs.get_system<Input_system>();
    Voxel_system& voxel_system = ecs.get_system<Voxel_system>();

    // update camera position
    double rel_time = core.prev_time - core.start_time;
    uint32_t camera_entity = input_system.player_camera;
    Camera& camera_camera = ecs.get_component<Camera>(camera_entity);
    Transform& camera_transform = ecs.get_component<Transform>(camera_entity);

    Transform& sun_transform = ecs.get_component<Transform>(render_system.sun_camera);
    Camera& sun_cam = ecs.get_component<Camera>(render_system.sun_camera);
    
    // get octree center
    pvec3 octree_center = camera_transform.position;
    if(input_system.player_collider != NULL_ENTITY) octree_center = ecs.get_component<Transform>(input_system.player_collider).position;

    if(prev_size != render_target.target_size) {
        prev_size = render_target.target_size;

        for(uint32_t f : render_target.framebuffers) {
            render_system.framebuffers[f].resize(prev_size);
        }
    }
    for(uint32_t f : render_target.framebuffers) {
        render_system.bind_framebuffer(f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    render_system.bind_framebuffer(render_target.framebuffers[0]);

    sun_transform.position = camera_transform.position;

    float threshold = pow(2.0f, 60.0f);
    vec3 sun_dir = vec3(render_system.sun_pos - camera_transform.position);
    if(abs(sun_dir.x) > threshold || abs(sun_dir.y) > threshold || abs(sun_dir.z) > threshold) sun_dir /= threshold;
    sun_dir = normalize(vec3(sun_dir));
    Transform sun_pos2;

    mat3 sun_ori = create_rot_mat(vec3(0, 0, 1), sun_dir);

    sun_transform.orientation = sun_ori;
    sun_pos2.position = sun_dir;
    sun_pos2.orientation = sun_ori;

    float dist = 128;
    vec2 aspect = render_target.target_size;
    float f = tan(camera_camera.fov * 0.5f * (M_PI / 180));
    vec3 corner = vec3(f, f * (aspect.y / aspect.x), -1.0f);
    corner *= dist;
    std::vector<vec3> vs = {
        vec3(0, 0, 0),
        vec3(corner.x, corner.y, corner.z),
        vec3(-corner.x, corner.y, corner.z),
        vec3(corner.x, -corner.y, corner.z),
        vec3(-corner.x, -corner.y, corner.z),
    };
    
    vec3 minimum = vec3(__FLT_MAX__);
    vec3 maximum = vec3(-__FLT_MAX__);

    std::vector<vec3> vvs;

    for(vec3& v : vs) {
        v = camera_transform.orientation * v;
        
        v = transpose(sun_ori) * v;

        minimum.x = min(minimum.x, v.x);
        minimum.y = min(minimum.y, v.y);
        minimum.z = min(minimum.z, v.z);
        maximum.x = max(maximum.x, v.x);
        maximum.y = max(maximum.y, v.y);
        maximum.z = max(maximum.z, v.z);
    }

    vec3 size_v = (maximum - minimum) * 0.5f;
    vec3 center = size_v + minimum;
    float min_max = dist;

    //sun_transform.position += sun_ori * center;
    sun_cam.near_plane = -size_v.z;
    sun_cam.far_plane = size_v.z;
    vec2 wh = vec2(size_v.x * 2, size_v.y * 2);

    pvec3 ring_pos;
    mat4 p_ori;

    pvec3 sun_position = render_system.sun_pos;

    uint32_t seed = 0xEEEE;

    for(uint32_t camera : render_system.collectors[2].entities) {
        Camera& camera_camera = ecs.get_component<Camera>(camera);
        Transform& camera_pos = ecs.get_component<Transform>(camera);

        render_system.rel_center = vec3(render_system.atmo_center - camera_pos.position);
        render_system.rel_sun = normalize(vec3(render_system.sun_pos - camera_pos.position) - render_system.rel_center);

        uvec2 size = render_target.target_size;

        if(camera_camera.orthogonal) camera_camera.proj = core.get_infinite_proj_matrix_ortho(size, camera_camera.near_plane, camera_camera.far_plane, 1.0f, 0.0f, wh.x, wh.y);
        else camera_camera.proj = core.get_infinite_proj_matrix(size, camera_camera.fov, camera_camera.near_plane, 1.0f, 0.0f);
        
        render_system.bind_framebuffer(camera_camera.framebuffer);

        for(uint32_t entity : render_system.collectors[4].entities) {
            render_system.render_entity(entity, camera, sun_pos2.position);
        }

        for(uint32_t entity : render_system.collectors[3].entities) {
            render_system.render_billboard(entity, camera, sun_pos2.position);
        }

        render_system.render_debug_lines(camera);

        pvec3 planet_pos = pvec3(float(base_radius) * normalize(vec3(1.0f, 1.0f, 0.0f)) * 250.0f) + sun_position;
        Transform gg_transform;
        
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }
    
    std::array<float, 2> scatter = render_system.get_scatter(-render_system.rel_center, 30000.0f, 1.0477 * 0x100000, render_system.rel_sun);
    render_system.render_particles(camera_entity, scatter[0], scatter[1], prev_size);

    vec3 sun_p = vec3(sun_position - camera_transform.position);

    {   
        vec3 color = get_color_hsv(0.45f / 6.0f, 0.8f, 1.0f);
        vec3 size = vec3(75.6f * 0x100000); // mass is 0.65 solar masses

        std::shared_ptr<Mesh> mesh = core.meshes["planet"];            

        mat4 view = mat4(transpose(camera_transform.orientation));

        double time = core.prev_time;

        glm::mat4 model_0 = rotate(float(time * M_PI * 2) * 0.005f, vec3(0, 0, 1)) * scale(size); // orientation
        glm::mat4 model_1 = core.get_model_matrix(sun_position, camera_transform.position);

        mat4 model = model_1 * model_0;// * model_2;

        core.shaders["sun_shader"]->use();

        core.textures["density_map_perlin"]->bind(0);
        core.textures["density_map_voronoi2"]->bind(1);

        mesh->buffer->bind();

        glUniformMatrix4fv(0, 1, false, &camera_camera.proj[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &model[0][0]);
        //glUniform3fv(3, 1, &light_direction[0]);
        glUniform3f(4, 0.0f, 0.0f, 0.0f);
        glUniform1f(5, 0.0f);
        glUniform1d(6, time);
        glUniform3fv(7, 1, &color[0]);

        mesh->buffer->draw_indices(GL_TRIANGLES);
    }
    // shadows

    // ray traced volumetrics

    // galaxy
    
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE); 
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    {   
        pnum light_year = pow(2, 48);

        std::vector<mat4> galaxies(8);
        pvec3 galaxy_pos;
        mat4 model;
        
        galaxy_pos = pvec3(0.0p);
        model = core.get_model_matrix(galaxy_pos, camera_transform.position);
        galaxies[0] = model;
    
        vec3 color = vec3(1.0f, 0.2f, 0.85f);
        vec3 core_size = float(light_year) * vec3(600.0f, 600.0f, 450.0f);

        mat4 view = mat4((transpose(camera_transform.orientation)));
        vec3 bounding_box = float(light_year) * vec3(2000.0f, 2000.0f, 450.0f);

        core.shaders["galaxy_shader"]->use();

        glUniformMatrix4fv(0, 1, false, &camera_camera.proj[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &model[0][0]);
        glUniform3fv(3, 1, &bounding_box[0]);
        glUniform1f(4, scatter[0]);
        glUniform3fv(5, 1, &color.x);
        glUniform3fv(6, 1, &core_size.x);
        glUniformMatrix4fv(7, 8, false, &galaxies[0][0][0]);

        render_system.framebuffers[0].textures[3].bind(0);
        core.textures["density_map_perlin"]->bind(1);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    //

    Camera& camera_player = ecs.get_component<Camera>(render_system.player_camera);
    Transform& transform_player = ecs.get_component<Transform>(render_system.player_camera);
    Camera& camera_sun = ecs.get_component<Camera>(render_system.sun_camera);
    Transform& transform_sun = ecs.get_component<Transform>(render_system.sun_camera);

    mat4 sun_view = mat4(transpose(transform_sun.orientation));
    mat4 player_view = mat4(transpose(transform_player.orientation));

    vec3 rel_pos = transform_sun.position - transform_player.position;
    vec3 light_pos = sun_pos2.position - transform_player.position;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    render_system.bind_framebuffer(render_target.framebuffers[0]);
    glBlendFunc(GL_ONE, GL_ZERO);

    core.shaders["screen_depth_shader"]->use();
    render_system.framebuffers[0].textures[0].bind(0);
    render_system.framebuffers[0].textures[3].bind(1);
    render_system.framebuffers[0].textures[1].bind(2);
    render_system.framebuffers[0].textures[2].bind(3);
    render_system.framebuffers[2].textures[2].bind(4);
    render_system.framebuffers[2].textures[1].bind(5);
    core.textures["density_map_perlin"]->bind(6);

    glUniformMatrix4fv(0, 1, false, &camera_player.proj[0][0]);
    glUniformMatrix4fv(1, 1, false, &player_view[0][0]);

    glUniformMatrix4fv(2, 1, false, &camera_sun.proj[0][0]);
    glUniformMatrix4fv(3, 1, false, &sun_view[0][0]);
    glUniform3fv(4, 1, &rel_pos.x);
    glUniform3fv(5, 1, &light_pos.x);
    
    glUniform3fv(6, 1, &sun_p.x);
    //glUniformMatrix4fv(7, 1, false, &disc_matrix[0][0]);
    //glUniform2f(8, min_ring, max_ring);
    //glUniformMatrix4fv(9, 1, false, &moon_matrix[0][0]);
    glUniform1ui(10, seed);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE); 

    //

    //pvec3 torus_position = pvec3(normalize(vec3(1.0f, 1.0, 0.0)) * astronomical_unit);
    //mat4 torus_matrix = mat4(transpose(torus_ori)) * core.get_model_matrix_inv(torus_position, camera_transform.position);

    pvec3 earth_position = render_system.atmo_center;
    mat4 earth_matrix = core.get_model_matrix_inv(earth_position, camera_transform.position);
    
    // rings
    
    core.shaders["ray_shader"]->use();

    render_system.framebuffers[0].textures[3].bind(0);
    core.textures["density_map_perlin"]->bind(1);
    core.textures["density_map_voronoi"]->bind(2);
    
    glUniformMatrix4fv(0, 1, false, &camera_player.proj[0][0]);
    glUniformMatrix4fv(1, 1, false, &player_view[0][0]);

    glUniform3fv(2, 1, &sun_p.x);
    //glUniformMatrix4fv(3, 1, false, &disc_matrix[0][0]);
    //glUniform2f(4, min_ring, max_ring);
    //glUniformMatrix4fv(5, 1, false, &moon_matrix[0][0]);
    glUniform1ui(6, seed);
    glUniform1d(7, rel_time);
    //glUniformMatrix4fv(8, 1, false, &torus_matrix[0][0]);
    glUniformMatrix4fv(9, 1, false, &earth_matrix[0][0]);

    float planet_radius = 1.0477f * 0x100000;
    glUniform3f(10, planet_radius, planet_radius, planet_radius);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    if(input_system.debug_mode) render_system.render_grid(render_system.player_camera);  
    //if(input_system.debug_mode) render_star_octree(octree_center, camera_entity);

    glDisable(GL_DEPTH_TEST);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE); 

    // icons
    render_system.render_icons(camera_entity, prev_size);
    if(input_system.debug_mode) render_system.render_debug(camera_entity);

    if(core.cursor_disabled) render_system.render_crosshair(prev_size);
};

Render_system::Render_system() {
    Signature s = ecs.update_signature<Transform>();
    collectors.push_back(Collector(s, false));

    // placeholder
    s = ecs.update_signature<Transform>();
    collectors.push_back(Collector(s, false));

    s = ecs.update_signature<Camera>();
    ecs.update_signature<Transform>(s);
    collectors.push_back(Collector(s, false));

    s = ecs.update_signature<Transform>();
    ecs.update_signature<Billboard_animation>(s);
    collectors.push_back(Collector(s, false));

    s = ecs.update_signature<Mesh_component>();
    ecs.update_signature<Transform>(s);
    collectors.push_back(Collector(s, false));   
        
    // 5
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));

    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));
    
    s = ecs.update_signature<Transform>();
    collectors.push_back(Collector(s, false)); // placeholder
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));
    
    // 10
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));  
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));

    // 15
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));   
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));
    
    s = ecs.update_signature<Transform>(); // placeholder
    collectors.push_back(Collector(s, false));

    framebuffers.emplace_back(Framebuffer({start_x, start_y}, {{{GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}, GL_COLOR_ATTACHMENT0, 0}, {{GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, GL_COLOR_ATTACHMENT1, 1}, {{GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, GL_COLOR_ATTACHMENT2, 2}, {{GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT}, GL_DEPTH_ATTACHMENT}}));
    framebuffers.emplace_back(Framebuffer({start_x, start_y}, {{{GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}, GL_COLOR_ATTACHMENT0, 0}, {{GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT}, GL_DEPTH_ATTACHMENT}}));
    framebuffers.emplace_back(Framebuffer({4096, 4096}, {{{GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, GL_COLOR_ATTACHMENT1, 1}, {{GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, GL_COLOR_ATTACHMENT2, 2}, {{GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT}, GL_DEPTH_ATTACHMENT}}));
    //framebuffers.emplace_back(Framebuffer({800, 600}, {{{GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}, GL_COLOR_ATTACHMENT0, 0}, {{GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT}, GL_DEPTH_ATTACHMENT}}));
    //framebuffers.emplace_back(Framebuffer({800, 600}, {{{GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}, GL_COLOR_ATTACHMENT0, 0}, {{GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT}, GL_DEPTH_ATTACHMENT}}));
    //framebuffers.emplace_back(Framebuffer({800, 600}, {{{GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}, GL_COLOR_ATTACHMENT0, 0}, {{GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT}, GL_DEPTH_ATTACHMENT}}));

    framebuffer_link = {0, 0, 0};

    Random random(5);
    for(int i = 0; i < 64; ++i) {
        vec3 v = random.unit_vector();
        if(v.z < 0) v.z = -v.z;

        ssao_samples.push_back(v);
    }

    Render_Target target;
    target.func = std::move(target_func);
    target.framebuffers = {0, 1};
    targets.push_back(std::move(target));
}

void Render_system::resize_framebuffers() {
    for(int i = 0; i < framebuffers.size(); ++i) {
        float link = framebuffer_link[i];

        if(link > 0) {
            framebuffers[i].resize((vec2)screen_size * link);
        }
    }
}

void Render_system::bind_framebuffer(uint32_t i) {
    framebuffers[i].bind();
    glViewport(0, 0, framebuffers[i].textures[0].size.x, framebuffers[i].textures[0].size.y);
}

void Render_system::bind_default_framebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, core.window.viewport_size.x, core.window.viewport_size.y);
}

/*
glm::vec3 pos;
glm::vec4 color;
glm::vec2 size;
glm::vec2 tex_coord;
glm::vec2 tex_size;
glm::vec3 orient_center;
uint32_t settings;
*/
void Render_system::render_particles(uint32_t camera, float scatter, float atmo, ivec2 size) {
    if(!vv->initialized) vv->init();
    
    std::vector<Particle_data> particle_data;

    Transform& tf = ecs.get_component<Transform>(camera);
    Camera& cm = ecs.get_component<Camera>(camera);

    std::string ps_name = typeid(Particle_system).name();
    Particle_system& ps = ecs.get_system<Particle_system>();

    mat3 view = transpose(tf.orientation);
        
    std::vector<Particle_sort> indices(ps.particles.size());
    for(uint32_t i = 0; i < ps.particles.size(); ++i) {
        indices[i] = {i, 1.0};
    }

    //std::sort(indices.begin(), indices.end());

    for(Particle_sort index : indices) {
        Particle& p = ps.particles[index.id];
        float frac_lifetime = 0.0f;
        if(p.max_lifetime != 0.0f) p.current_lifetime / p.max_lifetime;
        glm::vec2 size = p.start_size + (p.end_size - p.start_size) * frac_lifetime;
        glm::vec4 color = p.start_color + (p.end_color - p.start_color) * frac_lifetime;
        vec3 position = p.position - tf.position;

        particle_data.push_back(Particle_data(position, color, size, p.tex_coord, p.tex_size, p.settings, p.orient_center));
    }

    for(Star star : ps.stars) {
        float new_brightness = clamp((int)floor(star.brightness - scatter * 8.0f), -1, 2);
        float ii = clamp(1.0f - (atmo - 0.25f) * 2.0f, 0.0f, 1.0f);

        if(new_brightness != -1) {
            vec4 tex = ps.star_sprites[new_brightness];
            
            Particle& p = star.particle;
            p.start_size = tex.zw() * 2.0f;
            p.end_size = tex.zw() * 2.0f;
            p.tex_coord = tex.xy();
            p.tex_size = tex.zw();

            float frac_lifetime = 0.0f;
            if(p.max_lifetime != 0.0f) p.current_lifetime / p.max_lifetime;
            glm::vec2 size = p.start_size + (p.end_size - p.start_size) * frac_lifetime;
            glm::vec4 color = (p.start_color + (p.end_color - p.start_color) * frac_lifetime) * ii + vec4(1.0f) * (1.0f - ii);
            vec3 position = p.position - tf.position;

            particle_data.push_back(Particle_data(position, color, size, p.tex_coord, p.tex_size, p.settings, p.orient_center));
        }
    }
    
    //glDisable(GL_DEPTH_TEST);

    vv->vertex_buffer_data(particle_data.data(), particle_data.size(), sizeof(Particle_data), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Particle_data), 0);
    vv->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 3);
    vv->add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 7);
    vv->add_vertex_attribute(3, 2, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 9);
    vv->add_vertex_attribute(4, 2, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 11);
    vv->add_vertex_attribute(5, 3, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 13);
    vv->add_vertex_attribute(6, 1, GL_INT, false, sizeof(Particle_data), sizeof(float) * 16);
    
    std::shared_ptr<Shader> particle_shader = core.shaders["particle_shader"];
    std::shared_ptr<Texture> particle_texture = core.textures["particles"];

    mat4 view_4 = mat4(view);

    particle_shader->use();
    vv->bind();

    particle_texture->bind(1);
    glUniformMatrix4fv(0, 1, false, &view_4[0][0]);
    glUniformMatrix4fv(1, 1, false, &cm.proj[0][0]);
    glUniform2i(3, size.x, size.y);

    vv->draw_vertices(GL_POINTS);
}

void Render_system::render_icons(uint32_t camera, ivec2 size) {
    if(!vv->initialized) vv->init();
    
    std::vector<Particle_data> particle_data;

    Transform& tf = ecs.get_component<Transform>(camera);
    Camera& cm = ecs.get_component<Camera>(camera);

    std::string ps_name = typeid(Particle_system).name();
    Particle_system& ps = ecs.get_system<Particle_system>();

    mat3 view = transpose(tf.orientation);
        
    std::vector<Particle_sort> indices(ps.icons.size());
    for(uint32_t i = 0; i < ps.icons.size(); ++i) {
        indices[i] = {i, 1.0};
    }

    for(Particle_sort index : indices) {
        Particle& p = ps.icons[index.id];
        float frac_lifetime = 0.0f;
        if(p.max_lifetime != 0.0f) p.current_lifetime / p.max_lifetime;
        
        glm::vec2 size = p.start_size + (p.end_size - p.start_size) * frac_lifetime;
        glm::vec4 color = p.start_color + (p.end_color - p.start_color) * frac_lifetime;
        vec3 position = p.position - tf.position;

        particle_data.push_back(Particle_data(position, color, size, p.tex_coord, p.tex_size, p.settings, p.orient_center));
    }
    
    //glDisable(GL_DEPTH_TEST);

    vv->vertex_buffer_data(particle_data.data(), particle_data.size(), sizeof(Particle_data), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Particle_data), 0);
    vv->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 3);
    vv->add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 7);
    vv->add_vertex_attribute(3, 2, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 9);
    vv->add_vertex_attribute(4, 2, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 11);
    vv->add_vertex_attribute(5, 3, GL_FLOAT, false, sizeof(Particle_data), sizeof(float) * 13);
    vv->add_vertex_attribute(6, 1, GL_INT, false, sizeof(Particle_data), sizeof(float) * 16);
    
    std::shared_ptr<Shader> particle_shader = core.shaders["particle_shader"];
    std::shared_ptr<Texture> particle_texture = core.textures["particles"];

    mat4 view_4 = mat4(view);

    particle_shader->use();
    vv->bind();

    particle_texture->bind(1);
    glUniformMatrix4fv(0, 1, false, &view_4[0][0]);
    glUniformMatrix4fv(1, 1, false, &cm.proj[0][0]);
    glUniform2i(3, size.x, size.y);

    vv->draw_vertices(GL_POINTS);

    ps.icons.clear();
}

void Render_system::render_entity(uint32_t entity, uint32_t camera, pvec3 light_pos) {
    glEnable(GL_CULL_FACE);
    Mesh_component& mc = ecs.get_component<Mesh_component>(entity);
    Transform& t = ecs.get_component<Transform>(entity);

    if(!mc.mesh->skip) {
        Transform& camera_transform = ecs.get_component<Transform>(camera);
        Camera& camera_camera = ecs.get_component<Camera>(camera);

        pvec3 apos;

        bool is_torus = false;

        float darkness_value = 0.5f;

        apos = t.position;

        mat4 view = mat4(transpose(camera_transform.orientation));

        glm::mat4 model_0 = t.orientation;
        glm::mat4 model_1 = core.get_model_matrix(t.position, camera_transform.position);
        //glm::mat4 model_2 = scale(vec3(2, 2, 2));

        mat4 model = model_1 * model_0;// * model_2;

        if(ecs.has_component<Color_map>(entity)) {
            Color_map& cm = ecs.get_component<Color_map>(entity);

            core.shaders["color_map"]->use();

            mc.texture->bind(0);
            cm.texture->bind(1);

            mc.mesh->buffer->bind();

            vec3 apos2 = apos;

            glUniformMatrix4fv(0, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(1, 1, false, &view[0][0]);
            glUniformMatrix4fv(2, 1, false, &model[0][0]);
            glUniform3fv(3, 1, &light_direction[0]);
            glUniform3fv(4, 1, &apos2[0]);
            glUniform1f(5, darkness_value);
            glUniform3fv(6, 1, &cm.center_offset.x);

            if(mc.cull) glDisable(GL_CULL_FACE);

            mc.mesh->buffer->draw_indices(GL_TRIANGLES);
            
            if(mc.cull) glEnable(GL_CULL_FACE);
        } else {


            core.shaders["model_shader"]->use();

            mc.texture->bind(0);

            mc.mesh->buffer->bind();
            
            //if(mc.mesh->color != vec3(1.0)) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            vec3 apos2 = apos;

            glUniformMatrix4fv(0, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(1, 1, false, &view[0][0]);
            glUniformMatrix4fv(2, 1, false, &model[0][0]);
            glUniform3fv(3, 1, &light_direction[0]);
            glUniform3fv(4, 1, &apos2[0]);
            glUniform1f(5, darkness_value);
            glUniform3fv(6, 1, &mc.mesh->color.x);

            float planet_radius = 1.0477f * 0x100000;

            glUniform3fv(7, 1, &rel_center[0]);
            glUniform3fv(8, 1, &rel_sun[0]);
            glUniform3f(9, planet_radius, planet_radius, planet_radius);
            glUniform1f(10, 30000.0f);

            glUniform1f(11, 0.0f);


            //if(mc.cull) glDisable(GL_CULL_FACE);

            mc.mesh->buffer->draw_indices(GL_TRIANGLES);
            
            //if(mc.cull) glEnable(GL_CULL_FACE);
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
}

void Render_system::render_debug(uint32_t camera) {
    glLineWidth(1);
    
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    Camera& camera_camera = ecs.get_component<Camera>(camera);

    mat4 view = mat4(transpose(camera_transform.orientation));

    Physics_system& ps = ecs.get_system<Physics_system>();

    for(uint32_t entity : ps.collectors[0].entities) {
        Collider& collider = ecs.get_component<Collider>(entity);
        Transform& t = ecs.get_component<Transform>(entity);

        glm::mat4 model_1 = core.get_model_matrix(t.position, camera_transform.position);
        mat4 view = mat4(transpose(camera_transform.orientation));

        if(collider.allow_rotation) {
            glDisable(GL_DEPTH_TEST);

            vec3 angular_velocity = collider.angular_momentum;

            std::vector<vec3> points = {vec3(0, 0, 0), angular_velocity};
            vv->vertex_buffer_data(points.data(), points.size(), sizeof(vec3), GL_STREAM_DRAW);
            vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);

            core.shaders["color_shader"]->use();

            vv->bind();

            glUniformMatrix4fv(0, 1, false, &view[0][0]);
            glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(2, 1, false, &model_1[0][0]);
            glUniform4f(3, 1.0f, 0.25f, 1.0f, 1.0f);

            vv->draw_vertices(GL_LINES);

            glEnable(GL_DEPTH_TEST);
        }

        if(!collider.is_static) {
            glDisable(GL_DEPTH_TEST);

            std::vector<vec3> points = {vec3(0, 0, 0), collider.velocity};
            vv->vertex_buffer_data(points.data(), points.size(), sizeof(vec3), GL_STREAM_DRAW);
            vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);

            core.shaders["color_shader"]->use();

            vv->bind();

            glUniformMatrix4fv(0, 1, false, &view[0][0]);
            glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(2, 1, false, &model_1[0][0]);
            glUniform4f(3, 1.0f, 1.0f, 0.25f, 1.0);

            vv->draw_vertices(GL_LINES);

            glEnable(GL_DEPTH_TEST);
        }
    }
    std::vector<vec3> points_v;
    std::vector<vec3> points_r;
    std::vector<vec3> points_n;

    float scale = 0.0125f;
    glDisable(GL_DEPTH_TEST);

    glm::mat4 model = core.get_model_matrix(vec3(0.0f), camera_transform.position);

    for(Collision_constraint cc : ps.collision_constraints) {
        for(col_constraint& constraint : cc.constraints) {
            Contact_point cp = constraint.constraint_point;
            float length = 0.0625;

            pvec3 pb;
            if(cc.b == NULL_ENTITY) pb = cp.b + cc.ta->position;
            else pb = cp.b + cc.tb->position;
            
            glm::mat4 model_1 = core.get_model_matrix(pb, camera_transform.position);
            
            std::vector<vec3> normal = {vec3(0, 0, 0), constraint.normal * length};
            std::vector<vec3> tangent = {vec3(0, 0, 0), constraint.tangent * length, vec3(0, 0, 0), constraint.bitangent * length};
            
            glDisable(GL_DEPTH_TEST);

            vv->vertex_buffer_data(normal.data(), normal.size(), sizeof(vec3), GL_STREAM_DRAW);
            vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);

            core.shaders["color_shader"]->use();

            vv->bind();

            glUniformMatrix4fv(0, 1, false, &view[0][0]);
            glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(2, 1, false, &model_1[0][0]);
            glUniform4f(3, 1.0f, 0.25f, 0.25f, 1.0);

            vv->draw_vertices(GL_LINES);

            
            vv->vertex_buffer_data(tangent.data(), tangent.size(), sizeof(vec3), GL_STREAM_DRAW);
            vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);

            core.shaders["color_shader"]->use();

            vv->bind();

            glUniformMatrix4fv(0, 1, false, &view[0][0]);
            glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(2, 1, false, &model_1[0][0]);
            glUniform4f(3, 0.25f, 0.25f, 1.0f, 1.0);

            vv->draw_vertices(GL_LINES);

            model_1 = core.get_model_matrix(cp.a + cc.ta->position, camera_transform.position);
            
            normal = {vec3(0, 0, 0), constraint.normal * length};
            tangent = {vec3(0, 0, 0), constraint.tangent * length, vec3(0, 0, 0), constraint.bitangent * length};

            vv->vertex_buffer_data(normal.data(), normal.size(), sizeof(vec3), GL_STREAM_DRAW);
            vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);

            core.shaders["color_shader"]->use();

            vv->bind();

            glUniformMatrix4fv(0, 1, false, &view[0][0]);
            glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(2, 1, false, &model_1[0][0]);
            glUniform4f(3, 1.0f, 1.0f, 0.25f, 1.0);

            vv->draw_vertices(GL_LINES);

            
            vv->vertex_buffer_data(tangent.data(), tangent.size(), sizeof(vec3), GL_STREAM_DRAW);
            vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);

            core.shaders["color_shader"]->use();

            vv->bind();

            glUniformMatrix4fv(0, 1, false, &view[0][0]);
            glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
            glUniformMatrix4fv(2, 1, false, &model_1[0][0]);
            glUniform4f(3, 1.0f, 0.25f, 1.0f, 1.0);

            vv->draw_vertices(GL_LINES);
            
            glEnable(GL_DEPTH_TEST);
        }
    }
    core.shaders["color_shader"]->use();
    glUniformMatrix4fv(0, 1, false, &view[0][0]);
    glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
    glUniformMatrix4fv(2, 1, false, &model[0][0]);

    // normal
    
    vv->vertex_buffer_data(points_n.data(), points_n.size(), sizeof(vec3), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);
    vv->bind();

    glUniform4f(3, 1.0f, 0.25f, 0.25f, 1.0);

    vv->draw_vertices(GL_LINES);

    // velocity

    vv->vertex_buffer_data(points_v.data(), points_v.size(), sizeof(vec3), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);
    vv->bind();

    glUniform4f(3, 1.0f, 1.0f, 0.25f, 1.0);

    vv->draw_vertices(GL_LINES);

    // rotation

    vv->vertex_buffer_data(points_r.data(), points_r.size(), sizeof(vec3), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);
    vv->bind();

    glUniform4f(3, 1.0f, 0.25f, 1.0f, 1.0);

    vv->draw_vertices(GL_LINES);
    

    glEnable(GL_DEPTH_TEST);
}

struct Entity_uniform_buffer_data {
    mat4 matrix;
    vec4 apos;
};

float hash_float(uint32_t seed) {
    return to_float(hash(seed)) * 0.5f + 0.5f;
}

void Render_system::call() {
    Physics_system& ps = ecs.get_system<Physics_system>();
    Input_system& input_system = ecs.get_system<Input_system>();
    Voxel_system& voxel_system = ecs.get_system<Voxel_system>();

    // update camera position
    double rel_time = core.prev_time - core.start_time;
    uint32_t camera_entity = input_system.player_camera;
    Camera& camera_camera = ecs.get_component<Camera>(camera_entity);
    Transform& camera_transform = ecs.get_component<Transform>(camera_entity);

    Transform& sun_transform = ecs.get_component<Transform>(sun_camera);
    Camera& sun_cam = ecs.get_component<Camera>(sun_camera);
    
    // get octree center
    pvec3 octree_center = camera_transform.position;
    if(input_system.player_collider != NULL_ENTITY) octree_center = ecs.get_component<Transform>(input_system.player_collider).position;

    glDepthFunc(GL_GEQUAL);
    glClearDepth(0.0f);
    glDepthRange(0, 1);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glDisable(GL_DEPTH_CLAMP);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

    if(uvec2(core.window.viewport_size) != screen_size) {
        screen_size = core.window.viewport_size;

        resize_framebuffers();
    }
    
    for(int i = 0; i < framebuffers.size(); ++i) {
        bind_framebuffer(i);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    bind_default_framebuffer();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(Render_Target& target : targets) target.func(target);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    bind_default_framebuffer();
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

    GUI_system& gui = ecs.get_system<GUI_system>();
    Physics_system& physics_system = ecs.get_system<Physics_system>();

    render_gui();

    render_cursor();
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glfwSwapBuffers(core.window.window);
}

void Render_system::render_cursor() {
    Input_system& input_system = ecs.get_system<Input_system>();
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    ivec2 size;
    ivec4 tex_range;
    ivec2 rel_pos;

    switch(gui_system.cursor_mode) {
        case CURSOR_CLICK: 
            size = {10, 16};
            tex_range = {0, 0, 5, 8};
            rel_pos = {0, 0};
            break;
        case CURSOR_DRAG_T: 
            size = {10, 18};
            tex_range = {0, 8, 5, 9};
            rel_pos = {-5, 9};
            break;
        case CURSOR_DRAG_TR: 
            size = {14, 14};
            tex_range = {0, 22, 7, 7};
            rel_pos = {-7, 7};
            break;
        case CURSOR_DRAG_R: 
            size = {18, 10};
            tex_range = {0, 17, 9, 5};
            rel_pos = {-9, 5};
            break;
        case CURSOR_DRAG_BR: 
            size = {14, 14};
            tex_range = {0, 29, 7, 7};
            rel_pos = {-7, 7}; 
            break;
        case CURSOR_DRAG_B: 
            size = {10, 18};
            tex_range = {0, 8, 5, 9};
            rel_pos = {-5, 9};
            break;
        case CURSOR_DRAG_BL: 
            size = {14, 14};
            tex_range = {0, 22, 7, 7};
            rel_pos = {-7, 7};
            break;
        case CURSOR_DRAG_L: 
            size = {18, 10};
            tex_range = {0, 17, 9, 5};
            rel_pos = {-9, 5};
            break;
        case CURSOR_DRAG_TL:
            size = {14, 14};
            tex_range = {0, 29, 7, 7};
            rel_pos = {-7, 7}; 
            break;
        case CURSOR_TEXT:
            size = {6, 14};
            tex_range = {0, 36, 3, 7};
            rel_pos = {-3, 7}; 
            break;
    }
    
    vec2 pos = core.cursor_pos;
    
    if(!core.cursor_disabled) {
        std::vector<UI_vertex> v = {
            UI_vertex({0, -size.y, 1.0f}, tex_range.xy()),
            UI_vertex({size.x, -size.y, 1.0f}, tex_range.xy() + ivec2(tex_range.z, 0)),
            UI_vertex({0, 0, 1.0f}, tex_range.xy() + ivec2(0, tex_range.w)),
            UI_vertex({size.x, 0, 1.0f}, tex_range.xy() + ivec2(tex_range.z, tex_range.w)),
        };

        for(UI_vertex& vv : v) {
            vv.pos += vec3(pos + vec2(rel_pos), 0.0f);
            vv.data = 0x1;
        }

        v = {v[0], v[1], v[3], v[0], v[3], v[2]};

        if(!vv->initialized) vv->init();
        vv->vertex_buffer_data(v.data(), v.size(), sizeof(UI_vertex), GL_STREAM_DRAW);

        vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(UI_vertex), 0);
        vv->add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(UI_vertex), 3 * sizeof(float));
        vv->add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(UI_vertex), 5 * sizeof(float));
        vv->add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(UI_vertex), 9 * sizeof(float));
        vv->add_vertex_attribute(4, 1, GL_INT, false, sizeof(UI_vertex), 13 * sizeof(float));

        std::shared_ptr<Shader> ui_shader = core.shaders["gui_shader"];
        std::shared_ptr<Texture> ui_texture = core.textures["icons"];

        glm::mat3 view_mat;
        glm::mat3 trans_mat;

        glm::ivec2 half_viewport_size = core.window.viewport_size / 2;

        view_mat = glm::scale(glm::translate(glm::identity<glm::mat3>(), {-1, -1}), glm::vec2{1.0 / half_viewport_size.x, 1.0 / half_viewport_size.y});
        trans_mat = glm::identity<glm::mat3>();

        ui_shader->use();
        ui_texture->bind(1);
        vv->bind();

        glUniformMatrix3fv(0, 1, false, &view_mat[0][0]);
        glUniformMatrix3fv(1, 1, false, &trans_mat[0][0]);

        vv->draw_vertices(GL_TRIANGLES);
    }
}

void Render_system::render_shapes() {
    glLineWidth(2);
    Physics_system& ps = ecs.get_system<Physics_system>();

    // c

    static std::vector<vec2> vs;
    static float timer = 10.0f;
    if(ps.sim_active) timer += core.delta_time;

    uint32_t num = 12;
    if(timer >= 0.35f) {
        vs.clear();
        for(int i = 0; i < num; ++i) {
            vec2 s = {core.random(), core.random()};
            vs.push_back(s);
        }
        timer = 0.0f;
    }

    std::vector<uint32_t> ids = convex_hull(vs);

    std::vector<Color_vertex> vs2;
    
    
    for(int i = 0; i < ids.size(); ++i) {
        int i0 = i;
        int i1 = (i + 1) % ids.size();

        vs2.push_back({vec3(vs[ids[i0]], 0.5f), vec4(1.0f, 0.0f, 0.0f, 1.0f)});
        vs2.push_back({vec3(vs[ids[i1]], 0.5f), vec4(1.0f, 0.0f, 0.0f, 1.0f)});
    }

    /*
    for(int i = 0; i < ids.size(); ++i) {
        int i0 = i;
        //int i1 = (i + 1) % ids.size();

        vs2.push_back({vec3(ids[i0], 0.5f), (i % 2 == 0) ? vec4(0.0f, 1.0f, 0.0f, 1.0f) : vec4(0.0f, 0.0f, 1.0f, 1.0f)});
        //vs2.push_back(vec3(ids[i1], 0.5f));
    }
    */

    //for(vec2 v : ids) vs2.push_back(vec3(v, 0.5f));
    /*
    for(int i = 0; i < ids.size(); ++i) {
        int i0 = i;
        int i1 = (i + 1) % ids.size();

        vs2.push_back(vec3(vs[i0], 0.5));
        vs2.push_back(vec3(vs[i1], 0.5));
    }*/

    vv->vertex_buffer_data(vs2.data(), vs2.size(), sizeof(Color_vertex), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Color_vertex), 0);
    vv->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(Color_vertex), sizeof(float) * 3);

    mat4 view_mat = glm::identity<glm::mat3>();
    mat4 proj_mat = glm::identity<glm::mat3>();
    mat4 model_mat = glm::identity<glm::mat3>();

    core.shaders["color_vertex_shader"]->use();
    vv->bind();

    glUniformMatrix4fv(0, 1, false, &view_mat[0][0]);
    glUniformMatrix4fv(1, 1, false, &proj_mat[0][0]);
    glUniformMatrix4fv(2, 1, false, &model_mat[0][0]);

    vv->draw_vertices(GL_LINES);

    std::vector<vec3> vs3;
    for(vec2 v : vs) {
        vs3.push_back(vec3(v + vec2(-0.01f, -0.01f), 0.5f));
        vs3.push_back(vec3(v + vec2(0.01f, -0.01f), 0.5f));
        vs3.push_back(vec3(v + vec2(0.01f, 0.01f), 0.5f));
        vs3.push_back(vec3(v + vec2(-0.01f, -0.01f), 0.5f));
        vs3.push_back(vec3(v + vec2(0.01f, 0.01f), 0.5f));
        vs3.push_back(vec3(v + vec2(-0.01f, 0.01f), 0.5f));
    }

    vv->vertex_buffer_data(vs3.data(), vs3.size(), sizeof(vec3), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);
    
    core.shaders["color_shader"]->use();
    vv->bind();

    glUniformMatrix4fv(0, 1, false, &view_mat[0][0]);
    glUniformMatrix4fv(1, 1, false, &proj_mat[0][0]);
    glUniformMatrix4fv(2, 1, false, &model_mat[0][0]);
    glUniform4f(3, 1.0f, 1.0f, 0.0f, 1.0f);

    vv->draw_vertices(GL_TRIANGLES);
}

void Render_system::render_grid(uint32_t camera) {
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    Camera& camera_camera = ecs.get_component<Camera>(camera);

    mat4 view = mat4(transpose(camera_transform.orientation));

    pvec3 o = {camera_transform.position.x.fraction, camera_transform.position.y.fraction, camera_transform.position.z};

    int64_t i0 = 0x1000000000000;
    int64_t i1 = 0x1000000;

    ivec2 offset_major = {camera_transform.position.x.sector / i0, camera_transform.position.y.sector / i0};
    if(offset_major.x * i0 > camera_transform.position.x.sector) {
        offset_major.x -= 1;
    }
    if(offset_major.y * i0 > camera_transform.position.y.sector) {
        offset_major.y -= 1;
    }

    uvec2 offset_minor0;
    int64_t ix = camera_transform.position.x.sector - int64_t(offset_major.x) * i0;
    int64_t iy = camera_transform.position.y.sector - int64_t(offset_major.y) * i0;
    offset_minor0.x = ix / i1;
    offset_minor0.y = iy / i1;
    if(offset_minor0.x * i1 > ix) {
        offset_minor0.x -= 1;
    }
    if(offset_minor0.y * i1 > iy) {
        offset_minor0.y -= 1;
    }

    uvec2 offset_minor1;
    offset_minor1.x = camera_transform.position.x.sector - int64_t(offset_major.x) * i0 - int64_t(offset_minor0.x) * i1;
    offset_minor1.y = camera_transform.position.y.sector - int64_t(offset_major.y) * i0 - int64_t(offset_minor0.y) * i1;
    

    glm::mat4 model = core.get_model_matrix(vec3(0.0f), o);

    core.shaders["grid_shader"]->use();

    glUniformMatrix4fv(0, 1, false, &model[0][0]);
    glUniformMatrix4fv(1, 1, false, &view[0][0]);
    glUniformMatrix4fv(2, 1, false, &camera_camera.proj[0][0]);
    glUniform2iv(3, 1, &offset_major[0]);
    glUniform2uiv(4, 1, &offset_minor0[0]);
    glUniform2uiv(5, 1, &offset_minor1[0]);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

struct mesh_element {
    std::vector<uint32_t> vertices;
    std::vector<uint32_t> center;
};

void create_mesh(Mesh_component& mc, std::vector<vec3> v, vec3 radius) {
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<vec3> vs;
    std::vector<uint32_t> indices;
    std::vector<mesh_element> elements;

    if(v.size() == 1) {
        vs = {
            v[0] + vec3(radius.x, 0.0f, 0.0f),
            v[0] + vec3(0.0f, radius.y, 0.0f),
            v[0] + vec3(0.0f, 0.0f, radius.z),
            v[0] + vec3(-radius.x, 0.0f, 0.0f),
            v[0] + vec3(0.0f, -radius.y, 0.0f),
            v[0] + vec3(0.0f, 0.0f, -radius.z),
        };

        std::vector<mesh_element> elements_cap = {
            mesh_element({0, 1, 2}, {0}),
            mesh_element({1, 3, 2}, {0}),
            mesh_element({3, 4, 2}, {0}),
            mesh_element({4, 0, 2}, {0}),
            
            mesh_element({0, 5, 1}, {0}),
            mesh_element({1, 5, 3}, {0}),
            mesh_element({3, 5, 4}, {0}),
            mesh_element({4, 5, 0}, {0}),
        };
        
        elements.insert(elements.end(), elements_cap.begin(), elements_cap.end());
    } else if(v.size() == 2) {
        vs = {
            v[0] + vec3(radius.x, 0.0f, 0.0f),
            v[0] + vec3(0.0f, radius.y, 0.0f),
            v[0] + vec3(-radius.x, 0.0f, 0.0f),
            v[0] + vec3(0.0f, -radius.y, 0.0f),
            v[0] + vec3(0.0f, 0.0f, -radius.z),

            v[1] + vec3(radius.x, 0.0f, 0.0f),
            v[1] + vec3(0.0f, radius.y, 0.0f),
            v[1] + vec3(-radius.x, 0.0f, 0.0f),
            v[1] + vec3(0.0f, -radius.y, 0.0f),
            v[1] + vec3(0.0f, 0.0f, radius.z),
        };

        std::vector<mesh_element> elements_cap = {
            // bottom pyramid
            mesh_element({0, 1, 4}, {0}),
            mesh_element({1, 2, 4}, {0}),
            mesh_element({2, 3, 4}, {0}),
            mesh_element({3, 0, 4}, {0}),
            
            // top pyramid
            mesh_element({5, 9, 6}, {1}),
            mesh_element({6, 9, 7}, {1}),
            mesh_element({7, 9, 8}, {1}),
            mesh_element({8, 9, 5}, {1})
        };

        std::vector<mesh_element> elements_cyl = {
            // bottom pyramid
            // 0, 1, 5, 6
            // 1, 3, 6, 7
            // 3, 4, 7, 8
            // 4, 0, 8, 5
            mesh_element({0, 1, 5, 6}, {0, 1}),

            mesh_element({1, 2, 6, 7}, {0, 1}),

            mesh_element({2, 3, 7, 8}, {0, 1}),

            mesh_element({3, 0, 8, 5}, {0, 1}),
        };

        elements.insert(elements.end(), elements_cap.begin(), elements_cap.end());
        elements.insert(elements.end(), elements_cyl.begin(), elements_cyl.end());
    } else if(v.size() == 3) {
        vec3 normal = normalize(cross(v[0] - v[2], v[1] - v[2]));
        vec3 v01 = normalize(cross(normal, v[0] - v[1]));
        vec3 v12 = normalize(cross(normal, v[1] - v[2]));
        vec3 v20 = normalize(cross(normal, v[2] - v[0]));

        vs = {
            v[0] + normal * radius.x,
            v[1] + normal * radius.x,
            v[2] + normal * radius.x,
            v[0] - normal * radius.x,
            v[1] - normal * radius.x,
            v[2] - normal * radius.x,

            v[0] + v01 * radius.x,
            v[1] + v01 * radius.x,
            
            v[1] + v12 * radius.x,
            v[2] + v12 * radius.x,
            
            v[2] + v20 * radius.x,
            v[0] + v20 * radius.x,
        };
        
        std::vector<mesh_element> elements_face = {
            mesh_element({0, 1, 2}, {}),
            mesh_element({3, 4, 5}, {}),
        };
            
        std::vector<mesh_element> elements_cyl = {
            // bottom pyramid
            // 0, 1, 5, 6
            // 1, 3, 6, 7
            // 3, 4, 7, 8
            // 4, 0, 8, 5
            mesh_element({0, 6, 1, 7}, {0, 1}),
            mesh_element({6, 3, 7, 4}, {0, 1}),

            mesh_element({1, 8, 2, 9}, {1, 2}),
            mesh_element({8, 4, 9, 5}, {1, 2}),

            mesh_element({2, 10, 0, 11}, {2, 0}),
            mesh_element({10, 5, 11, 3}, {2, 0}),
        };

        std::vector<mesh_element> elements_cap = {
            // bottom pyramid
            mesh_element({0, 6, 11}, {0}),
            mesh_element({6, 11, 3}, {0}),
            
            mesh_element({1, 7, 8}, {1}),
            mesh_element({7, 8, 4}, {1}),
            
            mesh_element({2, 9, 10}, {2}),
            mesh_element({9, 10, 5}, {2}),
        };
        
        elements.insert(elements.end(), elements_cap.begin(), elements_cap.end());
        elements.insert(elements.end(), elements_cyl.begin(), elements_cyl.end());
        elements.insert(elements.end(), elements_face.begin(), elements_face.end());
    } else {
        vec3 center = vec3(0.0f);
        for(vec3 vv : v) center += vv;
        center /= float(v.size());

        std::vector<shape_face> faces = Physics_system::triangulate_merge(v);

        std::cout << faces.size() << "\n";

        std::vector<mesh_element> elements_face = {

        };
            
        std::vector<mesh_element> elements_cyl = {

        };

        std::vector<mesh_element> elements_cap = {

        };

        std::unordered_map<uint32_t, std::vector<uint32_t>> edges;
        std::unordered_map<uint32_t, std::vector<uint32_t>> vertices;

        for(shape_face& face : faces) {
            uint32_t start = vs.size();

            for(int i = 0; i < face.vertices.size(); ++i) {
                uint32_t ia = face.vertices[i];

                vec3 a = v[ia];

                vs.push_back(a + face.normal * radius);
            }

            for(int i = 2; i < face.vertices.size(); ++i) {
                uint32_t va = start;
                uint32_t vb = start + i - 1;
                uint32_t vc = start + i;

                elements_face.push_back(mesh_element({va, vb, vc}, {}));
            }
            
            for(int i = 0; i < face.vertices.size(); ++i) {
                uint32_t ia = face.vertices[i];
                uint32_t ib = face.vertices[(i + 1) % face.vertices.size()];
                
                uint32_t va = start + i;
                uint32_t vb = start + (i + 1) % face.vertices.size();

                uint32_t e0 = min(ia, ib) | (max(ia, ib) << 16);

                if(ia < ib) {
                    edges[e0].push_back(va);
                    edges[e0].push_back(vb);
                } else {
                    edges[e0].push_back(vb);
                    edges[e0].push_back(va);
                }
                
            }

            for(int i = 0; i < face.vertices.size(); ++i) {
                uint32_t ia = face.vertices[i];

                uint32_t va = start + i;
                
                vertices[ia].push_back(va);
            }
        }

        for(auto& [id, cyl] : edges) {
            elements_cyl.push_back(mesh_element({cyl[0], cyl[2], cyl[1], cyl[3]}, {id & 0xFFFF, (id >> 16) & 0xFFFF}));
        }

        for(auto& [id, cap] : vertices) {
            elements_cap.push_back(mesh_element({cap[0], cap[1], cap[2]}, {id}));
        }
        
        elements.insert(elements.end(), elements_cap.begin(), elements_cap.end());
        elements.insert(elements.end(), elements_cyl.begin(), elements_cyl.end());
        elements.insert(elements.end(), elements_face.begin(), elements_face.end());

        /*
        m.v_tris = std::shared_ptr<Vertices>(new Vertices);
        m.v_tris->init();
        
        m.v_lines = std::shared_ptr<Vertices>(new Vertices);
        m.v_lines->init();
        
        float sphere_segments = max(64, int32_t(8 * max(radius.x, radius.y)));

        std::vector<vec2> vv;
        if(radius.x == 0.0f && radius.y == 0.0f) {
            for(int i = 0; i < v.size(); ++i) {
                vv.push_back(v[i]);
            }
        } else {
            vec2 sum = vec2(0.0f);
            for(int i = 0; i < v.size(); ++i) {
                sum += v[i];
            }
            sum /= v.size();

            std::unordered_set<uint32_t> cc;
            vec2 first_normal = vec2(0.0f);
            vec2 prev_normal = vec2(0.0f);
            for(int i = 0; i < v.size(); ++i) {
                uint16_t a = i;
                uint16_t b = (i + 1) % v.size();
                if(a == b) {
                    vec2 v0 = v[a];

                    float angle_a = 0.0f;
                    float angle_b = 2 * M_PI;

                    float angle_per_segment = 2 * M_PI / sphere_segments;

                    for(float j = angle_a + angle_per_segment; j < angle_b; j += angle_per_segment) {
                        vec2 vc = {cos(j), sin(j)};
                        vc *= radius;
                        vc = v0 + vc;
                        
                        vv.push_back(vc);
                    }
                } else {
                    vec2 v0 = v[a];
                    vec2 v1 = v[b];
                    
                    if(a > b) {
                        uint16_t temp = a;
                        a = b;
                        b = temp;
                    }

                    vec2 direction = normalize(v[b] - v[a]);
                    vec2 normal = vec2(direction.y, -direction.x);
                    if(dot(normal, sum - v[a]) > 0.0f) {
                        normal = -normal;
                    }

                    uint32_t c = (uint32_t)a | ((uint32_t)b << 16);
                    if(cc.contains(c)) {
                        normal = -normal;
                    } else {
                        cc.insert(c);
                    }

                    // create previous sphere
                    if(prev_normal.x != 0.0f || prev_normal.y != 0.0f) {
                        vec2 va = prev_normal * radius;
                        vec2 vb = normal * radius;
                        float angle_a = atan2(va.y, va.x);
                        float angle_b = atan2(vb.y, vb.x);

                        if(angle_b < angle_a) angle_a -= 2 * M_PI;

                        float angle_per_segment = 2 * M_PI / sphere_segments;

                        for(float j = angle_a + angle_per_segment; j < angle_b; j += angle_per_segment) {
                            vec2 vc = {cos(j), sin(j)};
                            vc *= radius;
                            vc = v0 + vc;

                            vv.push_back(vc);
                        }
                    }

                    vv.push_back(v0 + normal * radius);
                    vv.push_back(v1 + normal * radius);

                    // create first sphere
                    if(i == v.size() - 1) {
                        vec2 va = normal * radius;
                        vec2 vb = first_normal * radius;
                        float angle_a = atan2(va.y, va.x);
                        float angle_b = atan2(vb.y, vb.x);

                        if(angle_b < angle_a) angle_a -= 2 * M_PI;

                        float angle_per_segment = 2 * M_PI / sphere_segments;

                        for(float j = angle_a + angle_per_segment; j < angle_b; j += angle_per_segment) {
                            vec2 vc = {cos(j), sin(j)};
                            vc *= radius;
                            vc = v1 + vc;

                            vv.push_back(vc);
                        }
                    }


                    prev_normal = normal;
                    if(first_normal.x == 0.0f && first_normal.y == 0.0f) first_normal = normal;
                }
            }
        }

        float min_dist = FLT_MAX;
        std::vector<Object_vertex> vvv;
        for(int i = 0 ; i < vv.size(); ++i) {
            vec2 v0 = vv[i];
            vec2 v1 = vv[(i + 1) % vv.size()];

            vec2 origin_v = -v0;
            float len = length(v1 - v0);
            vec2 dir = (v1 - v0) / len;
            float f = dot(dir, origin_v);
            f = clamp(f, 0.0f, len);
            vec2 closest_point = f * dir + v0;

            min_dist = min(length(closest_point), min_dist);



            Object_vertex ov;
            ov.v = vec3(v0, 0.5);
            vvv.push_back(ov);

            ov.v = vec3(v1, 0.5);
            vvv.push_back(ov);
        }

        m.v_lines->vertex_buffer_data(vvv.data(), vvv.size(), sizeof(Object_vertex), GL_STATIC_DRAW);
        m.v_lines->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(float) * 3, 0);

        vvv.clear();
        */
    }
    
    std::unordered_map<uint32_t, std::vector<uint32_t>> unique_curves;
    std::unordered_map<uint32_t, std::vector<uint32_t>> curves;

    uint32_t curve_size = 5;

    if(radius.x == 0.0f && radius.y == 0.0f && radius.z == 0.0f) curve_size = 2;

    for(mesh_element& element : elements) {
        if(element.vertices.size() == 3 && element.center.size() == 1) {
            vec3 avg = vs[element.vertices[0]] + vs[element.vertices[1]] + vs[element.vertices[2]];
            avg /= 3.0f;

            if(dot(cross(vs[element.vertices[0]] - vs[element.vertices[2]], vs[element.vertices[1]] - vs[element.vertices[2]]), avg) > 0.0f) element.vertices = {element.vertices[0], element.vertices[2], element.vertices[1]};
            uint32_t id = (element.vertices[0] << 16) | (element.vertices[1] << 8) | element.vertices[2];

            curves.emplace(id, std::vector<uint32_t>());
            std::vector<uint32_t>& curve_indices = curves[id];

            for(int i = 0; i < 3; ++i) {
                uint32_t a = element.vertices[i];
                uint32_t b = element.vertices[(i + 1) % 3];

                uint32_t unique_id = min(a, b) | (max(a, b) << 16);
                
                std::vector<uint32_t>* unique_curve;

                if(!unique_curves.contains(unique_id)) {
                    unique_curves.emplace(unique_id, std::vector<uint32_t>());

                    unique_curve = &unique_curves[unique_id];
                    
                    unique_curve->push_back(a);
                    for(int j = 1; j < curve_size - 1; ++j) {
                        float jj = float(j) / (curve_size - 1);

                        vec3 v = vs[a] + (vs[b] - vs[a]) * jj;

                        unique_curve->push_back(vs.size());
                        vs.push_back(v);
                    }
                    unique_curve->push_back(b);
                } else {
                    unique_curve = &unique_curves[unique_id];
                }

                if(unique_curve->operator[](0) == a) {
                    curve_indices.insert(curve_indices.end(), unique_curve->begin(), unique_curve->end());
                } else {
                    curve_indices.insert(curve_indices.end(), unique_curve->rbegin(), unique_curve->rend());
                }
            }
        } else if(element.vertices.size() == 4 && element.center.size() == 2) {
            vec3 avg = vs[element.vertices[0]] + vs[element.vertices[1]] + vs[element.vertices[2]] + vs[element.vertices[3]];
            avg /= 4.0f;

            if(dot(cross(vs[element.vertices[0]] - vs[element.vertices[2]], vs[element.vertices[1]] - vs[element.vertices[2]]), avg) > 0.0f) element.vertices = {element.vertices[1], element.vertices[0], element.vertices[3], element.vertices[2]};

            uint32_t id = (element.vertices[0] << 24) | (element.vertices[1] << 16) | (element.vertices[2] << 8) | element.vertices[3];

            curves.emplace(id, std::vector<uint32_t>());
            std::vector<uint32_t>& curve_indices = curves[id];

            for(int i = 0; i < 2; ++i) {
                uint32_t a = element.vertices[i * 2];
                uint32_t b = element.vertices[i * 2 + 1];

                uint32_t unique_id = min(a, b) | (max(a, b) << 16);
                
                std::vector<uint32_t>* unique_curve;

                if(!unique_curves.contains(unique_id)) {
                    unique_curves.emplace(unique_id, std::vector<uint32_t>());

                    unique_curve = &unique_curves[unique_id];
                    
                    unique_curve->push_back(a);
                    for(int j = 1; j < curve_size - 1; ++j) {
                        float jj = float(j) / (curve_size - 1);

                        vec3 v = vs[a] + (vs[b] - vs[a]) * jj;

                        unique_curve->push_back(vs.size());
                        vs.push_back(v);
                    }
                    unique_curve->push_back(b);
                } else {
                    unique_curve = &unique_curves[unique_id];
                }

                if(unique_curve->operator[](0) == a) {
                    curve_indices.insert(curve_indices.end(), unique_curve->begin(), unique_curve->end());
                } else {
                    curve_indices.insert(curve_indices.end(), unique_curve->rbegin(), unique_curve->rend());
                }
            }
        }
    }

    for(mesh_element& element : elements) {
        if(element.vertices.size() == 3 && element.center.size() == 1) {
            uint32_t a = element.vertices[0];
            uint32_t b = element.vertices[1];
            uint32_t c = element.vertices[2];
            uint32_t d = element.center[0];

            uint32_t key = (a << 16) | (b << 8) | c;
            
            vec3 normal = cross(vs[a] - vs[c], vs[b] - vs[c]);
            vec3 center = (vs[a] + vs[b] + vs[c]) / 3.0f;

            //if(dot(normal, center) > 0.0f) values = {values[0], values[2], values[1]};

            std::vector<uint32_t>& curve = curves[key];

            std::vector<uint32_t> triangle_indices;

            for(int j = 0; j < curve_size; ++j) {
                if(j == 0) {
                    triangle_indices.push_back(curve[0]);
                } else if(j == 1) {
                    triangle_indices.push_back(curve[curve_size * 3 - 2]);
                    triangle_indices.push_back(curve[1]);
                } else if(j == curve_size - 1) {
                    for(int i = curve_size * 2 - 1; i > curve_size - 1; --i) {
                        triangle_indices.push_back(curve[i]);
                    }
                } else {
                    uint32_t start_i = curve[curve_size * 3 - j - 1];
                    uint32_t end_i = curve[j];
                    vec3 start = vs[start_i];
                    vec3 end = vs[end_i];
                    
                    triangle_indices.push_back(curve[curve_size * 3 - j - 1]);

                    for(int k = 1; k < j; ++k) {
                        float f = float(k) / (j);
                        vec3 s = start + (end - start) * f;
                        //s = normalize(s - v[d]) * radius.x + v[d];

                        uint32_t index = vs.size();
                        vs.push_back(s);
                        triangle_indices.push_back(index);
                    }
                    
                    triangle_indices.push_back(curve[j]);
                }
            }
            
            std::vector<uint32_t> new_indices;

            uint32_t accum = 1;
            for(int j = 1; j < curve_size; ++j) {
                for(int k = 0; k < j; ++k) {
                    uint32_t a = accum + k;
                    uint32_t b = accum + k + 1;
                    uint32_t c = accum + k - j;

                    new_indices.push_back(triangle_indices[a]);
                    new_indices.push_back(triangle_indices[b]);
                    new_indices.push_back(triangle_indices[c]);

                    if(k != j - 1) {
                        a = accum + k + 1;
                        b = accum + k - j + 1;
                        c = accum + k - j;

                        new_indices.push_back(triangle_indices[a]);
                        new_indices.push_back(triangle_indices[b]);
                        new_indices.push_back(triangle_indices[c]);
                    }
                }

                accum += j + 1;
            }

            for(uint32_t index : new_indices) {
                vec3& s = vs[index];
                s = normalize(s - v[d]) * radius.x + v[d];
                if(isnan(s.x)) s = v[d];
            }

            indices.insert(indices.end(), new_indices.begin(), new_indices.end());
        } else if(element.vertices.size() == 4 && element.center.size() == 2) {
            uint32_t a = element.vertices[0];
            uint32_t b = element.vertices[1];
            uint32_t c = element.vertices[2];
            uint32_t d = element.vertices[3];
            
            uint32_t key = (a << 24) | (b << 16) | (c << 8) | d;
            
            std::vector<uint32_t>& curve = curves[key];

            std::vector<uint32_t> new_indices;

            for(int j = 0; j < curve_size - 1; ++j) {
                uint32_t a = j;
                uint32_t b = j + curve_size;
                uint32_t c = j + curve_size + 1;

                new_indices.push_back(curve[a]);
                new_indices.push_back(curve[b]);
                new_indices.push_back(curve[c]);

                a = j;
                b = j + curve_size + 1;
                c = j + 1;

                new_indices.push_back(curve[a]);
                new_indices.push_back(curve[b]);
                new_indices.push_back(curve[c]);
            }

            uint32_t counter = 0;
            for(uint32_t index : curve) {
                vec3 cc = v[element.center[counter / curve_size]];

                vec3& s = vs[index];
                s = normalize(s - cc) * radius.x + cc;
                if(isnan(s.x)) s = v[d];
                ++counter;
            }

            indices.insert(indices.end(), new_indices.begin(), new_indices.end());
        } else if(element.center.size() == 0) {
            vec3 avg = vs[element.vertices[0]] + vs[element.vertices[1]] + vs[element.vertices[2]];
            avg /= 3.0f;
            if(dot(cross(vs[element.vertices[0]] - vs[element.vertices[2]], vs[element.vertices[1]] - vs[element.vertices[2]]), avg) < 0.0f) element.vertices = {element.vertices[0], element.vertices[2], element.vertices[1]};
            
            indices.insert(indices.end(), element.vertices.begin(), element.vertices.end());
        }
    }

    std::unordered_map<uint32_t, vec3> normals;

    for(int i = 0; i < indices.size() / 3; ++i) {
        uint32_t a = indices[i * 3];
        uint32_t b = indices[i * 3 + 1];
        uint32_t c = indices[i * 3 + 2];

        vec3 va = vs[a];
        vec3 vb = vs[b];
        vec3 vc = vs[c];

        vec3 normal = cross(va - vc, vb - vc);

        if(!normals.contains(a)) normals[a] = vec3(0.0f);
        if(!normals.contains(b)) normals[b] = vec3(0.0f);
        if(!normals.contains(c)) normals[c] = vec3(0.0f);

        normals[a] += normal;
        normals[b] += normal;
        normals[c] += normal;
    }

    for(auto& [key, norm] : normals) norm = normalize(norm);

    std::vector<Mesh_vertex> mvs;
    for(int i = 0; i < vs.size(); ++i) {
        Mesh_vertex mv;
        mv.normal = normals[i];
        mv.position = vs[i];
        mv.tex_coords = vec2(56, 8) / 256.0f;

        mvs.push_back(mv);
    }

    mesh->add_vertices(mvs, indices);
    mesh->load_buffer();
    
    mc.mesh = mesh;
    mc.texture = core.textures["tilesheet"];
}

void Render_system::render_billboard(uint32_t entity, uint32_t camera, pvec3 light_pos) {
    Billboard_animation& bb = ecs.get_component<Billboard_animation>(entity);
    Transform& t = ecs.get_component<Transform>(entity);
    
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    Camera& camera_camera = ecs.get_component<Camera>(camera);

    vec2 tex_size = bb.texture->size.xy();

    pvec3 apos;

    vec3 ld = vec3(0.0);//normalize(light_direction);

    std::vector<Mesh_vertex> mvs = {
        Mesh_vertex({-0.5f, 0, 0}, ld, {0, 0}),
        Mesh_vertex({0.5f, 0, 0}, ld, {1, 0}),
        Mesh_vertex({-0.5f, 0, 1}, ld, {0, 1}),
        Mesh_vertex({0.5f, 0, 1}, ld, {1, 1}),
    };

    float darkness_value = 0.5f;
    
    apos = t.position;

    mat4 view = mat4(transpose(camera_transform.orientation));


    glm::mat4 model_0 = t.orientation;
    glm::mat4 model_1 = core.get_model_matrix(t.position, camera_transform.position);
    
    mat4 model = model_1;

    vec3 up = vec3(model_0 * vec4(0, 0, 1, 0));
    vec3 dir;
    if(camera_camera.orthogonal) dir = vec3(camera_transform.orientation * vec3(0, 0, -1));
    else dir = vec3(t.position - camera_transform.position);
    dir = normalize(dir - up * dot(dir, up));
    vec3 x = normalize(cross(dir, up));

    vec3 forward = t.orientation * vec3(0, 1, 0);
    vec3 side = t.orientation * vec3(1, 0, 0);

    mat3 rot = rotate_to(up, vec3(0, 0, 1));

    uint32_t sprite_dir = 0;
    vec3 dir_u = rot * -dir;
    vec3 side_u = rot * side;

    float cos_v = dot(side_u, dir_u);
    float sin_v = side_u.x * dir_u.y - side_u.y * dir_u.x;

    if(!bb.use_axis) {
        up = camera_transform.orientation[1];
        x = camera_transform.orientation[0];
    }

    bb.current_time += core.delta_time;
    float total_time = 0.0f;
    for(auto b : bb.animations[bb.current_animation].frames) {
        total_time += b.duration;
    }
    float rel_time = fmod(bb.current_time, total_time);

    int i = 0;
    total_time = 0.0f;
    for(auto b : bb.animations[bb.current_animation].frames) {
        total_time += b.duration;
        if(total_time > rel_time) break;
        ++i;
    }
    bb.current_frame = clamp(i, 0, int(bb.animations[bb.current_animation].frames.size()) - 1);

    // get direction
    
    uint32_t num_regions = bb.animations[bb.current_animation].frames[bb.current_frame].regions.size();

    if(num_regions == 1) {
        sprite_dir = 0;
    } else if(num_regions == 4) {
        float angle = atan2(sin_v, cos_v);
        angle *= (180.0f / M_PI);
        
        angle += 45.0f;
        angle /= 360.0f;
        angle *= 4;
        angle = floor(angle);
        if(angle > 3) angle -= 4;
        else if(angle < 0) angle += 4;
        sprite_dir = angle;
    } else if(num_regions == 8) {
        float angle = atan2(sin_v, cos_v);
        angle *= (180.0f / M_PI);
        
        angle += 22.5f;
        angle /= 360.0f;
        angle *= 8;
        angle = floor(angle);
        if(angle > 7) angle -= 8;
        else if(angle < 0) angle += 8;
        sprite_dir = angle;
    }

    float length = 0.0f;

    for(Mesh_vertex& v : mvs) {
        v.position = vec3(1, 0, 0) * v.position.x * bb.size.x + vec3(0, 0, 1) * v.position.z * bb.size.y;
        v.position += vec3(0, 0, bb.height);
        vec4 region = bb.animations[bb.current_animation].frames[bb.current_frame].regions[sprite_dir];
        v.tex_coords = region.xy() + v.tex_coords * region.zw();
        v.tex_coords /= tex_size;

        v.bone_weights = vec4(1.0f, 1.0f, 1.0f, v.position.x);
    }

    if(bb.sway_period != 0.0f) {
        float sway = bb.sway_dist * sin((core.current_time / bb.sway_period + bb.sway_offset) * (2.0 * M_PI));
        mvs[2].position += sway * x;
        mvs[3].position += sway * x;
    }

    mvs = {
        mvs[0],
        mvs[1],
        mvs[3],
        mvs[0],
        mvs[3],
        mvs[2]
    };

    if(!vv->initialized) vv->init();
    vv->vertex_buffer_data(mvs.data(), 6, sizeof(Mesh_vertex), GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Mesh_vertex), 0);
    vv->add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(Mesh_vertex), 3 * sizeof(float));
    vv->add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(Mesh_vertex), 6 * sizeof(float));
    vv->add_vertex_attribute(3, 4, GL_INT, false, sizeof(Mesh_vertex), 8 * sizeof(float));
    vv->add_vertex_attribute(4, 4, GL_FLOAT, false, sizeof(Mesh_vertex), 12 * sizeof(float));

    model = model * mat4(create_rot_mat(-cross(x, up), up));

    //glm::mat4 model_2 = scale(vec3(2, 2, 2));

    //mat4 model = model_1 * model_0;// * model_2;

    core.shaders["model_shader"]->use();
    bb.texture->bind(0);

    vec3 apos2 = apos;

    glUniformMatrix4fv(0, 1, false, &camera_camera.proj[0][0]);
    glUniformMatrix4fv(1, 1, false, &view[0][0]);
    glUniformMatrix4fv(2, 1, false, &model[0][0]);
    glUniform3fv(3, 1, &light_direction[0]);
    glUniform3fv(4, 1, &apos2[0]);
    glUniform1f(5, darkness_value);
    
    float planet_radius = 1.0477f * 0x100000;

    glUniform3fv(7, 1, &rel_center[0]);
    glUniform3fv(8, 1, &rel_sun[0]);
    glUniform3f(9, planet_radius, planet_radius, planet_radius);
    glUniform1f(10, 30000.0f);

    glUniform1f(11, -bb.size.x * 0.25f);

    if(abs(dot(normalize(vec3(camera_transform.position - t.position)), up)) < 0.9999f || camera_camera.orthogonal) {
        vv->draw_vertices(GL_TRIANGLES);
    }
}

void Render_system::render_debug_lines(uint32_t camera) {
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    Camera& camera_camera = ecs.get_component<Camera>(camera);

    Particle_system& particle_system = ecs.get_system<Particle_system>();

    glDisable(GL_DEPTH_TEST);

    if(!vv->initialized) vv->init();
    vv->vertex_buffer_data(particle_system.ps_vertices.data(), particle_system.ps_vertices.size(), sizeof(vec3), GL_STREAM_DRAW);
    vv->index_buffer_data(particle_system.ps_indices.data(), particle_system.ps_indices.size(), GL_UNSIGNED_INT, 4, GL_STREAM_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(vec3), 0);

    glm::mat4 model = core.get_model_matrix(particle_system.rel_pos, camera_transform.position);
    glm::mat4 view = mat4(transpose(camera_transform.orientation));
    glm::mat4 proj = camera_camera.proj;

    core.shaders["color_shader"]->use();

    vec3 color = vec3(1.0f);

    vv->bind();

    glUniformMatrix4fv(0, 1, false, &view[0][0]);
    glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
    glUniformMatrix4fv(2, 1, false, &model[0][0]);
    glUniform4f(3, color.x, color.y, color.z, 0.0f);

    vv->draw_indices(GL_LINES);
    
    glEnable(GL_DEPTH_TEST);
}

void Render_system::render_gui() {
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    
    if(!vv->initialized) vv->init();
    vv->vertex_buffer_data(gui_system.vertices.data(), gui_system.vertices.size(), sizeof(UI_vertex), GL_STREAM_DRAW);

    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(UI_vertex), 0);
    vv->add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(UI_vertex), 3 * sizeof(float));
    vv->add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(UI_vertex), 5 * sizeof(float));
    vv->add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(UI_vertex), 9 * sizeof(float));
    vv->add_vertex_attribute(4, 1, GL_INT, false, sizeof(UI_vertex), 13 * sizeof(float));

    std::shared_ptr<Shader> ui_shader = core.shaders["gui_shader"];
    std::shared_ptr<Texture> ui_texture = core.textures["icons"];
    std::shared_ptr<Texture> text_texture = core.textures["text_texture"];

    uint32_t i = 0;

    glm::mat3 view_mat;
    glm::mat3 trans_mat;

    glm::ivec2 half_viewport_size = core.window.viewport_size / 2;

    view_mat = glm::scale(glm::translate(glm::identity<glm::mat3>(), {-1, -1}), glm::vec2{1.0 / half_viewport_size.x, 1.0 / half_viewport_size.y});
    trans_mat = glm::identity<glm::mat3>();

    ui_shader->use();
    text_texture->bind(0);
    ui_texture->bind(1);
    framebuffers[0].textures[0].bind(2);
    vv->bind();

    glUniformMatrix3fv(0, 1, false, &view_mat[0][0]);
    glUniformMatrix3fv(1, 1, false, &trans_mat[0][0]);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_GEQUAL);

    vv->draw_vertices(GL_TRIANGLES);
}

void Render_system::render_crosshair(ivec2 target_size) {
    vec2 size = vec2(10, 10);
    ivec4 tex_range = vec4(0, 43, 5, 5);
    vec2 rel_pos = target_size / 2 + ivec2(-5, 5);
    vec2 pos = vec3(0.0f);

    std::vector<UI_vertex> v = {
        UI_vertex({0, -size.y, 1.0f}, tex_range.xy()),
        UI_vertex({size.x, -size.y, 1.0f}, tex_range.xy() + ivec2(tex_range.z, 0)),
        UI_vertex({0, 0, 1.0f}, tex_range.xy() + ivec2(0, tex_range.w)),
        UI_vertex({size.x, 0, 1.0f}, tex_range.xy() + ivec2(tex_range.z, tex_range.w)),
    };

    for(UI_vertex& vv : v) {
        vv.pos += vec3(pos + vec2(rel_pos), 0.0f);
        vv.data = 0x1;
    }

    v = {v[0], v[1], v[3], v[0], v[3], v[2]};

    if(!vv->initialized) vv->init();
    vv->vertex_buffer_data(v.data(), v.size(), sizeof(UI_vertex), GL_STREAM_DRAW);

    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(UI_vertex), 0);
    vv->add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(UI_vertex), 3 * sizeof(float));
    vv->add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(UI_vertex), 5 * sizeof(float));
    vv->add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(UI_vertex), 9 * sizeof(float));
    vv->add_vertex_attribute(4, 1, GL_INT, false, sizeof(UI_vertex), 13 * sizeof(float));

    std::shared_ptr<Shader> ui_shader = core.shaders["gui_shader"];
    std::shared_ptr<Texture> ui_texture = core.textures["icons"];

    glm::mat3 view_mat;
    glm::mat3 trans_mat;

    glm::vec2 half_viewport_size = vec2(target_size) / 2.0f;

    view_mat = glm::scale(glm::translate(glm::identity<glm::mat3>(), {-1, -1}), glm::vec2{1.0 / half_viewport_size.x, 1.0 / half_viewport_size.y});
    trans_mat = glm::identity<glm::mat3>();

    ui_shader->use();
    ui_texture->bind(1);
    vv->bind();

    glUniformMatrix3fv(0, 1, false, &view_mat[0][0]);
    glUniformMatrix3fv(1, 1, false, &trans_mat[0][0]);

    vv->draw_vertices(GL_TRIANGLES);
}

/*
void Render_system::render_octree(pvec3 player_pos, uint32_t camera) {
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    Camera& camera_camera = ecs.get_component<Camera>(camera);

    pvec3 center = atmo_center;

    std::vector<vec3> ps = {
        vec3(0, 0, 0),
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(1, 1, 0),
        vec3(0, 0, 1),
        vec3(1, 0, 1),
        vec3(0, 1, 1),
        vec3(1, 1, 1),
        
        vec3(0, 0, 0),
        vec3(0, 1, 0),
        vec3(1, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 0, 1),
        vec3(0, 1, 1),
        vec3(1, 0, 1),
        vec3(1, 1, 1),
        
        vec3(0, 0, 0),
        vec3(0, 0, 1),
        vec3(1, 0, 0),
        vec3(1, 0, 1),
        vec3(0, 1, 0),
        vec3(0, 1, 1),
        vec3(1, 1, 0),
        vec3(1, 1, 1),
    };
    

    std::vector<Color_vertex> vs;

    float l2 = log2(2400000.0f * 1.5f);
    l2 = ceil(l2);

    int max_power = l2 + 1;
    int min_power = 4;

    pvec3 origin = center;
    vec3 rel = player_pos - origin;

    // compute octree
    //std::vector<Octree_cell> octree = compute_octree(max_power, min_power, rel, 2.0f);

    float offset = pow(2.0f, l2 + 1) * 0.5f;
    for(Octree_cell& cell : octree) {
        if(cell.is_leaf) {
            float side_length = pow(2.0f, cell.id.w);
            vec3 center = vec3(cell.id.xyz()) * side_length;

            //float s = l2 + 1 - 4;
            //float opacity = float(cell.id.w - 3) / (s + 1) * 0.25f;

            for(vec3 v : ps) vs.push_back(Color_vertex{v * side_length + center - offset, vec4(1.0f, 0.25f, 0.45f, 1.0f)});
        }
    }


    glLineWidth(1);

    vv->vertex_buffer_data(vs.data(), vs.size(), sizeof(Color_vertex), GL_DYNAMIC_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Color_vertex), 0);
    vv->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(Color_vertex), sizeof(float) * 3);

    glm::mat4 model = core.get_model_matrix(center, camera_transform.position);
    glm::mat4 view = mat4(transpose(camera_transform.orientation));
    glm::mat4 proj = camera_camera.proj;

    core.shaders["color_vertex_shader"]->use();

    vv->bind();

    glUniformMatrix4fv(0, 1, false, &view[0][0]);
    glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
    glUniformMatrix4fv(2, 1, false, &model[0][0]);

    vv->draw_vertices(GL_LINES);
}


void Render_system::render_star_octree(pvec3 player_pos, uint32_t camera) {
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    Camera& camera_camera = ecs.get_component<Camera>(camera);

    pvec3 center = pvec3(0.0f);

    std::vector<vec3> ps = {
        vec3(0, 0, 0),
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(1, 1, 0),
        vec3(0, 0, 1),
        vec3(1, 0, 1),
        vec3(0, 1, 1),
        vec3(1, 1, 1),
        
        vec3(0, 0, 0),
        vec3(0, 1, 0),
        vec3(1, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 0, 1),
        vec3(0, 1, 1),
        vec3(1, 0, 1),
        vec3(1, 1, 1),
        
        vec3(0, 0, 0),
        vec3(0, 0, 1),
        vec3(1, 0, 0),
        vec3(1, 0, 1),
        vec3(0, 1, 0),
        vec3(0, 1, 1),
        vec3(1, 1, 0),
        vec3(1, 1, 1),
    };
    

    std::vector<Color_vertex> vs;

    int max_power = 72;
    int min_power = 56;

    float size = pow(2.0f, max_power);

    pvec3 origin = center;
    vec3 rel = player_pos - origin;

    // compute octree
    //std::vector<Octree_cell> octree = compute_octree(max_power, min_power, rel, 2.0f);

    float offset = size * 0.5f;
    for(Octree_cell& cell : octree) {
        if(cell.is_leaf && cell.id.w == min_power) {
            float side_length = pow(2.0f, cell.id.w);
            vec3 center = vec3(cell.id.xyz()) * side_length;

            //float s = max_power - min_power;
            //loat opacity = float(cell.id.w - min_power + 1) / (s + 1) * 0.25f;

            for(vec3 v : ps) vs.push_back(Color_vertex{v * side_length + center - offset, vec4(0.25f, 1.0f, 0.45f, 1.0f)});
        }
    }

    glLineWidth(1);

    vv->vertex_buffer_data(vs.data(), vs.size(), sizeof(Color_vertex), GL_DYNAMIC_DRAW);
    vv->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Color_vertex), 0);
    vv->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(Color_vertex), sizeof(float) * 3);

    glm::mat4 model = core.get_model_matrix(center, camera_transform.position);
    glm::mat4 view = mat4(transpose(camera_transform.orientation));
    glm::mat4 proj = camera_camera.proj;

    core.shaders["color_vertex_shader"]->use();

    vv->bind();

    glUniformMatrix4fv(0, 1, false, &view[0][0]);
    glUniformMatrix4fv(1, 1, false, &camera_camera.proj[0][0]);
    glUniformMatrix4fv(2, 1, false, &model[0][0]);

    vv->draw_vertices(GL_LINES);
}*/


std::array<float, 2> Render_system::get_scatter(vec3 rel_position, float atmo_thickness, float planet_radius, vec3 sun_direction) {
    if(abs(rel_position.x) + abs(rel_position.y) + abs(rel_position.z) > pow(2.0f, 32)) return {0.0f, 0.0f};

    float ii = 1.0 - (length(rel_position) - planet_radius) / atmo_thickness;
    ii = clamp(ii, 0.0f, 1.0f);

    vec3 normal = normalize(rel_position);

    float ndotl = dot(normal, sun_direction);
    float wrap = 0.15;
    float ndotl_wrap = clamp((ndotl + wrap) / (1.0 + wrap), 0.0, 1.0);
    ndotl_wrap = min(ndotl_wrap * 1.5, 1.0);

    float extinction_factor = ndotl_wrap * ii;

    return {extinction_factor, ii};
}