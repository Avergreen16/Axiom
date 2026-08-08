#pragma once

#include "ecs.hpp"
#include "wrapper.hpp"
#include "random.hpp"
#include "utility.hpp"

const uint32_t chunk_size = 20;
const uint32_t num_threads = 8;

struct crater_population {
    float min_size;
    float max_size;
    float distribution;
    int num_craters;
    int num_ejecta;
};

struct crater {
    vec3 position;
    float radius = 0.1f;
    float ejecta = 0.0f;
    float age = 0.0f;
    float height = 0.0f;
};

struct Voxel_chunk {
    ivec4 id;

    std::vector<float> voxels = std::vector<float>(chunk_size * chunk_size * chunk_size);
    uint32_t scale = 0;

    std::vector<vec3> collision_vertices;
    std::vector<Mesh_vertex> mesh_vertices;

    void mesh();

    void insert_vertices(uint32_t entity);

    void initialize(std::function<float(ivec3)> SDF);
};

struct Voxel_load_data {
    ivec4 id;
    ivec4 parent;
    std::vector<ivec4> children;
    bool leaf_flag = true;
    bool delete_flag = false;
    uint8_t mask = 0x0;
};

struct Crater_partition {
    float region_size;
    std::unordered_map<ivec3, std::vector<uint32_t>, Hash_coord> partition;
};

struct Voxel_field {
    std::unordered_map<ivec4, uint32_t, Hash_coord> chunks;

    std::vector<std::shared_ptr<Octree_cell>> octree;
    std::unordered_map<ivec4, Voxel_load_data, Hash_coord> data;

    std::unordered_set<ivec4, Hash_coord> existing_chunks;
    std::unordered_map<ivec4, uint16_t, Hash_coord> children_mask;
    
    std::unordered_set<ivec4, Hash_coord> delete_buffer;
    
    std::vector<std::shared_ptr<Voxel_chunk>> chunk_buffer;
    std::vector<std::shared_ptr<Voxel_chunk>> front_chunk_buffer;
    uint32_t insert_index = 0;
    uint32_t insert_index_2 = 0;
    
    vec3 prev_octree_center;
    
    // planet data

    float planet_radius;
    
    std::vector<crater_population> populations;
    std::vector<Crater_partition> partitions;
    std::vector<crater> craters;

    void create_craters();
};

struct Voxel_system : System {
    bool run_octrees = true;

    std::atomic<int> active_chunk_threads = 0;
    Thread_pool threads = Thread_pool(num_threads);

    //

    std::mutex delete_mutex;
    std::mutex buffer_mutex;
    
    std::atomic<pvec3> octree_center = pvec3(0.0);

    std::thread chunk_thread;
    

    Voxel_system();

    void call();

    void thread_func();

    static float smooth_min(float a, float b, float k);

    static float crater_func(float f, float depth, float steepness_inner, float steepness_outer, float rim_width);

    static float big_crater_func(float f, float depth, float steepness_inner, float steepness_outer, float rim_width, float steepness_center, float width_center);

    static float bias_func(float x, float bias);

    static float sample_moon_func(vec3 pos, float seed);

    static float get_elev(Voxel_field& field, vec3 pos, int num_craters, float noise);
};