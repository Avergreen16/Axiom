#pragma once;
#include "wrapper.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

struct Mesh_vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;

    glm::ivec4 bone_ids = {-1, -1, -1, -1};
    glm::vec4 bone_weights = vec4(1.0f, 1.0f, 1.0f, 0.0f);
};

struct Mesh_vertex_batch {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;

    uint32_t mesh_id;
};

struct Bone {
    std::string name;

    glm::mat4 offset;
    glm::mat4 inverse_offset;
    glm::mat4 transformation;
    
    int32_t parent;
    std::vector<uint32_t> children;
};

struct Model;

struct Mesh {
    Model* parent_model = 0;

    std::string name;

    std::vector<Mesh_vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Bone> bones;
    std::unordered_map<std::string, uint32_t> name_lookup;

    std::vector<uint32_t> faces;

    std::shared_ptr<Vertices> buffer;

    vec3 color = vec3(1.0f);

    bool skip = false;

    void add_vertices(std::vector<Mesh_vertex> vertices, std::vector<uint32_t> indices);

    void load_buffer();
};

template<typename Type>
struct Frame {
    Type value;
    float time;
};

struct Bone_animation {
    std::string name;

    std::vector<Frame<glm::vec3>> positions;
    std::vector<Frame<glm::quat>> rotations;
    std::vector<Frame<glm::vec3>> scales;
};

struct Bone_orientation {
    std::string name;

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

struct Animation {
    std::string name;
    
    float duration;
    float tps;

    std::vector<Bone_animation> bone_animations;
};

struct Model {
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
    std::unordered_map<std::string, Animation> animations;

    Model(std::string filepath, bool load_buffers = false);

    Model() = default;

    Model(Model&& a);

    void process_mesh(aiMesh* mesh, const aiScene* scene);

    void process_node(aiNode* node, const aiScene* scene);
    
    void process_node_bones(aiNode* node, const aiScene* scene);

    void process_animation(aiAnimation* animation, const aiScene* scene);
    
    void load_model(std::string filepath);
};

struct Animator {
    static std::vector<Bone_orientation> get_orientations(Animation* animation, std::unordered_map<std::string, uint32_t>* mapping, float time);

    static std::vector<Bone_orientation> interpolate(std::vector<Bone_orientation>& a, std::vector<Bone_orientation>& b, float interpolate);

    static std::vector<glm::mat4> get_matrices(std::vector<Bone>* bones, std::unordered_map<std::string, uint32_t>* mapping, std::vector<Bone_orientation>& a);

    static mat4 get_matrix(std::string bone_name, std::vector<Bone>* bones, std::unordered_map<std::string, uint32_t>* mapping, std::vector<Bone_orientation>& a);

    static void skin_mesh(Mesh* m, std::vector<mat4>& bone_matrices, std::vector<Mesh_vertex>& vertices);
};