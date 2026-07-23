#include "voxel.hpp"
#include "core.hpp"
#include "physics.hpp"
#include "input.hpp"
#include "render.hpp"

void Voxel_chunk::initialize(std::function<float(ivec3)> SDF) {
    for(int z = 0; z < chunk_size; ++z) {
        for(int y = 0; y < chunk_size; ++y) {
            for(int x = 0; x < chunk_size; ++x) {
                ivec3 pos = ivec3(x, y, z);
                pos -= 2;

                voxels[x + y * chunk_size + z * chunk_size * chunk_size] = SDF(pos);
            }
        }
    }
}

void Voxel_chunk::mesh() {
    // vec4 color = vec4(0x8B / 0xFF, 0x2E / 0xFF, 0x40 / 0xFF, 1.0f);
    
    float multiplier = uint32_t(1) << scale;

    std::vector<vec3> normals = std::vector<vec3>(chunk_size * chunk_size * chunk_size);
    std::vector<vec3> vertices = std::vector<vec3>(chunk_size * chunk_size * chunk_size);
    std::vector<vec3> offsets = std::vector<vec3>(chunk_size * chunk_size * chunk_size);

    for(int z = 0; z < chunk_size - 1; ++z) {
        for(int y = 0; y < chunk_size - 1; ++y) {
            for(int x = 0; x < chunk_size - 1; ++x) {
                vec3 gradient = vec3(voxels[(x + 1) + y * chunk_size + z * chunk_size * chunk_size] - voxels[x + y * chunk_size + z * chunk_size * chunk_size], voxels[x + (y + 1) * chunk_size + z * chunk_size * chunk_size] - voxels[x + y * chunk_size + z * chunk_size * chunk_size], voxels[x + y * chunk_size + (z + 1) * chunk_size * chunk_size] - voxels[x + y * chunk_size + z * chunk_size * chunk_size]);
                normals[x + y * chunk_size + z * chunk_size * chunk_size] = -normalize(gradient);
            }
        }
    }

    struct edge {
        uint64_t id;
        vec3 v;
    };

    std::vector<edge> edges;

    vec3 offset = vec3(chunk_size * 0.5f);

    for(int z = 1; z < chunk_size - 2; ++z) {
        for(int y = 1; y < chunk_size - 2; ++y) {
            for(int x = 1; x < chunk_size - 2; ++x) {
                vec3 v = {x, y, z};
                v -= 2.0f;

                float f0 = voxels[x + y * chunk_size + z * chunk_size * chunk_size];
                float f1 = voxels[(x + 1) + y * chunk_size + z * chunk_size * chunk_size];
                float f2 = voxels[x + (y + 1) * chunk_size + z * chunk_size * chunk_size];
                float f3 = voxels[(x + 1) + (y + 1) * chunk_size + z * chunk_size * chunk_size];
                float f4 = voxels[x + y * chunk_size + (z + 1) * chunk_size * chunk_size];
                float f5 = voxels[(x + 1) + y * chunk_size + (z + 1) * chunk_size * chunk_size];
                float f6 = voxels[x + (y + 1) * chunk_size + (z + 1) * chunk_size * chunk_size];
                float f7 = voxels[(x + 1) + (y + 1) * chunk_size + (z + 1) * chunk_size * chunk_size];

                bool neg = f0 <= 0 || f1 <= 0 || f2 <= 0 || f3 <= 0 || f4 <= 0 || f5 <= 0 || f6 <= 0 || f7 <= 0;
                bool pos = f0 > 0 || f1 > 0 || f2 > 0 || f3 > 0 || f4 > 0 || f5 > 0 || f6 > 0 || f7 > 0;

                if(neg && pos) {
                    vec3 avg = vec3(0.0f);
                    uint32_t count = 0;

                    // x values
                    if(f0 <= 0 != f1 <= 0) {
                        ++count;

                        float x = f0 / (f0 - f1);
                        avg += vec3(x, 0.0f, 0.0f);
                    }
                    
                    if(f2 <= 0 != f3 <= 0) {
                        ++count;

                        float x = f2 / (f2 - f3);
                        avg += vec3(x, 1.0f, 0.0f);
                    }

                    if(f4 <= 0 != f5 <= 0) {
                        ++count;

                        float x = f4 / (f4 - f5);
                        avg += vec3(x, 0.0f, 1.0f);
                    }
                    
                    if(f6 <= 0 != f7 <= 0) {
                        ++count;

                        float x = f6 / (f6 - f7);
                        avg += vec3(x, 1.0f, 1.0f);
                    }

                    // y values
                    if(f0 <= 0 != f2 <= 0) {
                        ++count;

                        float y = f0 / (f0 - f2);
                        avg += vec3(0.0f, y, 0.0f);
                    }
                    
                    if(f1 <= 0 != f3 <= 0) {
                        ++count;

                        float y = f1 / (f1 - f3);
                        avg += vec3(1.0f, y, 0.0f);
                    }

                    if(f4 <= 0 != f6 <= 0) {
                        ++count;

                        float y = f4 / (f4 - f6);
                        avg += vec3(0.0f, y, 1.0f);
                    }
                    
                    if(f5 <= 0 != f7 <= 0) {
                        ++count;

                        float y = f5 / (f5 - f7);
                        avg += vec3(1.0f, y, 1.0f);
                    }
                    
                    // z values
                    if(f0 <= 0 != f4 <= 0) {
                        ++count;

                        float z = f0 / (f0 - f4);
                        avg += vec3(0.0f, 0.0f, z);
                    }
                    
                    if(f1 <= 0 != f5 <= 0) {
                        ++count;

                        float z = f1 / (f1 - f5);
                        avg += vec3(1.0f, 0.0f, z);
                    }

                    if(f2 <= 0 != f6 <= 0) {
                        ++count;

                        float z = f2 / (f2 - f6);
                        avg += vec3(0.0f, 1.0f, z);
                    }
                    
                    if(f3 <= 0 != f7 <= 0) {
                        ++count;

                        float z = f3 / (f3 - f7);
                        avg += vec3(1.0f, 1.0f, z);
                    }

                    count = max(1u, count);

                    avg /= count;

                    vertices[x + y * chunk_size + z * chunk_size * chunk_size] = (v + 0.5f + avg) * multiplier;
                } else {
                    vertices[x + y * chunk_size + z * chunk_size * chunk_size] = (v + 0.5f) * multiplier;
                }
            }
        }
    }

    std::vector<Mesh_vertex> vs;
    
    std::unordered_set<vec3, Hash_coord> physics_voxels;
    
    for(int z = 2; z < chunk_size - 2; ++z) {
        for(int y = 2; y < chunk_size - 2; ++y) {
            for(int x = 2; x < chunk_size - 2; ++x) {
                float f0 = voxels[x + y * chunk_size + z * chunk_size * chunk_size];
                float fx = voxels[(x + 1) + y * chunk_size + z * chunk_size * chunk_size];
                float fy = voxels[x + (y + 1) * chunk_size + z * chunk_size * chunk_size];
                float fz = voxels[x + y * chunk_size + (z + 1) * chunk_size * chunk_size];
                
                if(f0 > 0) {
                    bool add = false;

                    if(fx <= 0) {
                        add = true;
                        uint32_t i0 = x + (y - 1) * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i1 = x + y * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i2 = x + (y - 1) * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i3 = x + y * chunk_size + z * chunk_size * chunk_size;

                        vec3 v0 = vertices[i0];
                        vec3 v1 = vertices[i1];
                        vec3 v2 = vertices[i2];
                        vec3 v3 = vertices[i3];

                        vec3 n0 = normals[i0];
                        vec3 n1 = normals[i1];
                        vec3 n2 = normals[i2];
                        vec3 n3 = normals[i3];

                        vec2 tc = vec2(56, 8) / 256.0f;

                        Mesh_vertex mv0;
                        mv0.position = v0;
                        mv0.normal = n0;
                        mv0.tex_coords = tc;
                        mv0.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv1;
                        mv1.position = v1;
                        mv1.normal = n1;
                        mv1.tex_coords = tc;
                        mv1.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv2;
                        mv2.position = v2;
                        mv2.normal = n2;
                        mv2.tex_coords = tc;
                        mv2.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv3;
                        mv3.position = v3;
                        mv3.normal = n3;
                        mv3.tex_coords = tc;
                        mv3.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);

                        //

                        uint32_t size = vs.size();

                        vs.push_back(mv0);
                        vs.push_back(mv1);
                        vs.push_back(mv3);
                        vs.push_back(mv0);
                        vs.push_back(mv3);
                        vs.push_back(mv2);
                        
                        uint64_t e0 = uint64_t(i0) | (uint64_t(i1) << 32);
                        uint64_t e1 = uint64_t(i2) | (uint64_t(i3) << 32);
                        uint64_t e2 = uint64_t(i0) | (uint64_t(i2) << 32);
                        uint64_t e3 = uint64_t(i1) | (uint64_t(i3) << 32);

                        edge ee;
                        ee.id = e0;
                        ee.v = v2;
                        edges.push_back(ee);
                        
                        ee.id = e1;
                        ee.v = v0;
                        edges.push_back(ee);
                        
                        ee.id = e2;
                        ee.v = v1;
                        edges.push_back(ee);
                        
                        ee.id = e3;
                        ee.v = v0;
                        edges.push_back(ee);
                    }
                    if(fy <= 0) {
                        add = true;

                        uint32_t i0 = (x - 1) + y * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i1 = x + y * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i2 = (x - 1) + y * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i3 = x + y * chunk_size + z * chunk_size * chunk_size;

                        vec3 v0 = vertices[i0];
                        vec3 v1 = vertices[i1];
                        vec3 v2 = vertices[i2];
                        vec3 v3 = vertices[i3];

                        vec3 n0 = normals[i0];
                        vec3 n1 = normals[i1];
                        vec3 n2 = normals[i2];
                        vec3 n3 = normals[i3];

                        vec2 tc = vec2(56, 8) / 256.0f;

                        Mesh_vertex mv0;
                        mv0.position = v0;
                        mv0.normal = n0;
                        mv0.tex_coords = tc;
                        mv0.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv1;
                        mv1.position = v1;
                        mv1.normal = n1;
                        mv1.tex_coords = tc;
                        mv1.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv2;
                        mv2.position = v2;
                        mv2.normal = n2;
                        mv2.tex_coords = tc;
                        mv2.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv3;
                        mv3.position = v3;
                        mv3.normal = n3;
                        mv3.tex_coords = tc;
                        mv3.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);

                        //

                        vs.push_back(mv1);
                        vs.push_back(mv0);
                        vs.push_back(mv2);
                        vs.push_back(mv1);
                        vs.push_back(mv2);
                        vs.push_back(mv3);
                        
                        uint64_t e0 = uint64_t(i0) | (uint64_t(i1) << 32);
                        uint64_t e1 = uint64_t(i2) | (uint64_t(i3) << 32);
                        uint64_t e2 = uint64_t(i0) | (uint64_t(i2) << 32);
                        uint64_t e3 = uint64_t(i1) | (uint64_t(i3) << 32);

                        edge ee;
                        ee.id = e0;
                        ee.v = v2;
                        edges.push_back(ee);
                        
                        ee.id = e1;
                        ee.v = v0;
                        edges.push_back(ee);
                        
                        ee.id = e2;
                        ee.v = v1;
                        edges.push_back(ee);
                        
                        ee.id = e3;
                        ee.v = v0;
                        edges.push_back(ee);
                    }
                    if(fz <= 0) {
                        add = true;
                        
                        uint32_t i0 = (x - 1) + (y - 1) * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i1 = x + (y - 1) * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i2 = (x - 1) + y * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i3 = x + y * chunk_size + z * chunk_size * chunk_size;

                        vec3 v0 = vertices[i0];
                        vec3 v1 = vertices[i1];
                        vec3 v2 = vertices[i2];
                        vec3 v3 = vertices[i3];

                        vec3 n0 = normals[i0];
                        vec3 n1 = normals[i1];
                        vec3 n2 = normals[i2];
                        vec3 n3 = normals[i3];

                        vec2 tc = vec2(56, 8) / 256.0f;

                        Mesh_vertex mv0;
                        mv0.position = v0;
                        mv0.normal = n0;
                        mv0.tex_coords = tc;
                        mv0.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv1;
                        mv1.position = v1;
                        mv1.normal = n1;
                        mv1.tex_coords = tc;
                        mv1.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv2;
                        mv2.position = v2;
                        mv2.normal = n2;
                        mv2.tex_coords = tc;
                        mv2.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv3;
                        mv3.position = v3;
                        mv3.normal = n3;
                        mv3.tex_coords = tc;
                        mv3.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);

                        //

                        vs.push_back(mv0);
                        vs.push_back(mv1);
                        vs.push_back(mv3);
                        vs.push_back(mv0);
                        vs.push_back(mv3);
                        vs.push_back(mv2);
                        
                        uint64_t e0 = uint64_t(i0) | (uint64_t(i1) << 32);
                        uint64_t e1 = uint64_t(i2) | (uint64_t(i3) << 32);
                        uint64_t e2 = uint64_t(i0) | (uint64_t(i2) << 32);
                        uint64_t e3 = uint64_t(i1) | (uint64_t(i3) << 32);

                        edge ee;
                        ee.id = e0;
                        ee.v = v2;
                        edges.push_back(ee);
                        
                        ee.id = e1;
                        ee.v = v0;
                        edges.push_back(ee);
                        
                        ee.id = e2;
                        ee.v = v1;
                        edges.push_back(ee);
                        
                        ee.id = e3;
                        ee.v = v0;
                        edges.push_back(ee);
                    }

                    if(add) physics_voxels.insert(vec3(x, y, z));
                } else {
                    if(fx > 0) {
                        physics_voxels.insert(vec3(x + 1, y, z));

                        uint32_t i0 = x + (y - 1) * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i1 = x + y * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i2 = x + (y - 1) * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i3 = x + y * chunk_size + z * chunk_size * chunk_size;

                        vec3 v0 = vertices[i0];
                        vec3 v1 = vertices[i1];
                        vec3 v2 = vertices[i2];
                        vec3 v3 = vertices[i3];

                        vec3 n0 = normals[i0];
                        vec3 n1 = normals[i1];
                        vec3 n2 = normals[i2];
                        vec3 n3 = normals[i3];

                        vec2 tc = vec2(56, 8) / 256.0f;

                        Mesh_vertex mv0;
                        mv0.position = v0;
                        mv0.normal = n0;
                        mv0.tex_coords = tc;
                        mv0.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv1;
                        mv1.position = v1;
                        mv1.normal = n1;
                        mv1.tex_coords = tc;
                        mv1.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv2;
                        mv2.position = v2;
                        mv2.normal = n2;
                        mv2.tex_coords = tc;
                        mv2.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv3;
                        mv3.position = v3;
                        mv3.normal = n3;
                        mv3.tex_coords = tc;
                        mv3.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);

                        //

                        vs.push_back(mv0);
                        vs.push_back(mv3);
                        vs.push_back(mv1);
                        vs.push_back(mv0);
                        vs.push_back(mv2);
                        vs.push_back(mv3);
                        
                        uint64_t e0 = uint64_t(i0) | (uint64_t(i1) << 32);
                        uint64_t e1 = uint64_t(i2) | (uint64_t(i3) << 32);
                        uint64_t e2 = uint64_t(i0) | (uint64_t(i2) << 32);
                        uint64_t e3 = uint64_t(i1) | (uint64_t(i3) << 32);

                        edge ee;
                        ee.id = e0;
                        ee.v = v2;
                        edges.push_back(ee);
                        
                        ee.id = e1;
                        ee.v = v0;
                        edges.push_back(ee);
                        
                        ee.id = e2;
                        ee.v = v1;
                        edges.push_back(ee);
                        
                        ee.id = e3;
                        ee.v = v0;
                        edges.push_back(ee);
                    }
                    if(fy > 0) {
                        physics_voxels.insert(vec3(x, y + 1, z));

                        uint32_t i0 = (x - 1) + y * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i1 = x + y * chunk_size + (z - 1) * chunk_size * chunk_size;
                        uint32_t i2 = (x - 1) + y * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i3 = x + y * chunk_size + z * chunk_size * chunk_size;

                        vec3 v0 = vertices[i0];
                        vec3 v1 = vertices[i1];
                        vec3 v2 = vertices[i2];
                        vec3 v3 = vertices[i3];

                        vec3 n0 = normals[i0];
                        vec3 n1 = normals[i1];
                        vec3 n2 = normals[i2];
                        vec3 n3 = normals[i3];

                        vec2 tc = vec2(56, 8) / 256.0f;

                        Mesh_vertex mv0;
                        mv0.position = v0;
                        mv0.normal = n0;
                        mv0.tex_coords = tc;
                        mv0.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv1;
                        mv1.position = v1;
                        mv1.normal = n1;
                        mv1.tex_coords = tc;
                        mv1.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv2;
                        mv2.position = v2;
                        mv2.normal = n2;
                        mv2.tex_coords = tc;
                        mv2.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv3;
                        mv3.position = v3;
                        mv3.normal = n3;
                        mv3.tex_coords = tc;
                        mv3.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);

                        //

                        vs.push_back(mv1);
                        vs.push_back(mv2);
                        vs.push_back(mv0);
                        vs.push_back(mv1);
                        vs.push_back(mv3);
                        vs.push_back(mv2);
                        
                        uint64_t e0 = uint64_t(i0) | (uint64_t(i1) << 32);
                        uint64_t e1 = uint64_t(i2) | (uint64_t(i3) << 32);
                        uint64_t e2 = uint64_t(i0) | (uint64_t(i2) << 32);
                        uint64_t e3 = uint64_t(i1) | (uint64_t(i3) << 32);

                        edge ee;
                        ee.id = e0;
                        ee.v = v2;
                        edges.push_back(ee);
                        
                        ee.id = e1;
                        ee.v = v0;
                        edges.push_back(ee);
                        
                        ee.id = e2;
                        ee.v = v1;
                        edges.push_back(ee);
                        
                        ee.id = e3;
                        ee.v = v0;
                        edges.push_back(ee);
                    }
                    if(fz > 0) {
                        physics_voxels.insert(vec3(x, y, z + 1));

                        uint32_t i0 = (x - 1) + (y - 1) * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i1 = x + (y - 1) * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i2 = (x - 1) + y * chunk_size + z * chunk_size * chunk_size;
                        uint32_t i3 = x + y * chunk_size + z * chunk_size * chunk_size;

                        vec3 v0 = vertices[i0];
                        vec3 v1 = vertices[i1];
                        vec3 v2 = vertices[i2];
                        vec3 v3 = vertices[i3];

                        vec3 n0 = normals[i0];
                        vec3 n1 = normals[i1];
                        vec3 n2 = normals[i2];
                        vec3 n3 = normals[i3];

                        vec2 tc = vec2(56, 8) / 256.0f;

                        Mesh_vertex mv0;
                        mv0.position = v0;
                        mv0.normal = n0;
                        mv0.tex_coords = tc;
                        mv0.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv1;
                        mv1.position = v1;
                        mv1.normal = n1;
                        mv1.tex_coords = tc;
                        mv1.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv2;
                        mv2.position = v2;
                        mv2.normal = n2;
                        mv2.tex_coords = tc;
                        mv2.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);
                        
                        Mesh_vertex mv3;
                        mv3.position = v3;
                        mv3.normal = n3;
                        mv3.tex_coords = tc;
                        mv3.bone_weights = vec4(1.0f, 1.0f, 1.0f, 1.0);

                        //

                        vs.push_back(mv0);
                        vs.push_back(mv3);
                        vs.push_back(mv1);
                        vs.push_back(mv0);
                        vs.push_back(mv2);
                        vs.push_back(mv3);
                        
                        uint64_t e0 = uint64_t(i0) | (uint64_t(i1) << 32);
                        uint64_t e1 = uint64_t(i2) | (uint64_t(i3) << 32);
                        uint64_t e2 = uint64_t(i0) | (uint64_t(i2) << 32);
                        uint64_t e3 = uint64_t(i1) | (uint64_t(i3) << 32);

                        edge ee;
                        ee.id = e0;
                        ee.v = v2;
                        edges.push_back(ee);
                        
                        ee.id = e1;
                        ee.v = v0;
                        edges.push_back(ee);
                        
                        ee.id = e2;
                        ee.v = v1;
                        edges.push_back(ee);
                        
                        ee.id = e3;
                        ee.v = v0;
                        edges.push_back(ee);
                    }
                }
            }
        }
    }

    uint32_t num_physics_triangles = vs.size() / 3;

    /**/
    std::sort(edges.begin(), edges.end(), 
        [](edge& a, edge& b) {
            return a.id < b.id;
        }
    );

    for(int i = 0; i < edges.size();) {
        uint64_t ii = edges[i].id;
        int start = i;
        int end = i + 1;

        int count = 0;
        while(true) {
            if(edges[end].id == ii) {
                ++end;
            } else break;
        }

        if(end - start == 1) {
            uint32_t a = ii & 0xFFFFFFFF;
            uint32_t b = (ii >> 32) & 0xFFFFFFFF;

            vec3 va = vertices[a];
            vec3 vb = vertices[b];
            vec3 na = normals[a];
            vec3 nb = normals[b];

            vec3 dir = vb - va;
            if(dot(cross(dir, edges[i].v - va), na) < 0.0) {
                std::swap(va, vb);
                std::swap(na, nb);
                std::swap(a, b);
                dir = -dir;
            }

            vec3 out_a = cross(na, dir);
            vec3 out_b = cross(na, dir);

            offsets[a] += normalize(out_a) * multiplier;
            offsets[b] += normalize(out_b) * multiplier;
        }
        i = end;
    }

    // skirt
    for(int i = 0; i < edges.size();) {
        uint64_t ii = edges[i].id;
        int start = i;
        int end = i + 1;

        int count = 0;
        while(true) {
            if(edges[end].id == ii) {
                ++end;
            } else break;
        }

        if(end - start == 1) {
            uint32_t a = ii & 0xFFFFFFFF;
            uint32_t b = (ii >> 32) & 0xFFFFFFFF;

            vec3 va = vertices[a];
            vec3 vb = vertices[b];
            vec3 na = normals[a];
            vec3 nb = normals[b];

            vec3 dir = vb - va;
            if(dot(cross(dir, edges[i].v - va), na) < 0.0) {
                std::swap(va, vb);
                std::swap(na, nb);
                std::swap(a, b);
                dir = -dir;
            }

            vec3 vc = va - na * multiplier * 1.0f - offsets[a] * 0.5f;
            vec3 vd = vb - nb * multiplier * 1.0f - offsets[b] * 0.5f;

            vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f);//vec4(1.0f, 0.25f, 0.25f, 1.0f);
            
            Mesh_vertex mva;
            mva.position = va;
            mva.normal = na;
            mva.bone_weights = color;
            
            Mesh_vertex mvb;
            mvb.position = vb;
            mvb.normal = nb;
            mvb.bone_weights = color;
            
            Mesh_vertex mvc;
            mvc.position = vc;
            mvc.normal = na;
            mvc.bone_weights = color;
            
            Mesh_vertex mvd;
            mvd.position = vd;
            mvd.normal = nb;
            mvd.bone_weights = color;

            vs.push_back(mvc);
            vs.push_back(mvd);
            vs.push_back(mvb);
            vs.push_back(mvc);
            vs.push_back(mvb);
            vs.push_back(mva);
        }
        i = end;
    }

    //ivec4 tex = ivec4(0, 96, 32, 32);
    //vec4 color = vec4(hex_color(0x0A2044), 1.0f);
    ivec4 tex = ivec4(0, 128, 16, 16);
    //vec4 color = vec4(hex_color(0xefba86), 1.0f);
    vec4 color = vec4(hex_color(0x777777), 1.0f);

    for(int i = 0; i < vs.size() / 6; ++i) {
        Mesh_vertex& a = vs[i * 6];
        Mesh_vertex& b = vs[i * 6 + 1];
        Mesh_vertex& c = vs[i * 6 + 2];
        Mesh_vertex& d = vs[i * 6 + 3];
        Mesh_vertex& e = vs[i * 6 + 4];
        Mesh_vertex& f = vs[i * 6 + 5];

        a.bone_weights *= color;
        b.bone_weights *= color;
        c.bone_weights *= color;
        d.bone_weights *= color;
        e.bone_weights *= color;
        f.bone_weights *= color; 

        vec3 normal = normalize(cross(a.position - c.position, b.position - c.position) + cross(d.position - f.position, e.position - f.position));

        normal /= max(abs(normal.x), max(abs(normal.y), abs(normal.z)));
        normal = round(normal);
        normal = normalize(normal);

        mat3 ori = rotate_to(vec3(0.0f, 0.0f, 1.0f), normal);
        vec3 x = ori[0];
        vec3 y = ori[1];

        a.tex_coords = vec2(dot(x, a.position), dot(y, a.position)) / 16.0f;
        b.tex_coords = vec2(dot(x, b.position), dot(y, b.position)) / 16.0f;
        c.tex_coords = vec2(dot(x, c.position), dot(y, c.position)) / 16.0f;
        d.tex_coords = vec2(dot(x, d.position), dot(y, d.position)) / 16.0f;
        e.tex_coords = vec2(dot(x, e.position), dot(y, e.position)) / 16.0f;
        f.tex_coords = vec2(dot(x, f.position), dot(y, f.position)) / 16.0f;

        a.bone_ids = tex;
        b.bone_ids = tex;
        c.bone_ids = tex;
        d.bone_ids = tex;
        e.bone_ids = tex;
        f.bone_ids = tex;
    }

    std::vector<vec3> triangles;

    if(scale == 0) {
        std::vector<Mesh_vertex> mvs;

        for(int i = 0; i < num_physics_triangles; ++i) {
            Mesh_vertex a = vs[i * 3];
            Mesh_vertex b = vs[i * 3 + 1];
            Mesh_vertex c = vs[i * 3 + 2];

            a.bone_weights = vec4(1.0f, 0.25f, 0.25f, 1.0f);
            b.bone_weights = vec4(1.0f, 0.25f, 0.25f, 1.0f);
            c.bone_weights = vec4(1.0f, 0.25f, 0.25f, 1.0f);

            mvs.push_back(a);
            mvs.push_back(b);
            mvs.push_back(c);

            vec3 av = a.position;
            vec3 bv = b.position;
            vec3 cv = c.position;

            triangles.push_back(av);
            triangles.push_back(bv);
            triangles.push_back(cv);
        }
            
        /*
        for(vec3 v : physics_voxels) {
            vec3 v0 = vertices[(v.x - 1) + (v.y - 1) * chunk_size + (v.z - 1) * chunk_size * chunk_size];
            vec3 v1 = vertices[v.x + (v.y - 1)* chunk_size + (v.z - 1) * chunk_size * chunk_size];
            vec3 v2 = vertices[(v.x - 1) + v.y * chunk_size + (v.z - 1) * chunk_size * chunk_size];
            vec3 v3 = vertices[v.x + v.y * chunk_size + (v.z - 1) * chunk_size * chunk_size];
            vec3 v4 = vertices[(v.x - 1) + (v.y - 1) * chunk_size + v.z * chunk_size * chunk_size];
            vec3 v5 = vertices[v.x + (v.y - 1) * chunk_size + v.z * chunk_size * chunk_size];
            vec3 v6 = vertices[(v.x - 1) + v.y * chunk_size + v.z * chunk_size * chunk_size];
            vec3 v7 = vertices[v.x + v.y * chunk_size + v.z * chunk_size * chunk_size];
            vec3 rel_pos = v - offset;


            vec3 avg = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7;
            avg /= 8.0f;
            for(vec3& vv : shape.vertices) {
                vv -= avg;
            }

            Physics_system::initialize_collision_shape(shape);
            
            collider.collision_shapes.push_back(Convex_collider());
            collider.collision_shapes[collider.collision_shapes.size() - 1].position = avg;
            collider.collision_shapes[collider.collision_shapes.size() - 1].collision_shape = std::make_shared<Collision_shape>(shape);  
        }
        */
    }

    collision_vertices = triangles;
    mesh_vertices = vs;
}

void Voxel_chunk::insert_vertices(uint32_t entity) {
    Transform& transform = ecs.get_component<Transform>(entity);
    Mesh_component& mc = ecs.get_component<Mesh_component>(entity);

    if(scale == 0) {
        Collider& collider = ecs.get_component<Collider>(entity);
        collider.mass = 0.0f;
        collider.is_static = true;
        collider.collision_shapes.clear();

        for(int i = 0; i < collision_vertices.size() / 3; ++i) {
            vec3 av = collision_vertices[i * 3];
            vec3 bv = collision_vertices[i * 3 + 1];
            vec3 cv = collision_vertices[i * 3 + 2];

            vec3 avg = (av + bv + cv) / 3.0f;
            
            Collision_shape shape;
            shape.mass = 0.0f;
            shape.radius = 0.0f;
            shape.vertices = {
                av - avg, bv - avg, cv - avg
            };

            shape.faces = {shape_face{{0, 1, 2}, normalize(cross(av - cv, bv - cv))}};
            
            collider.collision_shapes.push_back(Convex_collider());
            collider.collision_shapes[collider.collision_shapes.size() - 1].position = avg;
            collider.collision_shapes[collider.collision_shapes.size() - 1].collision_shape = std::make_shared<Collision_shape>(shape);  
        }
        
        //std::cout << "f";
        Physics_system& ps = ecs.get_system<Physics_system>();
        collider.create_BVH(id.xyz());
        //std::cout << "f";
    }
    
    if(!mc.mesh) {
        std::shared_ptr<Mesh> mesh(new Mesh);
        mc.mesh = mesh;
        mc.texture = core.textures["tilesheet"];
    }

    std::vector<uint32_t> indices(mesh_vertices.size());
    for(int i = 0; i < mesh_vertices.size(); ++i) {
        indices[i] = i;
    }

    if(mesh_vertices.size()) {
        mc.mesh->add_vertices(mesh_vertices, indices);
        mc.mesh->load_buffer();
    } else {
        mc.mesh->skip = true;
    }
}

Voxel_system::Voxel_system() {
    Signature s = ecs.update_signature<Voxel_chunk>();
    collectors.push_back(Collector(s));
    
    s = ecs.update_signature<Voxel_field>();
    collectors.push_back(Collector(s));

    chunk_thread = std::thread(
        [&]() {
            thread_func();
        }
    );
}

void Voxel_system::call() {
    Physics_system& ps = ecs.get_system<Physics_system>();
    Input_system& input_system = ecs.get_system<Input_system>();
    Render_system& render_system = ecs.get_system<Render_system>();

    uint32_t camera_entity = input_system.player_camera;
    Camera& camera_camera = ecs.get_component<Camera>(camera_entity);
    Transform& camera_transform = ecs.get_component<Transform>(camera_entity);

    for(uint32_t planet_entity : collectors[1].entities) {
        Voxel_field& field = ecs.get_component<Voxel_field>(planet_entity);
        Transform& transform = ecs.get_component<Transform>(planet_entity);

        
        float l2 = log2(field.planet_radius * 2.5f);
        l2 = ceil(l2);
        int max_power = l2;
        int min_power = 4;
        float size = uint32_t(1) << max_power;

        octree_center = camera_transform.position;

        // insert finished chunks
        uint32_t num_insert = 64;
        buffer_mutex.lock();
        field.front_chunk_buffer.insert(field.front_chunk_buffer.end(), field.chunk_buffer.begin(), field.chunk_buffer.end());
        field.chunk_buffer.clear();
        buffer_mutex.unlock();
        
        uint32_t i = 0;
        for(; field.insert_index < field.front_chunk_buffer.size(); ++field.insert_index) {
            std::shared_ptr<Voxel_chunk> chunk = field.front_chunk_buffer[field.insert_index];
            if(!field.chunks.contains(chunk->id)) {
                uint32_t entity = NULL_ENTITY;
                if(chunk->mesh_vertices.size()) {
                    // insert
                    entity = ecs.insert_entity();

                    float p2 = uint32_t(1) << chunk->id.w;
                    vec3 chunk_pos = p2 * vec3(chunk->id.xyz());
                    uint32_t scale = chunk->id.w - min_power;

                    Transform tf;
                    tf.orientation = identity<mat3>();
                    tf.position = transform.position + pvec3(chunk_pos - size * 0.5f);
                    
                    Mesh_component mc;
                    ecs.insert_component(entity, tf);
                    ecs.insert_component(entity, mc);
                    ecs.insert_component(entity, *chunk.get());

                    if(scale == 0) {
                        Collider cc;
                        ecs.insert_component(entity, cc);
                    }

                    Voxel_chunk& vc0 = ecs.get_component<Voxel_chunk>(entity);
                    vc0.insert_vertices(entity);
                }

                delete_mutex.lock();
                field.chunks.emplace(chunk->id, entity);
                delete_mutex.unlock();

                ++i;

                --active_chunk_threads;
            }
            
            //if(i > num_insert) {
            //    break;
            //}
        }

        if(field.insert_index == field.front_chunk_buffer.size()) {
            field.front_chunk_buffer.clear();
            field.insert_index = 0;
        }

        // delete unneeded chunks
        delete_mutex.lock();
        for(ivec4 v : field.delete_buffer) {
            if(field.chunks.contains(v)) {
                if(field.chunks[v] != NULL_ENTITY) ecs.erase_entity(field.chunks[v]);
                field.chunks.erase(v);
            }
        }
        field.delete_buffer.clear();
        delete_mutex.unlock();
    }
    
    // data 
    
    /*
    for(ivec4 v : remove_set) {
        if(chunks.contains(v)) {
            ecs.erase_entity(chunks[v]);
            chunks.erase(v);
            existing_chunks.erase(v);
        }
    }
    remove_set.clear();
    */

    /*
    for(auto& vc : front_buffer_insert) {
        ivec4 id = vc->id;

        float p2 = uint32_t(1) << id.w;

        vec3 chunk_pos = p2 * vec3(id.xyz());

        uint32_t entity = ecs.insert_entity();

        Transform tf;
        tf.orientation = identity<mat3>();
        tf.position = render_system.atmo_center + pvec3(chunk_pos - size * 0.5f);

        Mesh_component mc;
        ecs.insert_component(entity, tf);
        ecs.insert_component(entity, mc);
        ecs.insert_component(entity, *vc.get());

        uint32_t scale = id.w - min_power;

        if(scale == 0) {
            Collider cc;
            ecs.insert_component(entity, cc);
        }

        Voxel_chunk& vc0 = ecs.get_component<Voxel_chunk>(entity);
        vc0.insert_vertices(entity);
        //for(ivec4 v : remove) front_buffer_remove.insert(v)
        
        chunk_mutex.lock();
        chunks.emplace(id, entity);
        chunk_mutex.unlock();
        
        children_mask.emplace(id, uint16_t(0xFFFF));
        std::vector<ivec4> parents = get_parents(id);
        int counter = 0;
        for(ivec4 v : parents) {
            if(children_mask.contains(v)) {
                if(counter == 1) break;
                else {
                    ++counter;
                    children_mask.emplace(v, 0x0000);
                }
            } else {
                front_buffer_remove.emplace(v);
                children_mask.emplace(v, 0x0000);
            }
        }
    }

    for(auto it = front_buffer_remove.begin(); it != front_buffer_remove.end();) {
        bool erase = false;
        ivec4 b = *it;

        if(b.w > 4) {
            uint16_t& k = children_mask[b];

            if((k & 0xFF00) == 0x8000) {
                if(chunks.contains(b)) {
                    uint32_t c = chunks[b];
                    ecs.erase_entity(c);

                    chunk_mutex.lock();
                    existing_mutex.lock();
                    chunks.erase(b);
                    existing_chunks.erase(b);
                    chunk_mutex.unlock();
                    existing_mutex.unlock();
                }

                erase = true;
            } else {
                std::vector<ivec4> children = get_children(b);

                int ii = 0;
                for(ivec4 bb : children) {
                    if(children_mask.contains(bb)) {
                        uint16_t kk = children_mask[bb];
                        if(kk == 0xFF || kk == 0xFFFF) {
                            k |= uint16_t(1) << ii;
                        }
                    } else std::cout << bb.x << " " << bb.y << " " << bb.z << " " << bb.w << " ";
                    ++ii;
                }

                if(k == 0xFF) {
                    if(chunks.contains(b)) {
                        uint32_t c = chunks[b];
                        ecs.erase_entity(c);

                        chunk_mutex.lock();
                        existing_mutex.lock();
                        chunks.erase(b);
                        existing_chunks.erase(b);
                        chunk_mutex.unlock();
                        existing_mutex.unlock();
                    }

                    //for(ivec4 bb : children) children_mask.erase(bb);

                    erase = true;
                }
            }
        } else {
            children_mask[b] = 0xFF;

            if(chunks.contains(b)) {
                uint32_t c = chunks[b];
                ecs.erase_entity(c);

                chunk_mutex.lock();
                existing_mutex.lock();
                chunks.erase(b);
                existing_chunks.erase(b);
                chunk_mutex.unlock();
                existing_mutex.unlock();
            }

            erase = true;
        }

        if(erase) it = front_buffer_remove.erase(it);
        else ++it;
    }

    std::cout << front_buffer_remove.size() << "\n";

    for(uint32_t entity : collectors[0].entities) {
        Voxel_chunk& vc = ecs.get_component<Voxel_chunk>(entity);

        if(!chunks.contains(vc.id)) {
            std::cout << "ERROR ";
        }
    }
    */
}

void Voxel_system::thread_func() {
    while(!ecs.has_system<Render_system>()) {}
    // systems
    Physics_system& ps = ecs.get_system<Physics_system>();
    Input_system& input_system = ecs.get_system<Input_system>();
    Render_system& render_system = ecs.get_system<Render_system>();

    static double prev_time = get_time();
    static double time = 0.0;

    while(true) {
        pvec3 oc = octree_center.load();
        
        double new_time = get_time();
        time += new_time - prev_time;
        prev_time = new_time;
        
        for(uint32_t planet_entity : collectors[1].entities) {
            Voxel_field& field = ecs.get_component<Voxel_field>(planet_entity);
            Transform& transform = ecs.get_component<Transform>(planet_entity);
            // state variables
            float l2 = log2(field.planet_radius * 2.5f);
            l2 = ceil(l2);
            int max_power = l2;
            int min_power = 4;
            float size = uint32_t(1) << max_power;
            
            //

            vec3 new_center = apply_matrix(transpose(transform.orientation), oc - transform.position);
            
            vec3 diff = new_center - field.prev_octree_center;
            
            if(length(diff) > 2.0 && time > 0.25 && run_octrees) {
                field.prev_octree_center = new_center;

                vec3 rel = field.prev_octree_center;
                field.octree = compute_octree_with_neighbors(max_power, min_power, rel, sqrt(0.75f), 64.0f, max_power - 4);


                std::unordered_set<ivec4, Hash_coord> leaves; // ids that need a mesh
                std::unordered_set<ivec4, Hash_coord> new_data; // ids that are in the new octree
                std::unordered_set<ivec4, Hash_coord> remove; // ids in the old octree but not in the new octree
                std::unordered_set<ivec4, Hash_coord> add; // ids in the new octree but not in the old octree

                for(auto& s : field.octree) {
                    new_data.insert(s->id);
                    if(s->is_leaf) leaves.insert(s->id);
                }
                for(auto& key : new_data) {
                    if(!field.data.contains(key)) add.insert(key);
                }
                for(auto& [key, vs] : field.data) {
                    if(!new_data.contains(key)) remove.insert(key);
                }

                //

                for(ivec4 v : add) {
                    Voxel_load_data d;
                    d.id = v;
                    
                    d.parent = ivec4(v.xyz() / 2, v.w + 1);

                    field.data.emplace(v, d);
                }

                for(auto& [k, c] : field.data) {
                    if(remove.contains(k)) {
                        c.delete_flag = true;
                    } else c.delete_flag = false;
                    
                    if(leaves.contains(k)) {
                        c.leaf_flag = true;
                    } else {
                        c.leaf_flag = false;
                    }
                }

                time = 0.0;
                field.insert_index_2 = 0;
            }

            for(; field.insert_index_2 < field.octree.size(); ++field.insert_index_2) {
                if(active_chunk_threads >= 96) {
                    --field.insert_index_2;
                    break;
                }

                std::shared_ptr<Octree_cell>& cell = field.octree[field.insert_index_2];

                if(cell->is_leaf) {
                    if(!field.existing_chunks.contains(cell->id)) {
                        field.existing_chunks.insert(cell->id);

                        ++active_chunk_threads;
                        threads.add_task(
                            [&, cid = cell->id]() {
                                float l2 = log2(field.planet_radius * 2.5f);
                                l2 = ceil(l2);
                                int max_power = l2;
                                int min_power = 4;
                                float size = uint32_t(1) << max_power;

                                //

                                float p2 = uint32_t(1) << cid.w;

                                vec3 chunk_pos = p2 * vec3(cid.xyz());

                                uint32_t scale = cid.w - min_power;

                                std::shared_ptr<Voxel_chunk> vc(new Voxel_chunk);
                                vc->scale = scale;
                                uint32_t voxel_size = uint32_t(1) << scale;

                                std::vector<float> noise = Noise_gen::perlin_noise(chunk_pos + (0.5f - 2.0f) * float(voxel_size), 256.0f, 3, 0xB3, ivec3(chunk_size), p2 / 16);
                                //std::vector<float> noise2 = Noise_gen::perlin_noise(chunk_pos + (0.5f - 2.0f) * float(voxel_size), 0x10, 3, 0xB3, ivec3(chunk_size), p2 / 16);
                                
                                std::function<float(ivec3)> lambda = [&](ivec3 pos) -> float {
                                    dvec3 v = ((vec3(pos) + 0.5f)) * float(voxel_size) + (chunk_pos - size * 0.5f);

                                    dvec3 r = vec3(field.planet_radius);
                                    float density = r.x - length(v);

                                    ivec3 p2 = pos + 2;

                                    uint32_t i = p2.x + p2.y * chunk_size + p2.z * chunk_size * chunk_size;
                                    
                                    float elev;
                                    if(field.craters.size()) elev = get_elev(field, v, field.craters.size(), noise[p2.x + p2.y * chunk_size + p2.z * chunk_size * chunk_size]);
                                    else elev = noise[i] * 16.0f;// * clamp(noise2[i] + 0.5f, 0.0f, 1.0f) * 0x8000;
                                    
                                    density += elev;

                                    return density;
                                };

                                vc->initialize(lambda);
                                vc->mesh();

                                vc->id = cid;

                                buffer_mutex.lock();
                                field.chunk_buffer.push_back(vc);
                                buffer_mutex.unlock();
                            }
                        );
                    }
                }
            }

            std::vector<ivec4> path = {ivec4(0, 0, 0, max_power)};
            std::vector<uint32_t> path_ids = {0};
            std::vector<uint8_t> masks = {0x0};
            std::vector<bool> del = {0x0};
            std::vector<uint8_t> accum;

            std::unordered_set<ivec4, Hash_coord> delete_set;
            std::unordered_set<ivec4, Hash_coord> erase_from_data;
            bool children_delete = false;
            uint32_t num_leaves = 0;

            delete_mutex.lock();
            auto current_chunks = field.chunks;
            delete_mutex.unlock();

            while(true) {
                ivec4 current = path.back();
                Voxel_load_data& d = field.data[current];
                if(d.leaf_flag) ++num_leaves;

                if(d.leaf_flag && !d.delete_flag && current_chunks.contains(current)) {
                    masks.back() = 0xFF;
                    del.back() = true;
                    children_delete = true;
                } else {
                    if(children_delete && field.existing_chunks.contains(current)) {
                        delete_set.emplace(current);
                    }
                }

                if(children_delete && !del.back()) erase_from_data.insert(current);

                ivec4 child = ivec4(current.xyz() * 2, current.w - 1);
                if(path_ids.back() == 8 || !field.data.contains(child) || child.w < min_power) {
                    if(path.size() == 1) break; // finish

                    if(masks.back() == 0xFF) {
                        masks[masks.size() - 2] |= (uint8_t(1) << (path_ids[path_ids.size() - 2] - 1));
                        if(!del.back()) delete_set.insert(current);
                    }
                    accum.push_back(masks.back());

                    path_ids.pop_back();
                    path.pop_back();
                    masks.pop_back();

                    if(del.back()) children_delete = false;
                    del.pop_back();
                } else {
                    std::vector<ivec4> children = get_children(current);
                    path.push_back(children[path_ids.back()]);

                    ++path_ids.back();
                    path_ids.push_back(0);
                    masks.push_back(0x0);
                    del.push_back(false);
                }
            }

            for(ivec4 v : erase_from_data) field.data.erase(v);

            std::vector<ivec4> erase_mesh;
            
            for(auto it = delete_set.begin(); it != delete_set.end();) {
                if(current_chunks.contains(*it)) {
                    erase_mesh.push_back(*it);
                    field.existing_chunks.erase(*it);

                    it = delete_set.erase(it);
                } else ++it;
            }

            delete_mutex.lock();
            field.delete_buffer.insert(erase_mesh.begin(), erase_mesh.end());
            delete_mutex.unlock();
        }
    }
}

float Voxel_system::smooth_min(float a, float b, float k) {
    float h = clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return mix(b, a, h) - k * h * (1.0f - h);
};

float Voxel_system::crater_func(float f, float depth, float steepness_inner, float steepness_outer, float rim_width) {
    float walls = (f * f - 1) * steepness_inner;
    float rim = (f - (1.0f + rim_width));
    rim = rim * rim * steepness_outer;

    float result = smooth_min(walls, rim, 0.01f);

    return result;
};
//smooth_min(walls, rim, 0.05f);

float Voxel_system::big_crater_func(float f, float depth, float steepness_inner, float steepness_outer, float rim_width, float steepness_center, float width_center) {
    float walls = (f * f - 1) * steepness_inner;
    float rim = (f - (1.0f + rim_width));
    rim = rim * rim * steepness_outer;

    float peak_pos = f - width_center;
    peak_pos = peak_pos * peak_pos * steepness_center;
    peak_pos += depth;
    if(f > width_center) peak_pos = mix(depth, -1.0f, smoothstep(width_center, 1.5f, f));

    float peak_neg = f + width_center;
    peak_neg = max(peak_neg, 0.0f);
    peak_neg = peak_neg * peak_neg * steepness_center;

    float crater_floor = mix(depth, -1.0f, smoothstep(1.0f, 1.5f, f));

    float peak = smooth_min(peak_pos, peak_neg, 0.01f);

    float result = smooth_min(walls, rim, 0.01f);
    if(steepness_center != 0.0f) result = smooth_min(result, peak, -0.01f);
    result = smooth_min(result, crater_floor, -0.01f);

    return result;
};

float Voxel_system::bias_func(float x, float bias) {
    float k = pow(1 - bias, 3);
    return x * k / (x * k - x + 1);
};

float Voxel_system::sample_moon_func(vec3 pos, float seed) {
    float n0 = 0.0f;//Noise_gen::perlin_noise(pos, 0.45f, 3, seed);
    //n0 *= 0.65f;

    vec3 forward = vec3(1, 0, 0);
    float d = smoothstep(0.0f, 1.0f, dot(normalize(pos), forward));
    d -= 0.15f;
    n0 -= d * 0.7f;
    n0 += 0.5f;
    n0 *= 1.5f;

    return n0;
};

float Voxel_system::get_elev(Voxel_field& field, vec3 pos, int num_craters, float noise) {
    float ret = 0.0;
    float avg_dimension = field.planet_radius;
    float radius = 0.1f;
    
    vec3 n = normalize(pos);

    uint32_t num_buckets = 24;
    uint32_t seed = 0xB3;

    float crater_depth = 0.0f;

    //
    for(Crater_partition& partition : field.partitions) {
        ivec3 b = floor(n / partition.region_size);
        auto& bucket = partition.partition[b];
        
        for(int i : bucket) {
            if(i < num_craters) {
                crater& c = field.craters[i];
                
                vec3 rel = c.position - n;
                float dist = length(rel);
                dist /= c.radius;

                float variation = 0.1f;

                if(dist < 1.5f + variation) {
                    dist += noise * variation;
                    dist = max(0.001f, dist);

                    if(dist < 1.5f) {
                        float crater_scale = c.radius * 0.075;
                        float height = crater_scale * avg_dimension;
                        float depth = height;
                        //if(height > depth) depth = (height * 0.125f + depth) / 1.125f;

                        //vec2 ret = crater_func(dist, -depth, height - c.height, height * 2.0f, 0.5f);
                        float ret;
                        if(c.radius > 0.05f) ret = big_crater_func(dist, -c.radius * 0.125f, 1.75f, 1.75f, 0.5f, 3.0f, 0.5f);
                        else ret = crater_func(dist, -depth, 1.25f, 1.75f, 0.5f);

                        float new_depth = -ret;
                        
                        float floor = c.height - height;
                        float target_height = floor - crater_depth;
                        if(target_height < 0.0) {
                            float a = target_height * new_depth;
                            crater_depth = a + crater_depth;
                        }
                    }
                }
            }
        }
    }

    //if(crater_depth != 0.0f) std::cout << crater_depth << " ";

    return crater_depth;
}

/*
vec3 c_normal = -normalize(vec3(1.0f, 0.6f, 0.3f));
vec3 c_pos = c_normal;

vec3 n = normalize(pos);
vec3 offset = n - c_pos;

float dist = length(offset);

float crater_scale = radius * 0.075;
float height = crater_scale * avg_dimension;
float depth = height;//(0.0025f) * avg_dimension;

dist /= radius;

if(dist < 1.5f) {
    float d = crater_func(dist, -depth, 1.25f, 1.75f, 0.5f);
    ret += d * 0x4000;
}

float Voxel_system::get_elev(vec3 pos, int num_craters) {
    float ret = 0.0;
    float avg_dimension = 1.4f * 0x100000;
    float radius = 0.1f;
    
    vec3 n = normalize(pos);

    uint32_t num_buckets = 24;
    uint32_t seed = 0xB3;

    //
    ivec3 b = floor(n * float(num_buckets));
    auto& bucket = partition[b];

    float crater_depth = 0.0f;
    
    for(int i : bucket) {
        if(i < num_craters) {
            crater& c = craters[i];
            
            vec3 rel = c.position - n;
            float dist = length(rel);
            dist /= c.radius;

            float variation = 0.0f;

            if(dist < 1.5f + variation) {
                //float noise_v = Noise_gen::perlin_noise(rel / c.radius, 0.2f, 2.0f, seed) * 0.25f;
                //noise_v += Noise_gen::perlin_noise(rel / c.radius, 0.5f, 2.0f, seed);
                //dist += noise_v * variation;
                dist = max(0.001f, dist);

                if(dist < 1.5f) {
                    float crater_scale = c.radius * 0.075;
                    float height = crater_scale * avg_dimension;
                    float depth = height;//(0.0025f) * avg_dimension;
                    //if(height > depth) depth = (height * 0.125f + depth) / 1.125f;

                    //vec2 ret = crater_func(dist, -depth, height - c.height, height * 2.0f, 0.5f);
                    float ret;
                    //if(c.radius > 0.05f) ret = big_crater_func(dist, -c.radius * 0.125f, 1.75f, 1.75f, 0.5f, 3.0f, 0.5f);
                    ret = crater_func(dist, -depth, 1.25f, 1.75f, 0.5f);

                    float new_depth = -ret;
                    
                    float floor = c.height - height;
                    float target_height = floor - crater_depth;
                    if(target_height < 0.0) {
                        float a = target_height * new_depth;
                        crater_depth = a + crater_depth;
                    }
                }
            }
        }
    }

    //if(crater_depth != 0.0f) std::cout << crater_depth << " ";

    return crater_depth;
}
*/

void Voxel_field::create_craters() {
    uint32_t seed = 0xB3;
    //vec3 position = origin + pvec3(normalize(vec3(0.8f, 0.5f, -0.3f)) * 2400000.0f * 10.0f);
    mat3 orientation = identity<mat3>();//rotate_to(vec3(1.0f, 0.0f, 0.0f), normalize(origin - position)); 
    vec3 dimensions = vec3(planet_radius);
    float amplitude = 5000;
    float noise_freq = 0.3f;
    float noise_offset = 0.0f;
    float age_value = 1.0f;
    float ejecta_value = 0.25f;
    float blend_value = 0.125f;

    Random random(seed);

    float avg_dimension = (dimensions.x + dimensions.y + dimensions.z) / 3.0f;
    float max_dimension = max(max(dimensions.x, dimensions.y), dimensions.z);
    vec3 ratio = dimensions / avg_dimension;
    
    int num_prev = 0;
    for(int j = 0; j < populations.size(); ++j) {
        crater_population& pop = populations[j];

        Crater_partition partition;
        partition.region_size = (pop.max_size + pop.min_size) * 0.5f;

        for(int i = 0; i < pop.num_craters; ++i) {
            crater c;
            c.position = random.unit_vector() * ratio;
            float r = abs(random());
            r = Voxel_system::bias_func(r, 0.6f);

            c.radius = r * (pop.max_size - pop.min_size) + pop.min_size;

            if(pop.num_craters - i < pop.num_ejecta) {
                c.ejecta = c.radius * (5.0f + 15.0f * abs(random()));
                //c.ejecta = c.radius * 9.0f;

                c.age = float(pop.num_craters - i) / pop.num_ejecta;
                c.age = pow(c.age, age_value);
                //c.age = pow(c.age, 4.0f);
            }

            craters.push_back(c);



            float max_rad = c.radius * 1.5f;//max(c.radius * 1.5f, c.ejecta);

            vec3 mmin = c.position - max_rad;
            vec3 mmax = c.position + max_rad;
            ivec3 rmin = floor(mmin / partition.region_size);
            ivec3 rmax = floor(mmax / partition.region_size);

            //std::cout << rmin.x << " "  << rmin.y << " "  << rmin.z << " " << rmax.x << " "  << rmax.y << " " << rmax.z << "\n";
            //rmin = clamp(rmin, ivec3(-max_bucket - 1), ivec3(max_bucket));
            //rmax = clamp(rmax, ivec3(-max_bucket - 1), ivec3(max_bucket));

            for(int z = rmin.z; z <= rmax.z; ++z) {
                for(int y = rmin.y; y <= rmax.y; ++y) {
                    for(int x = rmin.x; x <= rmax.x; ++x) {
                        ivec3 bucket = ivec3(x, y, z);

                        if(!partition.partition.contains(bucket)) partition.partition.emplace(bucket, std::vector<uint32_t>());

                        auto& p = partition.partition[bucket];

                        p.push_back(i + num_prev);
                    }
                }
            }
        }

        partitions.push_back(partition);

        num_prev += pop.num_craters;
    }

    crater c;
    c.age = 0.0f;
    c.ejecta = 0.0f;
    c.position = -normalize(vec3(1.0f, 0.6f, 0.3f));
    c.radius = 1.0f;

    //craters = {c};

    for(int i = 0; i < craters.size(); ++i) {
        crater& c = craters[i];
        vec3 pos = c.position * avg_dimension;

        float height = Voxel_system::get_elev(*this, pos, i - 1, 0.0f);
        c.height = height;
    }
}