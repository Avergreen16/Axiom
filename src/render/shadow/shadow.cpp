#include <render/shadow/shadow.hpp>
#include <render/camera3d.hpp>
#include <render/func.hpp>
#include <render/target.hpp>
#include <render/system.hpp>

namespace axiom {

void get_bounding_box(std::vector<vec3> vertices, vec3& min, vec3& max) {
    min = vec3(axiom::max_float);
    max = vec3(-axiom::max_float);

    for(vec3 v : vertices) {
        min = glm::min(min, v);
        max = glm::max(max, v);
    }
}

std::vector<vec3> get_vertices(std::vector<vec3> directions, float near, float far) {
    return {
        directions[0] * near / -directions[0].z,
        directions[1] * near / -directions[1].z,
        directions[2] * near / -directions[2].z,
        directions[3] * near / -directions[3].z,

        directions[0] * far / -directions[0].z,
        directions[1] * far / -directions[1].z,
        directions[2] * far / -directions[2].z,
        directions[3] * far / -directions[3].z,
    };
}

void shadow_renderer::call() {
    axiom::camera3d camera = ecs.get_component<axiom::camera3d>(camera_entity);
    axiom::transform3d camera_transform = ecs.get_component<axiom::transform3d>(camera_entity);
    
    mat4 model = glm::identity<mat4>();
    mat4 view = axiom::get_view(camera, camera_transform);
    mat4 proj = axiom::get_proj(camera);

    std::vector<mat4> shadow_proj(num_cascades);
    std::vector<mat4> shadow_view(num_cascades);

    //
    
    glEnable(GL_DEPTH_CLAMP);

    auto vfunc = [this](int t) {
        return texture_size * base_pixel_size * pow(cascade_factor, t);
    };

    mat3 orientation = glm::identity<mat3>();
    orientation = (mat3)glm::rotate(-(altitude - 90.0f) / 360.0f * 2.0f * axiom::pi, vec3(0.0f, 1.0f, 0.0f)) * orientation;
    orientation = (mat3)glm::rotate(azimuth / 360.0f * 2.0f * axiom::pi, vec3(0.0f, 0.0f, 1.0f)) * orientation;

    std::vector<vec3> frustum = {
        vec3(-1.0f, -1.0f, 0.5f),
        vec3(1.0f, -1.0f, 0.5f),
        vec3(-1.0f, 1.0f, 0.5f),
        vec3(1.0f, 1.0f, 0.5f),
    };

    mat4 inv_proj = glm::inverse(axiom::get_proj(camera));
    for(vec3& v : frustum) {
        vec4 vv = inv_proj * vec4(v, 1.0f);
        vv /= vv.w;

        v = glm::normalize(vv.xyz());
    }

    for(auto& fb : framebuffers) fb.resize(ivec2(texture_size));

    //

    float prev = camera.near;

    for(int i = 0; i < num_cascades; ++i) {
        float width = vfunc(i);
        float texel_size = width / texture_size;

        float boundary = 1.0f;

        float size = width * 0.25f;
        float delta = size;

        while(true) {
            std::vector<vec3> vvs = get_vertices(frustum, prev, size);
            for(vec3& v : vvs) v = transpose(orientation) * camera_transform.orientation * v;

            vec3 min;
            vec3 max;

            get_bounding_box(vvs, min, max);

            vec3 s = max - min;

            if(glm::max(glm::max(s.x, s.y), s.z) > width - boundary) {
                delta *= 0.5f;
                size -= delta;
            } else {
                delta *= 0.5f;
                size += delta;
            }

            if(delta < texel_size * 4) break;
        }
        
        std::vector<vec3> vvs = get_vertices(frustum, camera.near, size);
        for(vec3& v : vvs) v = transpose(orientation) * camera_transform.orientation * v;

        vec3 min;
        vec3 max;
        get_bounding_box(vvs, min, max);
        
        mat3 t = glm::identity<mat3>();//orientation; //
        vec3 vx = t[0];
        vec3 vy = t[1];
        vec3 vz = t[2];

        axiom::transform3d ncamera_transform = camera_transform;

        vec3 center = (min + max) * 0.5f + transpose(orientation) * camera_transform.position;
        
        float texel = texel_size * 4.0f;

        float cx = dot(center, vx);
        float cy = dot(center, vy);
        float cz = dot(center, vz);
        cx = round(cx / texel) * texel;
        cy = round(cy / texel) * texel;
        cz = round(cz / texel) * texel;

        center = cx * vx + cy * vy + cz * vz - transpose(orientation) * camera_transform.position;

        //std::cout << dot((center + ocenter) / texel_size, vx) << " " << dot((center + ocenter) / texel_size, vy) << "\n";



        //

        prev = size;

        shadow_proj[i] = axiom::get_ortho_proj_matrix(-width * 0.5f, width * 0.5f, -width * 0.5f, width * 0.5f, -width * 0.5f, width * 0.5f);

        shadow_view[i] = glm::translate(-center) * mat4(transpose(orientation));
    }

    for(int i = 0; i < num_cascades; ++i) {
        render_func(framebuffers[i], camera_transform, shadow_view[i], shadow_proj[i]);
    }

    //

    glDisable(GL_DEPTH_TEST);
    
    target->framebuffer.bind();

    static axiom::vertices vertices;
    if(!vertices.initialized) vertices.init();

    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)
    };

    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

    auto& shadow_shader = axiom::get_shader("shadow");
    shadow_shader.use();
    
    std::vector<mat4> views;
    std::vector<mat4> projs;
    int i = 0;
    for(int i = 0; i < num_cascades; ++i) {
        framebuffers[i].textures[0].bind(i + 3);
        framebuffers[i].textures[1].bind(i + 8);
    }

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    axiom::push_uniform(8, 5, shadow_view.data());
    axiom::push_uniform(13, 5, shadow_proj.data());

    //

    axiom::push_uniform(19, texture_size);
    axiom::push_uniform(20, base_pixel_size);
    axiom::push_uniform(21, cascade_factor);
    axiom::push_uniform(22, contrast);

    target->framebuffer.textures[1].bind(0);
    target->framebuffer.textures[2].bind(1);
    target->framebuffer.textures[3].bind(2);

    vertices.draw_vertices_triangles();
    
    glDisable(GL_DEPTH_CLAMP);
    glEnable(GL_DEPTH_TEST);

    //

    /*
    //

    axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::get_component<axiom::camera3d>(camera);

    camera_cam.aspect = vec2(f.size) / (float)glm::min(f.size.x, f.size.y);
    
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    // render shape

    glEnable(GL_DEPTH_TEST);
    
    auto& collector_color = axiom::global_core.ecs->collectors["color_mesh3d"];

    for(uint entity : collector_color.entities) {
        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
        axiom::color_mesh3d& mesh = axiom::get_component<axiom::color_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& color_shader = msystem.shaders["color3d"];
        
        vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

        //
        
        color_shader.use();

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform1f(3, msystem.light_contrast);

        mesh.vertices->draw_vertices(GL_TRIANGLES);
    }

    auto& collector_texture = axiom::global_core.ecs->collectors["texture_mesh3d"];

    for(uint entity : collector_texture.entities) {
        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
        axiom::texture_mesh3d& mesh = axiom::get_component<axiom::texture_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& texture_shader = msystem.shaders["texture3d"];
        
        vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

        //
        
        texture_shader.use();
        mesh.texture->bind(0);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform1f(3, msystem.light_contrast);

        mesh.vertices->draw_vertices(GL_TRIANGLES);
    }

    auto& collector_texture_range = axiom::global_core.ecs->collectors["texture_range_mesh3d"];

    for(uint entity : collector_texture_range.entities) {
        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
        axiom::texture_range_mesh3d& mesh = axiom::get_component<axiom::texture_range_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& texture_shader = msystem.shaders["texture_range3d"];

        //
        
        texture_shader.use();
        mesh.texture->bind(0);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);

        mesh.vertices->draw_vertices(GL_TRIANGLES);
    }

    //

    // flag
    */
}

void shadow_renderer::create(uint num_cascades, float cascade_factor, float base_pixel_size, uint texture_size, uint camera, render_target* target, std::function<void(framebuffer&, transform3d&, mat4, mat4)> render_func) {
    shadow_renderer renderer {
        .num_cascades = num_cascades,
        .cascade_factor = cascade_factor,
        .base_pixel_size = base_pixel_size,
        .texture_size = texture_size,
        .camera_entity = camera,
        .target = target,
        .render_func = render_func
    };

    //

    renderer.framebuffers.reserve(num_cascades);
    for(int i = 0; i < num_cascades; ++i) {
        std::vector<fb_tex_params> params {
            fb_tex_params{
                .format = axiom::texture_format::RGBA8,
                .attachment = axiom::texture_attachment::COLOR0,
                .binding = 1,
            },
            fb_tex_params{
                .format = axiom::texture_format::DEPTH32,
                .attachment = axiom::texture_attachment::DEPTH,
            }
        };

        renderer.framebuffers.emplace_back(std::move(axiom::framebuffer(ivec2(texture_size), std::move(params))));
    }

    axiom::render_system& rs = ecs.get_system<axiom::render_system>();

    rs.shadow_renderers.push_back(std::move(renderer));

    target->shadow = &rs.shadow_renderers.back();
} 

}