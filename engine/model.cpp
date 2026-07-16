#include "model.hpp"

void Mesh::add_vertices(std::vector<Mesh_vertex> vertices, std::vector<uint32_t> indices) {
    this->vertices = vertices;
    this->indices = indices;
}

void Mesh::load_buffer() {
    buffer = std::shared_ptr<Vertices>(new Vertices());

    buffer->init();
    buffer->vertex_buffer_data(vertices.data(), vertices.size(), sizeof(Mesh_vertex), GL_STATIC_DRAW);
    buffer->index_buffer_data(indices.data(), indices.size(), GL_UNSIGNED_INT, sizeof(uint32_t), GL_STATIC_DRAW);
    buffer->add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(Mesh_vertex), 0);
    buffer->add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(Mesh_vertex), 3 * sizeof(float));
    buffer->add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(Mesh_vertex), 6 * sizeof(float));
    buffer->add_vertex_attribute(3, 4, GL_INT, false, sizeof(Mesh_vertex), 8 * sizeof(float));
    buffer->add_vertex_attribute(4, 4, GL_FLOAT, false, sizeof(Mesh_vertex), 12 * sizeof(float));
}

Model::Model(std::string filepath, bool load_buffers) {
    load_model(filepath);
    if(load_buffers) {
        for(auto& [_, m] : meshes) {
            m->load_buffer();
        }
    }
}

std::string split(std::string str, char c) {
    int f = -1;
    for(int i = 0; i < str.size(); ++i) {
        if(str[i] == c) {
            f = i;
            break;
        }
    }

    if(f != -1) return str.substr(0, f);
    else return str;
}

inline glm::mat4 matrix_to_glm(const aiMatrix4x4& from) {
    glm::mat4 to;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

void Model::process_mesh(aiMesh* mesh, const aiScene* scene) {
    Mesh new_mesh;
    new_mesh.parent_model = this;

    for(unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Mesh_vertex v;
        // process vertex positions, normals and texture coordinates
        v.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
        v.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

        if(mesh->HasTextureCoords(0)) v.tex_coords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};

        v.bone_ids = {-1, -1, -1, -1};

        new_mesh.vertices.push_back(v);
    }

    // process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];

        int num_indices = (max((int)face.mNumIndices - 3, 0) + 1) * 3;
        int v = 2;
        for(unsigned int j = 0; j < face.mNumIndices - 2; j++) {
            new_mesh.indices.push_back(face.mIndices[0]);
            new_mesh.indices.push_back(face.mIndices[v - 1]);
            new_mesh.indices.push_back(face.mIndices[v]);
            ++v;
        }

        new_mesh.faces.push_back(num_indices);
    }

    for(unsigned int i = 0; i < mesh->mNumBones; ++i) {
        aiBone* bone = mesh->mBones[i];

        for(unsigned int j = 0; j < bone->mNumWeights; ++j) {
            aiVertexWeight weight = bone->mWeights[j];

            Mesh_vertex* v = &new_mesh.vertices[weight.mVertexId];

            for(int k = 0; k < 4; ++k) {
                if(v->bone_ids[k] == -1) {
                    v->bone_ids[k] = new_mesh.bones.size();
                    v->bone_weights[k] = weight.mWeight;

                    break;
                }
            }
        }

        Bone b;
        b.name = std::string(bone->mName.data);

        new_mesh.name_lookup.emplace(b.name, new_mesh.bones.size());
        b.offset = matrix_to_glm(bone->mOffsetMatrix);
        b.inverse_offset = glm::inverse(b.offset);

        new_mesh.bones.push_back(b);
    }

    new_mesh.name = mesh->mName.C_Str();
    new_mesh.name = split(new_mesh.name, '.');
    std::string name = new_mesh.name;

    meshes.emplace(name, std::make_shared<Mesh>(std::move(new_mesh)));
}

Model::Model(Model&& a) {
    meshes = std::move(a.meshes);
    
    for(auto& [n, m] : meshes) {
        m->parent_model = this;
    }

    animations = std::move(a.animations);
}

void Model::process_animation(aiAnimation* animation, const aiScene* scene) {
    Animation new_animation;

    float duration = animation->mDuration;
    float tps = animation->mTicksPerSecond;

    new_animation.tps = tps;
    new_animation.duration = duration;

    for(int i = 0; i < animation->mNumChannels; ++i) {
        Bone_animation new_ba;


        aiNodeAnim* channel = animation->mChannels[i];

        new_ba.name = std::string(channel->mNodeName.C_Str());

        for(int k = 0; k < channel->mNumPositionKeys; ++k) {
            aiVectorKey key = channel->mPositionKeys[k];

            float time = key.mTime;
            glm::vec3 position = {key.mValue.x, key.mValue.y, key.mValue.z};

            new_ba.positions.push_back(Frame<glm::vec3>(position, time));
        }

        for(int k = 0; k < channel->mNumRotationKeys; ++k) {
            aiQuatKey key = channel->mRotationKeys[k];

            float time = key.mTime;
            glm::quat rotation = {key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z};

            new_ba.rotations.push_back(Frame<glm::quat>(rotation, time));
        }

        for(int k = 0; k < channel->mNumScalingKeys; ++k) {
            aiVectorKey key = channel->mScalingKeys[k];

            float time = key.mTime;
            glm::vec3 scale = {key.mValue.x, key.mValue.y, key.mValue.z};
            
            new_ba.scales.push_back(Frame<glm::vec3>(scale, time));
        }

        new_animation.bone_animations.push_back(new_ba);
    }

    new_animation.name = animation->mName.C_Str();
    new_animation.name = split(new_animation.name, '.');

    animations.emplace(new_animation.name, new_animation);
}

void Model::process_node(aiNode *node, const aiScene *scene) {
    // process all the node's meshes (if any)
    for(uint32_t i = 0; i < node->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 

        process_mesh(mesh, scene);			
    }

    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        process_node(node->mChildren[i], scene);
    }
}  

void Model::process_node_bones(aiNode *node, const aiScene *scene) {
    if(node->mName.C_Str()) {
        std::string name = node->mName.C_Str();

        for(auto& [_, m] : meshes) {
            if(m->name_lookup.contains(name)) {
                uint32_t i = m->name_lookup[name];
                Bone& self = m->bones[i];

                self.transformation = matrix_to_glm(node->mTransformation);

                if(node->mParent) {
                    std::string parent_name = node->mParent->mName.C_Str();

                    if(m->name_lookup.contains(parent_name)) {
                        int32_t parent_i = m->name_lookup[parent_name];
                        Bone& parent = m->bones[parent_i];

                        parent.children.push_back(i);
                        self.parent = parent_i;
                    } else {
                        self.parent = -1;
                    }
                } else {
                    self.parent = -1;
                }
            }
        }
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        process_node_bones(node->mChildren[i], scene);
    }
}  

float smoothstep(float a) {
    float a2 = a * a;
    return glm::clamp(3.0f * a2 - 2.0f * a2 * a, 0.0f, 1.0f);
}

std::vector<glm::mat4> Animator::get_matrices(std::vector<Bone>* bones, std::unordered_map<std::string, uint32_t>* mapping, std::vector<Bone_orientation>& a) {
    std::vector<glm::mat4> final_matrices(bones->size());
    std::fill(final_matrices.begin(), final_matrices.end(), glm::identity<glm::mat4>());

    std::vector<glm::mat4> relative_matrices(bones->size());

    for(int i = 0; i < bones->size(); ++i) {
        Bone_orientation& current_orientation = a[i];
        relative_matrices[i] = glm::translate(current_orientation.position) * glm::toMat4(current_orientation.rotation) * glm::scale(current_orientation.scale);
    }

    for(int i = 0; i < bones->size(); ++i) {
        Bone* current = &(*bones)[i];
        int current_index = i;
        final_matrices[i] = current->offset;
        while(true) {
            final_matrices[i] = relative_matrices[current_index] * final_matrices[i];

            if(current->parent == -1) break;
            current_index = current->parent;
            current = &(*bones)[current_index];
        }
    }

    return final_matrices;
}

mat4 Animator::get_matrix(std::string bone_name, std::vector<Bone>* bones, std::unordered_map<std::string, uint32_t>* mapping, std::vector<Bone_orientation>& a) {
    mat4 final_matrix = glm::identity<mat4>();

    uint32_t bone_id = (*mapping)[bone_name];

    Bone* current = &(*bones)[bone_id];
    int current_index = bone_id;
    final_matrix = current->offset;
    while(true) {
        Bone_orientation& current_orientation = a[current_index];
        mat4 relative_matrix = glm::translate(current_orientation.position) * glm::toMat4(current_orientation.rotation) * glm::scale(current_orientation.scale);

        final_matrix = relative_matrix * final_matrix;

        if(current->parent == -1) break;
        current_index = current->parent;
        current = &(*bones)[current_index];
    }

    return final_matrix;
}

std::vector<Bone_orientation> Animator::get_orientations(Animation* animation, std::unordered_map<std::string, uint32_t>* mapping, float time) {
    std::vector<Bone_orientation> orientations(mapping->size());

    for(Bone_animation b : animation->bone_animations) {
        if(mapping->contains(b.name)) {
            uint32_t i = (*mapping)[b.name];

            glm::vec3 position;
            glm::quat rotation;
            glm::vec3 scale;

            for(int f = 0; f < b.positions.size(); ++f) {
                Frame<glm::vec3>& frame = b.positions[f];

                float time_difference = time - frame.time;

                if(time_difference < 0) {
                    int f0 = glm::clamp(f - 1, 0, int(b.positions.size() - 1));

                    Frame<glm::vec3>& frame0 = b.positions[f0];

                    float i = 0.0f;
                    if(frame0.time != frame.time) i = (time - frame0.time) / (frame.time - frame0.time);

                    //i = smoothstep(i);

                    position = glm::mix(frame0.value, frame.value, i);
                    break;
                }

                if(f == b.positions.size() - 1) position = frame.value;
            }

            for(int f = 0; f < b.rotations.size(); ++f) {
                Frame<glm::quat>& frame = b.rotations[f];

                float time_difference = time - frame.time;

                if(time_difference < 0) {
                    int f0 = glm::clamp(f - 1, 0, int(b.rotations.size() - 1));

                    Frame<glm::quat>& frame0 = b.rotations[f0];

                    float i = 0.0f;
                    if(frame0.time != frame.time) i = (time - frame0.time) / (frame.time - frame0.time);

                    //i = smoothstep(i);

                    rotation = glm::slerp(frame0.value, frame.value, i);
                    break;
                }

                if(f == b.rotations.size() - 1) rotation = frame.value;
            }

            for(int f = 0; f < b.scales.size(); ++f) {
                Frame<glm::vec3>& frame = b.scales[f];

                float time_difference = time - frame.time;

                if(time_difference < 0) {
                    int f0 = glm::clamp(f - 1, 0, int(b.scales.size() - 1));

                    Frame<glm::vec3>& frame0 = b.scales[f0];

                    float i = 0.0f;
                    if(frame0.time != frame.time) i = (time - frame0.time) / (frame.time - frame0.time);

                    //i = smoothstep(i);

                    scale = glm::mix(frame0.value, frame.value, i);
                    break;
                }

                if(f == b.scales.size() - 1) scale = frame.value;
            }

            Bone_orientation& o = orientations[i];

            o.name = b.name;

            o.position = position;
            o.rotation = rotation;
            o.scale = scale;
        }
    }

    return orientations;
}

std::vector<Bone_orientation> Animator::interpolate(std::vector<Bone_orientation>& a, std::vector<Bone_orientation>& b, float interpolate) {
    std::vector<Bone_orientation> orientations(a.size());

    for(int i = 0; i < a.size(); ++i) {
        Bone_orientation& ba = a[i];
        Bone_orientation& bb = b[i];

        Bone_orientation& bi = orientations[i];

        bi.position = glm::mix(ba.position, bb.position, interpolate);
        bi.rotation = glm::slerp(ba.rotation, bb.rotation, interpolate);
        bi.scale = glm::mix(ba.scale, bb.scale, interpolate);

        bi.name = ba.name;
    }

    return orientations;
}

void Model::load_model(std::string filepath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, aiProcess_GenNormals);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    //directory = path.substr(0, path.find_last_of('/'));

    process_node(scene->mRootNode, scene);

    process_node_bones(scene->mRootNode, scene);

    for(int i = 0; i < scene->mNumAnimations; ++i) {
        process_animation(scene->mAnimations[i], scene);
    }
}

void Animator::skin_mesh(Mesh* m, std::vector<mat4>& bone_matrices, std::vector<Mesh_vertex>& vertices) {

    uint32_t i = 0;
    for(uint32_t f : m->faces) {
        std::vector<Mesh_vertex> vertices_face;

        for(int j = i; j < i + f; ++j) {
            vertices_face.push_back(m->vertices[m->indices[j]]);
        }

        for(Mesh_vertex& v : vertices_face) {
            vec4 pos = vec4(0.0, 0.0, 0.0, 0.0);
            vec4 normal = vec4(0.0, 0.0, 0.0, 0.0);

            for(int i = 0; i < 4; ++i) {
                if(v.bone_ids[i] != -1) {
                    vec4 local_pos = bone_matrices[v.bone_ids[i]] * vec4(v.position, 1.0);
                    pos += local_pos * v.bone_weights[i];

                    vec3 n_local = (mat3)bone_matrices[v.bone_ids[i]] * v.normal;
                    normal += vec4(n_local, 1.0) * v.bone_weights[i];
                }
            }

            if(pos.w == 0) pos = vec4(v.position, 1.0);
            else pos /= pos.w;
            v.position = pos;

            if(normal.w == 0) normal = vec4(v.normal, 1.0f);
            else normal /= normal.w;
            v.normal = normal.xyz();
        }
        
        /*std::vector<vec3> normals;
        for(int j = 0; j < vertices_face.size() / 3; ++j) {
            Mesh_vertex& a = vertices_face[j * 3];
            Mesh_vertex& b = vertices_face[j * 3 + 1];
            Mesh_vertex& c = vertices_face[j * 3 + 2];

            vec3 normal = normalize(cross(a.position - c.position, b.position - c.position));

            normals.push_back(normal);
        }

        vec3 avg_normal = {0, 0, 0};

        for(vec3 n : normals) {
            avg_normal += n;
        }
        avg_normal = normalize(avg_normal);

        for(Mesh_vertex& v : vertices_face) {
            v.normal = avg_normal;
        }*/

        vertices.insert(vertices.end(), vertices_face.begin(), vertices_face.end());

        i += f;
    }
}