#include "input.hpp"
#include "gui.hpp"
#include "core.hpp"
#include "render.hpp"
#include "billboard.hpp"
#include "voxel.hpp"
#include "building.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

std::shared_ptr<Texture> create_image_texture(std::string path, ivec4 range, vec3 color) {
    ivec3 size;
    
    uint8_t* data = stbi_load(path.data(), &size.x, &size.y, &size.z, 0);

    std::vector<uint8_t> bytes;
    bytes.resize(size.x * size.y * size.z);
    memcpy(bytes.data(), data, size.x * size.y * size.z);

    if(range.z == 0x7FFFFFFF) {
        range = vec4(0, 0, size.x, size.y);
    }

    auto retrieve = [](std::vector<uint8_t>& data, ivec3 size, ivec4 range) {
        std::vector<uint8_t> return_data;
        return_data.resize(range.z * range.w * size.z);

        for(int y = 0; y < range.w; ++y) {
            for(int x = 0; x < range.z; ++x) {
                uvec2 src_index = uvec2(range.x + x, range.y + y);
                uint32_t src_pos = src_index.y * size.x + src_index.x;
                uint32_t dst_pos = y * range.z + x;

                for(int i = 0; i < 4; ++i) {
                    return_data[dst_pos * 4 + i] = data[src_pos * 4 + i];
                }
            }
        }

        return return_data;
    };

    ivec2 output_size = range.zw();
    std::vector<uint8_t> output = retrieve(bytes, size, range);

    for(int i = 0; i < output.size() / 4; ++i) {
        vec3 output_color = {output[i * 4], output[i * 4 + 1], output[i * 4 + 2]};
        output_color = (output_color / 255.0f * color) * 255.0f;

        output[i * 4] = output_color[0];
        output[i * 4 + 1] = output_color[1];
        output[i * 4 + 2] = output_color[2];
    } 

    std::shared_ptr<Texture> t(new Texture(output.data(), uvec3(output_size.x, output_size.y, 1), GL_TEXTURE_2D, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, floor(log2(float(glm::min(range.z, range.w))))));

    return t;
}

Input_system::Input_system() {
    Signature s = ecs.update_signature<Camera>();
    ecs.update_signature<Transform>(s);
    
    collectors.push_back(Collector(s));
    
    collectors.push_back(Collector(s));
    
    collectors.push_back(Collector(s));

    s = ecs.update_signature<Collider>();
    ecs.update_signature<Transform>(s);
    collectors.push_back(Collector(s, false));
}

template<typename Type>
struct Weight_value {
    Type value;
    float weight = 1.0f;
};

vec3 get_color(float a) {
    float aa = a * 6;
    float c = fract(aa);

    vec3 color;
    if(aa < 1) {
        color = vec3(1.0f, c, 0.0f);
    } else if(aa < 2) {
        color = vec3(1.0f - c, 1.0f, 0.0f);
    } else if(aa < 3) {
        color = vec3(0.0f, 1.0f, c);
    } else if(aa < 4) {
        color = vec3(0.0f, 1.0f - c, 1.0f);
    } else if(aa < 5) {
        color = vec3(c, 0.0f, 1.0f);
    } else {
        color = vec3(1.0f, 0.0f, 1.0f - c);   
    }

    return color;
}

vec3 get_color_hsv(float h, float s, float v) {
    float aa = h * 6;
    float c = fract(aa);

    vec3 color;
    if(aa < 1) {
        color = vec3(1.0f, c, 0.0f);
    } else if(aa < 2) {
        color = vec3(1.0f - c, 1.0f, 0.0f);
    } else if(aa < 3) {
        color = vec3(0.0f, 1.0f, c);
    } else if(aa < 4) {
        color = vec3(0.0f, 1.0f - c, 1.0f);
    } else if(aa < 5) {
        color = vec3(c, 0.0f, 1.0f);
    } else {
        color = vec3(1.0f, 0.0f, 1.0f - c);   
    }

    color = (color * s + (1.0f - s)) * v;

    return color;
}

void Input_system::call() {
    if(core.pressed_buttons.contains(GLFW_KEY_F8)) summon_character();
    if(core.pressed_buttons.contains(GLFW_KEY_F9)) create_crates();

    //if(core.pressed_buttons.contains(GLFW_KEY_0)) debug_speed = 1.0f;

    if(core.pressed_buttons.contains(GLFW_KEY_ESCAPE)) {
        core.cursor_disabled = !core.cursor_disabled;
        if(core.cursor_disabled) {
            glfwSetInputMode(core.window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if(glfwRawMouseMotionSupported()) glfwSetInputMode(core.window.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        } else {
            glfwSetInputMode(core.window.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            //glfwSetInputMode(core.window.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetInputMode(core.window.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
    }

    if(core.pressed_buttons.contains(GLFW_KEY_F11)) {
        core.window.fullscreen = !core.window.fullscreen;

        if(core.window.fullscreen) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();

            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            ivec2 pos;
            glfwGetWindowPos(core.window.window, &pos.x, &pos.y);

            core.window.prev_pos.z = core.window.screen_size.x;
            core.window.prev_pos.w = core.window.screen_size.y;
            core.window.prev_pos.x = pos.x;
            core.window.prev_pos.y = pos.y;

            // switch to full screen
            glfwSetWindowMonitor(core.window.window, monitor, 0, 0, mode->width, mode->height, 0);
        } else {
            // restore last window size and position
            glfwSetWindowMonitor(core.window.window, nullptr,  core.window.prev_pos.x, core.window.prev_pos.y, core.window.prev_pos.z, core.window.prev_pos.w, 0 );
        }
    }

    if(core.pressed_buttons.contains(GLFW_KEY_F6)) {
        std::vector<uint8_t> pixels(core.window.screen_size.x * core.window.screen_size.y * 3);

        glReadPixels(0,0, core.window.screen_size.x, core.window.screen_size.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        
        std::string filename = "output/screenshot" + to_base(int64_t(get_absolute_time() * 10), 10, true) + ".png";
        std::cout << "screenshot saved as " << filename << std::endl;

        stbi_flip_vertically_on_write(true);

        stbi_write_png(filename.c_str(), core.window.screen_size.x, core.window.screen_size.y, 3, pixels.data(), 3 * core.window.screen_size.x);
    }

    if(core.pressed_buttons.contains(GLFW_KEY_F5)) {
        Physics_system& ps = ecs.get_system<Physics_system>();
        ps.sim_active = !ps.sim_active;
    }

    if(core.pressed_buttons.contains(GLFW_KEY_F4) || core.repeat_buttons.contains(GLFW_KEY_F4)) {
        Physics_system& ps = ecs.get_system<Physics_system>();
        
        if(!ps.sim_active) {
            ps.physics_loop();
        }
    }
    
    if(core.pressed_buttons.contains(GLFW_KEY_F3)) {
        uint32_t camera_entity = *collectors[0].entities.begin();

        Transform& tf = ecs.get_component<Transform>(camera_entity);

        std::cout << tf.position.x.sector << ", " << tf.position.x.fraction << " | " << tf.position.y.sector << ", " << tf.position.y.fraction << " | " << tf.position.z.sector << ", " << tf.position.z.fraction << "\n";
    }
    
    uint32_t camera = player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    Camera& camera_camera = ecs.get_component<Camera>(camera);

    // movement

    float rotate_x = 0;
    float rotate_y = 0;
    float rotate_z = 0;

    bool jump = false;

    static double coyote_timer = FLT_MAX;
    float coyote_time = 0.125f;

    Building_system& building_system = ecs.get_system<Building_system>();
    
    // movement
    glm::vec3 raw_movement = {0, 0, 0};
    float rotate_value = 0.0f;
    if(core.cursor_disabled) {
        rotate_x = -core.cursor_delta.x;
        rotate_y = -core.cursor_delta.y;
    }

    if((!core.key_map[GLFW_MOUSE_BUTTON_RIGHT] || building_system.building_active == false || building_system.mode != BUILDING_MODE_PLACE) && core.cursor_disabled) {
        if(core.cursor_disabled) {
            if(core.key_map[GLFW_KEY_Q]) {
                rotate_z += 1;
            }
            if(core.key_map[GLFW_KEY_E]) {
                rotate_z += -1;
            }

            rotate_x = -core.cursor_delta.x;
            rotate_y = -core.cursor_delta.y;
        }

        if(core.key_map[GLFW_KEY_A]) {
            raw_movement.x += -1;
        }
        if(core.key_map[GLFW_KEY_D]) {
            raw_movement.x += 1;
        } 
        if(core.key_map[GLFW_KEY_S]) {
            raw_movement.y += -1;
        }
        if(core.key_map[GLFW_KEY_W]) {
            raw_movement.y += 1;
        }
        if(core.key_map[GLFW_KEY_SPACE]) {
            raw_movement.z += 1;
        }
        if(core.key_map[GLFW_KEY_LEFT_SHIFT]) {
            raw_movement.z -= 1;
        }
        if(core.key_map[GLFW_KEY_Q]) {
            rotate_value += 1;
        }
        if(core.key_map[GLFW_KEY_E]) {
            rotate_value -= 1;
        }
    }

    static double bobbing_height = 0.175f;
    static double bobbing_freq = 0.5625f;
    static double bobbing_time = 0.0f;

    if(camera_mode == CAMERA_FREECAM) { 
        if(core.scroll_delta != 0.0f) {
            debug_speed *= pow(2, core.scroll_delta * 0.5f);
        }

        float len = length(raw_movement);
        if(len != 0.0f) raw_movement = glm::normalize(raw_movement);

        raw_movement = {raw_movement.x, raw_movement.z, -raw_movement.y};
        
        glm::vec3 translation_vec = camera_transform.orientation * raw_movement;
        
        vec3 dir = camera_transform.orientation[2];
        vec3 u = camera_transform.orientation[1];
        glm::mat3 rotate_y_mat = (mat3)glm::rotate(float(2 * M_PI * (1.0 / 1024) * rotate_y), glm::normalize(-glm::cross(u, dir)));
        glm::mat3 rotate_x_mat = (mat3)glm::rotate(float(2 * M_PI * (1.0 / 1024) * rotate_x), u);
        glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * M_PI * (1.0 / 128) * rotate_z * (core.delta_time * 60)), dir);

        camera_transform.orientation = rotate_z_mat * rotate_x_mat * rotate_y_mat * camera_transform.orientation;
        camera_transform.position += translation_vec * (float)core.delta_time * debug_speed;
    } else {
        Physics_system& ps = ecs.get_system<Physics_system>();
        Transform& player_collider_transform = ecs.get_component<Transform>(player_collider);
        Collider& player_collider_collider = ecs.get_component<Collider>(player_collider);
        Billboard_animation& player_billboard = ecs.get_component<Billboard_animation>(player_collider);

        //

        uint32_t new_animation = 0;

        vec3 player_forward = player_collider_transform.orientation[1];
        vec3 player_up = player_collider_transform.orientation[2];
        vec3 camera_forward = -camera_transform.orientation[2];

        vec3 nn = camera_forward - player_up * dot(player_up, camera_forward);
        nn = normalize(nn);

        if(!isnan(nn.x)) {
            float angle = atan2(dot(player_up, cross(player_forward, nn)), dot(player_forward, nn));

            player_collider_transform.orientation = mat3(rotate(angle, player_up)) * player_collider_transform.orientation;
        }


        coyote_timer += core.delta_time;

        static vec3 velocity;

        float max_dot = -FLT_MAX;
        uint32_t colliding_entity = NULL_ENTITY;
        pvec3 rel_pos;

        Physics_system& physics_system = ecs.get_system<Physics_system>();

        bool iii = false;
        
        for(uint32_t entity : player_collider_collider.colliding_with) {
            std::array<uint32_t, 2> fs;
            fs[0] = min(entity, player_collider);
            fs[1] = max(entity, player_collider);

            auto& ct = ps.collision_table[fs];

            auto& manifold = ct[0];

            for(auto& collision : manifold.points) {
                vec3 normal;
                if(manifold.a == player_collider) normal = manifold.normal;
                else normal = -manifold.normal;

                float d = dot(normal, player_collider_transform.orientation[2]);
                if(d > max_dot) {
                    max_dot = d;
                    if(player_collider < entity) rel_pos = collision.contact_point.b;
                    else rel_pos = collision.contact_point.a;
                    colliding_entity = entity;
                }

                max_dot = max(max_dot, d);
            }
        }

        if(max_dot > 0.4f) {
            if(colliding_entity != NULL_ENTITY) {
                Collider& ce_collider = ecs.get_component<Collider>(colliding_entity);
                Transform& ce_transform = ecs.get_component<Transform>(colliding_entity);

                velocity = ce_collider.get_velocity(ce_transform.orientation * vec3(rel_pos));
            } else {
                velocity = vec3(0.0f);
            }

            coyote_timer = 0.0f;
        }

        static bool jump = false;
        static bool j2 = false;
        static bool sprint = false;
        static bool moving = false;

        if(dot(player_collider_collider.velocity, player_collider_transform.orientation[2]) <= 0.0f) j2 = true;
        if(j2 && coyote_timer == 0.0f) {
            j2 = false;
            jump = true;
        }

        if(core.pressed_buttons.contains(GLFW_KEY_LEFT_CONTROL)) {
            sprint = true;
        }

        static vec3 prev_up = vec3(0.0f);
        static vec3 prev_x = vec3(0.0f);

        vec3 new_up = get_gravity(player_collider_transform.position);
        vec3 up = player_collider_transform.orientation[2];

        if(dot(new_up, up) < 1.0f && prev_up != new_up) {
            mat3 rot_to = rotate_to(up, new_up);
            player_collider_transform.orientation = rot_to * player_collider_transform.orientation;
            prev_up = new_up;
        }

        vec3 x_camera = camera_transform.orientation[0];
        vec3 z_collider = player_collider_transform.orientation[2];

        vec3 new_x_camera = normalize(x_camera - z_collider * dot(x_camera, z_collider));
        if(dot(x_camera, new_x_camera) < 0.9995f && prev_x != x_camera) {
            mat3 rot_to = rotate_to(x_camera, new_x_camera);

            camera_transform.orientation = rot_to * camera_transform.orientation;
            prev_x = camera_transform.orientation[0];
        }

        vec3 dir = camera_transform.orientation[2];
        vec3 u = player_collider_transform.orientation[2];
        glm::mat3 rotate_y_mat = (mat3)glm::rotate(float(2 * M_PI * (1.0 / 1024) * -rotate_y), new_x_camera);
        glm::mat3 rotate_x_mat = (mat3)glm::rotate(float(2 * M_PI * (1.0 / 1024) * rotate_x), u);
        //glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * M_PI * (1.0 / 128) * rotate_z * (core.delta_time * 60)), dir);
        camera_transform.orientation = rotate_x_mat * rotate_y_mat * camera_transform.orientation;

        // bobbing
        vec2 bobbing_pos = vec2(0.0f);
        static double sprint_blend = 0.0f;
        if(moving) {
            bobbing_time += core.delta_time;
            if(sprint) sprint_blend = clamp(sprint_blend + core.delta_time, 0.0, 0.5);
            else sprint_blend = clamp(sprint_blend - core.delta_time, 0.0, 0.5);

            new_animation = 1;
        } else if(bobbing_time != 0.0f) {
            float f = (bobbing_freq * 0.5f);
            float next_zero = ceil(bobbing_time / f) * f;

            bobbing_time += core.delta_time;
            if(bobbing_time >= next_zero) bobbing_time = 0.0f;
        } else {
            sprint_blend = 0.0f;
        }

        bobbing_height = mix(0.125f, 0.225f, smoothstep(sprint_blend / 0.5f));
        double l = bobbing_time - floor(bobbing_time / bobbing_freq) * bobbing_freq;
        l /= bobbing_freq;
        l *= M_PI * 2.0f;
        bobbing_pos.y = sin(l) * bobbing_height * 0.5f;
        if(bobbing_pos.y < 0.0f) bobbing_pos.y *= 1.5f;

        camera_transform.position = player_collider_transform.position + pvec3(player_collider_transform.orientation * vec3(bobbing_pos.x, 0.0f, character_size.y * 0.5f - character_size.x * 0.3125f + bobbing_pos.y));

        // movement

        float rmz = raw_movement.z;
        
        raw_movement = {raw_movement.x, -raw_movement.y, 0.0f};

        if(coyote_timer < coyote_time) {
            vec3 normal;
            float m = -FLT_MAX;
            for(vec3 v : player_collider_collider.colliding_normal) {
                float nm = dot(v, player_collider_transform.orientation[2]);

                if(nm > m){
                    m = nm;
                    normal = v;
                }
            }
            if(player_collider_collider.colliding_normal.size() == 0) normal = player_collider_transform.orientation[2];

            vec3 nx = camera_transform.orientation[0];
            vec3 ny = camera_transform.orientation[2];
            vec3 nz = player_collider_transform.orientation[2];
            nx = normalize(nx - nz * dot(nz, nx));
            ny = normalize(ny - nz * dot(nz, ny));
            
            nz = normal;
            vec3 nxa = camera_transform.orientation[0];
            vec3 nya = camera_transform.orientation[2];
            nxa = normalize(nxa - nz * dot(nz, nxa));
            nya = normalize(nya - nz * dot(nz, nya));
            
            if(coyote_timer == 0.0f) {
                if(length(raw_movement) != 0.0f) {
                    moving = true;
                    raw_movement = normalize(raw_movement);
                } else {
                    moving = false;
                    sprint = false;
                }

                vec3 delta_v = nx * raw_movement.x + ny * raw_movement.y;
                vec3 delta_va = nxa * raw_movement.x + nya * raw_movement.y;

                if(dot(delta_v, camera_transform.orientation[2]) > dot(delta_va, camera_transform.orientation[2])) delta_v = delta_va;
                else delta_v = (delta_v + delta_va) * 0.5f;

                float speed = 4.0f;
                float accel = 60.0f;
                if(sprint) {
                    speed = 8.0f;
                    accel = 90.0f;
                }

                vec3 target_vel = delta_v * speed;
                vec3 rel_vel = player_collider_collider.velocity - velocity;
                vec3 current_vel = rel_vel - nz * dot(nz, rel_vel);
                
                vec3 diff = target_vel - current_vel;
                float diff_len = length(diff);

                if(diff_len != 0.0f) {
                    vec3 diff_norm = diff / diff_len;

                    float delta = accel * core.delta_time;
                    delta = min(delta, diff_len);

                    player_collider_collider.velocity += diff_norm * delta;
                }
            }

            float jump_vel = 5.0f * character_size.x;
            if(jump) {
                if(rmz > 0.0f) {
                    player_collider_collider.velocity += nz * jump_vel;
                    jump = false;
                }
            }
        } else moving = false;

        player_billboard.set_animation(new_animation);
    }

    // holding values
    float grab_distance = 128.0f;
    float step = 2.0f;

    if(camera_mode == CAMERA_PLAYER) {
        grab_distance = 5.0f;
        step = 1.0f;
    }

    // holding
    GUI_system& gui_system = ecs.get_system<GUI_system>();
    Physics_system& ps = ecs.get_system<Physics_system>();
    vec3 dir = camera_transform.orientation * vec3(0, 0, -1);

    bool captured = true;

    if(!core.cursor_disabled) {
        vec2 frac = (core.cursor_pos - view_range.xy()) / view_range.zw();

        if(floor(frac) == vec2(0.0f, 0.0f)) {
            vec4 v = vec4(frac * 2.0f - 1.0f, 0.5f, 1.0f);
            vec4 pos = inverse(camera_camera.proj) * v;
            pos /= pos.w;

            dir = camera_transform.orientation * normalize(pos.xyz());
        } else captured = false;
    }

    if(captured) {
        if(select_target) {
            if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                uint32_t object;
                uint32_t shape;
                vec3 normal;
                pvec3 point;

                bool did_hit = ps.raycast(camera_transform.position, dir, step, grab_distance, raycast_mask, &object, &shape, &normal, &point);
                if(did_hit) {
                    object_target = object;
                    select_target = false;
                }
            }
        } else {
            if(held_object != NULL_ENTITY) {
                if(!core.key_map[GLFW_MOUSE_BUTTON_LEFT]) {

                    held_object = NULL_ENTITY;

                    ps.constraints.erase(ps.constraints.begin() + held_constraint);
                    held_constraint = NULL_ENTITY;
                } else {
                    Constraint& c = ps.constraints[held_constraint];
                    if(c.b != NULL_ENTITY) {
                        Transform& p_transform = ecs.get_component<Transform>(player_collider);
                        pvec3 pos = camera_transform.position + pvec3(dir * held_dist);

                        c.pos[0].b = transpose(p_transform.orientation) * (pos - p_transform.position);
                    } else {
                        c.pos[0].b = camera_transform.position + pvec3(dir * held_dist);
                    }
                }

                // change distance

                float dist_value = 0.0f;
                if(core.key_map[GLFW_KEY_R]) {
                    dist_value += 1.0f;
                }
                if(core.key_map[GLFW_KEY_F]) {
                    dist_value -= 1.0f;
                }
                held_dist *= pow(2.0f, dist_value * core.delta_time);

                held_dist = clamp(held_dist, 0.5f, grab_distance);
            } else {
                //if(gui_system.capture_widget == "" && gui_system.capture_window == "") {
                {
                    if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
                        uint32_t object;
                        uint32_t shape;
                        vec3 normal;
                        pvec3 point;

                        bool did_hit = ps.raycast(camera_transform.position, dir, step, grab_distance, raycast_mask, &object, &shape, &normal, &point);
                        if(did_hit) {
                            Physics_system& physics_system = ecs.get_system<Physics_system>();
                            Particle_system& particle_system = ecs.get_system<Particle_system>();
                            particle_system.ps_indices.clear();
                            particle_system.ps_vertices.clear();
                            
                            Transform& ta = ecs.get_component<Transform>(object);
                            Collider& cca = ecs.get_component<Collider>(object);
                            Convex_collider& ca = cca.collision_shapes[shape];
                            if(!physics_system.sim_active) {
                                float offset_v = 0.0f;//0.0625f;

                                bool ma = false;//ca.collision_shape->mass == 0.0f;
                                for(auto& a_face : ca.collision_shape->faces) {
                                    vec3 center_a = vec3(0.0f);
                                    vec3 offset = ta.orientation * ca.orientation * a_face.normal;

                                    for(int i = 0; i < a_face.vertices.size(); ++i) {

                                        int i0 = i;
                                        int i1 = (i + 1) % a_face.vertices.size();

                                        vec3 v0 = ta.orientation * (ca.orientation * ca.collision_shape->vertices[a_face.vertices[i0]] + (vec3)ca.position);
                                        vec3 v1 = ta.orientation * (ca.orientation * ca.collision_shape->vertices[a_face.vertices[i1]] + (vec3)ca.position);

                                        center_a += v0;

                                        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                        particle_system.ps_vertices.push_back(v0 + offset * offset_v);
                                        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                        particle_system.ps_vertices.push_back(v1 + offset * offset_v);
                                        particle_system.rel_pos = ta.position;
                                    }
                                    center_a /= a_face.vertices.size();
                                
                                    particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                    particle_system.ps_vertices.push_back(center_a);
                                    particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                    particle_system.ps_vertices.push_back(center_a + ta.orientation * a_face.normal * 0.25f);
                                }

                                Bounding_box bb = transform_bb(ca.bounding_box, vec3(0.0f), ta.orientation);

                                vec3 v0 = {bb.minimum.x, bb.minimum.y, bb.minimum.z};
                                vec3 v1 = {bb.maximum.x, bb.minimum.y, bb.minimum.z};
                                vec3 v2 = {bb.minimum.x, bb.maximum.y, bb.minimum.z};
                                vec3 v3 = {bb.maximum.x, bb.maximum.y, bb.minimum.z};
                                vec3 v4 = {bb.minimum.x, bb.minimum.y, bb.maximum.z};
                                vec3 v5 = {bb.maximum.x, bb.minimum.y, bb.maximum.z};
                                vec3 v6 = {bb.minimum.x, bb.maximum.y, bb.maximum.z};
                                vec3 v7 = {bb.maximum.x, bb.maximum.y, bb.maximum.z};

                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v0);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v1);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v2);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v3);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v4);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v5);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v6);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v7);

                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v0);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v2);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v1);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v3);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v4);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v6);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v5);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v7);

                                
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v0);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v4);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v1);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v5);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v2);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v6);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v3);
                                particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
                                particle_system.ps_vertices.push_back(v7);
                            } else {
                                if(camera_mode == CAMERA_FREECAM) {
                                    if(!cca.is_static) {
                                        held_object = object;
                                        held_constraint = ps.constraints.size();

                                        Constraint constraint;
                                        constraint.a = object;

                                        Transform& o_transform = ecs.get_component<Transform>(object);
                                        
                                        pos_constraint pc;
                                        pc.a = transpose(o_transform.orientation) * vec3(point - o_transform.position);
                                        pc.b = point;
                                        pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
                                        pc.softness = 0.0f;
                                        pc.spring = 0.5f;
                                        
                                        pc.max_impulse = FLT_MAX;
                                        //pc.is_grab = true;

                                        float diff = length(vec3(point - camera_transform.position));

                                        held_dist = diff;

                                        constraint.pos.push_back(pc);

                                        ps.constraints.push_back(constraint);
                                    }
                                } else if(camera_mode == CAMERA_PLAYER) {
                                    if(!cca.is_static) {
                                        held_object = object;
                                        held_constraint = ps.constraints.size();

                                        Constraint constraint;
                                        constraint.a = object;
                                        constraint.b = player_collider;

                                        Transform& o_transform = ecs.get_component<Transform>(object);
                                        Transform& p_transform = ecs.get_component<Transform>(player_collider);
                                        
                                        pos_constraint pc;
                                        pc.a = transpose(o_transform.orientation) * vec3(point - o_transform.position);
                                        pc.b = transpose(p_transform.orientation) * vec3(point - p_transform.position);
                                        pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
                                        pc.softness = 0.0f;
                                        pc.spring = 0.5f;
                                        
                                        pc.max_impulse = FLT_MAX;
                                        pc.max_impulse = held_force / (ps.fps * ps.substeps);

                                        float diff = length(vec3(point - camera_transform.position));

                                        held_dist = diff;

                                        constraint.pos.push_back(pc);

                                        ps.constraints.push_back(constraint);
                                    }
                                }
                            }
                        }
                    } else {
                        uint32_t object;
                        uint32_t shape;
                        vec3 normal;
                        pvec3 point;

                        bool did_hit = ps.raycast(camera_transform.position, dir, step, grab_distance, raycast_mask, &object, &shape, &normal, &point);

                        if(did_hit && false) {
                            Physics_system& physics_system = ecs.get_system<Physics_system>();
                            Particle_system& particle_system = ecs.get_system<Particle_system>();
                            particle_system.ps_indices.clear();
                            particle_system.ps_vertices.clear();

                            particle_system.rel_pos = point;
                            particle_system.ps_indices = {0, 1};
                            particle_system.ps_vertices = {
                                vec3(0.0f),
                                normal * 0.25f
                            };
                        }
                    }
                }
            }
        }
    }

    if(object_target != NULL_ENTITY) {
        Particle_system& particle_system = ecs.get_system<Particle_system>();

        Transform& transform = ecs.get_component<Transform>(object_target);
        Collider& collider = ecs.get_component<Collider>(object_target);

        particle_system.insert_icon(transform.position, vec4(1.0f), vec2(10, 10), vec4(31, 0, 5, 5), S_CONSTANT_SIZE_BIT | S_DEPTH_BIT);
        
        // bounding box

        particle_system.rel_pos = transform.position;

        Bounding_box bb = transform_bb(collider.bounding_box, vec3(0.0f), transform.orientation);

        vec3 v0 = {bb.minimum.x, bb.minimum.y, bb.minimum.z};
        vec3 v1 = {bb.maximum.x, bb.minimum.y, bb.minimum.z};
        vec3 v2 = {bb.minimum.x, bb.maximum.y, bb.minimum.z};
        vec3 v3 = {bb.maximum.x, bb.maximum.y, bb.minimum.z};
        vec3 v4 = {bb.minimum.x, bb.minimum.y, bb.maximum.z};
        vec3 v5 = {bb.maximum.x, bb.minimum.y, bb.maximum.z};
        vec3 v6 = {bb.minimum.x, bb.maximum.y, bb.maximum.z};
        vec3 v7 = {bb.maximum.x, bb.maximum.y, bb.maximum.z};

        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v0);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v1);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v2);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v3);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v4);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v5);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v6);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v7);

        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v0);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v2);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v1);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v3);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v4);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v6);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v5);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v7);

        
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v0);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v4);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v1);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v5);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v2);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v6);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v3);
        particle_system.ps_indices.push_back(particle_system.ps_vertices.size());
        particle_system.ps_vertices.push_back(v7);
    }
}

void Input_system::set_camera_mode(Camera_mode mode) {
    uint32_t camera = player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);

    camera_mode = mode;
    
    if(camera_mode == CAMERA_PLAYER) {
        if(player_collider == NULL_ENTITY) {
            vec2 size = character_size;

            Transform t;
            t.position = camera_transform.position;
            vec3 up = get_gravity(t.position);
            t.orientation = rotate_to(vec3(0.0, 0.0, 1.0), up);
            
            Collider c;
            c.collect = true;

            Collision_shape cs;
            cs.vertices = {vec3(0.0f, 0.0f, -(size.y - size.x) * 0.5f), vec3(0.0f, 0.0f, (size.y - size.x) * 0.5f)};
            cs.radius = size.x * 0.5f;
            cs.mass = 0.58f;
            c.collision_shapes.resize(1);
            c.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(cs);
            c.allow_rotation = false;

            vec3 shift = Physics_system::initialize_collider(c);
            t.position += t.orientation * shift;

            //Mesh_component m;
            //create_mesh(m, cs.vertices, vec3(cs.radius));
            //m.mesh->color = vec3(1.0f, 0.8f, 0.35f);

            // add character
            Billboard_animation bb;

            bb.texture = core.textures["averie"];

            int start = bb.texture->size.x / 48 - 8;
            int end = bb.texture->size.x / 48;

            int r = core.random.next() % (end - start) + start;

            vec2 size2 = vec2(4.0f, 4.0f);

            uint32_t i = 64;
            
            
            std::vector<Animation_frame> anim0 = {
                Animation_frame({{0, 0, i, i}}, 0.2f),
            };

            std::vector<Animation_frame> anim1 = {
                Animation_frame({{0, i, i, i}}, 0.25f),
                Animation_frame({{0, i * 2, i, i}}, 0.25f),
                Animation_frame({{0, i * 3, i, i}}, 0.25f),
                Animation_frame({{0, i * 4, i, i}}, 0.25f),
            };

            for(int j = 1; j < 8; ++j) {
                for(Animation_frame& f : anim0) {
                    f.regions.push_back(f.regions[0] + ivec4(i * j, 0, 0, 0));
                }

                for(Animation_frame& f : anim1) {
                    f.regions.push_back(f.regions[0] + ivec4(i * j, 0, 0, 0));
                }
            }

            bb.animations = {{anim0, 0}, {anim1, 0}};
            bb.current_animation = 0;
            bb.current_time = 0;
            bb.size = size2;
            bb.height = -1.75f;
            //

            uint32_t entity = ecs.insert_entity();
            ecs.insert_component(entity, t);
            ecs.insert_component(entity, c);
            ecs.insert_component(entity, bb);

            player_collider = entity;
        }
        
        raycast_mask.emplace(player_collider);

        if(held_object != NULL_ENTITY) {
            Physics_system& ps = ecs.get_system<Physics_system>();
            ps.constraints[held_constraint].pos[0].max_impulse = held_force / ps.fps;
        }
    } else if(camera_mode == CAMERA_FREECAM) {
        raycast_mask.clear();
        
        if(held_object != NULL_ENTITY) {
            Physics_system& ps = ecs.get_system<Physics_system>();
            ps.constraints[held_constraint].pos[0].max_impulse = FLT_MAX;
        }
    }
}

mat3 capsule_inertia(float m, float r, float h) {
    float cyl_area = M_PI * r * r * h;
    float hemi_area = 4.0f / 3.0f * M_PI * r * r * r * 0.5f;

    float total = cyl_area + hemi_area * 2.0f;

    float cyl_mass = m * cyl_area / total;
    float hemi_mass = m * hemi_area / total;


    float Ixx = cyl_mass * (r * r / 4.0f + h * h / 12.0f) + 2.0f * hemi_mass * (2 * r * r / 5.0f + 3.0f * r * h / 8.0f + h * h / 2.0f);
    float Iyy = Ixx;
    float Izz = cyl_mass * (r * r / 2.0f) + 2.0f * hemi_mass * (2.0f * r * r / 5.0f);

    return mat3(
        vec3(Ixx, 0, 0),
        vec3(0, Iyy, 0),
        vec3(0, 0, Izz)
    );
}

void create_chain() {
    //vec3 color = vec3(1.0f, 0.35f, 0.55f);
    vec3 color = vec3(0.35f, 0.35f, 1.0f);

    Input_system& input_system = ecs.get_system<Input_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    
    Physics_system& ps = ecs.get_system<Physics_system>();
    std::unordered_set<uint32_t> collision_mask;

    uint32_t num_links = input_system.num_links;

    float scale = input_system.object_scale;

    float sep = 0.05f * scale;
    vec2 size = vec2(0.25f, 1.5f) * scale;

    Transform t;
    t.position = camera_transform.position;
    vec3 up = camera_transform.orientation * vec3(0, 0, -1);
    t.orientation = rotate_to(vec3(0.0, 0.0, 1.0), up);
    
    Collider c;
    Collision_shape cs;
    cs.vertices = {vec3(0.0f, 0.0f, -(size.y - size.x) * 0.5f), vec3(0.0f, 0.0f, (size.y - size.x) * 0.5f)};
    cs.radius = size.x * 0.5f;
    cs.mass = 1.0f;
    c.collision_shapes.resize(1);
    c.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(cs);
    c.allow_rotation = true;

    vec3 shift = Physics_system::initialize_collider(c);

    //c.inertia_tensor[1][1] = c.inertia_tensor[0][0];
    //c.inertia_tensor[2][2] = c.inertia_tensor[0][0];
    //c.inverse_inertia_tensor = inverse(c.inertia_tensor);

    /*
    mat3 i = capsule_inertia(0.035f, size.x * 0.5f, (size.y - size.x));

    std::cout << "NEW TENSOR\n";
    std::cout << i[0].x << " " << i[0].y << " " << i[0].z << "\n";
    std::cout << i[1].x << " " << i[1].y << " " << i[1].z << "\n";
    std::cout << i[2].x << " " << i[2].y << " " << i[2].z << "\n";
    
    std::cout << "\nOLD TENSOR\n";
    std::cout << c.inertia_tensor[0].x << " " << c.inertia_tensor[0].y << " " << c.inertia_tensor[0].z << "\n";
    std::cout << c.inertia_tensor[1].x << " " << c.inertia_tensor[1].y << " " << c.inertia_tensor[1].z << "\n";
    std::cout << c.inertia_tensor[2].x << " " << c.inertia_tensor[2].y << " " << c.inertia_tensor[2].z << "\n";

    c.inertia_tensor = i;
    c.inverse_inertia_tensor = inverse(i);
    */

    t.position += t.orientation * shift;
    t.position += t.orientation * vec3(0.0f, 0.0f, size.y * 0.5f);

    Mesh_component m;
    create_mesh(m, cs.vertices, vec3(cs.radius));
    m.mesh->color = color;

    uint32_t prev_entity = NULL_ENTITY;
    uint32_t first_entity;

    for(int i = 0; i < num_links; ++i) {    
        uint32_t capsule = ecs.insert_entity();
        collision_mask.emplace(capsule);

        ecs.insert_component(capsule, m);
        ecs.insert_component(capsule, t);
        ecs.insert_component(capsule, c);
        
        DOF_constraint dof;
        dof.a = capsule;
        dof.b = capsule;
        dof.locked_axis = vec3(0.0f, 0.0f, 1.0f);
        dof.factor = 20.0f;

        ps.dof_constraints.push_back(dof);
        
        if(prev_entity != NULL_ENTITY) {
            Constraint constraint;
            constraint.a = prev_entity;
            constraint.b = capsule;

            pos_constraint pc;
            pc.a = vec3(0, 0, (size.y + sep) * 0.5f);
            pc.b = vec3(0, 0, -(size.y + sep) * 0.5f);
            pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
            constraint.pos.push_back(pc);

            //pc.compliance = 0.001;

            ps.constraints.push_back(constraint);
        } else first_entity = capsule;

        t.position += t.orientation * vec3(0, 0, (size.y + sep));

        prev_entity = capsule;
    }

    float asteroid_radius = 0.5f * scale;
    
    Collider c2;
    Collision_shape cs2;
    cs2.vertices = {vec3(0.0f)};
    cs2.radius = asteroid_radius;
    cs2.mass = 3.0f;
    c2.collision_shapes.resize(1);
    c2.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(cs2);
    c2.allow_rotation = true;

    shift = Physics_system::initialize_collider(c2);

    t.position += t.orientation * shift;
    t.position += t.orientation * vec3(0.0f, 0.0f, asteroid_radius);

    Mesh_component m2;
    create_mesh(m2, cs2.vertices, vec3(cs2.radius));
    m2.mesh->color = color;
    
    t.position = camera_transform.position + pvec3(t.orientation * vec3(0.0f, 0.0f, (size.y + sep) * num_links + asteroid_radius));

    uint32_t asteroid = ecs.insert_entity();
    ecs.insert_component(asteroid, m2);
    ecs.insert_component(asteroid, t);
    ecs.insert_component(asteroid, c2);

    {
        Constraint constraint;
        constraint.a = prev_entity;
        constraint.b = asteroid;

        pos_constraint pc;
        pc.a = vec3(0, 0, (size.y + sep) * 0.5f);
        pc.b = vec3(0, 0, -(asteroid_radius + sep * 0.5f));
        pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
        constraint.pos.push_back(pc);
        
        ps.constraints.push_back(constraint);
        collision_mask.emplace(asteroid);

        DOF_constraint dof;
        dof.a = asteroid;
        dof.locked_axis = vec3(0.0f, 0.0f, 1.0f);
        dof.factor = 20000.0f;

        ps.dof_constraints.push_back(dof);
    }

    //

    t.position = camera_transform.position + pvec3(t.orientation * -vec3(0.0f, 0.0f, asteroid_radius));

    /*
    asteroid = ecs.insert_entity();
    ecs.insert_component(asteroid, m2);
    ecs.insert_component(asteroid, t);
    ecs.insert_component(asteroid, c2);

    {
        Constraint constraint;
        constraint.a = asteroid;
        constraint.b = first_entity;

        pos_constraint pc;
        pc.a = vec3(0, 0, asteroid_radius + sep * 0.5f);
        pc.b = vec3(0, 0, -(size.y + sep) * 0.5f);
        pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
        constraint.pos.push_back(pc);

        ps.constraints.push_back(constraint);
        collision_mask.emplace(asteroid);
        
        DOF_constraint dof;
        dof.a = asteroid;
        dof.locked_axis = vec3(0.0f, 0.0f, 1.0f);
        dof.factor = 20000.0f;

        ps.dof_constraints.push_back(dof);
    }

    for(uint32_t link : collision_mask) {
        Collider& c = ecs.get_component<Collider>(link);
        c.collision_mask = collision_mask;
    }
    */
}

/*
void create_capsule() {
    Input_system& input_system = ecs.get_system<Input_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    
    Physics_system& ps = ecs.get_system<Physics_system>();
    std::unordered_set<uint32_t> collision_mask;

    //vec2 size = vec2(0.5f, 12.0f);
    vec2 size = vec2(4.0f, 40.0f);

    Transform t;
    t.position = camera_transform.position;
    vec3 up = camera_transform.orientation * vec3(0, 0, -1);
    t.orientation = rotate_to(vec3(0.0, 0.0, 1.0), up);
    
    Collider c;
    Collision_shape cs;
    cs.vertices = {vec3(0.0f, 0.0f, -(size.y - size.x) * 0.5f), vec3(0.0f, 0.0f, (size.y - size.x) * 0.5f)};
    cs.radius = size.x * 0.5f;
    cs.mass = 0.4f * size.x * size.x * size.y;
    c.collision_shapes.resize(1);
    c.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(cs);
    c.allow_rotation = true;

    vec3 shift = Physics_system::initialize_collider(c, {0.75f});

    t.position += t.orientation * shift;
    t.position += t.orientation * vec3(0.0f, 0.0f, size.y * 0.5f);

    Mesh_component m;
    create_mesh(m, cs.vertices, vec3(cs.radius));
    m.mesh->color = vec3(1.0f, 0.85f, 0.35f);
    //m.mesh->color = vec3(1.0f, 0.85f, 0.35f);

    uint32_t capsule = ecs.insert_entity();
    ecs.insert_component(capsule, t);
    ecs.insert_component(capsule, c);
    ecs.insert_component(capsule, m);
}
*/

/*
Input_system& input_system = ecs.get_system<Input_system>();

uint32_t camera = input_system.player_camera;
Transform& camera_transform = ecs.get_component<Transform>(camera);

Physics_system& ps = ecs.get_system<Physics_system>();
std::unordered_set<uint32_t> collision_mask;

//vec2 size = vec2(0.5f, 12.0f);
vec3 size = vec3(8.0f, 8.0f, 8.0f);

Transform t;
t.position = camera_transform.position;
vec3 up = camera_transform.orientation * vec3(0, 0, -1);
t.orientation = rotate_to(vec3(0.0, 0.0, 1.0), up);

Collider c;
Collision_shape cs;
cs.vertices = {vec3(-0.5f, -0.5f, -0.5f), vec3(0.5f, -0.5f, -0.5f), vec3(-0.5f, 0.5f, -0.5f), vec3(0.5f, 0.5f, -0.5f), vec3(-0.5f, -0.5f, 0.5f), vec3(0.5f, -0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(0.5f, 0.5f, 0.5f)};
for(vec3& v : cs.vertices) v *= size;
cs.radius = 1.0f;
c.collision_shapes.resize(1);
c.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(cs);
c.allow_rotation = true;

vec3 shift = Physics_system::initialize_collider(c, {0.75f});

t.position += t.orientation * shift;
t.position += t.orientation * vec3(0.0f, 0.0f, size.y * 0.5f);

Mesh_component m;
create_mesh(m, cs.vertices, vec3(cs.radius));
m.mesh->color = vec3(0.25f, 1.0f, 0.25f);
//m.mesh->color = vec3(1.0f, 0.85f, 0.35f);

uint32_t capsule = ecs.insert_entity();
ecs.insert_component(capsule, t);
ecs.insert_component(capsule, c);
ecs.insert_component(capsule, m);
*/

/* compound shape
void create_capsule() {
    Input_system& input_system = ecs.get_system<Input_system>();
    Building_system& building_system = ecs.get_system<Building_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);

    // 

    uint32_t edge_size = 1;
    int num = 10;
    int thickness = 5;
    Build build;
    
    for(int x = 0; x < num; ++x) {
        for(int y = 0; y < num; ++y) {
            for(int z = 0; z < num; ++z) {
                if(x < thickness || x >= num - thickness || y < thickness || y >= num - thickness || z < thickness || z >= num - thickness) {
                    Brick b;
                    b.color = random_color(core.random());
                    b.orientation_x = vec3(1.0f, 0.0f, 0.0f);
                    b.orientation_y = vec3(0.0f, 1.0f, 0.0f);
                    b.orientation_z = vec3(0.0f, 0.0f, 1.0f);
                    b.position = ivec3(x, y, z) * (int)edge_size;
                    b.size = ivec3(edge_size);
                    b.primitive_id = 0;

                    build.bricks.push_back(b);
                }
            }
        }
    }

    std::vector<ivec3> vs = {
        ivec3(-1, -2, 0),
        ivec3(1, -2, 0),

        ivec3(-1, -1, 0),
        ivec3(0, -1, 0),
        ivec3(1, -1, 0),

        ivec3(-2, 0, 0),
        ivec3(-1, 0, 0),
        ivec3(0, 0, 0),
        ivec3(1, 0, 0),
        ivec3(2, 0, 0),

        ivec3(0, 1, 0),
        ivec3(0, 2, 0)
    };

    vec3 color1 = random_color(core.random()) * 0.65f + 0.35f;
    vec3 color2 = random_color(core.random()) * 0.65f + 0.35f;

    for(ivec3 v : vs) {
        float dist = min(1.0f, length((vec3)v) / 3.0f);

        Brick b;
        b.color = color1 * (1.0f - dist) + color2 * dist;

        b.orientation_x = vec3(1.0f, 0.0f, 0.0f);
        b.orientation_y = vec3(0.0f, 1.0f, 0.0f);
        b.orientation_z = vec3(0.0f, 0.0f, 1.0f);
        b.position = v * (int)edge_size;
        b.size = ivec3(edge_size);
        b.primitive_id = 0;

        build.bricks.push_back(b);
    }

    uint32_t entity = ecs.insert_entity();

    ecs.insert_component(entity, camera_transform);
    ecs.insert_component(entity, build);

    building_system.create_build_collider(entity);
    building_system.mesh_build(entity);
}
*/

void create_capsule() {
    Input_system& input_system = ecs.get_system<Input_system>();
    Building_system& building_system = ecs.get_system<Building_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);

    // 

    int num = 6;
    float inner_radius = 6.0f;
    float thickness = 6.0f;
    float depth = 6.0f;

    float inner_width = inner_radius * 2.0f * tan(M_PI / num);
    float outer_width = (inner_radius + thickness) * 2.0f * tan(M_PI / num);

    std::vector<vec3> vertices = {
        vec3(-1.0f, 0.0f, -1.0f),
        vec3(1.0f, 0.0f, -1.0f),
        vec3(-1.0f, 0.0f, 1.0f),
        vec3(1.0f, 0.0f, 1.0f),
    };

    std::vector<vec3> vs;

    for(vec3 v : vertices) {
        vs.push_back(v * vec3(inner_width, 1.0f, depth) * 0.5f);
        vs.push_back(v * vec3(outer_width, 1.0f, depth) * 0.5f + vec3(0.0f, thickness, 0.0f));
    }
    
    Collider c;
    c.collision_shapes.resize(num);
    c.allow_rotation = true;


    Convex_collider cc;
    cc.position = vec3(0.0f, inner_radius, 0.0f);
    cc.orientation = identity<mat3>();

    mat3 rot = glm::rotate(float(2 * M_PI * (1.0f / num)), vec3(0.0f, 0.0f, 1.0f));

    //

    std::vector<float> densities;

    for(int i = 0; i < num; ++i) {
        densities.push_back(0.8f);

        Collision_shape cs;
        cs.vertices = vs;
        cs.radius = 0.0f;
        
        cc.collision_shape = std::make_shared<Collision_shape>(cs);
        c.collision_shapes[i] = cc;

        //

        cc.position += cc.orientation * vec3(-inner_width, 0.0f, 0.0f);

        vec3 vs = cc.orientation * vec3(inner_width * 0.5f, 0.0f, depth * 0.5f) + cc.position;

        cc.position -= vs;
        cc.position = rot * cc.position;
        cc.orientation = rot * cc.orientation;
        cc.position += vs;
    }

    

    //

    Physics_system::initialize_collider(c, densities);
    c.create_BVH();

    Transform t = camera_transform;
    //t.position += t.orientation * shift;

    uint32_t entity = ecs.insert_entity();
    ecs.insert_component(entity, t);
    ecs.insert_component(entity, c);

    // fuchsia vec3(1.0f, 0.25f, 0.5f)
    // yellow vec3(1.0f, 0.875f, 0.375f)
    create_mesh_from_collider(entity, get_color_hsv(2.25f / 6.0f, 0.65f, 0.65f));

    /*
    std::vector<vec3> vs = {
        vec3(0.0f, 0.0f, 0.0f),
        vec3(64.0f, 0.0f, 0.0f),
        vec3(6.0f, 12.0f, 0.0f),
        vec3(58.0f, 12.0f, 0.0f),
        
        // 

        vec3(8.0f, 16.0f, 0.0f),
        vec3(22.0f, 16.0f, 0.0f),
        vec3(24.0f, 48.0f, 0.0f),
        vec3(32.0f, 48.0f, 0.0f),
        vec3(32.0f, 36.0f, 0.0f),

        //

        vec3(42.0f, 16.0f, 0.0f),
        vec3(56.0f, 16.0f, 0.0f),
        vec3(32.0f, 48.0f, 0.0f),
        vec3(32.0f, 36.0f, 0.0f),
        vec3(40.0f, 48.0f, 0.0f),
    };

    std::vector<int> ranges = {
        0, 4,
        4, 9,
        9, 14
    };

    for(vec3& v : vs) v /= 2.0f;

    //

    Collider c;
    c.collision_shapes.resize(3);
    c.allow_rotation = true;

    std::vector<float> densities;

    for(int i = 0; i < 3; ++i) {
        densities.push_back(0.8f);

        std::vector<vec3> vertices(vs.begin() + ranges[i * 2], vs.begin() + ranges[i * 2 + 1]);

        vec3 center = vec3(0.0f);
        for(vec3 v : vertices) center += v;
        center /= (float)vertices.size();

        std::vector<vec3> nvs;

        for(vec3& v : vertices) {
            nvs.push_back((v - center) + vec3(0.0f, 0.0f, 2.0f));
            nvs.push_back((v - center) - vec3(0.0f, 0.0f, 2.0f));
        }

        vertices = nvs;

        //
        
        Convex_collider cc;
        cc.position = center;
        cc.orientation = identity<mat3>();

        Collision_shape cs;
        cs.vertices = vertices;
        cs.radius = 0.0f;
        
        cc.collision_shape = std::make_shared<Collision_shape>(cs);
        c.collision_shapes[i] = cc;
    }



    //

    Physics_system::initialize_collider(c, densities);
    c.create_BVH();

    Transform t = camera_transform;

    uint32_t entity = ecs.insert_entity();
    ecs.insert_component(entity, t);
    ecs.insert_component(entity, c);

    create_mesh_from_collider(entity, vec3(1.0f, 0.26f, 0.43f));
    */
}
/* alter logo
std::vector<vec3> vs = {
    vec3(0.0f, 0.0f, 0.0f),
    vec3(64.0f, 0.0f, 0.0f),
    vec3(6.0f, 12.0f, 0.0f),
    vec3(58.0f, 12.0f, 0.0f),
    
    // 

    vec3(8.0f, 16.0f, 0.0f),
    vec3(22.0f, 16.0f, 0.0f),
    vec3(24.0f, 48.0f, 0.0f),
    vec3(32.0f, 48.0f, 0.0f),
    vec3(32.0f, 36.0f, 0.0f),

    //

    vec3(42.0f, 16.0f, 0.0f),
    vec3(56.0f, 16.0f, 0.0f),
    vec3(32.0f, 48.0f, 0.0f),
    vec3(32.0f, 36.0f, 0.0f),
    vec3(40.0f, 48.0f, 0.0f),
};

std::vector<int> ranges = {
    0, 4,
    4, 9,
    9, 14
};

for(vec3& v : vs) v /= 4.0f;

//

Collider c;
c.collision_shapes.resize(3);
c.allow_rotation = true;

std::vector<float> densities;

for(int i = 0; i < 3; ++i) {
    densities.push_back(0.8f);

    std::vector<vec3> vertices(vs.begin() + ranges[i * 2], vs.begin() + ranges[i * 2 + 1]);

    vec3 center = vec3(0.0f);
    for(vec3 v : vertices) center += v;
    center /= (float)vertices.size();

    std::vector<vec3> nvs;

    for(vec3& v : vertices) {
        nvs.push_back((v - center) + vec3(0.0f, 0.0f, 2.0f));
        nvs.push_back((v - center) - vec3(0.0f, 0.0f, 2.0f));
    }

    vertices = nvs;

    //
    
    Convex_collider cc;
    cc.position = center;
    cc.orientation = identity<mat3>();

    Collision_shape cs;
    cs.vertices = vertices;
    cs.radius = 0.0f;
    
    cc.collision_shape = std::make_shared<Collision_shape>(cs);
    c.collision_shapes[i] = cc;
}



//

Physics_system::initialize_collider(c, densities);
c.create_BVH();

Transform t = camera_transform;

uint32_t entity = ecs.insert_entity();
ecs.insert_component(entity, t);
ecs.insert_component(entity, c);

create_mesh_from_collider(entity, vec3(1.0f, 0.26f, 0.43f));
*/

void summon_character() {
    static bool b = false;
    Input_system& input_system = ecs.get_system<Input_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);

    uint32_t entity = ecs.insert_entity();

    Collider cl2;
    Collision_shape shape2;
    shape2.mass = 0.58f;
    shape2.vertices = {vec3(0, 0, -1.25f), vec3(0, 0, 1.25f)};
    shape2.radius = 0.5f;
    cl2.allow_rotation = false;//true;
    cl2.allow_gravity = true;
    cl2.collision_shapes.resize(1);
    cl2.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(shape2);
    Physics_system::initialize_collider(cl2);

    Billboard_animation bb;

    bb.texture = core.textures["averie"];

    int start = bb.texture->size.x / 48 - 8;
    int end = bb.texture->size.x / 48;

    int r = core.random.next() % (end - start) + start;

    vec2 size = vec2(4.0f, 4.0f);

    uint32_t i = 64;
    
    
    std::vector<Animation_frame> anim0 = {
        Animation_frame({{0, 0, i, i}}, 0.2f),
    };

    for(int i = 1; i < 8; ++i) {
        anim0[0].regions.push_back(anim0[0].regions[0] + ivec4(64 * i, 0, 0, 0));
    }

    std::vector<Animation_frame> anim1 = {
        Animation_frame({{0, i, i, i}}, 0.275f),
        Animation_frame({{0, i * 2, i, i}}, 0.275f),
        Animation_frame({{0, i * 3, i, i}}, 0.275f),
        Animation_frame({{0, i * 4, i, i}}, 0.275f),
    };

    for(int j = 1; j < 8; ++j) {
        for(Animation_frame& f : anim1) {
            f.regions.push_back(f.regions[0] + ivec4(i * j, 0, 0, 0));
        }
    }

    if(!b && false) {
        bb.texture = core.textures["shooter"];
        anim0 = {Animation_frame({{0, 128, 64, 64}}, 0.5f), Animation_frame({{0, 64, 64, 64}}, 0.15f), Animation_frame({{0, 0, 64, 64}}, 0.15f)};
        size = vec2(4.0f, 4.0f);
    }

    bb.animations = {{anim1, 0}, {anim0, 0}};
    bb.current_animation = 0;
    bb.current_time = 0;
    bb.size = size;
    bb.height = -1.75f;

    Transform tf = camera_transform;
    tf.orientation = rotate_to(vec3(0.0f, 0.0f, 1.0f), get_gravity(tf.position));
    //tf.orientation = new_ori;
    //tf.position += tf.orientation * vec3(0, 0, 2.25f);

    ecs.insert_component(entity, tf);
    ecs.insert_component(entity, cl2);
    ecs.insert_component(entity, bb);
    b = true;
}

void create_crates() {
    Input_system& input_system = ecs.get_system<Input_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);

    float scale = 1.0f;

    ivec3 num_cubes = input_system.num_cubes;
    vec3 separation = vec3(scale * 0.25f);
    vec3 size = vec3(scale) * vec3(1.0f);

    std::vector<vec3> cube_verts = {
        vec3(-1, -1, -1),
        vec3(1, -1, -1),
        vec3(-1, 1, -1),
        vec3(1, 1, -1),
        vec3(-1, -1, 1),
        vec3(1, -1, 1),
        vec3(-1, 1, 1),
        vec3(1, 1, 1),
    };

    Transform t;
    t.position = vec3(0.0f);

    t.orientation = rotate_to(vec3(0, 0, 1), get_gravity(vec3(camera_transform.position)));

    // create collider

    for(int x = 0; x < num_cubes.x; ++x) {
        for(int y = 0; y < num_cubes.y; ++y) {
            for(int z = 0; z < num_cubes.z; ++z) {
                Transform& camera_tf = ecs.get_component<Transform>(camera);
                Collider cl;
                Collision_shape shape;

                vec3 a = vec3(vec3(num_cubes) * -0.5f * (separation + size) + (vec3(x, y, z) + 0.5f) * (separation + size));
                pvec3 position = camera_tf.position + pvec3(camera_transform.orientation * a);

                shape.mass = 0.25f * size.x * size.y * size.z;

                bool is_ellipsoid = false;//core.random() < 0.0f;

                if(is_ellipsoid) {
                    shape.vertices = {vec3(0.0f)};
                    shape.split_radius = size * 0.5f;
                } else {
                    shape.vertices = cube_verts;
                    for(vec3& v : shape.vertices) v *= size * 0.5f;
                    shape.split_radius = vec3(0.0f);
                }
                //shape.vertices = {vec3(0.0)};
                //shape.radius = size.x * 0.5f;

                cl.allow_rotation = true;
                cl.allow_gravity = true;
                cl.is_static = false;
                cl.collision_shapes.clear();
                cl.collision_shapes.resize(1);
                cl.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(shape);
                vec3 offset = Physics_system::initialize_collider(cl, {0.5f});
                Physics_system::create_bounding_box(cl);

                Transform tt = t;

                tt.position = position;
                tt.orientation = camera_transform.orientation;

                uint32_t entity = ecs.insert_entity();
                ecs.insert_component(entity, tt);
                ecs.insert_component(entity, cl);
                
                float saturation = 0.75f;
                vec3 color = get_color(abs(core.random())) * 0.7f + 0.3f;
                //color *= light;
                //create_sphere_mesh(entity, vec3(size.x * 0.5f), color);
                if(is_ellipsoid) {
                    create_sphere_mesh(entity, size * 0.5f, color);
                } else {
                    color = vec3(1.0f);

                    //create_cube_mesh(entity, size * 0.5f, vec3(1.0f), {{16, 48, 8, 8}, {16, 48, 8, 8}, {16, 48, 8, 8}, {16, 48, 8, 8}, {16, 48, 8, 8}, {16, 48, 8, 8}}, core.textures["tilesheet"]);
                    create_cube_mesh(entity, size * 0.5f, vec3(1.0f), {{160, 0, 16, 16}, {160, 0, 16, 16}, {160, 0, 16, 16}, {160, 0, 16, 16}, {160, 0, 16, 16}, {160, 0, 16, 16}}, core.textures["tilesheet"]);
                    //create_cube_mesh(entity, size * 0.5f, vec3(1.0f), {{160, 16, 16, 32}, {160, 16, 16, 32}, {160, 16, 16, 32}, {160, 16, 16, 32}, {160, 0, 16, 16}, {160, 0, 16, 16}}, core.textures["tilesheet"]);
                    //create_cube_mesh(entity, size * 0.5f, vec3(1.0f), {{176, 0, 32, 16}, {176, 0, 32, 16}, {176, 0, 32, 16}, {176, 0, 32, 16}, {176, 16, 32, 32}, {176, 16, 32, 32}}, core.textures["tilesheet"]);
                }
            }
        }
    }
}

void summon_statue() {
    Input_system& input_system = ecs.get_system<Input_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);

    vec3 size = vec3(1.5f, 1.5f, 1.0f);

    std::vector<vec3> cube_verts = {
        vec3(-1, -1, -1),
        vec3(1, -1, -1),
        vec3(-1, 1, -1),
        vec3(1, 1, -1),
        vec3(-1, -1, 1),
        vec3(1, -1, 1),
        vec3(-1, 1, 1),
        vec3(1, 1, 1),
    };

    // create collider

    Transform& camera_tf = ecs.get_component<Transform>(camera);
    Collider cl;
    Collision_shape shape;
    
    Transform t = camera_tf;

    pvec3 position = camera_tf.position;

    shape.mass = 3.0f * size.x * size.y * size.z;

    shape.vertices = cube_verts;
    for(vec3& v : shape.vertices) v *= size * 0.5f;
    shape.split_radius = vec3(0.0f);

    cl.allow_rotation = true;
    cl.allow_gravity = true;
    cl.is_static = false;
    cl.collision_shapes.clear();
    cl.collision_shapes.resize(1);
    cl.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(shape);
    vec3 offset = Physics_system::initialize_collider(cl, {2.75});

    Transform tt = t;

    tt.position = position;
    tt.orientation = camera_transform.orientation;

    uint32_t entity = ecs.insert_entity();
    ecs.insert_component(entity, tt);
    ecs.insert_component(entity, cl);
    
    create_cube_mesh(entity, size * 0.5f, vec3(1.0f), {{0, 80, 24, 16}, {0, 80, 24, 16}, {0, 80, 24, 16}, {0, 64, 24, 16}, {24, 72, 24, 24}, {24, 72, 24, 24}}, core.textures["statue"]);

    Mesh_component& m = ecs.get_component<Mesh_component>(entity);

    uint32_t base = entity;

    // character

    entity = ecs.insert_entity();

    Collider cl2;
    Collision_shape shape2;
    shape2.mass = 2.5f;
    shape2.vertices = {vec3(0, 0, -1.25f), vec3(0, 0, 1.25f)};
    shape2.radius = 0.5f;
    cl2.allow_rotation = true;
    cl2.allow_gravity = true;
    cl2.collision_shapes.resize(1);
    cl2.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(shape2);
    Physics_system::initialize_collider(cl2, {2.75f});

    Billboard_animation bb;

    bb.texture = core.textures["statue"];
    
    std::vector<Animation_frame> anim0 = {
        Animation_frame({{0, 0, 64, 64}}, 0.2f),
    };

    bb.animations = {{anim0, 0}};
    bb.current_animation = 0;
    bb.current_time = 0;
    bb.size = vec2(4, 4);
    bb.height = -1.75f;

    Transform tf = camera_transform;
    tf.position += tf.orientation * vec3(0.0f, 0.0f, 2.25f);

    ecs.insert_component(entity, tf);
    ecs.insert_component(entity, cl2);
    ecs.insert_component(entity, bb);

    Constraint c;
    c.a = entity;
    c.b = base;

    pos_constraint pc;
    pc.a = {0.0f, 0.0f, -1.75f};
    pc.b = {0.0f, 0.0f, 0.5f};
    pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};

    c.pos = {pc};

    rot_constraint rc;
    rc.a = vec3(0.0f, 0.0f, 1.0f);
    rc.b = vec3(0.0f, 0.0f, 1.0f);
    rc.vs = {vec3(1, 0, 0), vec3(0, 1, 0)};
    rc.c = base;

    c.rot = {rc};

    Physics_system& ps = ecs.get_system<Physics_system>();
    ps.constraints.push_back(c);
}

struct hash_vec3 {
    std::size_t operator()(vec3 v) const {
        return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 6) ^ (std::hash<float>()(v.z) << 12);
    }
};

void create_platform() {
    Input_system& input_system = ecs.get_system<Input_system>();

    auto create_chain = [&](pvec3 pos, vec3 down, uint32_t num_links, vec2 size, float sep, vec3 color) {
        Physics_system& ps = ecs.get_system<Physics_system>();
        std::unordered_set<uint32_t> collision_mask;

        Transform t;
        t.position = pos;
        vec3 up = -down;
        t.orientation = rotate_to(vec3(0.0, 0.0, 1.0), up);

        Collider c;
        Collision_shape cs;
        cs.vertices = {vec3(0.0f, 0.0f, -(size.y - size.x) * 0.5f), vec3(0.0f, 0.0f, (size.y - size.x) * 0.5f)};
        cs.radius = size.x * 0.5f;
        cs.mass = 0.4f * size.x * size.x * size.y;
        c.collision_shapes.resize(1);
        c.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(cs);
        c.allow_rotation = true;

        vec3 shift = Physics_system::initialize_collider(c);

        t.position += t.orientation * shift;
        t.position += t.orientation * vec3(0.0f, 0.0f, size.y * 0.5f);

        Mesh_component m;
        create_mesh(m, cs.vertices, vec3(cs.radius));
        m.mesh->color = color;

        uint32_t prev_entity = NULL_ENTITY;
        uint32_t first_entity;

        for(int i = 0; i < num_links; ++i) {    
            uint32_t capsule = ecs.insert_entity();
            collision_mask.emplace(capsule);

            ecs.insert_component(capsule, m);
            ecs.insert_component(capsule, t);
            ecs.insert_component(capsule, c);
            
            DOF_constraint dof;
            dof.a = capsule;
            dof.b = capsule;
            dof.locked_axis = vec3(0.0f, 0.0f, 1.0f);
            dof.factor = 20000.0f;

            ps.dof_constraints.push_back(dof);
            
            if(prev_entity != NULL_ENTITY) {
                Constraint constraint;
                constraint.a = prev_entity;
                constraint.b = capsule;

                pos_constraint pc;
                pc.a = vec3(0, 0, (size.y + sep) * 0.5f);
                pc.b = vec3(0, 0, -(size.y + sep) * 0.5f);
                pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
                constraint.pos.push_back(pc);

                //pc.compliance = 0.001;

                ps.constraints.push_back(constraint);
            } else first_entity = capsule;

            t.position += t.orientation * vec3(0, 0, (size.y + sep));

            prev_entity = capsule;
        }

        for(uint32_t link : collision_mask) {
            Collider& c = ecs.get_component<Collider>(link);
            //c.collision_mask = collision_mask;
        }

        // anchor
        Constraint constraint;
        constraint.a = first_entity;

        pos_constraint pc;
        pc.a = vec3(0, 0, -(size.y + sep) * 0.5f);
        pc.b = pos;
        pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
        constraint.pos.push_back(pc);

        //pc.compliance = 0.001;

        ps.constraints.push_back(constraint);

        return prev_entity;
    };


    auto create_base = [&](pvec3 position, mat3 orientation, vec3 size, vec3 color) {
        Transform tf;
        tf.position = position;
        tf.orientation = orientation;

        uint32_t entity = ecs.insert_entity();

        Collider collider;
        Collision_shape shape;

        shape.vertices = {
            vec3(-1, -1, -1),
            vec3(1, -1, -1),
            vec3(-1, 1, -1),
            vec3(1, 1, -1),
            vec3(-1, -1, 1),
            vec3(1, -1, 1),
            vec3(-1, 1, 1),
            vec3(1, 1, 1),
        };
        for(vec3& v : shape.vertices) v *= size * 0.5f;
        shape.split_radius = vec3(0.0f);
        shape.mass = 0.4f * size.x * size.y * size.z;

        collider.allow_rotation = true;
        //collider.allow_gravity = true;
        collider.collision_shapes.clear();
        collider.collision_shapes.resize(1);
        collider.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(shape);
        Physics_system::initialize_collider(collider);

        ecs.insert_component(entity, tf);
        ecs.insert_component(entity, collider);

        create_cube_mesh(entity, size * 0.5f, color, {0, 96, 32, 32});

        return entity;
    };

    Physics_system& ps = ecs.get_system<Physics_system>();

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);

    vec3 down = get_gravity(camera_transform.position);

    mat3 ori = rotate_to(vec3(0.0f, 0.0f, 1.0f), down);
    float sep = 32.0f;
    vec2 link_size = vec2(1.0f, 4.0f);
    float link_sep = 0.05f;

    uint32_t base = create_base(camera_transform.position + pvec3(ori * vec3(0.0f, 0.0f, -(link_size.y + link_sep) * 8.0f - 0.5f)), ori, vec3(sep + link_size.x * 2.0f, sep + link_size.x * 2.0f, 1.0f), vec3(1.0f, 0.35f, 0.35f));

    uint32_t e0 = create_chain(camera_transform.position + pvec3(ori * vec3(sep * 0.5f, sep * 0.5f, 0.0f)), down, 8, link_size, link_sep, vec3(1.0f, 0.35f, 0.35f));
    uint32_t e1 = create_chain(camera_transform.position + pvec3(ori * vec3(-sep * 0.5f, sep * 0.5f, 0.0f)), down, 8, link_size, link_sep, vec3(1.0f, 0.35f, 0.35f));
    uint32_t e2 = create_chain(camera_transform.position + pvec3(ori * vec3(sep * 0.5f, -sep * 0.5f, 0.0f)), down, 8, link_size, link_sep, vec3(1.0f, 0.35f, 0.35f));
    uint32_t e3 = create_chain(camera_transform.position + pvec3(ori * vec3(-sep * 0.5f, -sep * 0.5f, 0.0f)), down, 8, link_size, link_sep, vec3(1.0f, 0.35f, 0.35f));

    Constraint constraint;
    constraint.a = e0;
    constraint.b = base;
    pos_constraint pc;
    pc.a = vec3(0, 0, (link_size.y + link_sep) * 0.5f);
    pc.b = vec3(sep * 0.5f + link_size.x * 0.5f, sep * 0.5f + link_size.x * 0.5f, 0.5f);
    pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
    constraint.pos.push_back(pc);
    //pc.compliance = 0.001;
    ps.constraints.push_back(constraint);

    constraint.pos.clear();
    constraint.a = e1;
    constraint.b = base;
    pc.a = vec3(0, 0, (link_size.y + link_sep) * 0.5f);
    pc.b = vec3(-(sep * 0.5f + link_size.x * 0.5f), sep * 0.5f + link_size.x * 0.5f, 0.5f);
    pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
    constraint.pos.push_back(pc);
    //pc.compliance = 0.001;
    ps.constraints.push_back(constraint);

    constraint.pos.clear();
    constraint.a = e2;
    constraint.b = base;
    pc.a = vec3(0, 0, (link_size.y + link_sep) * 0.5f);
    pc.b = vec3(sep * 0.5f + link_size.x * 0.5f, -(sep * 0.5f + link_size.x * 0.5f), 0.5f);
    pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
    constraint.pos.push_back(pc);
    //pc.compliance = 0.001;
    ps.constraints.push_back(constraint);

    constraint.pos.clear();
    constraint.a = e3;
    constraint.b = base;
    pc.a = vec3(0, 0, (link_size.y + link_sep) * 0.5f);
    pc.b = vec3(-(sep * 0.5f + link_size.x * 0.5f), -(sep * 0.5f + link_size.x * 0.5f), 0.5f);
    pc.vs = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
    constraint.pos.push_back(pc);
    //pc.compliance = 0.001;
    ps.constraints.push_back(constraint);
}