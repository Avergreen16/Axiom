#include "building.hpp"
#include "physics.hpp"
#include "input.hpp"
#include "core.hpp"
#include "gui.hpp"

vec3 random_color(float seed) {
    vec3 v;

    float f = fract(seed) * 6;
    float ff = fract(f);

    if(f < 1) {
        return vec3(1.0f, ff, 0.0f);
    } else if(f < 2) {
        return vec3(1.0f - ff, 1.0f, 0.0f);
    } else if(f < 3) {
        return vec3(0.0f, 1.0f, ff);
    } else if(f < 4) {
        return vec3(0.0f, 1.0f - ff, 1.0f);
    } else if(f < 5) {
        return vec3(ff, 0.0f, 1.0f);
    } else {
        return vec3(1.0f, 0.0f, 1.0f - ff);
    }
}

void Building_system::mesh_primitive(uint32_t entity, vec3 color, std::vector<vec3> vertices, std::vector<uint32_t> indices) {
    // create mesh
    
    vec3 center = vec3(0.0f);
    for(vec3 v : vertices) center += v;
    center /= vertices.size();

    Mesh_component& mc = ecs.get_component<Mesh_component>(entity);
    std::shared_ptr<Mesh> mesh(new Mesh);

    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;

    for(int triangle = 0; triangle < indices.size() / 3; ++triangle) {
        std::vector<uint32_t> triangle_indices = {indices[triangle * 3], indices[triangle * 3 + 1], indices[triangle * 3 + 2]};

        vec3 v0 = vertices[triangle_indices[0]];
        vec3 v1 = vertices[triangle_indices[1]];
        vec3 v2 = vertices[triangle_indices[2]];

        vec3 n0;
        vec3 n1;
        vec3 n2;

        vec3 normal = normalize(cross(v0 - v2, v1 - v2));

        /*
        if(dot(normal, center - v0) > 0) {
            normal = -normal;
            vec3 temp = v0;
            v0 = v1;
            v1 = temp;

            temp = n0;
            n0 = n1;
            n1 = temp;
        }
        */

        n0 = normal;
        n1 = normal;
        n2 = normal;

        Mesh_vertex mv;
        mv.tex_coords = vec2(0.5f);

        mv.position = v0;
        mv.normal = n0;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3);
        
        mv.position = v1;
        mv.normal = n1;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 1);
        
        mv.position = v2;
        mv.normal = n2;
        mesh_vertices.push_back(mv);
        mesh_indices.push_back(triangle * 3 + 2);
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();
    mc.mesh = mesh;
    mc.cull = false;
    
    Format format = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
    std::vector<uint8_t> pixels = {uint8_t(color.x * 255), uint8_t(color.y * 255), uint8_t(color.z * 255), uint8_t(255)};
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(Texture(pixels.data(), uvec3(1, 1, 1), GL_TEXTURE_2D, format));
    
    mc.texture = texture;
}

Building_system::Building_system() {
    primitives = {
        // cube
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f)}},
    
        // slope
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 1.0f)}},

        // corner tetrahedron
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)}},

        // edge tetrahedron
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 1.0f)}},

        // corner
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)}},

        // corner slope
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 1.0f)}},
    };

    for(auto& prim : primitives) {
        auto tris = triangulate(prim.vertices);
        auto faces = Physics_system::triangulate_merge(prim.vertices);

        vec3 center = vec3(0.0f);
        for(vec3 v : prim.vertices) center += v;
        center /= prim.vertices.size();

        for(uint32_t i = 0; i < tris.size(); i += 3) {
            vec3 normal = cross(prim.vertices[tris[i]] - prim.vertices[tris[i + 2]], prim.vertices[tris[i + 1]] - prim.vertices[tris[i + 2]]);

            if(dot(normal, center - prim.vertices[tris[i]]) > 0) {
                normal = -normal;
                uint32_t temp = tris[i];
                tris[i] = tris[i + 1];
                tris[i + 1] = temp;
            }
        }

        prim.triangles = tris;
        prim.faces = faces;

        mat3 scale_mat = glm::scale(vec3(0.25f, 0.25f, 0.25f));

        float vol;

        // 0.0103894 5.77131e-05 0.000105374 5.77131e-05 0.0103894 5.77131e-05 0.000105374 5.77131e-05 0.0103893
        auto d = Physics_system::calculate_M(prim.vertices, vec3(0.0f), vol);
        mat3 M = scale_mat * d.first * transpose(scale_mat); // transpose does nothing here because scale_mat is diagonal
        prim.M = M;
        prim.center_of_mass = 0.25f * d.second;
        prim.volume = vol * pow(0.25f, 3);
        
        /*
        mat3 m0 = prim.M;
        m0 = inv_translate_M(prim.center_of_mass, m0, 1.0f);
        m0 = Physics_system::inertia_tensor(m0);
        std::cout << " " << d.second << " " << m0[0] << " " << m0[1] << " " << m0[2] << "\n";

        vec3 v = vec3(0.25f, 0.25f, 0.25f) * 0.5f;
        float f = 1.0f / 12.0f * (0.25f * 0.25f + 0.25f * 0.25f);

        std::cout << f << "\n";

        std::cout << "\n";
        */
    }

    std::vector<Primitive> tile_primitives = {
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f)}, {0, 1, 3, 0, 3, 2, 1, 0, 3, 3, 0, 2}, {{{0, 1, 3, 2}}, {{2, 3, 1, 0}}}},
        {{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)}, {0, 1, 2, 1, 0, 2}, {{{0, 1, 2}}, {{1, 0, 2}}}}
    };

    for(auto& prim : tile_primitives) {
        for(auto& face : prim.faces) {
            face.normal = normalize(cross(prim.vertices[face.vertices[0]] - prim.vertices[face.vertices[2]], prim.vertices[face.vertices[1]] - prim.vertices[face.vertices[2]]));
        }

        mat3 scale_mat = glm::scale(vec3(0.25f));

        std::vector<vec3> vvs = prim.vertices;

        float vol;

        auto d = Physics_system::calculate_M_flat(prim.vertices, 0.0625, vol);
        mat3 M = scale_mat * d.first * transpose(scale_mat); // transpose does nothing here because scale_mat is diagonal
        prim.M = M;
        prim.center_of_mass = 0.25f * d.second;
        prim.volume = vol * pow(0.25f, 3);
    }

    primitives.insert(primitives.end(), tile_primitives.begin(), tile_primitives.end());
}

void Building_system::call() {
    Input_system& input_system = ecs.get_system<Input_system>();
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Particle_system& particle_system = ecs.get_system<Particle_system>();
    Physics_system& physics_system = ecs.get_system<Physics_system>();

    Transform& camera_transform = ecs.get_component<Transform>(input_system.player_camera);
    
    auto snap_vector = [&](vec3 v) {
        vec3 n;
        if(abs(v.x) >= abs(v.y) && abs(v.x) >= abs(v.z)) {
            n = vec3(sign(v.x), 0.0f, 0.0f);
        } else if(abs(v.y) > abs(v.x) && abs(v.y) >= abs(v.z)) {
            n = vec3(0.0f, sign(v.y), 0.0f);
        } else {
            n = vec3(0.0f, 0.0f, sign(v.z));
        }

        return n;
    };

    if(!building_active) {
        if(building_entity != NULL_ENTITY) ecs.erase_entity(building_entity);
        building_entity = NULL_ENTITY;
        mode = BUILDING_MODE_PLACE;
        reset_b = true;
    } else {
        ivec3 rel;
        uint32_t object;

        vec3 anchor;
        
        if(active_primitive == 6) {
            static int lock = -1;
            static vec3 axis;

            if(building_entity == NULL_ENTITY || reset_b) {
                lock = -1;

                reset_b = false;
                if(building_entity != NULL_ENTITY) ecs.erase_entity(building_entity);

                active_size = ivec3(1);
                active_offset = ivec3(0);

                Transform transform;
                Mesh_component mesh;

                transform.orientation = identity<mat3>();
                ori_x = vec3(0.0f, 0.0f, 0.0f);
                ori_y = vec3(1.0f, 0.0f, 0.0f);
                ori_z = vec3(0.0f, 1.0f, 0.0f);

                uint32_t entity = ecs.insert_entity();
                ecs.insert_component(entity, transform);
                ecs.insert_component(entity, mesh);

                building_entity = entity; 
                building_reference = NULL_ENTITY;
            }

            Transform& building_entity_transform = ecs.get_component<Transform>(building_entity);
            Mesh_component& building_entity_mesh = ecs.get_component<Mesh_component>(building_entity);
            vec3 c = snap_vector(camera_transform.orientation[2]);

            ivec3 rel;
            uint32_t object;

            vec3 anchor;

            static vec3 prev_ori_x;
            static vec3 prev_ori_y;
            static vec3 prev_ori_z;
            static bool up = false;
            static bool edge_lock = false;
            
            if(core.pressed_buttons.contains(GLFW_KEY_U)) up = !up;

            static float edit_dist = 4.0f;

            static std::vector<uint32_t> locked_vertices;

            //

            if(mode == BUILDING_MODE_PLACE) {

                active_size = ivec3(1);
                
                building_entity_transform.position = camera_transform.position + pvec3(camera_transform.orientation * vec3(0.0f, 0.0f, -4.0f));

                uint32_t shape;
                vec3 normal;
                pvec3 point;
                std::unordered_set<uint32_t> mask;
                if(input_system.player_collider != NULL_ENTITY) mask.insert(input_system.player_collider);

                physics_system.raycast(camera_transform.position, -camera_transform.orientation[2], 1.0f, 5.0f, mask, &object, &shape, &normal, &point);

                //

                if(object != NULL_ENTITY && ecs.has_component<Build>(object)) {
                    Collider& collider = ecs.get_component<Collider>(object);
                    Transform& tf = ecs.get_component<Transform>(object);
                    Build& build = ecs.get_component<Build>(object);
                    
                    vec3 snap_normal = snap_vector(transpose(tf.orientation) * normal);

                    Convex_collider& cc = collider.collision_shapes[shape];
                    vec3 ppp = vec3(point - tf.position);
                    vec3 p = transpose(tf.orientation) * ppp + build.center_of_mass;

                    vec3 n = transpose(tf.orientation) * normal;
                    
                    /*
                    vec3 snap_normal = snap_vector(transpose(tf.orientation) * camera_transform.orientation[1]);
                    ivec3 snap_normal1 = snap_vector(transpose(tf.orientation) * -camera_transform.orientation[2]);
                    ivec3 snap_normal3 = snap_vector(transpose(tf.orientation) * camera_transform.orientation[0]);
                    ivec3 snap_normal2 = snap_vector(transpose(tf.orientation) * normal);
                    if(abs(snap_normal2) == ivec3(abs(snap_normal)) || ) {
                        p -= vec3(snap_normal1) * 0.125f;
                    } else if() {
                        p -= vec3(snap_normal3) * 0.125f;
                    }
                    */
                   
                    vec3 snap_normal2 = snap_vector(transpose(tf.orientation) * -camera_transform.orientation[2]);
                    mat3 rot = rotate_to(transpose(tf.orientation) * -camera_transform.orientation[2], snap_normal2);
                    vec3 snap_normal0 = snap_vector(rot * transpose(tf.orientation) * camera_transform.orientation[0]);
                    vec3 snap_normal1 = cross(snap_normal0, snap_normal2);

                    if(!up) {
                        edge_lock = false;
                        
                        vec3 m = p * 4.0f;
                        rel = floor(m + n * 0.01f);
                        vec3 v = vec3(rel) / 4.0f;

                        vec3 offset = vec3(0.125f);

                        building_entity_transform.orientation = tf.orientation;
                        building_entity_transform.position = pvec3(tf.orientation * (v - build.center_of_mass + offset)) + tf.position;

                        vec3 u = (ivec3(-snap_normal) + 1) / 2;
                        
                        vec3 a;
                        vec3 b;
                        if(snap_normal.x != 0) {
                            a = vec3(0, 1, 0);
                            b = vec3(0, 0, 1);
                        } else if(snap_normal.y != 0) {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 0, 1);
                        } else {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 1, 0);
                        }
                        
                        std::vector<vec3> snap = {
                            vec3(rel) + u,
                            vec3(rel) + u + a,
                            vec3(rel) + u + b,
                            vec3(rel) + u + a + b,
                        };

                        std::vector<float> us = {
                            0, 
                            0, 
                            0,
                            0
                        };

                        uint32_t i = 0;
                        for(vec3& v : snap) {
                            vec3 r = m - v;
                            float aa = dot(n, r);
                            float bb = dot(n, snap_normal);
                            float cc = aa / bb;

                            //v = v + snap_normal * cc;
                            us[i] = cc;
                            ++i;
                        }

                        float min_u = *std::min_element(us.begin(), us.end());
                        float max_u = *std::max_element(us.begin(), us.end());
                        
                        float da = dot(transpose(tf.orientation) * -camera_transform.orientation[1], a);
                        float db = dot(transpose(tf.orientation) * -camera_transform.orientation[1], b);

                        vec3 dir;

                        if(abs(da) > abs(db)) {
                            dir = b;
                        } else {
                            dir = a;
                        }

                        if(abs(min_u - max_u) < 0.025f) ori_x = u;

                        else ori_x = u + snap_normal * ceil(min_u + 0.025f);
                        ori_y = a;
                        ori_z = b;

                        prev_ori_x = ivec3(ori_x);
                        prev_ori_y = ori_y;
                        prev_ori_z = ori_z;

                        if(ori_z == snap_normal) {
                            locked_vertices = {0, 1};
                            //std::cout << "a";
                        } else if(ori_z == -snap_normal) {
                            locked_vertices = {2, 3};
                            //std::cout << "b";
                        } else if(ori_y == snap_normal) {
                            locked_vertices = {0, 2};
                            //std::cout << "c";
                        } else if(ori_y == -snap_normal) {
                            locked_vertices = {1, 3};
                            //std::cout << "d";
                        } else {
                            locked_vertices = {};
                        }
                    } else {
                        edge_lock = true;

                        vec3 m = p * 4.0f;
                        rel = floor(m + n * 0.01f);
                        vec3 v = vec3(rel) / 4.0f;

                        vec3 u = (ivec3(-snap_normal) + 1) / 2;

                        vec3 offset = vec3(0.125f);

                        building_entity_transform.orientation = tf.orientation;
                        building_entity_transform.position = pvec3(tf.orientation * (v - build.center_of_mass + offset)) + tf.position;
                        
                        vec3 a;
                        vec3 b;
                        if(snap_normal.x != 0) {
                            a = vec3(0, 1, 0);
                            b = vec3(0, 0, 1);
                        } else if(snap_normal.y != 0) {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 0, 1);
                        } else {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 1, 0);
                        }
                        
                        std::vector<vec3> snap = {
                            vec3(rel) + u + a * 0.5f,
                            vec3(rel) + u + b * 0.5f,
                            vec3(rel) + u + b + a * 0.5f,
                            vec3(rel) + u + a + b * 0.5f,
                        };

                        std::vector<float> us = {
                            0, 
                            0, 
                            0,
                            0
                        };

                        uint32_t i = 0;
                        for(vec3& v : snap) {
                            vec3 r = m - v;
                            float aa = dot(n, r);
                            float bb = dot(n, snap_normal);
                            float cc = aa / bb;

                            v = v + snap_normal * cc;
                            us[i] = cc;
                            if(abs(us[i]) < 0.03f) us[i] = 0.0f;
                            ++i;
                        }

                        std::vector<float> ls = {
                            length(snap[0] - m),
                            length(snap[1] - m),
                            length(snap[2] - m),
                            length(snap[3] - m),
                        };

                        float mx = *std::min_element(ls.begin(), ls.end());

                        vec3 origin = vec3(0);

                        float epsilon = 0.01f;

                        if(mx == ls[0]) {
                            u += snap_normal * floor(us[0] + 0.03f * sign(us[0]));

                            a = a;
                        } else if(mx == ls[1]) {
                            u += snap_normal * floor(us[1] + 0.03f * sign(us[1]));

                            a = b;
                        } else if(mx == ls[2]) {
                            u += snap_normal * floor(us[2] + 0.03f * sign(us[2]));

                            origin += b;
                            a = a;
                        } else if(mx == ls[3]) {
                            u += snap_normal * floor(us[3] + 0.03f * sign(us[3]));
                            
                            origin += a;
                            a = b;
                        }

                        ori_x = origin + u;
                        ori_y = a;
                        ori_z = ivec3(snap_normal);

                        prev_ori_x = ivec3(ori_x);
                        prev_ori_y = ori_y;
                        prev_ori_z = ori_z;

                        locked_vertices = {0, 1};
                    }
                } else {
                    if(core.key_map[GLFW_MOUSE_BUTTON_RIGHT]) {
                        vec3 up = camera_transform.orientation[1];
                        vec3 left = camera_transform.orientation[0];
                        vec3 forward = -camera_transform.orientation[2];

                        mat3 rot_x = rotate(float(M_PI * 0.75f * core.delta_time), up);
                        mat3 rot_y = rotate(float(M_PI * 0.75f * core.delta_time), left);
                        mat3 rot_z = rotate(float(M_PI * 0.75f * core.delta_time), forward);

                        if(core.key_map[GLFW_KEY_A]) {
                            building_entity_transform.orientation = transpose(rot_x) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_D]) {
                            building_entity_transform.orientation = rot_x * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_W]) {
                            building_entity_transform.orientation = transpose(rot_y) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_S]) {
                            building_entity_transform.orientation = rot_y * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_Q]) {
                            building_entity_transform.orientation = transpose(rot_z) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_E]) {
                            building_entity_transform.orientation = rot_z * building_entity_transform.orientation;
                        }
                    }

                    prev_ori_x = ivec3(ori_x) + active_offset;
                    prev_ori_y = ori_y;
                    prev_ori_z = ori_z;

                    building_entity_transform.position += pvec3(building_entity_transform.orientation * vec3(0.0f, 0.0f, 0.125f));
                }
                /*
                auto corner_vector = [&](vec3 v) {
                    std::vector<vec3> choices = {
                        vec3(-1, -1, -1),
                        vec3(1, -1, -1),
                        vec3(-1, 1, -1),
                        vec3(1, 1, -1),
                        vec3(-1, -1, 1),
                        vec3(1, -1, 1),
                        vec3(-1, 1, 1),
                        vec3(1, 1, 1),
                    };

                    vec3 vv;
                    float f = -FLT_MAX;
                    for(vec3 vec : choices) {
                        float d = dot(vec, v);
                        if(d > f) {
                            f = d;
                            vv = vec;
                        }
                    }

                    return (vv + 1.0f) / 2.0f;
                };
                */
            } else if(mode == BUILDING_MODE_SIZE_BASE) {
                mat3 base_ori;
                vec3 offset = vec3(0.0f);
                if(building_reference != NULL_ENTITY) {
                    Transform& ref_tf = ecs.get_component<Transform>(building_reference);
                    Build& ref_build = ecs.get_component<Build>(building_reference);
                    
                    building_entity_transform.orientation = ref_tf.orientation;
                    building_entity_transform.position = pvec3(ref_tf.orientation * (vec3(reference_pos) * 0.25f - ref_build.center_of_mass + vec3(0.125f))) + ref_tf.position;
                    base_ori = ref_tf.orientation;
                    offset = ref_build.center_of_mass;
                } else {
                    base_ori = building_entity_transform.orientation;
                }

                vec3 snap_normal2 = snap_vector(transpose(base_ori) * -camera_transform.orientation[2]);
                mat3 rot = rotate_to(transpose(base_ori) * -camera_transform.orientation[2], snap_normal2);
                vec3 snap_normal0 = snap_vector(rot * transpose(base_ori) * camera_transform.orientation[0]);
                vec3 snap_normal1 = cross(snap_normal0, snap_normal2);
                
                uint32_t shape;
                vec3 normal;
                pvec3 point;
                std::unordered_set<uint32_t> mask;
                if(input_system.player_collider != NULL_ENTITY) mask.insert(input_system.player_collider);

                float p = edit_dist;
                if(physics_system.raycast(camera_transform.position, -camera_transform.orientation[2], edit_dist * 0.1f, edit_dist, mask, &object, &shape, &normal, &point, 0.0625f)) {
                    p = length(vec3(point - camera_transform.position));
                };
                
                pvec3 pos = camera_transform.position + pvec3(camera_transform.orientation * vec3(0.0f, 0.0f, -p));

                vec3 rel = transpose(building_entity_transform.orientation) * vec3(pos - building_entity_transform.position) * 4.0f - prev_ori_x + 0.5f;

                vec3 pp = round(rel);

                vec3 pa = vec3(0.0f);
                vec3 pb = vec3(0.0f) + prev_ori_y;
                vec3 pc = vec3(0.0f) + prev_ori_z;
                vec3 pd = vec3(0.0f) + prev_ori_y + prev_ori_z;

                float la = length(pp - pa);
                float lb = length(pp - pb);
                float lc = length(pp - pc);
                float ld = length(pp - pd);

                uint32_t i = 0;

                std::vector<float> ls;
                ls.reserve(4);
                if(std::find(locked_vertices.begin(), locked_vertices.end(), 0) == locked_vertices.end()) ls.push_back(la);
                if(std::find(locked_vertices.begin(), locked_vertices.end(), 1) == locked_vertices.end()) ls.push_back(lb);
                if(std::find(locked_vertices.begin(), locked_vertices.end(), 2) == locked_vertices.end()) ls.push_back(lc);
                if(std::find(locked_vertices.begin(), locked_vertices.end(), 3) == locked_vertices.end()) ls.push_back(ld);
                
                float lmin = FLT_MAX;
                for(float l : ls) lmin = min(lmin, l);

                vec3 na;
                vec3 nb;
                vec3 nc;
                vec3 nd;

                vec3 ori1 = transpose(building_entity_transform.orientation) * camera_transform.orientation[0];

                if(abs(dot(prev_ori_z, ori1)) > abs(dot(prev_ori_y, ori1)) && !edge_lock) {
                    vec3 ori = prev_ori_z;

                    if(lmin == la && std::find(locked_vertices.begin(), locked_vertices.end(), 0) == locked_vertices.end()) {
                        na = pp;
                        nd = pd;
                        nc = nd + ori * dot(ori, na - nd);
                        nb = na + (nd - nc);

                        //std::cout << "a";
                    } else if(lmin == lb && std::find(locked_vertices.begin(), locked_vertices.end(), 1) == locked_vertices.end()) {
                        nb = pp;
                        nc = pc;
                        na = nb + ori * dot(ori, nc - nb);
                        nd = nc + (nb - na);

                        //std::cout << "b";
                    } else if(lmin == lc && std::find(locked_vertices.begin(), locked_vertices.end(), 2) == locked_vertices.end()) {
                        nc = pp;
                        nb = pb;

                        nd = nb + ori * dot(ori, nc - nb);
                        na = nb + (nc - nd);

                        //std::cout << "c";
                    } else if(lmin == ld && std::find(locked_vertices.begin(), locked_vertices.end(), 3) == locked_vertices.end()) {
                        na = pa;
                        nd = pp;
                        nc = nd - ori * dot(ori, nd - na);
                        nb = na + (nd - nc);

                        //std::cout << "d";
                    }
                    
                    // derive ori_y, ori_z

                    ori_x = vec3(0.0f);
                    ori_y = nb - na;
                    ori_z = nc - na;
                    active_offset = prev_ori_x + na;

                    float ly = length(ori_y);
                    float lz = length(ori_z);

                    if(ly == 0 && lz == 0) {
                        ori_y = prev_ori_y;
                        ori_z = prev_ori_z;
                        active_offset = prev_ori_x;
                    } else {
                        if(ly == 0) {
                            ori_y = ori_z;
                            ori_z = prev_ori_z;
                            active_offset = prev_ori_x + prev_ori_y * dot(na, prev_ori_y);
                        }
                        if(lz == 0) {
                            ori_z = ori_y;
                            ori_y = prev_ori_y;
                            active_offset = prev_ori_x + prev_ori_z * dot(na, prev_ori_z);
                        }
                    }
                } else {
                    vec3 ori = prev_ori_y;
                    
                    if(lmin == la && std::find(locked_vertices.begin(), locked_vertices.end(), 0) == locked_vertices.end()) {
                        na = pp;
                        nd = pd;
                        nb = nd + ori * dot(ori, na - nd);
                        nc = na + (nd - nb);

                        //std::cout << "e";
                    } else if(lmin == lb && std::find(locked_vertices.begin(), locked_vertices.end(), 1) == locked_vertices.end()) {
                        nb = pp;
                        nc = pc;
                        nd = nb + ori * dot(ori, nc - nb);
                        na = nc + (nb - nd);

                        //std::cout << "f";
                    } else if(lmin == lc && std::find(locked_vertices.begin(), locked_vertices.end(), 2) == locked_vertices.end()) {
                        nc = pp;
                        nb = pb;

                        na = nc + ori * dot(ori, nb - nc);
                        nd = nb + (nc - na);

                        //std::cout << "g";
                    } else if(lmin == ld && std::find(locked_vertices.begin(), locked_vertices.end(), 3) == locked_vertices.end()) {
                        na = pa;
                        nd = pp;
                        nb = nd + ori * dot(ori, na - nd);
                        nc = na + (nd - nb);

                        //std::cout << "h";
                    }
                    
                    // derive ori_y, ori_z

                    ori_x = vec3(0.0f);
                    ori_y = nb - na;
                    ori_z = nc - na;
                    active_offset = prev_ori_x + na;

                    float ly = length(ori_y);
                    float lz = length(ori_z);

                    if(ly == 0 && lz == 0) {
                        ori_y = prev_ori_y;
                        ori_z = prev_ori_z;
                        active_offset = prev_ori_x;
                    } else {
                        if(ly == 0) {
                            ori_y = ori_z;
                            ori_z = prev_ori_z;
                            active_offset = prev_ori_x + prev_ori_y * dot(na, prev_ori_y);
                        }
                        if(lz == 0) {
                            ori_z = ori_y;
                            ori_y = prev_ori_y;
                            active_offset = prev_ori_x + prev_ori_z * dot(na, prev_ori_z);
                        }
                    }
                }


                // focus side
                // 0 -> 
            }

            float power = 0.0f;
            if(core.key_map[GLFW_KEY_R]) power += 1.0f;
            if(core.key_map[GLFW_KEY_F]) power -= 1.0f;
            edit_dist *= pow(2.0f, power * core.delta_time);

            edit_dist = clamp(edit_dist, 0.5f, 6.0f);
            
            // mesh primitive
            std::vector<vec3> vertices = {
                ori_x,
                ori_x + ori_y,
                ori_x + ori_z,
                ori_x + ori_y + ori_z
            };
            for(vec3& v : vertices) v = (v - vec3(0.5f, 0.5f, 0.5f)) * 0.25f * vec3(active_size) + vec3(active_offset) * 0.25f;// + c * 0.0625f;
            mesh_primitive(building_entity, current_color, vertices, primitives[active_primitive].triangles);

            // insert
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {// && gui_system.capture == 0xFFFFFFFF) {
                if(mode == BUILDING_MODE_PLACE) {
                    // place
                    mode = BUILDING_MODE_SIZE_BASE;
                    axis_lock = true;

                    if(object != NULL_ENTITY) {
                        reference_pos = rel;
                        building_reference = object;
                    }

                    edit_dist = 4.0f;
                } else if(mode == BUILDING_MODE_SIZE_BASE) {
                    if(building_reference != NULL_ENTITY) {
                        Build& build = ecs.get_component<Build>(building_reference);

                        Brick brick;
                        brick.color = current_color;

                        mat3 ori = mat3(ori_x, ori_y, cross(ori_x, ori_y));
                        //ori = transpose(ori);

                        ivec3 start = reference_pos + active_offset + active_size;
                        ivec3 end = start + active_size;



                        ivec3 size = ivec3(ori * vec3(active_size));
                        ivec3 rel_pos = 

                        brick.orientation_x = ori_x;
                        brick.orientation_y = ori_y;
                        brick.orientation_z = ori_z;

                        brick.position = reference_pos + active_offset + active_size;
                        brick.size = active_size;
                        brick.primitive_id = active_primitive;

                        build.bricks.push_back(brick);
                        
                        create_build_collider(building_reference);
                        mesh_build(building_reference);
                        
                        //

                        ecs.erase_entity(building_entity);
                        building_entity = NULL_ENTITY;
                        building_active = false;
                    } else {
                        Transform& tf = ecs.get_component<Transform>(building_entity);

                        Collider collider;
                        Collision_shape shape;
                        Build build;

                        Brick brick;

                        brick.color = current_color;

                        brick.orientation_x = ori_x;
                        brick.orientation_y = ori_y;
                        brick.orientation_z = ori_z;

                        brick.position = ivec3(0);
                        brick.size = active_size;
                        brick.primitive_id = active_primitive;

                        build.bricks.push_back(brick);

                        mat3 orientation = {brick.orientation_x, brick.orientation_y, cross(vec3(brick.orientation_x), vec3(brick.orientation_y))};

                        //
                        vec3 ao = vec3(active_offset + (active_size - 1)) * 0.25f;

                        Transform transform;
                        transform.orientation = tf.orientation;
                        transform.position = tf.position + pvec3(tf.orientation * (vec3(0.125f) + ao));

                        uint32_t entity = ecs.insert_entity();
                        ecs.insert_component(entity, transform);
                        ecs.insert_component(entity, collider);
                        ecs.insert_component(entity, build);
                        
                        create_build_collider(entity);
                        mesh_build(entity);
                        
                        //

                        ecs.erase_entity(building_entity);
                        building_entity = NULL_ENTITY;
                        building_active = false;
                    }

                    current_color = random_color(core.random()) * 0.65f + 0.35f;
                }
            }
        } else if(active_primitive == 7) {
            // triangle

            static int lock = -1;
            static vec3 axis;
            static uint32_t rot_v = 0;

            if(building_entity == NULL_ENTITY || reset_b) {
                lock = -1;

                reset_b = false;
                if(building_entity != NULL_ENTITY) ecs.erase_entity(building_entity);

                active_size = ivec3(1);
                active_offset = ivec3(0);

                Transform transform;
                Mesh_component mesh;

                transform.orientation = identity<mat3>();
                ori_x = vec3(0.0f, 0.0f, 0.0f);
                ori_y = vec3(1.0f, 0.0f, 0.0f);
                ori_z = vec3(0.0f, 1.0f, 0.0f);

                uint32_t entity = ecs.insert_entity();
                ecs.insert_component(entity, transform);
                ecs.insert_component(entity, mesh);

                building_entity = entity; 
                building_reference = NULL_ENTITY;

                rot_v = 0;
            }

            Transform& building_entity_transform = ecs.get_component<Transform>(building_entity);
            Mesh_component& building_entity_mesh = ecs.get_component<Mesh_component>(building_entity);
            vec3 c = snap_vector(camera_transform.orientation[2]);

            ivec3 rel;
            uint32_t object;

            vec3 anchor;

            static vec3 prev_ori_x;
            static vec3 prev_ori_y;
            static vec3 prev_ori_z;
            static bool up = false;
            static bool edge_lock = false;
            
            if(core.pressed_buttons.contains(GLFW_KEY_U)) up = !up;

            static float edit_dist = 4.0f;

            static std::vector<uint32_t> locked_vertices;
            
            static std::vector<int> floating_vertices;

            //

            if(mode == BUILDING_MODE_PLACE) {

                active_size = ivec3(1);
                
                building_entity_transform.position = camera_transform.position + pvec3(camera_transform.orientation * vec3(0.0f, 0.0f, -4.0f));

                uint32_t shape;
                vec3 normal;
                pvec3 point;
                std::unordered_set<uint32_t> mask;
                if(input_system.player_collider != NULL_ENTITY) mask.insert(input_system.player_collider);

                physics_system.raycast(camera_transform.position, -camera_transform.orientation[2], 1.0f, 5.0f, mask, &object, &shape, &normal, &point);

                //

                if(object != NULL_ENTITY && ecs.has_component<Build>(object)) {
                    Collider& collider = ecs.get_component<Collider>(object);
                    Transform& tf = ecs.get_component<Transform>(object);
                    Build& build = ecs.get_component<Build>(object);
                    
                    vec3 snap_normal = snap_vector(transpose(tf.orientation) * normal);

                    Convex_collider& cc = collider.collision_shapes[shape];
                    vec3 ppp = vec3(point - tf.position);
                    vec3 p = transpose(tf.orientation) * ppp + build.center_of_mass;

                    vec3 n = transpose(tf.orientation) * normal;
                    
                    /*
                    vec3 snap_normal = snap_vector(transpose(tf.orientation) * camera_transform.orientation[1]);
                    ivec3 snap_normal1 = snap_vector(transpose(tf.orientation) * -camera_transform.orientation[2]);
                    ivec3 snap_normal3 = snap_vector(transpose(tf.orientation) * camera_transform.orientation[0]);
                    ivec3 snap_normal2 = snap_vector(transpose(tf.orientation) * normal);
                    if(abs(snap_normal2) == ivec3(abs(snap_normal)) || ) {
                        p -= vec3(snap_normal1) * 0.125f;
                    } else if() {
                        p -= vec3(snap_normal3) * 0.125f;
                    }
                    */
                   
                    vec3 snap_normal2 = snap_vector(transpose(tf.orientation) * -camera_transform.orientation[2]);
                    mat3 rot = rotate_to(transpose(tf.orientation) * -camera_transform.orientation[2], snap_normal2);
                    vec3 snap_normal0 = snap_vector(rot * transpose(tf.orientation) * camera_transform.orientation[0]);
                    vec3 snap_normal1 = cross(snap_normal0, snap_normal2);

                    if(!up) {
                        edge_lock = false;
                        
                        vec3 m = p * 4.0f;
                        rel = floor(m + n * 0.01f);
                        vec3 v = vec3(rel) / 4.0f;

                        vec3 offset = vec3(0.125f);

                        building_entity_transform.orientation = tf.orientation;
                        building_entity_transform.position = pvec3(tf.orientation * (v - build.center_of_mass + offset)) + tf.position;

                        vec3 u = (ivec3(-snap_normal) + 1) / 2;
                        
                        vec3 a;
                        vec3 b;
                        if(snap_normal.x != 0) {
                            a = vec3(0, 1, 0);
                            b = vec3(0, 0, 1);
                        } else if(snap_normal.y != 0) {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 0, 1);
                        } else {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 1, 0);
                        }
                        
                        std::vector<vec3> snap = {
                            vec3(rel) + u,
                            vec3(rel) + u + a,
                            vec3(rel) + u + b,
                            vec3(rel) + u + a + b,
                        };

                        std::vector<float> us = {
                            0, 
                            0, 
                            0,
                            0
                        };

                        uint32_t i = 0;
                        for(vec3& v : snap) {
                            vec3 r = m - v;
                            float aa = dot(n, r);
                            float bb = dot(n, snap_normal);
                            float cc = aa / bb;

                            //v = v + snap_normal * cc;
                            us[i] = cc;
                            ++i;
                        }

                        float min_u = *std::min_element(us.begin(), us.end());
                        float max_u = *std::max_element(us.begin(), us.end());
                        
                        float da = dot(transpose(tf.orientation) * -camera_transform.orientation[1], a);
                        float db = dot(transpose(tf.orientation) * -camera_transform.orientation[1], b);

                        vec3 dir;

                        if(abs(da) > abs(db)) {
                            dir = b;
                        } else {
                            dir = a;
                        }

                        if(abs(min_u - max_u) < 0.025f) ori_x = u;

                        else ori_x = u + snap_normal * ceil(min_u + 0.025f);
                        ori_y = a;
                        ori_z = b;
                        
                        if(rot_v % 4 == 1) {
                            ori_y = b;
                            ori_z = -a;
                            ori_x += a;
                        } else if(rot_v % 4 == 2) {
                            ori_y = -a;
                            ori_z = -b;
                            ori_x += a + b;
                        } else if(rot_v % 4 == 3) {
                            ori_y = -b;
                            ori_z = a;
                            ori_x += b;
                        }

                        prev_ori_x = ivec3(ori_x);
                        prev_ori_y = ori_y;
                        prev_ori_z = ori_z;

                        if(ori_z == snap_normal) {
                            locked_vertices = {0, 1};
                            //std::cout << "a";
                        } else if(ori_z == -snap_normal) {
                            locked_vertices = {2, 3};
                            //std::cout << "b";
                        } else if(ori_y == snap_normal) {
                            locked_vertices = {0, 2};
                            //std::cout << "c";
                        } else if(ori_y == -snap_normal) {
                            locked_vertices = {1, 3};
                            //std::cout << "d";
                        } else {
                            locked_vertices = {};
                        }
                    } else {
                        edge_lock = true;

                        vec3 m = p * 4.0f;
                        rel = floor(m + n * 0.01f);
                        vec3 v = vec3(rel) / 4.0f;

                        vec3 u = (ivec3(-snap_normal) + 1) / 2;

                        vec3 offset = vec3(0.125f);

                        building_entity_transform.orientation = tf.orientation;
                        building_entity_transform.position = pvec3(tf.orientation * (v - build.center_of_mass + offset)) + tf.position;
                        
                        vec3 a;
                        vec3 b;
                        if(snap_normal.x != 0) {
                            a = vec3(0, 1, 0);
                            b = vec3(0, 0, 1);
                        } else if(snap_normal.y != 0) {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 0, 1);
                        } else {
                            a = vec3(1, 0, 0);
                            b = vec3(0, 1, 0);
                        }
                        
                        std::vector<vec3> snap = {
                            vec3(rel) + u + a * 0.5f,
                            vec3(rel) + u + b * 0.5f,
                            vec3(rel) + u + b + a * 0.5f,
                            vec3(rel) + u + a + b * 0.5f,
                        };

                        std::vector<float> us = {
                            0, 
                            0, 
                            0,
                            0
                        };

                        uint32_t i = 0;
                        for(vec3& v : snap) {
                            vec3 r = m - v;
                            float aa = dot(n, r);
                            float bb = dot(n, snap_normal);
                            float cc = aa / bb;

                            v = v + snap_normal * cc;
                            us[i] = cc;
                            if(abs(us[i]) < 0.03f) us[i] = 0.0f;
                            ++i;
                        }

                        std::vector<float> ls = {
                            length(snap[0] - m),
                            length(snap[1] - m),
                            length(snap[2] - m),
                            length(snap[3] - m),
                        };

                        float mx = *std::min_element(ls.begin(), ls.end());

                        vec3 origin = vec3(0);

                        float epsilon = 0.01f;

                        if(mx == ls[0]) {
                            u += snap_normal * floor(us[0] + 0.03f * sign(us[0]));

                            a = a;
                        } else if(mx == ls[1]) {
                            u += snap_normal * floor(us[1] + 0.03f * sign(us[1]));

                            a = b;
                        } else if(mx == ls[2]) {
                            u += snap_normal * floor(us[2] + 0.03f * sign(us[2]));

                            origin += b;
                            a = a;
                        } else if(mx == ls[3]) {
                            u += snap_normal * floor(us[3] + 0.03f * sign(us[3]));
                            
                            origin += a;
                            a = b;
                        }

                        ori_x = origin + u;
                        ori_y = a;
                        ori_z = ivec3(snap_normal);
                        
                        if(rot_v % 2 == 1) {
                            ori_x += ori_y;
                            ori_y = -ori_y;
                        }

                        prev_ori_x = ivec3(ori_x);
                        prev_ori_y = ori_y;
                        prev_ori_z = ori_z;

                        locked_vertices = {0, 1};
                    }

                    if(core.key_map[GLFW_MOUSE_BUTTON_RIGHT]) {
                        if(core.pressed_buttons.contains(GLFW_KEY_E)) ++rot_v;
                        else if(core.pressed_buttons.contains(GLFW_KEY_Q)) --rot_v;
                    }
                } else {
                    if(core.key_map[GLFW_MOUSE_BUTTON_RIGHT]) {
                        vec3 up = camera_transform.orientation[1];
                        vec3 left = camera_transform.orientation[0];
                        vec3 forward = -camera_transform.orientation[2];

                        mat3 rot_x = rotate(float(M_PI * 0.75f * core.delta_time), up);
                        mat3 rot_y = rotate(float(M_PI * 0.75f * core.delta_time), left);
                        mat3 rot_z = rotate(float(M_PI * 0.75f * core.delta_time), forward);

                        if(core.key_map[GLFW_KEY_A]) {
                            building_entity_transform.orientation = transpose(rot_x) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_D]) {
                            building_entity_transform.orientation = rot_x * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_W]) {
                            building_entity_transform.orientation = transpose(rot_y) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_S]) {
                            building_entity_transform.orientation = rot_y * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_Q]) {
                            building_entity_transform.orientation = transpose(rot_z) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_E]) {
                            building_entity_transform.orientation = rot_z * building_entity_transform.orientation;
                        }
                    }

                    prev_ori_x = ivec3(ori_x) + active_offset;
                    prev_ori_y = ori_y;
                    prev_ori_z = ori_z;

                    building_entity_transform.position += pvec3(building_entity_transform.orientation * vec3(0.0f, 0.0f, 0.125f));
                }
                /*
                auto corner_vector = [&](vec3 v) {
                    std::vector<vec3> choices = {
                        vec3(-1, -1, -1),
                        vec3(1, -1, -1),
                        vec3(-1, 1, -1),
                        vec3(1, 1, -1),
                        vec3(-1, -1, 1),
                        vec3(1, -1, 1),
                        vec3(-1, 1, 1),
                        vec3(1, 1, 1),
                    };

                    vec3 vv;
                    float f = -FLT_MAX;
                    for(vec3 vec : choices) {
                        float d = dot(vec, v);
                        if(d > f) {
                            f = d;
                            vv = vec;
                        }
                    }

                    return (vv + 1.0f) / 2.0f;
                };
                */
            } else if(mode == BUILDING_MODE_SIZE_BASE) {
                mat3 base_ori;
                vec3 offset = vec3(0.0f);
                if(building_reference != NULL_ENTITY) {
                    Transform& ref_tf = ecs.get_component<Transform>(building_reference);
                    Build& ref_build = ecs.get_component<Build>(building_reference);
                    
                    building_entity_transform.orientation = ref_tf.orientation;
                    building_entity_transform.position = pvec3(ref_tf.orientation * (vec3(reference_pos) * 0.25f - ref_build.center_of_mass + vec3(0.125f))) + ref_tf.position;
                    base_ori = ref_tf.orientation;
                    offset = ref_build.center_of_mass;
                } else {
                    base_ori = building_entity_transform.orientation;
                }

                vec3 snap_normal2 = snap_vector(transpose(base_ori) * -camera_transform.orientation[2]);
                mat3 rot = rotate_to(transpose(base_ori) * -camera_transform.orientation[2], snap_normal2);
                vec3 snap_normal0 = snap_vector(rot * transpose(base_ori) * camera_transform.orientation[0]);
                vec3 snap_normal1 = cross(snap_normal0, snap_normal2);
                
                uint32_t shape;
                vec3 normal;
                pvec3 point;
                std::unordered_set<uint32_t> mask;
                if(input_system.player_collider != NULL_ENTITY) mask.insert(input_system.player_collider);

                float p = edit_dist;
                if(physics_system.raycast(camera_transform.position, -camera_transform.orientation[2], edit_dist * 0.1f, edit_dist, mask, &object, &shape, &normal, &point, 0.0625f)) {
                    p = length(vec3(point - camera_transform.position));
                };
                
                pvec3 pos = camera_transform.position + pvec3(camera_transform.orientation * vec3(0.0f, 0.0f, -p));

                vec3 rel = transpose(building_entity_transform.orientation) * vec3(pos - building_entity_transform.position) * 4.0f - prev_ori_x + 0.5f;

                vec3 pp = round(rel);

                //

                vec3 pa = vec3(0.0f);
                vec3 pb = vec3(0.0f) + prev_ori_y;
                vec3 pc = vec3(0.0f) + prev_ori_z;
                vec3 pd = vec3(0.0f) + prev_ori_y + prev_ori_z; // placeholder

                float la = length(pp - pa);
                float lb = length(pp - pb);
                float lc = length(pp - pc);
                float ld = length(pp - pd);

                uint32_t i = 0;

                std::vector<float> ls;
                ls.reserve(4);
                if(std::find(locked_vertices.begin(), locked_vertices.end(), 0) == locked_vertices.end()) ls.push_back(la);
                if(std::find(locked_vertices.begin(), locked_vertices.end(), 1) == locked_vertices.end()) ls.push_back(lb);
                if(std::find(locked_vertices.begin(), locked_vertices.end(), 2) == locked_vertices.end()) ls.push_back(lc);
                ///if(std::find(locked_vertices.begin(), locked_vertices.end(), 3) == locked_vertices.end()) ls.push_back(ld);
                
                float lmin = FLT_MAX;
                for(float l : ls) lmin = min(lmin, l);

                vec3 na;
                vec3 nb;
                vec3 nc;

                vec3 ori1 = transpose(building_entity_transform.orientation) * camera_transform.orientation[0];
                
                if(lmin == la && std::find(locked_vertices.begin(), locked_vertices.end(), 0) == locked_vertices.end()) {
                    na = pp;
                    nb = pb + prev_ori_z * dot(prev_ori_z, na - pa);
                    nc = pc + prev_ori_y * dot(prev_ori_y, na - pa);

                    if(dot(prev_ori_y, nb - na) <= 0.0f) nb -= prev_ori_y * 1.0f;
                    if(dot(prev_ori_z, nc - na) <= 0.0f) nc -= prev_ori_z * 1.0f;

                    floating_vertices = {1, 2};
                } else if(lmin == lb && std::find(locked_vertices.begin(), locked_vertices.end(), 1) == locked_vertices.end()) {
                    nb = pp;
                    nc = pc; 
                    na = nc + prev_ori_z * dot(prev_ori_z, nb - nc);

                    if(dot(na - nc, pa - pc) <= 0.0f) nc -= prev_ori_z;

                    floating_vertices = {2};
                } else if(lmin == lc && std::find(locked_vertices.begin(), locked_vertices.end(), 2) == locked_vertices.end()) {
                    nc = pp;
                    nb = pb;
                    na = nb + prev_ori_y * dot(prev_ori_y, nc - nb);

                    if(dot(na - nb, pa - pb) <= 0.0f) nb -= prev_ori_y;

                    floating_vertices = {1};
                }

                // derive ori_y, ori_z

                ori_x = vec3(0.0f);
                ori_y = nb - na;
                ori_z = nc - na;
                active_offset = prev_ori_x + na;

                float ly = length(ori_y);
                float lz = length(ori_z);

                std::cout << ori_y << " | " << ori_z << "\n";

                if(ly == 0 && lz == 0) {
                    ori_y = prev_ori_y;
                    ori_z = prev_ori_z;
                    active_offset = prev_ori_x;
                } else {
                    if(ly == 0) {
                        ori_y = prev_ori_y;
                    }
                    if(lz == 0) {
                        ori_z = prev_ori_z;
                    }
                }
            } else if(mode == BUILDING_MODE_SIZE_HEIGHT) {
                mat3 base_ori;
                vec3 offset = vec3(0.0f);
                if(building_reference != NULL_ENTITY) {
                    Transform& ref_tf = ecs.get_component<Transform>(building_reference);
                    Build& ref_build = ecs.get_component<Build>(building_reference);
                    
                    building_entity_transform.orientation = ref_tf.orientation;
                    building_entity_transform.position = pvec3(ref_tf.orientation * (vec3(reference_pos) * 0.25f - ref_build.center_of_mass + vec3(0.125f))) + ref_tf.position;
                    base_ori = ref_tf.orientation;
                    offset = ref_build.center_of_mass;
                } else {
                    base_ori = building_entity_transform.orientation;
                }

                vec3 snap_normal2 = snap_vector(transpose(base_ori) * -camera_transform.orientation[2]);
                mat3 rot = rotate_to(transpose(base_ori) * -camera_transform.orientation[2], snap_normal2);
                vec3 snap_normal0 = snap_vector(rot * transpose(base_ori) * camera_transform.orientation[0]);
                vec3 snap_normal1 = cross(snap_normal0, snap_normal2);
                
                uint32_t shape;
                vec3 normal;
                pvec3 point;
                std::unordered_set<uint32_t> mask;
                if(input_system.player_collider != NULL_ENTITY) mask.insert(input_system.player_collider);

                float p = edit_dist;
                if(physics_system.raycast(camera_transform.position, -camera_transform.orientation[2], edit_dist * 0.1f, edit_dist, mask, &object, &shape, &normal, &point, 0.0625f)) {
                    p = length(vec3(point - camera_transform.position));
                };
                
                pvec3 pos = camera_transform.position + pvec3(camera_transform.orientation * vec3(0.0f, 0.0f, -p));

                vec3 rel = transpose(building_entity_transform.orientation) * vec3(pos - building_entity_transform.position) * 4.0f - vec3(active_offset) + 0.5f;

                vec3 pp = round(rel);

                //

                std::vector<vec3> vertices = {
                    prev_ori_x,
                    prev_ori_x + prev_ori_y,
                    prev_ori_x + prev_ori_z
                };

                std::vector<float> ls = {
                    FLT_MAX,
                    FLT_MAX,
                    FLT_MAX
                };

                for(int i : floating_vertices) {
                    vec3 v = vertices[i];

                    ls[i] = length(v - rel);
                };

                ori_x = prev_ori_x;
                ori_y = prev_ori_y;
                ori_z = prev_ori_z;

                float min_l = *std::min_element(ls.begin(), ls.end());

                if(min_l == ls[0]) { // vertex 0
                    // never true
                } else if(min_l == ls[1]) { // vertex 1
                    vec3 v;

                    vec3 a = vertices[0];
                    vec3 b = vertices[2];

                    if(a.x == b.x) {
                        v.x = pp.x;
                    } else {
                        if(abs(a.x - rel.x) < abs(b.x - rel.x)) v.x = a.x;
                        else v.x = b.x;
                    }

                    if(a.y == b.y) {
                        v.y = pp.y;
                    } else {
                        if(abs(a.y - rel.y) < abs(b.y - rel.y)) v.y = a.y;
                        else v.y = b.y;
                    }

                    if(a.z == b.z) {
                        v.z = pp.z;
                    } else {
                        if(abs(a.z - rel.z) < abs(b.z - rel.z)) v.z = a.z;
                        else v.z = b.z;
                    }

                    ori_y = v - vertices[0];
                } else if(min_l == ls[2]) { // vertex 2
                    vec3 v;

                    vec3 a = vertices[0];
                    vec3 b = vertices[1];

                    if(a.x == b.x) {
                        v.x = pp.x;
                    } else {
                        if(abs(a.x - rel.x) < abs(b.x - rel.x)) v.x = a.x;
                        else v.x = b.x;
                    }

                    if(a.y == b.y) {
                        v.y = pp.y;
                    } else {
                        if(abs(a.y - rel.y) < abs(b.y - rel.y)) v.y = a.y;
                        else v.y = b.y;
                    }

                    if(a.z == b.z) {
                        v.z = pp.z;
                    } else {
                        if(abs(a.z - rel.z) < abs(b.z - rel.z)) v.z = a.z;
                        else v.z = b.z;
                    }

                    ori_z = v - vertices[0];
                }

                if(ori_z == vec3(0.0f) || abs(dot(normalize(ori_z), normalize(ori_y)) > 0.998)) ori_z = prev_ori_z;
            }

            float power = 0.0f;
            if(core.key_map[GLFW_KEY_R]) power += 1.0f;
            if(core.key_map[GLFW_KEY_F]) power -= 1.0f;
            edit_dist *= pow(2.0f, power * core.delta_time);

            edit_dist = clamp(edit_dist, 0.5f, 6.0f);
            
            // mesh primitive
            std::vector<vec3> vertices = {
                ori_x,
                ori_x + ori_y,
                ori_x + ori_z,
                ori_x + ori_y + ori_z
            };
            for(vec3& v : vertices) v = (v - vec3(0.5f, 0.5f, 0.5f)) * 0.25f * vec3(active_size) + vec3(active_offset) * 0.25f;// + c * 0.0625f;
            mesh_primitive(building_entity, current_color, vertices, primitives[active_primitive].triangles);

            // insert
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {// && gui_system.capture == 0xFFFFFFFF) {
                if(mode == BUILDING_MODE_PLACE) {
                    // place
                    mode = BUILDING_MODE_SIZE_BASE;
                    axis_lock = true;

                    if(object != NULL_ENTITY) {
                        reference_pos = rel;
                        building_reference = object;
                    }

                    edit_dist = 4.0f;
                } else if(mode == BUILDING_MODE_SIZE_BASE) {
                    mode = BUILDING_MODE_SIZE_HEIGHT;
                    rel_origin = anchor;

                    prev_ori_x = ori_x;
                    prev_ori_y = ori_y;
                    prev_ori_z = ori_z;
                } else if(mode == BUILDING_MODE_SIZE_HEIGHT) {
                    if(building_reference != NULL_ENTITY) {
                        Build& build = ecs.get_component<Build>(building_reference);

                        Brick brick;
                        brick.color = current_color;

                        mat3 ori = mat3(ori_x, ori_y, cross(ori_x, ori_y));
                        //ori = transpose(ori);

                        ivec3 start = reference_pos + active_offset + active_size;
                        ivec3 end = start + active_size;



                        ivec3 size = ivec3(ori * vec3(active_size));

                        brick.orientation_x = ori_x;
                        brick.orientation_y = ori_y;
                        brick.orientation_z = ori_z;

                        brick.position = reference_pos + active_offset + active_size;
                        brick.size = active_size;
                        brick.primitive_id = active_primitive;

                        build.bricks.push_back(brick);
                        
                        create_build_collider(building_reference);
                        mesh_build(building_reference);
                        
                        //

                        ecs.erase_entity(building_entity);
                        building_entity = NULL_ENTITY;
                        building_active = false;
                    } else {
                        Transform& tf = ecs.get_component<Transform>(building_entity);

                        Collider collider;
                        Collision_shape shape;
                        Build build;

                        Brick brick;

                        brick.color = current_color;

                        brick.orientation_x = ori_x;
                        brick.orientation_y = ori_y;
                        brick.orientation_z = ori_z;

                        brick.position = ivec3(0);
                        brick.size = active_size;
                        brick.primitive_id = active_primitive;

                        build.bricks.push_back(brick);

                        mat3 orientation = {brick.orientation_x, brick.orientation_y, cross(vec3(brick.orientation_x), vec3(brick.orientation_y))};

                        //
                        vec3 ao = vec3(active_offset + (active_size - 1)) * 0.25f;

                        Transform transform;
                        transform.orientation = tf.orientation;
                        transform.position = tf.position + pvec3(tf.orientation * (vec3(0.125f) + ao));

                        uint32_t entity = ecs.insert_entity();
                        ecs.insert_component(entity, transform);
                        ecs.insert_component(entity, collider);
                        ecs.insert_component(entity, build);
                        
                        create_build_collider(entity);
                        mesh_build(entity);
                        
                        //

                        ecs.erase_entity(building_entity);
                        building_entity = NULL_ENTITY;
                        building_active = false;
                    }

                    current_color = random_color(core.random()) * 0.65f + 0.35f;
                }
            }
        } else {
            if(building_entity == NULL_ENTITY || reset_b) {
                reset_b = false;
                if(building_entity != NULL_ENTITY) ecs.erase_entity(building_entity);

                active_size = ivec3(1);
                active_offset = ivec3(0);

                Transform transform;
                Mesh_component mesh;

                transform.orientation = identity<mat3>();
                ori_x = vec3(1.0f, 0.0f, 0.0f);
                ori_y = vec3(0.0f, 1.0f, 0.0f);

                uint32_t entity = ecs.insert_entity();
                ecs.insert_component(entity, transform);
                ecs.insert_component(entity, mesh);

                building_entity = entity;
                building_reference = NULL_ENTITY;
            }

            Transform& building_entity_transform = ecs.get_component<Transform>(building_entity);
            Mesh_component& building_entity_mesh = ecs.get_component<Mesh_component>(building_entity);

            mat3 orientation = mat3(ori_x, ori_y, cross(ori_x, ori_y));

            if(mode == BUILDING_MODE_PLACE) {
                active_size = ivec3(1);
                
                building_entity_transform.position = camera_transform.position + pvec3(camera_transform.orientation * vec3(0.0f, 0.0f, -4.0f));

                uint32_t shape;
                vec3 normal;
                pvec3 point;
                std::unordered_set<uint32_t> mask;
                if(input_system.player_collider != NULL_ENTITY) mask.insert(input_system.player_collider);

                physics_system.raycast(camera_transform.position, -camera_transform.orientation[2], 1.0f, 5.0f, mask, &object, &shape, &normal, &point);

                if(object != NULL_ENTITY && ecs.has_component<Build>(object)) {
                    Collider& collider = ecs.get_component<Collider>(object);
                    Transform& tf = ecs.get_component<Transform>(object);
                    Build& build = ecs.get_component<Build>(object);

                    Convex_collider& cc = collider.collision_shapes[shape];
                    vec3 origin = cc.position;
                    vec3 p = transpose(tf.orientation) * (vec3(point - tf.position) + normal * 0.125f) + build.center_of_mass;

                    rel = floor(p * 4.0f);
                    vec3 v = vec3(rel) / 4.0f;

                    vec3 offset = vec3(0.125f);

                    building_entity_transform.orientation = tf.orientation;
                    building_entity_transform.position = pvec3(tf.orientation * (v - build.center_of_mass + offset)) + tf.position;
                    
                    if(core.key_map[GLFW_MOUSE_BUTTON_RIGHT]) {
                        mat3 base_ori = tf.orientation * camera_transform.orientation;

                        vec3 n = snap_vector(base_ori[0]);
                        base_ori = rotate_to(base_ori[0], n) * base_ori;

                        n = snap_vector(base_ori[1]);
                        float theta = atan2(dot(base_ori[0], cross(base_ori[1], n)), dot(base_ori[1], n));

                        base_ori = mat3(rotate(theta, base_ori[0])) * base_ori;

                        //

                        vec3 up = base_ori[1];
                        vec3 left = base_ori[0];
                        vec3 forward = -base_ori[2];

                        mat3 rot_x = rotate(float(M_PI * 0.5f), up);
                        mat3 rot_y = rotate(float(M_PI * 0.5f), left);
                        mat3 rot_z = rotate(float(M_PI * 0.5f), forward);

                        if(core.pressed_buttons.contains(GLFW_KEY_A)) {
                            ori_x = snap_vector(transpose(rot_x) * vec3(ori_x));
                            ori_y = snap_vector(transpose(rot_x) * vec3(ori_y));
                        }
                        if(core.pressed_buttons.contains(GLFW_KEY_D)) {
                            ori_x = snap_vector(rot_x * vec3(ori_x));
                            ori_y = snap_vector(rot_x * vec3(ori_y));
                        }
                        if(core.pressed_buttons.contains(GLFW_KEY_W)) {
                            ori_x = snap_vector(transpose(rot_y) * vec3(ori_x));
                            ori_y = snap_vector(transpose(rot_y) * vec3(ori_y));
                        }
                        if(core.pressed_buttons.contains(GLFW_KEY_S)) {
                            ori_x = snap_vector(rot_y * vec3(ori_x));
                            ori_y = snap_vector(rot_y * vec3(ori_y));
                        }
                        if(core.pressed_buttons.contains(GLFW_KEY_Q)) {
                            ori_x = snap_vector(transpose(rot_z) * vec3(ori_x));
                            ori_y = snap_vector(transpose(rot_z) * vec3(ori_y));
                        }
                        if(core.pressed_buttons.contains(GLFW_KEY_E)) {
                            ori_x = snap_vector(rot_z * vec3(ori_x));
                            ori_y = snap_vector(rot_z * vec3(ori_y));
                        }
                    }
                } else {
                    if(core.key_map[GLFW_MOUSE_BUTTON_RIGHT]) {
                        vec3 up = camera_transform.orientation[1];
                        vec3 left = camera_transform.orientation[0];
                        vec3 forward = -camera_transform.orientation[2];

                        mat3 rot_x = rotate(float(M_PI * 0.75f * core.delta_time), up);
                        mat3 rot_y = rotate(float(M_PI * 0.75f * core.delta_time), left);
                        mat3 rot_z = rotate(float(M_PI * 0.75f * core.delta_time), forward);

                        if(core.key_map[GLFW_KEY_A]) {
                            building_entity_transform.orientation = transpose(rot_x) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_D]) {
                            building_entity_transform.orientation = rot_x * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_W]) {
                            building_entity_transform.orientation = transpose(rot_y) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_S]) {
                            building_entity_transform.orientation = rot_y * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_Q]) {
                            building_entity_transform.orientation = transpose(rot_z) * building_entity_transform.orientation;
                        }
                        if(core.key_map[GLFW_KEY_E]) {
                            building_entity_transform.orientation = rot_z * building_entity_transform.orientation;
                        }
                    }
                }
            } else if(mode == BUILDING_MODE_SIZE_BASE) {
                mat3 base_ori;
                if(building_reference != NULL_ENTITY) {
                    Transform& ref_tf = ecs.get_component<Transform>(building_reference);
                    Build& ref_build = ecs.get_component<Build>(building_reference);
                    
                    building_entity_transform.orientation = ref_tf.orientation;
                    building_entity_transform.position = pvec3(ref_tf.orientation * (vec3(reference_pos) * 0.25f - ref_build.center_of_mass + vec3(0.125f))) + ref_tf.position;
                    base_ori = ref_tf.orientation;
                } else {
                    base_ori = building_entity_transform.orientation;
                }

                vec3 dir = transpose(base_ori) * -camera_transform.orientation[2];
                vec3 rel_pos = transpose(base_ori) * (camera_transform.position - building_entity_transform.position);

                if(abs(dir.x) >= abs(dir.y) && abs(dir.x) >= abs(dir.z)) {
                    axis = vec3(1.0f, 0.0f, 0.0f);
                } else if(abs(dir.y) > abs(dir.x) && abs(dir.y) >= abs(dir.z)) {
                    axis = vec3(0.0f, 1.0f, 0.0f);
                } else if(abs(dir.z) > abs(dir.x) && abs(dir.z) > abs(dir.y)) {
                    axis = vec3(0.0f, 0.0f, 1.0f);
                }

                float dist = dot(-rel_pos, axis) / dot(dir, axis);
                vec3 pos = rel_pos + dir * dist;

                pos *= 4.0f;
                
                ivec3 rel = round(pos);
                if(active_primitive == 6 || active_primitive == 7) {
                    active_size = ivec3(abs(rel.x) + 1, abs(rel.y) + 1, rel.z);
                    active_offset = min(ivec3(0), ivec3(rel.xy(), 0));
                } else {
                    active_size = abs(rel) + 1;
                    active_offset = min(ivec3(0), rel);
                }
            } else if(mode == BUILDING_MODE_SIZE_HEIGHT) {
                mat3 base_ori;
                if(building_reference != NULL_ENTITY) {
                    Transform& ref_tf = ecs.get_component<Transform>(building_reference);
                    Build& ref_build = ecs.get_component<Build>(building_reference);
                    
                    building_entity_transform.orientation = ref_tf.orientation;
                    building_entity_transform.position = pvec3(ref_tf.orientation * (vec3(reference_pos) * 0.25f - ref_build.center_of_mass + vec3(0.125f))) + ref_tf.position;
                    base_ori = ref_tf.orientation;
                } else {
                    base_ori = building_entity_transform.orientation;
                }

                vec3 dir = transpose(base_ori) * -camera_transform.orientation[2];
                vec3 rel_pos = transpose(base_ori) * (camera_transform.position - building_entity_transform.position);

                vec3 new_axis;
                if(axis.x) {
                    if(abs(dir.y) >= abs(dir.z)) {
                        new_axis = vec3(0.0f, 1.0f, 0.0f);
                    } else {
                        new_axis = vec3(0.0f, 0.0f, 1.0f);
                    }
                } else if(axis.y) {
                    if(abs(dir.x) >= abs(dir.z)) {
                        new_axis = vec3(1.0f, 0.0f, 0.0f);
                    } else {
                        new_axis = vec3(0.0f, 0.0f, 1.0f);
                    }
                } else if(axis.z) {
                    if(abs(dir.x) >= abs(dir.y)) {
                        new_axis = vec3(1.0f, 0.0f, 0.0f);
                    } else {
                        new_axis = vec3(0.0f, 1.0f, 0.0f);
                    }
                }

                float dist = dot(-rel_pos, new_axis) / dot(dir, new_axis);
                vec3 pos = rel_pos + dir * dist;
                pos *= 4.0f;

                ivec3 rel = round(pos);
                //std::cout << round(pos) << "\n";

                if(active_primitive == 6 || active_primitive == 7) {
                    if(axis.x) {
                        active_size.x = abs(rel.x) + 1;
                        active_offset.x = min(0, rel.x);
                    } else if(axis.y) {
                        active_size.y = abs(rel.y) + 1;
                        active_offset.y = min(0, rel.y);
                    } else if(axis.z) {
                        active_size.z = rel.z;
                        active_offset.z = 0;
                    }
                } else {
                    if(axis.x) {
                        active_size.x = abs(rel.x) + 1;
                        active_offset.x = min(0, rel.x);
                    } else if(axis.y) {
                        active_size.y = abs(rel.y) + 1;
                        active_offset.y = min(0, rel.y);
                    } else if(axis.z) {
                        active_size.z = abs(rel.z) + 1;
                        active_offset.z = min(0, rel.z);
                    }
                }
            }
            
            std::vector<vec3> vertices;
            for(ivec3 v : primitives[active_primitive].vertices) {
                vec3 vv = ((orientation * (vec3(v) - 0.5f) + 0.5f) * vec3(active_size) + vec3(active_offset) - 0.5f) * 0.25f;
                vertices.push_back(vv);
            }
            mesh_primitive(building_entity, current_color, vertices, primitives[active_primitive].triangles);

            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {// && gui_system.capture == 0xFFFFFFFF) {
                if(mode == BUILDING_MODE_PLACE) {
                    // place
                    mode = BUILDING_MODE_SIZE_BASE;
                    axis_lock = true;

                    if(object != NULL_ENTITY) {
                        reference_pos = rel;
                        building_reference = object;
                    }
                } else if(mode == BUILDING_MODE_SIZE_BASE) {
                    mode = BUILDING_MODE_SIZE_HEIGHT;
                    rel_origin = anchor;
                } else if(mode == BUILDING_MODE_SIZE_HEIGHT) {
                    if(building_reference != NULL_ENTITY) {
                        Build& build = ecs.get_component<Build>(building_reference);

                        Brick brick;
                        brick.color = current_color;

                        mat3 ori = mat3(ori_x, ori_y, cross(ori_x, ori_y));
                        //ori = transpose(ori);

                        ivec3 start = reference_pos + active_offset + active_size;
                        ivec3 end = start + active_size;



                        ivec3 size = ivec3(ori * vec3(active_size));
                        ivec3 rel_pos = 

                        brick.orientation_x = ori_x;
                        brick.orientation_y = ori_y;

                        brick.position = reference_pos + active_offset + active_size;
                        brick.size = active_size;
                        brick.primitive_id = active_primitive;

                        build.bricks.push_back(brick);
                        
                        create_build_collider(building_reference);
                        mesh_build(building_reference);
                        
                        //

                        ecs.erase_entity(building_entity);
                        building_entity = NULL_ENTITY;
                        building_active = false;
                    } else {
                        Transform& tf = ecs.get_component<Transform>(building_entity);

                        Collider collider;
                        Collision_shape shape;
                        Build build;

                        Brick brick;
                        brick.color = current_color;

                        brick.orientation_x = vec3(1, 0, 0);
                        brick.orientation_y = vec3(0, 1, 0);

                        brick.position = ivec3(0);
                        brick.size = active_size;
                        brick.primitive_id = active_primitive;

                        build.bricks.push_back(brick);

                        mat3 orientation = {brick.orientation_x, brick.orientation_y, cross(vec3(brick.orientation_x), vec3(brick.orientation_y))};

                        //
                        vec3 ao = vec3(active_offset + (active_size - 1)) * 0.25f;

                        Transform transform;
                        transform.orientation = tf.orientation;
                        transform.position = tf.position + pvec3(tf.orientation * (vec3(0.125f) + ao));

                        uint32_t entity = ecs.insert_entity();
                        ecs.insert_component(entity, transform);
                        ecs.insert_component(entity, collider);
                        ecs.insert_component(entity, build);
                        
                        create_build_collider(entity);
                        mesh_build(entity);
                        
                        //

                        ecs.erase_entity(building_entity);
                        building_entity = NULL_ENTITY;
                        building_active = false;
                    }

                    current_color = random_color(core.random()) * 0.65f + 0.35f;
                }
            }
        }

        if(building_entity != NULL_ENTITY) {
            if(core.pressed_buttons.contains(GLFW_KEY_LEFT_CONTROL)) {
                ecs.erase_entity(building_entity);

                building_entity = NULL_ENTITY;
                building_active = false;
            }
        }
    }

    if(core.pressed_buttons.contains(GLFW_KEY_0) && false) {
        Transform pos = camera_transform;
        Collider collider;
        Build build;

        Brick brick;
        //brick.size = ivec3(36, 15, 12);
        //brick.primitive_id = 0;
        brick.size = ivec3(15, 15, 0);
        brick.primitive_id = 6;
        brick.position = ivec3(0, 0, 0);
        brick.orientation_x = ivec3(1, 0, 0);
        brick.orientation_y = ivec3(0, 1, 0);

        build.bricks.push_back(brick);
        
        uint32_t entity = ecs.insert_entity();
        ecs.insert_component(entity, pos);
        ecs.insert_component(entity, collider);
        ecs.insert_component(entity, build);

        create_build_collider(entity);
        mesh_build(entity);

        /*
        create_mesh_from_collider(entity, vec3(0.25f, 0.25f, 1.0f));

        for(int i = 0; i < primitives.size(); ++i) {
            Primitive& prim = primitives[i];

            Collider collider;
            Collision_shape shape;
            shape.vertices = prim.vertices;
            for(vec3& v : shape.vertices) v *= 0.25f;

            collider.collision_shapes.resize(1);
            collider.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(shape);

            collider.allow_gravity = true;
            collider.allow_rotation = true;

            vec3 offset = Physics_system::initialize_collider(collider, {0.5f});

            Transform pos_2 = pos;
            pos_2.position += pos_2.orientation * offset;

            pos.position += pos.orientation * vec3(0.0f, 0.0f, -0.5f);
        }
        */
    }
}

void Building_system::mesh_build(uint32_t entity) {
    if(!ecs.has_component<Mesh_component>(entity)) {
        Mesh_component mc;
        ecs.insert_component(entity, mc);
    }

    Build& build = ecs.get_component<Build>(entity);
    Mesh_component& mesh_component = ecs.get_component<Mesh_component>(entity);

    std::shared_ptr<Mesh> m(new Mesh);

    std::vector<Mesh_vertex> vertices;
    std::vector<uint32_t> indices;

    for(Brick b : build.bricks) {
        vec3 origin;
        mat3 orientation;
        orientation = {b.orientation_x, b.orientation_y, cross(vec3(b.orientation_x), vec3(b.orientation_y))};
        origin = b.position + min(ivec3(0.0f), -ivec3(vec3(b.size)));

        Primitive& primitive = primitives[b.primitive_id];

        if(b.primitive_id == 6) {
            std::vector<vec3> vs = {
                vec3(0.0f),
                b.orientation_y,
                b.orientation_z,
                b.orientation_y + b.orientation_z,
            };

            for(int i = 0; i < primitive.triangles.size(); ++i) {
                vec3 vertex = vs[primitive.triangles[i]];

                vertex = origin + vertex;

                Mesh_vertex mv;
                mv.position = vertex * 0.25f - build.center_of_mass;
                mv.tex_coords = vec2(56, 8) / 256.0f;
                mv.bone_weights = vec4(b.color, 1.0f);

                indices.push_back(vertices.size());
                vertices.push_back(mv);
            }
        } else if(b.primitive_id == 7) {
            std::vector<vec3> vs = {
                vec3(0.0f),
                b.orientation_y,
                b.orientation_z
            };

            for(int i = 0; i < primitive.triangles.size(); ++i) {
                vec3 vertex = vs[primitive.triangles[i]];

                vertex = origin + vertex;

                Mesh_vertex mv;
                mv.position = vertex * 0.25f - build.center_of_mass;
                mv.tex_coords = vec2(56, 8) / 256.0f;
                mv.bone_weights = vec4(b.color, 1.0f);

                indices.push_back(vertices.size());
                vertices.push_back(mv);
            }
        } else {
            for(int i = 0; i < primitive.triangles.size(); ++i) {
                vec3 vertex = primitive.vertices[primitive.triangles[i]];

                vertex = origin + (orientation * (vertex - 0.5f) + 0.5f) * vec3(b.size);

                Mesh_vertex mv;
                mv.position = vertex * 0.25f - build.center_of_mass;
                mv.tex_coords = vec2(56, 8) / 256.0f;
                mv.bone_weights = vec4(b.color, 1.0f);

                indices.push_back(vertices.size());
                vertices.push_back(mv);
            }
        }
    }
    
    for(int i = 0; i < vertices.size(); i += 3) {
        Mesh_vertex& mva = vertices[i];
        Mesh_vertex& mvb = vertices[i + 1];
        Mesh_vertex& mvc = vertices[i + 2];

        vec3 normal = normalize(cross(mva.position - mvc.position, mvb.position - mvc.position));
        mva.normal = normal;
        mvb.normal = normal;
        mvc.normal = normal;
    }

    m->add_vertices(vertices, indices);
    m->load_buffer();
    
    mesh_component.mesh = m;
    mesh_component.texture = core.textures["tilesheet"];
}

void Building_system::create_build_collider(uint32_t entity) {
    bool move_constraints = true;

    if(!ecs.has_component<Collider>(entity)) {
        Collider collider;
        ecs.insert_component(entity, collider);

        move_constraints = false;
    }
    
    Transform& transform = ecs.get_component<Transform>(entity);
    Build& build = ecs.get_component<Build>(entity);
    Collider& collider = ecs.get_component<Collider>(entity);
    collider = Collider();

    collider.allow_gravity = true;
    collider.allow_rotation = true;


    mat3 total_it = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };
    float mass = 0;
    vec3 center_pos = {0, 0, 0};

    for(Brick& b : build.bricks) {
        vec3 origin;
        mat3 orientation;
        orientation = {b.orientation_x, b.orientation_y, cross(vec3(b.orientation_x), vec3(b.orientation_y))};
        origin = vec3(b.position + min(ivec3(0.0f), -b.size)) * 0.25f;

        Primitive& primitive = primitives[b.primitive_id];

        std::shared_ptr<Collision_shape> shape(new Collision_shape);

        if(b.primitive_id >= 6) {

            if(b.primitive_id == 6) {
                std::vector<vec3> vs = {
                    vec3(0.0f),
                    b.orientation_y,
                    b.orientation_z,
                    b.orientation_y + b.orientation_z,
                };

                for(vec3& v : vs) v *= 0.25f;

                //

                float len_y = length(vec3(b.orientation_y));
                float len_z = length(vec3(b.orientation_z));

                float area = len_y * len_z;
                shape->mass = primitive.volume * area * 0.5f; // 0.5 is density

                //

                shape->vertices = vs;
                shape->faces = primitive.faces;

                mat3 inv_size = transpose(glm::scale(1.0f / vec3(b.orientation_y + b.orientation_z) * 0.25f));
                for(auto& v : shape->faces) {
                    v.normal = normalize(cross(shape->vertices[v.vertices[0]] - shape->vertices[v.vertices[2]], shape->vertices[v.vertices[1]] - shape->vertices[v.vertices[2]]));
                }

                vec3 n_y = vec3(b.orientation_y) / len_y;
                vec3 n_z = vec3(b.orientation_z) / len_z;
                mat3 mat = mat3(b.orientation_y, b.orientation_z, normalize(cross(n_y, n_z)));

                mat3 M = mat * primitive.M * transpose(mat);

                vec3 center_of_mass = mat * primitive.center_of_mass;
                M *= shape->mass;

                shape->inertia_tensor = M;
                shape->center_of_mass = center_of_mass;

                Convex_collider convex;
                convex.orientation = identity<mat3>();
                convex.position = origin;
                convex.collision_shape = shape;

                collider.collision_shapes.push_back(convex);
            } else if(b.primitive_id == 7) {
                std::vector<vec3> vs = {
                    vec3(0.0f),
                    b.orientation_y,
                    b.orientation_z
                };

                for(vec3& v : vs) v *= 0.25f;

                //

                float len_y = length(vec3(b.orientation_y));
                float len_z = length(vec3(b.orientation_z));

                float area = len_y * len_z;
                shape->mass = primitive.volume * area * 0.5f; // 0.5 is density

                //

                shape->vertices = vs;
                shape->faces = primitive.faces;

                mat3 inv_size = transpose(glm::scale(1.0f / vec3(b.orientation_y + b.orientation_z) * 0.25f));
                for(auto& v : shape->faces) {
                    v.normal = normalize(cross(shape->vertices[v.vertices[0]] - shape->vertices[v.vertices[2]], shape->vertices[v.vertices[1]] - shape->vertices[v.vertices[2]]));
                }

                vec3 n_y = vec3(b.orientation_y) / len_y;
                vec3 n_z = vec3(b.orientation_z) / len_z;
                mat3 mat = mat3(b.orientation_y, b.orientation_z, normalize(cross(n_y, n_z)));

                mat3 M = mat * primitive.M * transpose(mat);

                vec3 center_of_mass = mat * primitive.center_of_mass;
                M *= shape->mass;

                shape->inertia_tensor = M;
                shape->center_of_mass = center_of_mass;

                Convex_collider convex;
                convex.orientation = identity<mat3>();
                convex.position = origin;
                convex.collision_shape = shape;

                collider.collision_shapes.push_back(convex);
            }
        } else {
            shape->vertices = primitive.vertices;
            for(vec3& v : shape->vertices) v = (orientation * (v - 0.5f) + 0.5f) * 0.25f * vec3(b.size);
            
            shape->mass = primitive.volume * b.size.x * b.size.y * b.size.z * 0.5f; // 0.5 is density
            shape->faces = primitive.faces;

            mat3 inv_size = transpose(glm::scale(1.0f / vec3(b.size)));
            for(auto& v : shape->faces) v.normal = normalize(inv_size * orientation * v.normal);

            mat3 size_mat = glm::scale(vec3(b.size));

            mat3 A = orientation * size_mat;

            mat3 tensor = A * primitive.M * transpose(A);

            vec3 center_of_mass = A * primitive.center_of_mass;
            tensor *= shape->mass;

            shape->inertia_tensor = tensor;
            shape->center_of_mass = center_of_mass;

            Convex_collider convex;
            convex.orientation = identity<mat3>();
            convex.position = origin;
            convex.collision_shape = shape;

            collider.collision_shapes.push_back(convex);
        }
    }

    for(Convex_collider& cc : collider.collision_shapes) {
        auto& shape = *cc.collision_shape.get();

        center_pos += (shape.center_of_mass + vec3(cc.position)) * shape.mass;
        mass += shape.mass;
        
        mat3 inertial_tensor = shape.inertia_tensor;
        inertial_tensor = translate_M(vec3(cc.position) + shape.center_of_mass, inv_translate_M(shape.center_of_mass, shape.inertia_tensor, shape.mass), shape.mass);//Physics_system::translate_inertial_tensor(vec3(cc.position) + shape.center_of_mass, inertial_tensor, shape.mass);

        total_it += inertial_tensor;
    }

    center_pos /= mass;

    total_it = inv_translate_M(center_pos, total_it, mass);//Physics_system::translate_inertial_tensor_inverse(center_pos, total_it, mass);
    if(collider.allow_rotation) {
        for(Convex_collider& cc : collider.collision_shapes) {
            cc.position -= center_pos;
        }

        //float max_inertia = max(inertial_tensor[0][0], max(inertial_tensor[1][1], inertial_tensor[2][2]));
        //max_inertia = max_inertia * 0.25f;
        //inertial_tensor[0][0] = max(inertial_tensor[0][0], max_inertia);
        //inertial_tensor[1][1] = max(inertial_tensor[1][1], max_inertia);
        //inertial_tensor[2][2] = max(inertial_tensor[2][2], max_inertia);
        
        collider.inertia_tensor = Physics_system::inertia_tensor(total_it);
        
        collider.inverse_inertia_tensor = inverse(collider.inertia_tensor);
    }

    collider.mass = mass;

    vec3 diff = center_pos - build.center_of_mass;

    std::cout << center_pos << " " << build.center_of_mass << "\n";

    build.center_of_mass = center_pos;

    transform.position += transform.orientation * diff;

    collider.create_BVH();

    if(move_constraints) {
        Physics_system& ps = ecs.get_system<Physics_system>();
        for(uint32_t colliding_with : collider.colliding_with) {
            std::array<uint32_t, 2> key = {std::min(entity, colliding_with), std::max(entity, colliding_with)};

            auto& contacts = ps.collision_table.at(key);

            for(Manifold& m : contacts) {
                if(entity == m.a) {
                    for(auto& contact : m.points) {
                        contact.contact_point.a -= pvec3(diff);
                    }
                } else {
                    for(auto& contact : m.points) {
                        contact.contact_point.b -= pvec3(diff);
                    }
                }
            }
        }
    }


}