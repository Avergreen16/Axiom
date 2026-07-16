#pragma once
#include "ecs.hpp"
#include "model.hpp"
#include "random.hpp"
#include "physics.hpp"
#include "gui.hpp"
#include "particles.hpp"

const uint32_t batch_size = 128;
const float base_radius = 1200 * 2000;
const float astronomical_unit = base_radius * 2000; // 7500

struct Color_vertex {
    vec3 position;
    vec4 color;
};

float hash_float(uint32_t seed);

struct Batch {
    std::vector<uint32_t> mesh_entities;
    std::shared_ptr<Vertices> buffer;
    std::shared_ptr<Uniform_buffer> uniforms;
    std::vector<mat4> matrices;
    bool remesh = false;
    bool recreate_matrices = false;

    void load_vertices() {
        std::vector<Mesh_vertex_batch> v;
        std::vector<uint32_t> indices;

        for(uint32_t j = 0; j < mesh_entities.size(); ++j) {
            Mesh_component& mc = ecs.get_component<Mesh_component>(mesh_entities[j]);
            Mesh* m = mc.mesh.get();
            uint32_t start_index = v.size();
            for(Mesh_vertex& mv : m->vertices) {
                Mesh_vertex_batch mvb;
                mvb.position = mv.position;
                mvb.tex_coords = mv.tex_coords;
                mvb.normal = mv.normal;
                mvb.mesh_id = j;

                v.push_back(mvb);
            }
            
            for(uint32_t index : m->indices) {
                indices.push_back(start_index + index);
            }
        }

        buffer = std::shared_ptr<Vertices>(new Vertices());

        buffer->init();

        buffer->vertex_buffer_data(v.data(), v.size(), sizeof(Mesh_vertex_batch), GL_STREAM_DRAW);
        buffer->index_buffer_data(indices.data(), indices.size(), GL_UNSIGNED_INT, sizeof(uint32_t), GL_STREAM_DRAW);
        
        buffer->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Mesh_vertex_batch), 0);
        buffer->add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(Mesh_vertex_batch), 3 * sizeof(float));
        buffer->add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(Mesh_vertex_batch), 6 * sizeof(float));
        buffer->add_vertex_attribute(3, 1, GL_INT, false, sizeof(Mesh_vertex_batch), 8 * sizeof(float));
    }

    std::vector<mat4> get_model_matrices(pvec3 camera_pos) {
        std::vector<mat4> ret;

        for(uint32_t entity : mesh_entities) {
            Transform& t = ecs.get_component<Transform>(entity);
            
            mat4 model_0 = t.orientation;
            ret.push_back(core.get_model_matrix(t.position, camera_pos) * model_0);
        }

        return ret;
    }
};

struct Render_Target {
    ivec2 target_size;
    std::vector<uint32_t> framebuffers;

    std::function<void(Render_Target&)> func;
};

struct Render_system : System {
    std::vector<Framebuffer> framebuffers;
    std::vector<float> framebuffer_link;
    uvec2 screen_size = {start_x, start_y};

    uint32_t sun_camera;
    uint32_t player_camera;

    vec3 light_direction = vec3(1.0f, 0.0f, 0.0f);

    std::vector<vec3> ssao_samples;

    std::vector<Render_Target> targets;

    std::unordered_set<uint32_t> batched_entities;
    std::vector<Batch> batches;

    std::unordered_map<uint32_t, std::pair<pvec3, mat3>> previous_positions;

    std::shared_ptr<Vertices> vv = std::shared_ptr<Vertices>(new Vertices);
    std::shared_ptr<Storage_buffer> ss = std::shared_ptr<Storage_buffer>(new Storage_buffer);

    mat3 torus_ori = rotate_to(vec3(1.0, 0.0, 0.0), core.random.unit_vector());

    pvec3 sun_pos;

    pvec3 atmo_center;

    vec3 rel_center;
    vec3 rel_sun;

    Render_system();

    void resize_framebuffers();

    void bind_framebuffer(uint32_t i);

    void bind_default_framebuffer();

    void render_particles(uint32_t camera, float scatter, float atmo, ivec2 size);
    void render_icons(uint32_t camera, ivec2 size);
    
    void render_entity(uint32_t entity, uint32_t camera, pvec3 light_pos);
    
    void render_billboard(uint32_t entity, uint32_t camera, pvec3 light_pos);
    
    void render_cursor();
    
    void render_shapes();

    void render_debug(uint32_t camera);

    void render_grid(uint32_t camera);

    void render_debug_lines(uint32_t camera);

    void render_gui();

    void render_crosshair(ivec2 target_size);

    std::array<float, 2> get_scatter(vec3 position, float atmo_thickness, float planet_radius, vec3 sun_direction);

    void call();
};

void create_mesh(Mesh_component& mc, std::vector<vec3> v, vec3 radius);