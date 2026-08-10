#include <window.hpp>
#include <math.hpp>
#include <ecs.hpp>
#include <render.hpp>
#include <ui.hpp>
#include <platform.hpp>
#include <utilities.hpp>
#include <scene.hpp>
#include <physics-2d.hpp>
#include <physics-3d.hpp>

#include <nlohmann/json.hpp>

#include <test/chat.hpp>

#include <iostream>

struct main_system : axiom::system {
    axiom::window* win;
    axiom::vertices vertices;

    std::unordered_map<std::string, axiom::texture_asset> texture_assets;
    std::unordered_map<std::string, axiom::text_asset> text_assets;
    std::unordered_map<std::string, axiom::texture> textures;
    std::unordered_map<std::string, axiom::shader> shaders;

    std::vector<axiom::render_target> targets;

    uint frames = 0;
    float param = 0.0f;

    main_system(axiom::window* win_) {
        win = win_;

        axiom::text_asset vert = axiom::text_asset::load("resources/shaders/ui.vert");
        axiom::text_asset frag = axiom::text_asset::load("resources/shaders/ui.frag");
        axiom::texture_asset texasset = axiom::texture_asset::load("resources/textures/ui.png");
        
        shaders.emplace("ui", std::move(axiom::shader(vert, frag)));
        textures.emplace("ui", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));

        vertices.init();
    }

    void call() {
        // input

        if(win->pressed_buttons.contains(axiom::input_code::KEY_F5)) {
            axiom::physics_system3d& physics_system3d = axiom::global_core.ecs->get_system<axiom::physics_system3d>();

            physics_system3d.sim_active = !physics_system3d.sim_active;
        }

        ++frames;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if(win->pressed_buttons.contains(axiom::input_code::KEY_F6)) { // screenshot
            ivec2 size = win->viewport_size;

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
            asset.save("output/screenshot" + std::to_string(timestamp) + ".png");
        }

        if(win->pressed_buttons.contains(axiom::input_code::KEY_F11)) { // fullscreen
            if(win->is_fullscreen()) win->make_windowed();
            else {
                win->make_fullscreen();
            }
        }

        glViewport(0, 0, win->viewport_size.x, win->viewport_size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //
        
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glClearDepth(0.0f);
        glDepthRange(0, 1);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glDisable(GL_DEPTH_CLAMP);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

        shaders["ui"].use();
        textures["font_axiom_default"].bind(0);
        textures["ui"].bind(1);

        axiom::ui_system& ui_system = ecs->get_system<axiom::ui_system>();
        for(int i = 0; i < ui_system.target_textures.size(); ++i) {
            ui_system.target_textures[i]->bind(i + 2);
        }
        
        /*
        struct ui_vertex {
            vec3 pos;
            vec2 tex_pos;
            vec4 color = vec4(1.0f);
            vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
            uint data = 0;
        };
        */

        mat3 view_matrix = glm::translate(glm::identity<mat3>(), vec2(-1.0f, -1.0f)) * glm::scale(glm::identity<mat3>(), vec2(2.0f / win->viewport_size.x, 2.0f / win->viewport_size.y));
        mat3 trans_matrix = glm::identity<mat3>();

        vertices.vertex_buffer_data(ui_system.vertices.data(), ui_system.vertices.size(), sizeof(axiom::ui_vertex), GL_STREAM_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::ui_vertex), 0);
        vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 5);
        vertices.add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 9);
        vertices.add_vertex_attribute(4, 1, GL_UNSIGNED_INT, false, sizeof(axiom::ui_vertex), sizeof(float) * 13);

        vertices.bind();

        glUniformMatrix3fv(0, 1, false, &view_matrix[0][0]);
        glUniformMatrix3fv(1, 1, false, &trans_matrix[0][0]);

        vertices.draw_vertices(GL_TRIANGLES);

        glfwSwapBuffers(win->window_handle);
    }
};

void create_ui() {
    auto* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto* csystem = &axiom::global_core.ecs->get_system<chat_system>();
    auto* msystem = &axiom::global_core.ecs->get_system<main_system>();

    std::function<void()> switch_lipsum = [ui_system]() {
        ui_system->position(axiom::position_mode::TOP_LEFT);

        ui_system->input_reset();
        ui_system->input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("Lipsum", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
        ui_system->buffer(vec4(0.0f));
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);
        ui_system->buffer(vec4(2.0f));

        axiom::column_widget::insert();

        std::string lipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum neque mi, tincidunt vitae efficitur in, porta eget erat. Nam vitae leo nec ligula imperdiet lacinia. Praesent sed elit vitae diam finibus convallis at a leo. Duis finibus dolor nisl, vitae tristique lectus egestas a. Nullam quam lectus, fringilla a iaculis vel, suscipit sit amet lectus. Nulla rutrum dapibus enim et tincidunt. Suspendisse et lacus ac dui tristique bibendum et a orci. Donec maximus nulla quis scelerisque placerat. Sed sagittis quam est, vestibulum condimentum sem feugiat ac. Cras non est at nisl fringilla interdum. Vestibulum ut neque sagittis, dictum ligula non, gravida mi. Quisque a nunc lorem. Nam libero libero, aliquet eu tincidunt sit amet, sollicitudin suscipit ex. Curabitur lacinia magna augue, vitae laoreet nisl placerat a. Donec convallis nulla sed nulla lacinia, sed tempus orci volutpat. Quisque vitae turpis eu nisl cursus dignissim.

    Aliquam interdum lectus risus, id efficitur ipsum bibendum vitae. Donec nulla ante, pretium in ullamcorper nec, bibendum ac nunc. Vivamus metus nisl, suscipit ac commodo at, viverra at enim. Pellentesque egestas facilisis sagittis. Duis vel sodales augue. Aliquam erat volutpat. Nam vel lectus at dui congue tincidunt. Phasellus placerat aliquet urna eu congue. Quisque turpis mauris, accumsan at tincidunt ut, dapibus sit amet erat. Nullam enim felis, facilisis nec vulputate eu, congue et nunc. Nunc eros turpis, placerat ac sem eget, pulvinar ultrices arcu. Etiam placerat dui eros, eget commodo metus tempus et. Maecenas volutpat lacinia nisi, eu laoreet sapien ultrices nec.)";

        axiom::text_widget::insert(lipsum, axiom::text_alignment::LEFT, true);

        ui_system->input_z(0.0f);
    };

    std::function<void()> switch_render = [msystem, ui_system]() {
        std::function<void(axiom::render_target&)> render_func = [msystem](axiom::render_target& f) {
            static axiom::vertices vertices;
            static double rotation = 0.0f;

            float r = 0.75f;

            vec2 scale = vec2(f.size) / float(glm::min(f.size.x, f.size.y));
            mat4 matrix = glm::scale(vec3(1.0f / scale, 1.0f));
            mat4 rot = glm::rotate((float)rotation, vec3(0.0f, 0.0f, 1.0f));
            matrix = matrix * rot;

            rotation += axiom::global_core.ecs->delta_time * msystem->param;

            struct color_vertex {
                vec3 position;
                vec3 color;
                vec2 tex_coord;
            };

            /*
            std::vector<color_vertex> vs = {
                color_vertex({-sqrt(3.0f) * 0.5f * r, -0.5f * r, 0.5f}, {1.0f, 0.0f, 0.0f}, vec2(-sqrt(3.0f) * 0.5f, -0.5f)),
                color_vertex({0.0f, r, 0.5f}, {0.0f, 1.0f, 0.0f}, vec2(0.0f, 1.0f)),
                color_vertex({sqrt(3.0f) * 0.5f * r, -0.5f * r, 0.5f}, {0.0f, 0.0f, 1.0f}, vec2(sqrt(3.0f) * 0.5f, -0.5f))
            };
            */

            std::vector<color_vertex> vs = {
                color_vertex({-1.0f * r, -1.0f * r, 0.5f}, {1.0f, 0.0f, 0.0f}, vec2(0.0f, 0.0f)),
                color_vertex({1.0f * r, -1.0f * r, 0.5f}, {1.0f, 1.0f, 0.0f}, vec2(1.0f, 0.0f)),
                color_vertex({-1.0f * r, 1.0f * r, 0.5f}, {0.0f, 0.0f, 1.0f}, vec2(0.0f, 1.0f)),
                color_vertex({1.0f * r, 1.0f * r, 0.5f}, {0.0f, 1.0f, 0.0f}, vec2(1.0f, 1.0f)),
            };

            vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

            if(!vertices.initialized) vertices.init();

            vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(color_vertex), GL_STATIC_DRAW);

            vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex), 0);
            vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 3);
            vertices.add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 6);

            msystem->shaders["test"].use();
            msystem->textures["test"].bind(0);
            vertices.bind();

            glUniformMatrix4fv(0, 1, false, &matrix[0][0]);

            vertices.draw_vertices(GL_TRIANGLES);
        };
        
        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0};

        msystem->targets[1] = axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0), formats, attachments);

        //
        ui_system->input_reset();
        ui_system->input_z(0.1f);
        ui_system->buffer(vec4(0.0f));

        vec2 window_size = vec2(384, 384);
        axiom::window_widget::insert("Render", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        axiom::render_widget::insert(&msystem->targets[1], 0);
    };

    std::function<void()> switch_profiler = [msystem, ui_system]() {
        //
        ui_system->input_reset();
        ui_system->input_z(0.1f);
        ui_system->buffer(vec4(0.0f));

        vec2 window_size = vec2(384, 384);
        axiom::window_widget::insert("Profiler", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_purple);
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);

        ui_system->buffer(vec4(4.0f));
        ui_system->position(axiom::position_mode::TOP_LEFT);
        axiom::column_widget::insert();

        axiom::text_widget::insert("", axiom::text_alignment::LEFT, true, 
            [](std::string prev) {
                static double time = 1000.0f;

                time += axiom::global_core.ecs->delta_time;

                if(time > 1.0f) {
                    struct p_entry {
                        std::string name;
                        ulong occurances = 0;
                        ulong avg_time = 0;
                    };

                    ulong avg_time = 0;

                    std::unordered_map<std::string, p_entry> entries;

                    for(auto e : axiom::prof.frames) {
                        avg_time += e.end - e.start;

                        for(auto& entry : e.entries) {
                            if(!entries.contains(entry.name)) {
                                entries.emplace(entry.name, p_entry(entry.name));
                            }

                            p_entry& ee = entries[entry.name];

                            ee.avg_time += entry.end - entry.start;
                            ++ee.occurances;
                        }
                    }
                    

                    std::string str;
                    str += "FRAME\n";
                    str += "avg time: ";
                    str += axiom::to_base(float(avg_time) / axiom::prof.frames.size() / 1000.0f, 10, 3) + " ms" + "\n";

                    str += "\n";

                    for(auto& [k, entry] : entries) {
                        double time = double(entry.avg_time) / entry.occurances / 1000.0f;

                        str += entry.name + ": " + axiom::to_base(float(time), 10, 3) + " ms";

                        str + "\n";
                    }

                    //

                    time = 0.0f;

                    return str;
                } else {
                    return prev;
                }
            }
        );
    };
    
    std::shared_ptr<axiom::menu_node> node(new axiom::menu_node{
        "",
        {
            axiom::menu_node("Debug Windows", {
                axiom::menu_node("Lipsum", {}, switch_lipsum),
                axiom::menu_node("Render", {}, switch_render),
                axiom::menu_node("Profiler", {}, switch_profiler),
            })
        }
    });

    //

    std::function<void(axiom::tab_widget*)> func_chat_in = [ui_system, csystem](axiom::tab_widget* self) {
        ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        ui_system->position(axiom::position_mode::TOP_LEFT);

        axiom::panel_widget::insert();
        
        ui_system->buffer(vec4(0.0f));
        axiom::column_widget::insert();
        axiom::scroll_widget::insert(8.0f, true);
        ui_system->buffer(vec4(0.0f));

        float buffer = 8.0f;

        ui_system->buffer(vec4(4.0f));
        ulong message_root = axiom::column_widget::insert();

        //
        ui_system->input_step(2);
        axiom::spacer_widget::insert(vec2(0.0f), vec2(FLT_MAX), false, vec4(0.0f), true);
        ui_system->buffer(vec4(8.0f));
        ui_system->position(axiom::position_mode::BOTTOM_LEFT);
        axiom::column_widget::insert();

        axiom::text_box_widget::insert(FLT_MAX, vec2(8.0f, 8.0f), ""//, 
            /*[&ui_system, buffer, message_root, self_color](axiom::text_box_widget& self) {
                auto& root = ui_system.widgets[message_root];
                ulong time_delta = 2.0f * 60.0f * 1000000.0f;

                if(self.text[0]->string.size()) {
                    insert_message(message_root, "Averie", self.text[0]->string, axiom::get_timestamp(), self_color);
                    axiom::scroll_widget* parent = dynamic_cast<axiom::scroll_widget*>(ui_system.widgets[root->parent].get());
                    parent->scroll_pos = -FLT_MAX * 0.5f;
                    parent->anchor_widget = 0xFFFFFFFFFFFFFFFD;

                    self.text[0]->string = "";
                }
            }*/
        );
        
        ui_system->set_attrib(vec2(160, 16), vec2(FLT_MAX, 16), vec2(1.0f));

        csystem->enable(message_root);
    };

    
    std::function<void(axiom::tab_widget*)> func_chat_out = [ui_system, csystem](axiom::tab_widget* self) {
        csystem->disable();

        auto children = ui_system->get_children(self->children[0]);
        children.push_back(self->children[0]);
        
        ui_system->erase(children);
        self->children.clear();
    };

    std::function<void(axiom::tab_widget*)> func_settings = [ui_system, node, msystem](axiom::tab_widget* self) {
        ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        axiom::panel_widget::insert();

        ui_system->buffer(vec4(4.0f));
        ui_system->position(axiom::position_mode::TOP_LEFT);

        axiom::column_widget::insert();
        axiom::grid_widget::insert(3);

        //
        
        ui_system->position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("slider", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(FLT_MAX, 0), false);
        axiom::row_widget::insert();
        ui_system->set_attrib(vec2(192, 16), vec2(192, 16), vec2(1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6, axiom::color_blue, vec2(-4.0, 4.0f), 0.25f, 0.0f, "",
            [msystem](axiom::slider_widget& self) {
                if(!self.pressed) self.current_value = msystem->param;
                else msystem->param = self.current_value;
            }
        );
        ui_system->set_attrib(vec2(0, 16), vec2(FLT_MAX, 16), vec2(1.0f));

        axiom::input_box_widget<float>::insert(vec2(40, 16), 0.0f, 
            [msystem](axiom::input_box_widget<float>& self) {
                if(self.update) {
                    msystem->param = self.value;
                } else {
                    self.value = msystem->param;
                }
            }
        );

        ui_system->input_step();
    };

    axiom::screen_widget::insert("axiom text", axiom::color_blue);
    axiom::relative_widget::insert(
        [ui_system](axiom::relative_widget& widget) {
            axiom::screen_widget* parent = (axiom::screen_widget*)ui_system->widgets[widget.parent].get();
            widget.position = vec2(parent->header, parent->size.y - parent->header);

            for(auto child : widget.children) {
                auto& child_widget = ui_system->widgets[child];

                child_widget->position = widget.position;
                child_widget->size = vec2(parent->size.x - parent->header, parent->header);
            }
        }
    );

    ui_system->position(axiom::position_mode::CENTER_LEFT);
    axiom::row_widget::insert();

    axiom::button_widget::insert(vec2(40.0f, 16.0f), axiom::color_blue, "TEST", 
        [node, ui_system](axiom::button_widget& self) {
            if(self.pressed) {
                ui_system->position(axiom::position_mode::TOP_LEFT);

                ui_system->input_reset();
                axiom::menu_widget::insert(self.position, 0.001, axiom::color_blue, 200, 16, FLT_MAX, node, {});
            }
        }
    );
    
    ui_system->input_root(1);
    ui_system->position(axiom::position_mode::TOP_LEFT);

    axiom::split_widget::insert(axiom::layout_mode::ROW, {{1.0f, axiom::panel_mode::SCALE}, {1.0f, axiom::panel_mode::SCALE}});
    axiom::panel_widget::insert();
    
    ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
    axiom::tab_widget::insert(24.0f, 2.0f, {
        axiom::tab("Settings", 80.0f, axiom::color_blue, func_settings),
        axiom::tab("Chat", 80.0f, axiom::color_blue, func_chat_in, func_chat_out),
    });

    //
    //
    //

    ui_system->input_root(2);
    
    axiom::panel_widget::insert();
    
    //
    
    axiom::text_asset vert_asset = axiom::text_asset::load("resources/shaders/test.vert");
    axiom::text_asset frag_asset = axiom::text_asset::load("resources/shaders/test.frag");
    msystem->shaders.emplace("test", std::move(axiom::shader(vert_asset, frag_asset)));
    
    axiom::texture_asset tex_asset = axiom::texture_asset::load("resources/textures/test.png");
    msystem->textures.emplace("test", std::move(axiom::texture(tex_asset, axiom::texture_format::RGBA8)));

    axiom::text_asset grid_vert = axiom::text_asset::load("resources/shaders/grid.vert");
    axiom::text_asset grid_frag = axiom::text_asset::load("resources/shaders/grid.frag");
    msystem->shaders.emplace("grid", std::move(axiom::shader(grid_vert, grid_frag)));
    
    grid_vert = axiom::text_asset::load("resources/shaders/grid3d.vert");
    grid_frag = axiom::text_asset::load("resources/shaders/grid3d.frag");
    msystem->shaders.emplace("grid3d", std::move(axiom::shader(grid_vert, grid_frag)));
    
    axiom::text_asset color_vert = axiom::text_asset::load("resources/shaders/color.vert");
    axiom::text_asset color_frag = axiom::text_asset::load("resources/shaders/color.frag");
    msystem->shaders.emplace("color", std::move(axiom::shader(color_vert, color_frag)));
    
    color_vert = axiom::text_asset::load("resources/shaders/color3d.vert");
    color_frag = axiom::text_asset::load("resources/shaders/color3d.frag");
    msystem->shaders.emplace("color3d", std::move(axiom::shader(color_vert, color_frag)));
    
    ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 0.0f));
    msystem->targets.push_back(axiom::render_target());
    msystem->targets.push_back(axiom::render_target());

    //

    std::function<void(axiom::render_widget*, axiom::render_target*)> callback_func = [msystem, ui_system](axiom::render_widget* self, axiom::render_target* target) {
        static bool movement_capture = false;
        static bool raycast_capture = false;
        static float movement_speed = 1.0f;

        static uint constraint_index = 0xFFFFFFFF;
        static float constraint_dist = 0.0f;
        
        uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();
        axiom::transform3d& camera_transform = axiom::global_core.ecs->get_component<axiom::transform3d>(camera);

        if(ui_system->click_capture == self->self) {
            if(ui_system->window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
                if(raycast_capture == false) {
                    raycast_capture = true;
                    
                    uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();
                    axiom::transform3d& camera_transform = axiom::global_core.ecs->get_component<axiom::transform3d>(camera);
                    axiom::camera3d& camera_cam = axiom::global_core.ecs->get_component<axiom::camera3d>(camera);

                    //

                    vec2 screen_pos = (ui_system->window->cursor_pos - self->position) / self->size;
                    screen_pos = screen_pos * 2.0f - 1.0f;

                    vec4 vertex = vec4(screen_pos, 0.5f, 1.0f);

                    mat4 proj = axiom::get_proj(camera_cam);
                    mat4 inv_proj = glm::inverse(proj);

                    vertex = inv_proj * vertex;
                    vertex /= vertex.w;

                    vec3 dir = glm::normalize(camera_transform.orientation * vertex.xyz());

                    //

                    auto& psystem = axiom::global_core.ecs->get_system<axiom::physics_system3d>();

                    uint hit;
                    uint shape_hit;
                    vec3 normal;
                    vec3 point;
                    std::unordered_set<uint> mask;

                    psystem.raycast(camera_transform.position, dir, 1.0f, 20.0f, 0.0f, mask, &hit, &shape_hit, &normal, &point);

                    if(hit != axiom::NULL_ENTITY) {
                        axiom::position_constraint pc;
                        pc.vs = {vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)};
                        pc.a = hit;

                        axiom::transform3d& target_transform = axiom::global_core.ecs->get_component<axiom::transform3d>(hit);
                        pc.va = transpose(target_transform.orientation) * (point - target_transform.position);
                        pc.vb = point;

                        constraint_dist = length(point - camera_transform.position);

                        constraint_index = psystem.constraints.size();
                        psystem.constraints.push_back(std::make_unique<axiom::position_constraint>(pc));
                    }
                }
            } else {
                if(movement_capture == false && raycast_capture == false) {
                    ui_system->hide_cursor();
                    movement_capture = true;
                }
            }
        } else {
            if(raycast_capture && constraint_index != 0xFFFFFFFF) {
                auto& psystem = axiom::global_core.ecs->get_system<axiom::physics_system3d>();
                psystem.constraints.erase(psystem.constraints.begin() + constraint_index);

                constraint_index = 0xFFFFFFFF;
            }

            ui_system->show_cursor();
            movement_capture = false;
            raycast_capture = false;
        }

        if(ui_system->hover_capture == self->self) {
            if(ui_system->window->scroll_delta != 0.0f) {
                movement_speed *= pow(2, ui_system->window->scroll_delta * 0.5f);
            }
        }

        if(ui_system->click_capture == self->self) {
            if(movement_capture) {
                glm::vec3 raw_movement = {0, 0, 0};
                float rotate_value = 0.0f;

                vec3 rotate = vec3(0.0f);
                rotate.x = -ui_system->window->cursor_delta.x;
                rotate.y = -ui_system->window->cursor_delta.y;

                if(ui_system->window->input_map[axiom::input_code::KEY_Q]) {
                    rotate.z -= 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_E]) {
                    rotate.z += 1;
                }

                //

                if(ui_system->window->input_map[axiom::input_code::KEY_A]) {
                    raw_movement.x -= 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_D]) {
                    raw_movement.x += 1;
                } 
                if(ui_system->window->input_map[axiom::input_code::KEY_S]) {
                    raw_movement.z += 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_W]) {
                    raw_movement.z -= 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_SPACE]) {
                    raw_movement.y += 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
                    raw_movement.y -= 1;
                }

                //

                float len = length(raw_movement);
                if(len != 0.0f) raw_movement = glm::normalize(raw_movement);
                
                glm::vec3 translation_vec = camera_transform.orientation * raw_movement;
                
                vec3 dir = -camera_transform.orientation[2];
                vec3 u = camera_transform.orientation[1];
                glm::mat3 rotate_y_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * rotate.y), glm::normalize(glm::cross(u, dir)));
                glm::mat3 rotate_x_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * rotate.x), u);
                glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 128) * rotate.z * (axiom::global_core.ecs->delta_time * 60)), dir);

                camera_transform.orientation = rotate_z_mat * rotate_x_mat * rotate_y_mat * camera_transform.orientation;
                camera_transform.position += translation_vec * (float)axiom::global_core.ecs->delta_time * movement_speed;
            } else if(raycast_capture && constraint_index != 0xFFFFFFFF) {
                uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();
                axiom::transform3d& camera_transform = axiom::global_core.ecs->get_component<axiom::transform3d>(camera);
                axiom::camera3d& camera_cam = axiom::global_core.ecs->get_component<axiom::camera3d>(camera);

                //

                vec2 screen_pos = (ui_system->window->cursor_pos - self->position) / self->size;
                screen_pos = screen_pos * 2.0f - 1.0f;

                vec4 vertex = vec4(screen_pos, 0.5f, 1.0f);

                mat4 proj = axiom::get_proj(camera_cam);
                mat4 inv_proj = glm::inverse(proj);

                vertex = inv_proj * vertex;
                vertex /= vertex.w;

                vec3 dir = glm::normalize(camera_transform.orientation * vertex.xyz());

                vec3 new_point = camera_transform.position + dir * constraint_dist;
                
                //

                auto& psystem = axiom::global_core.ecs->get_system<axiom::physics_system3d>();

                axiom::position_constraint* c = dynamic_cast<axiom::position_constraint*>(psystem.constraints[constraint_index].get());
                
                c->vb = new_point;
            }
        }
            
        /*
        static vec2 cursor_pos = vec2(0.0f);
        static bool capture = false;
        static uint constraint = 0xFFFFFFFF;
        
        axiom::physics_system2d& physics = axiom::global_core.ecs->get_system<axiom::physics_system2d>();
        
        uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();

        axiom::transform2d& camera_transform = axiom::global_core.ecs->get_component<axiom::transform2d>(camera);
        axiom::camera2d& camera_cam = axiom::global_core.ecs->get_component<axiom::camera2d>(camera);
        
        cursor_pos = ui_system->window->cursor_pos;
        cursor_pos = (cursor_pos - (vec2)self->position - (0.5f * (vec2)self->size)) / (0.5f * (vec2)self->size);
        
        mat4 view = axiom::get_view(camera_cam, camera_transform);
        mat4 proj = axiom::get_proj(camera_cam);

        mat4 inv_view = glm::inverse(view);
        mat4 inv_proj = glm::inverse(proj);

        cursor_pos = inv_view * inv_proj * vec4(cursor_pos, 0.0f, 1.0f);
        
        //std::cout << ui_system->text_cursor << "\n";
        if(constraint != 0xFFFFFFFF || ui_system->text_cursor) capture = false;
        if(ui_system->click_capture == self->self) {
            if(ui_system->window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && !ui_system->text_cursor) {
                capture = true;
            }

            //

            if(capture) {
                vec2 delta = ui_system->window->cursor_delta / (0.5f * (vec2)self->size);

                vec2 world_delta = mat4(mat3(inv_view)) * inv_proj * vec4(delta, 0.0f, 1.0f);

                camera_transform.position -= world_delta;
            }

            // update constraint
            if(constraint != 0xFFFFFFFF) {
                axiom::constraint& cc = physics.constraints[constraint];
                cc.pos[0].b = cursor_pos;
            }

            if(ui_system->window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && ui_system->window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
                if(constraint == 0xFFFFFFFF) {
                    for(uint32_t entity : physics.collectors[0].entities) {
                        axiom::transform2d& transform = axiom::global_core.ecs->get_component<axiom::transform2d>(entity);
                        axiom::collider2d& collider = axiom::global_core.ecs->get_component<axiom::collider2d>(entity);

                        vec2 rel_point = glm::transpose(transform.orientation) * (cursor_pos - transform.position);

                        bool collide = false;

                        for(axiom::collision_shape2d& cs : collider.shapes) {
                            vec2 rel_point2 = transpose(cs.orientation) * (rel_point - cs.position);

                            collide |= axiom::physics_system2d::collision_point(cs.vertices, rel_point2);

                            if(collide) break;
                        }

                        if(collide) {
                            constraint = physics.constraints.size();

                            //

                            axiom::constraint cc;
                            cc.a = entity;

                            axiom::pos_constraint pc;
                            pc.a = rel_point;
                            pc.b = cursor_pos;
                            pc.vs = {vec2(1, 0), vec2(0, 1)};
                            pc.is_hold = true;

                            cc.pos.push_back(pc);

                            physics.constraints.push_back(cc);

                            break;
                        }
                    }
                }
            }
        }

        if(!ui_system->window->input_map[axiom::input_code::MOUSE_LEFT]) {
            if(constraint != 0xFFFFFFFF) {
                physics.constraints.erase(physics.constraints.begin() + constraint);
                constraint = 0xFFFFFFFF;
                capture = false;
            }
        }

        if(ui_system->hover_capture == self->self) {
            float zoom_delta = glm::pow(1.25f, ui_system->window->scroll_delta);
            if(zoom_delta != 1.0f) {
                vec2 offset = camera_transform.position - cursor_pos;
                offset /= zoom_delta;

                camera_transform.position = offset + cursor_pos;
                
                camera_cam.zoom *= zoom_delta;
            }
        }
        */
    };

    {
        std::function<void(axiom::render_target&)> render_func = [msystem, ui_system](axiom::render_target& f) {
            vec3 background = axiom::hex_color(0x1E1F2E);
            glClearColor(background.x, background.y, background.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            //

            glEnable(GL_CULL_FACE);
            
            static axiom::vertices vertices;
            if(!vertices.initialized) {
                vertices.init();
            }

            //

            uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();

            axiom::transform3d& camera_transform = axiom::global_core.ecs->get_component<axiom::transform3d>(camera);
            axiom::camera3d& camera_cam = axiom::global_core.ecs->get_component<axiom::camera3d>(camera);

            camera_cam.aspect = vec2(f.size) / (float)glm::min(f.size.x, f.size.y);
            
            mat4 view = axiom::get_view(camera_cam, camera_transform);
            mat4 proj = axiom::get_proj(camera_cam);

            // render shape

            glEnable(GL_DEPTH_TEST);
            
            auto& collector = axiom::global_core.ecs->collectors["color_mesh3d"];

            for(uint entity : collector.entities) {
                axiom::transform3d& transform = axiom::global_core.ecs->get_component<axiom::transform3d>(entity);
                axiom::color_mesh3d& mesh = axiom::global_core.ecs->get_component<axiom::color_mesh3d>(entity);

                mat4 model = axiom::get_model(transform, camera_transform);
                axiom::shader& color_shader = msystem->shaders["color3d"];
                
                vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

                //
                
                color_shader.use();

                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                glUniformMatrix4fv(2, 1, false, &proj[0][0]);
                glUniform3fv(3, 1, &light_dir.x);

                mesh.vertices->draw_vertices(GL_TRIANGLES);
            }

            {
                auto& psystem = axiom::global_core.ecs->get_system<axiom::physics_system3d>();

                /*
                for(uint entity : psystem.collectors[0].entities) {
                    axiom::collider3d& collider = axiom::global_core.ecs->get_component<axiom::collider3d>(entity);
                    axiom::transform3d& transform = axiom::global_core.ecs->get_component<axiom::transform3d>(entity);

                    axiom::bounding_box3d bounding_box = axiom::transform_bounding_box(collider.bounding_box, transform.position, transform.orientation);

                    std::vector<vec3> vs = {
                        vec3(bounding_box.minimum.x, bounding_box.minimum.y, bounding_box.minimum.z),
                        vec3(bounding_box.maximum.x, bounding_box.minimum.y, bounding_box.minimum.z),
                        vec3(bounding_box.minimum.x, bounding_box.maximum.y, bounding_box.minimum.z),
                        vec3(bounding_box.maximum.x, bounding_box.maximum.y, bounding_box.minimum.z),
                        vec3(bounding_box.minimum.x, bounding_box.minimum.y, bounding_box.maximum.z),
                        vec3(bounding_box.maximum.x, bounding_box.minimum.y, bounding_box.maximum.z),
                        vec3(bounding_box.minimum.x, bounding_box.maximum.y, bounding_box.maximum.z),
                        vec3(bounding_box.maximum.x, bounding_box.maximum.y, bounding_box.maximum.z),
                    };

                    vs = {
                        vs[0], vs[4],
                        vs[1], vs[5],
                        vs[2], vs[6],
                        vs[3], vs[7],

                        vs[0], vs[2],
                        vs[1], vs[3],
                        vs[4], vs[6],
                        vs[5], vs[7],
                        
                        vs[0], vs[1],
                        vs[2], vs[3],
                        vs[4], vs[5],
                        vs[6], vs[7],
                    };

                    std::vector<axiom::color_vertex3d> vvs;

                    for(vec3 v : vs) {
                        axiom::color_vertex3d vertex;
                        vertex.position = v - transform.position;
                        vertex.normal = vec3(0.0f, 0.0f, 1.0f);
                        vertex.color = vec3(1.0f);

                        vvs.push_back(vertex);
                    }

                    vertices.vertex_buffer_data(vvs.data(), vvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
                    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
                    vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
                    vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 6);

                    //


                    axiom::transform3d i_transform;
                    i_transform.position = transform.position;
                    i_transform.orientation = glm::identity<mat3>();
                    
                    mat4 model = axiom::get_model(i_transform, camera_transform);
                    axiom::shader& color_shader = msystem->shaders["color3d"];
                    
                    vec3 light_dir = vec3(0.0f, 0.0f, 1.0f);

                    //
                    
                    color_shader.use();

                    glUniformMatrix4fv(0, 1, false, &model[0][0]);
                    glUniformMatrix4fv(1, 1, false, &view[0][0]);
                    glUniformMatrix4fv(2, 1, false, &proj[0][0]);
                    glUniform3fv(3, 1, &light_dir.x);

                    vertices.draw_vertices(GL_LINES);
                }
                */

                /*
                glDisable(GL_DEPTH_TEST);

                {
                    std::vector<vec3> vs = axiom::debug_vertices[0];

                    axiom::transform3d i_transform;
                    i_transform.position = vec3(0.0f);
                    i_transform.orientation = glm::identity<mat3>();

                    std::vector<axiom::color_vertex3d> vvs;

                    for(vec3 v : vs) {
                        axiom::color_vertex3d vertex;
                        vertex.position = v;
                        vertex.normal = vec3(0.0f, 0.0f, 1.0f);
                        vertex.color = vec3(1.0f, 0.0f, 1.0f);

                        vvs.push_back(vertex);
                    }

                    vertices.vertex_buffer_data(vvs.data(), vvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
                    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
                    vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
                    vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 6);

                    //

                    mat4 model = axiom::get_model(i_transform, camera_transform);
                    axiom::shader& color_shader = msystem->shaders["color3d"];
                    
                    vec3 light_dir = vec3(0.0f, 0.0f, 1.0f);

                    //
                    
                    color_shader.use();

                    glUniformMatrix4fv(0, 1, false, &model[0][0]);
                    glUniformMatrix4fv(1, 1, false, &view[0][0]);
                    glUniformMatrix4fv(2, 1, false, &proj[0][0]);
                    glUniform3fv(3, 1, &light_dir.x);

                    glPointSize(3);
                    vertices.draw_vertices(GL_POINTS);
                }
                
                {
                    std::vector<vec3> vs = axiom::debug_vertices[1];

                    axiom::transform3d i_transform;
                    i_transform.position = vec3(0.0f);
                    i_transform.orientation = glm::identity<mat3>();

                    std::vector<axiom::color_vertex3d> vvs;

                    for(vec3 v : vs) {
                        axiom::color_vertex3d vertex;
                        vertex.position = v;
                        vertex.normal = vec3(0.0f, 0.0f, 1.0f);
                        vertex.color = vec3(1.0f, 1.0f, 0.0f);

                        vvs.push_back(vertex);
                    }

                    vertices.vertex_buffer_data(vvs.data(), vvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
                    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
                    vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
                    vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 6);

                    //

                    mat4 model = axiom::get_model(i_transform, camera_transform);
                    axiom::shader& color_shader = msystem->shaders["color3d"];
                    
                    vec3 light_dir = vec3(0.0f, 0.0f, 1.0f);

                    //
                    
                    color_shader.use();

                    glUniformMatrix4fv(0, 1, false, &model[0][0]);
                    glUniformMatrix4fv(1, 1, false, &view[0][0]);
                    glUniformMatrix4fv(2, 1, false, &proj[0][0]);
                    glUniform3fv(3, 1, &light_dir.x);

                    glPointSize(3);
                    vertices.draw_vertices(GL_POINTS);
                }
                */
                
                glEnable(GL_DEPTH_TEST);
            }

            // render grid
            //glDisable(GL_DEPTH_TEST);

            std::vector<vec2> vs = {
                vec2(-1.0f, -1.0f),
                vec2(1.0f, -1.0f),
                vec2(-1.0f, 1.0f),
                vec2(1.0f, 1.0f)
            };

            vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

            vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
            vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

            //

            axiom::transform3d grid_transform;
            grid_transform.position = vec3(0.0f);
            grid_transform.orientation = glm::identity<mat3>();
            
            mat4 model = axiom::get_model(grid_transform, camera_transform);

            axiom::shader& grid_shader = msystem->shaders["grid3d"];

            grid_shader.use();
            vertices.bind();

            glUniformMatrix4fv(0, 1, false, &model[0][0]);
            glUniformMatrix4fv(1, 1, false, &view[0][0]);
            glUniformMatrix4fv(2, 1, false, &proj[0][0]);

            vertices.draw_vertices(GL_TRIANGLES);

            //glDisable(GL_DEPTH_TEST);
        };

        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::DEPTH};

        msystem->targets[0] = axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0, 0), formats, attachments);
    }

    axiom::render_widget::insert(&msystem->targets[0], 0, callback_func);

    ui_system->position(axiom::position_mode::TOP_LEFT);

    ui_system->buffer(vec4(4.0f));
    axiom::column_widget::insert();
    axiom::match_widget::insert(vec4(0.5f, 0.5f, 0.5f, 0.25f), true);

    std::function<std::string(std::string)> fps_func = [msystem](std::string prev) {
        static double time = axiom::get_time();

        double current_time = axiom::get_time();
        if(current_time - time > 1.0) {

            double elapsed = current_time - time;

            double fps = double(msystem->frames) / elapsed;

            time = current_time;
            msystem->frames = 0;

            return "FPS: " + std::to_string(fps);
        }
        return prev;
    };

    axiom::text_widget::insert("FPS: ", axiom::text_alignment::LEFT, true, fps_func);

    std::function<std::string(std::string)> position_func = [msystem](std::string prev) {
        uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();

        axiom::transform3d& transform = axiom::global_core.ecs->get_component<axiom::transform3d>(camera);
        //axiom::camera2d& cam = axiom::global_core.ecs->get_component<axiom::camera2d>(camera);
        
        return "position: " + axiom::to_base(transform.position.x, 16, 3) + " " + axiom::to_base(transform.position.y, 16, 3) + " " + axiom::to_base(transform.position.z, 16, 3);
    };

    axiom::text_widget::insert("position: ", axiom::text_alignment::LEFT, true, position_func);

    std::function<std::string(std::string)> direction_func = [msystem](std::string prev) {
        uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();

        axiom::transform3d& transform = axiom::global_core.ecs->get_component<axiom::transform3d>(camera);
        //axiom::camera2d& cam = axiom::global_core.ecs->get_component<axiom::camera2d>(camera);
        vec3 direction = -transform.orientation[2];
        
        return "direction: " + axiom::to_base(direction.x, 16, 3) + " " + axiom::to_base(direction.y, 16, 3) + " " + axiom::to_base(direction.z, 16, 3);
    };

    axiom::text_widget::insert("direction: ", axiom::text_alignment::LEFT, true, direction_func);
    
    ui_system->input_step();
    ui_system->position(axiom::position_mode::TOP_RIGHT);
    ui_system->buffer(vec4(6.0f));
    axiom::row_widget::insert();

    axiom::button_widget::insert(vec2(30.0f), axiom::color_purple, vec4(0, 116, 12, 12), 
        [ui_system](axiom::button_widget& self) {
            static bool update = true;

            auto* physics = &axiom::global_core.ecs->get_system<axiom::physics_system3d>();
            auto& parent_widget = ui_system->widgets[self.parent];

            if(self.pressed) {
                physics->sim_active = !physics->sim_active;

                update = true;
            }

            if(update) {
                update = false;

                if(physics->sim_active) {
                    auto prev_children = parent_widget->children;
                    parent_widget->children = {self.self};
                    for(ulong child : prev_children) {
                        if(find(parent_widget->children.begin(), parent_widget->children.end(), child) == parent_widget->children.end()) {
                            ui_system->widgets.erase(child);
                        }
                    }

                    self.icon = vec4(0, 116, 12, 12);
                } else {
                    ui_system->input_set(self.parent);
                    ui_system->buffer(vec4(6.0f));

                    ulong step_button = axiom::button_widget::insert(vec2(30.0f), axiom::color_purple, vec4(24, 116, 12, 12), 
                        [physics](axiom::button_widget& self) {
                            if(self.pressed) {
                                physics->physics_loop();
                            }
                        }
                    );

                    parent_widget->children.pop_back();
                    parent_widget->children.insert(parent_widget->children.begin(), step_button);
                    
                    self.icon = vec4(12, 116, 12, 12);
                }
            }
        }
    );

    /*
    std::function<std::string(std::string)> widget_func = [ui_system](std::string prev) {
        return "widgets: " + std::to_string(ui_system->widgets.size());
    };

    axiom::text_widget::insert("widgets: ", axiom::text_alignment::LEFT, true, widget_func);
    */
};

void build_shape2d(vec2 pos, mat2 ori, float mass, std::vector<axiom::vertex_element2d> elements, bool is_static = false) {
    axiom::color_mesh2d mesh;
    axiom::collider2d collider;

    axiom::collision_shape2d shape;
    shape.vertices = elements;
    shape.mass = mass;
    collider.shapes.push_back(shape);

    collider.is_static = is_static;
    axiom::physics_system2d::calculate_inertia(collider);

    //

    std::vector<axiom::color_vertex2d> perimeter_vertices;
    std::vector<axiom::color_vertex2d> area_vertices;

    std::vector<vec2> perimeter;
    std::vector<vec2> area;
    axiom::create_mesh(collider, &perimeter, &area);

    for(vec2 v : perimeter) {
        perimeter_vertices.push_back(axiom::color_vertex2d(v, vec4(1.0f)));
    }
    
    for(vec2 v : area) {
        area_vertices.push_back(axiom::color_vertex2d(v, vec4(1.0f, 1.0f, 1.0f, 0.125f)));
    }

    mesh.v_lines = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_lines->init();
    mesh.v_lines->vertex_buffer_data(perimeter_vertices.data(), perimeter_vertices.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_lines->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_lines->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);
    
    mesh.v_tris = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_tris->init();
    mesh.v_tris->vertex_buffer_data(area_vertices.data(), area_vertices.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_tris->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_tris->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);

    //

    axiom::transform2d tf;
    tf.position = pos;
    tf.orientation = ori;

    uint entity = axiom::global_core.ecs->insert_entity();
    axiom::global_core.ecs->insert_component(entity, tf);
    axiom::global_core.ecs->insert_component(entity, mesh);
    axiom::global_core.ecs->insert_component(entity, collider);
}

void build_shape2d(vec2 pos, mat2 ori, std::vector<vec2> positions, std::vector<mat2> orientations, std::vector<float> masses, std::vector<std::vector<axiom::vertex_element2d>> elements, bool is_static = false) {
    axiom::color_mesh2d mesh;
    axiom::collider2d collider;

    for(int i = 0; i < elements.size(); ++i) {
        axiom::collision_shape2d shape;
        shape.vertices = elements[i];
        shape.mass = masses[i];
        collider.shapes.push_back(shape);
    }
    collider.is_static = is_static;
    axiom::physics_system2d::calculate_inertia(collider);

    //

    std::vector<axiom::color_vertex2d> perimeter_vertices;
    std::vector<axiom::color_vertex2d> area_vertices;

    std::vector<vec2> perimeter;
    std::vector<vec2> area;
    axiom::create_mesh(collider, &perimeter, &area);

    for(vec2 v : perimeter) {
        perimeter_vertices.push_back(axiom::color_vertex2d(v, vec4(1.0f)));
    }
    
    for(vec2 v : area) {
        area_vertices.push_back(axiom::color_vertex2d(v, vec4(1.0f, 1.0f, 1.0f, 0.125f)));
    }

    mesh.v_lines = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_lines->init();
    mesh.v_lines->vertex_buffer_data(perimeter_vertices.data(), perimeter_vertices.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_lines->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_lines->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);
    
    mesh.v_tris = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_tris->init();
    mesh.v_tris->vertex_buffer_data(area_vertices.data(), area_vertices.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_tris->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_tris->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);

    //

    axiom::transform2d tf;
    tf.position = pos;
    tf.orientation = ori;

    uint entity = axiom::global_core.ecs->insert_entity();
    axiom::global_core.ecs->insert_component(entity, tf);
    axiom::global_core.ecs->insert_component(entity, mesh);
    axiom::global_core.ecs->insert_component(entity, collider);
}

int main(int argc, char* argv[]) {
    axiom::window win = axiom::window(ivec2(64, 64), ivec2(512, 512), 6, "axiom test", false);

    //

    axiom::font_asset default_font = axiom::font_asset::load("resources/fonts/axiom_default.bdf");
    axiom::texture font_tex = std::move(axiom::texture(default_font.texture, axiom::texture_format::RGBA8));

    generate_placeholder();

    //

    win.hide_cursor();

    axiom::ecs ecs;
    ecs.make_active();
    
    axiom::ui_system ui_system(&win);
    ui_system.font_assets = {&default_font};

    ecs.register_system(ui_system);
    
    chat_system csystem;
    ecs.register_system(csystem);

    axiom::physics_system2d psystem2d;
    ecs.register_system(psystem2d);
    
    axiom::physics_system3d psystem3d;
    ecs.register_system(psystem3d);

    main_system bsystem(&win);
    bsystem.textures.emplace("font_axiom_default", std::move(font_tex));
    ecs.register_system(bsystem);
    
    axiom::signature sig = axiom::global_core.ecs->update_signature<axiom::transform3d>();
    axiom::global_core.ecs->update_signature<axiom::camera3d>(sig);
    axiom::collector col(sig);
    axiom::global_core.ecs->create_collector("camera", col);
    
    /*
    sig = axiom::global_core.ecs->update_signature<axiom::transform2d>();
    axiom::global_core.ecs->update_signature<axiom::color_mesh2d>(sig);
    col = axiom::collector(sig);
    axiom::global_core.ecs->create_collector("color_mesh", col);
    */
   
    sig = axiom::global_core.ecs->update_signature<axiom::transform3d>();
    axiom::global_core.ecs->update_signature<axiom::color_mesh3d>(sig);
    col = axiom::collector(sig);
    axiom::global_core.ecs->create_collector("color_mesh3d", col);

    //

    uint camera_entity = axiom::global_core.ecs->insert_entity();
    axiom::camera3d cam;
    axiom::transform3d tf;

    cam.fov = 90.0f;

    tf.position = vec3(0.0f, 0.0f, 1.0f);
    tf.orientation = axiom::rotate_to(vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));

    axiom::global_core.ecs->insert_component(camera_entity, cam);
    axiom::global_core.ecs->insert_component(camera_entity, tf);

    //

    auto create_mesh = [](axiom::color_mesh3d& mesh, std::vector<axiom::vertex_element3d>& elements, vec3 color) {
        std::vector<axiom::output_vertex> surface_vertices;
        std::vector<uint> surface_indices;
        axiom::create_mesh(elements, &surface_vertices, &surface_indices);

        std::unordered_map<uint, vec3> normals;

        for(uint i = 0; i < surface_indices.size(); i += 3) {
            uint i0 = surface_indices[i];
            uint i1 = surface_indices[i + 1];
            uint i2 = surface_indices[i + 2];

            axiom::output_vertex v0 = surface_vertices[i0];
            axiom::output_vertex v1 = surface_vertices[i1];
            axiom::output_vertex v2 = surface_vertices[i2];

            vec3 scaled_normal = cross(v0.position - v2.position, v1.position - v2.position);

            if(v0.element != v1.element || v1.element != v2.element || v2.element != v0.element) {
                i0 |= 0x80000000;
                i1 |= 0x80000000;
                i2 |= 0x80000000;
            }
            
            if(!normals.contains(i0)) normals[i0] = vec3(0.0f);
            if(!normals.contains(i1)) normals[i1] = vec3(0.0f);
            if(!normals.contains(i2)) normals[i2] = vec3(0.0f);

            if(!(elements[v0.element].radii.x + elements[v0.element].radii.y == 0.0f || 
            elements[v0.element].radii.y + elements[v0.element].radii.z == 0.0f || 
            elements[v0.element].radii.z + elements[v0.element].radii.x == 0.0f)) {
                normals[i0] += scaled_normal;
            }

            if(!(elements[v1.element].radii.x + elements[v1.element].radii.y == 0.0f || 
            elements[v1.element].radii.y + elements[v1.element].radii.z == 0.0f || 
            elements[v1.element].radii.z + elements[v1.element].radii.x == 0.0f)) {
                normals[i1] += scaled_normal;
            }

            if(!(elements[v2.element].radii.x + elements[v2.element].radii.y == 0.0f || 
            elements[v2.element].radii.y + elements[v2.element].radii.z == 0.0f || 
            elements[v2.element].radii.z + elements[v2.element].radii.x == 0.0f)) {
                normals[i2] += scaled_normal;
            }

            //
        };

        for(auto& [key, normal] : normals) {
            float len = length(normal);
            if(len != 0.0f) normal /= len;
        }

        for(uint i = 0; i < surface_indices.size(); i += 3) {
            uint i0 = surface_indices[i];
            uint i1 = surface_indices[i + 1];
            uint i2 = surface_indices[i + 2];

            axiom::output_vertex v0 = surface_vertices[i0];
            axiom::output_vertex v1 = surface_vertices[i1];
            axiom::output_vertex v2 = surface_vertices[i2];
            
            if(v0.element != v1.element || v0.element != v2.element || v1.element != v2.element) {
                i0 |= 0x80000000;
                i1 |= 0x80000000;
                i2 |= 0x80000000;
            }
            
            axiom::color_vertex3d vertex;
            vertex.color = color;

            vec3 n0 = normals[i0];
            vec3 n1 = normals[i1];
            vec3 n2 = normals[i2];

            bool o0 = length(n0) == 0.0f;
            bool o1 = length(n1) == 0.0f;
            bool o2 = length(n2) == 0.0f;

            if(o0 || o1 || o2) {
                vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));

                if(o0) n0 = normal;
                if(o1) n1 = normal;
                if(o2) n2 = normal;
            }

            vertex.position = v0.position;
            vertex.normal = n0;
            mesh.vs.push_back(vertex);
            
            vertex.position = v1.position;
            vertex.normal = n1;
            mesh.vs.push_back(vertex);
            
            vertex.position = v2.position;
            vertex.normal = n2;
            mesh.vs.push_back(vertex);
        };
        
        mesh.load();
    };

    auto create_collider = [](axiom::collider3d& collider, axiom::transform3d& transform, std::vector<axiom::vertex_element3d>& elements, bool is_static = false) {
        axiom::collision_shape3d shape;
        shape.elements = elements;
        collider.collision_shapes = {shape};

        collider.is_static = is_static;
        collider.allow_rotation = true;

        vec3 offset = axiom::initialize_collider(collider, {1.0f});
        transform.position += transform.orientation * offset;
        
        axiom::create_bounding_box(collider);
        
        //std::cout << collider.bounding_box.minimum << " " << collider.bounding_box.maximum << "\n";
    };

    axiom::random32 rand(0xFF55FF55);

    {   
        ivec3 array = ivec3(8, 8, 8);
        vec3 origin = vec3(0.0f, 0.0f, 6.0f);
        float sep = 0.75f;

        for(int x = 0; x < array.x; ++x) {
            for(int y = 0; y < array.y; ++y) {
                for(int z = 0; z < array.z; ++z) {
                    vec3 pos = vec3(x, y, z) + 0.5f - vec3(array) * 0.5f;
                    pos *= sep;
                    pos += origin;
                    
                    /*
                    axiom::transform3d transform;
                    axiom::color_mesh3d mesh;
                    axiom::collider3d collider;

                    transform.position = pos + origin;
                    transform.orientation = axiom::rotate_to(vec3(0.0f, 0.0f, 1.0f), rand.unit_vector());

                    float w = 0.25f;

                    std::vector<axiom::vertex_element3d> elements = {
                        axiom::vertex_element3d{vec3(-1.0f, -1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, 1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, -1.0f, 1.0f) * w},
                        axiom::vertex_element3d{vec3(-1.0f, 1.0f, 1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) * w},
                    };
                    
                    create_mesh(mesh, elements, axiom::hsv_color(0.0f, 0.75f, 1.0f));
                    create_collider(collider, transform, elements);

                    uint entity = axiom::global_core.ecs->insert_entity();
                    axiom::global_core.ecs->insert_component(entity, transform);
                    axiom::global_core.ecs->insert_component(entity, mesh);
                    axiom::global_core.ecs->insert_component(entity, collider);

                    */

                    axiom::transform3d transform;
                    axiom::color_mesh3d mesh;
                    axiom::collider3d collider;

                    transform.position = pos;
                    transform.orientation = glm::identity<mat3>();//axiom::rotate_to(vec3(0.0f, 0.0f, 1.0f), rand.unit_vector());
                    
                    float w = 0.25f;

                    std::vector<axiom::vertex_element3d> elements = {
                        /*
                        axiom::vertex_element3d{vec3(-1.0f, -1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, 1.0f, -1.0f) * w},
                        axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, -1.0f, 1.0f) * w},
                        axiom::vertex_element3d{vec3(-1.0f, 1.0f, 1.0f) * w},
                        axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) * w},
                        */

                        //axiom::vertex_element3d{vec3(0.0f, 0.0f, 0.5f), vec3(0.0f)},
                        //axiom::vertex_element3d{vec3(0.0f, 0.0f, 0.0f), vec3(0.25f, 0.25f, 0.0f)},
                        
                        axiom::vertex_element3d{vec3(0.0f, 0.0f, 0.0f), vec3(0.25f, 0.25f, 0.0f)},
                        axiom::vertex_element3d{vec3(0.0f, 0.0f, 0.5f), vec3(0.25f, 0.25f, 0.0f)},
                    };
                    
                    create_collider(collider, transform, elements);

                    for(auto& element : elements) element.center += collider.collision_shapes[0].position;
                    create_mesh(mesh, elements, axiom::hsv_color(4.0f, 0.7f, 1.0f)); // axiom::hsv_color(0.85f, 0.75f, 1.0f)
                    // axiom::hsv_color(0.0f, 0.7f, 1.0f) RED
                    // axiom::hsv_color(2.0f, 0.7f, 0.5f) GREEN
                    // axiom::hsv_color(4.0f, 0.7f, 1.0f)  BLUE

                    uint entity = axiom::global_core.ecs->insert_entity();
                    axiom::global_core.ecs->insert_component(entity, transform);
                    axiom::global_core.ecs->insert_component(entity, mesh);
                    axiom::global_core.ecs->insert_component(entity, collider);
                }
            }
        }
    }

    {
        
    }

    {
        axiom::transform3d transform;
        axiom::color_mesh3d mesh;
        axiom::collider3d collider;

        transform.position = vec3(0.0f, 0.0f, 0.0f);
        transform.orientation = glm::identity<mat3>();//axiom::rotate_to(vec3(0.0f, 0.0f, 1.0f), rand.unit_vector());

        vec3 w = vec3(64.0f, 64.0f, 2.0f);

        std::vector<axiom::vertex_element3d> elements = {
            axiom::vertex_element3d{vec3(-1.0f, -1.0f, -1.0f) * w},
            axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) * w},
            axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) * w},
            axiom::vertex_element3d{vec3(1.0f, 1.0f, -1.0f) * w},
            axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) * w},
            axiom::vertex_element3d{vec3(1.0f, -1.0f, 1.0f) * w},
            axiom::vertex_element3d{vec3(-1.0f, 1.0f, 1.0f) * w},
            axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) * w},
        };
        
        create_mesh(mesh, elements, axiom::hsv_color(0.0f, 0.0f, 0.6f));
        create_collider(collider, transform, elements, true);

        uint entity = axiom::global_core.ecs->insert_entity();
        axiom::global_core.ecs->insert_component(entity, transform);
        axiom::global_core.ecs->insert_component(entity, mesh);
        axiom::global_core.ecs->insert_component(entity, collider);
    }

    //

    create_ui();

    while(!win.should_close) {
        axiom::prof.start_frame();

        win.poll_events();

        ecs.do_frame();

        axiom::prof.end_frame();
    }
}

/* 2d code

axiom::signature sig = axiom::global_core.ecs->update_signature<axiom::transform2d>();
axiom::global_core.ecs->update_signature<axiom::camera2d>(sig);
axiom::collector col(sig);
axiom::global_core.ecs->create_collector("camera", col);

sig = axiom::global_core.ecs->update_signature<axiom::transform2d>();
axiom::global_core.ecs->update_signature<axiom::color_mesh2d>(sig);
col = axiom::collector(sig);
axiom::global_core.ecs->create_collector("color_mesh", col);

//
//
//

uint camera_entity = axiom::global_core.ecs->insert_entity();
axiom::camera2d cam;
axiom::transform2d tf;

cam.zoom = 0.125f;

tf.position = vec2(0.5f, 0.25f);
tf.orientation = glm::identity<mat2>(); //glm::rotate(glm::identity<mat3>(), axiom::pi * 0.125f);

axiom::global_core.ecs->insert_component(camera_entity, cam);
axiom::global_core.ecs->insert_component(camera_entity, tf);

//
{
    std::vector<vec2> positions;
    std::vector<mat2> orientations;
    std::vector<float> masses;
    std::vector<std::vector<axiom::vertex_element2d>> elements;

    float inner_radius = 16.0f;
    float outer_radius = 18.0f;
    uint segments = 32;

    for(int i = 0; i < segments; ++i) {
        int a0 = i;
        int a1 = i + 1;

        float angle_0 = axiom::pi * 2.0f * a0 / segments;
        float angle_1 = axiom::pi * 2.0f * a1 / segments;

        vec2 dir_0 = vec2(cos(angle_0), sin(angle_0));
        vec2 dir_1 = vec2(cos(angle_1), sin(angle_1));
        axiom::vertex_element2d va = axiom::vertex_element2d(dir_0 * inner_radius);
        axiom::vertex_element2d vb = axiom::vertex_element2d(dir_1 * inner_radius);
        axiom::vertex_element2d vc = axiom::vertex_element2d(dir_0 * outer_radius);
        axiom::vertex_element2d vd = axiom::vertex_element2d(dir_1 * outer_radius);

        float dist_ab = length(va.center - vb.center);
        vec2 sep_ab = (va.center - vb.center) / dist_ab;
        vec2 origin_ab = (va.center + vb.center) * 0.5f;
        
        float dist_cd = length(vc.center - vd.center);
        vec2 sep_cd = (vc.center - vd.center) / dist_cd;
        vec2 origin_cd = (vc.center + vd.center) * 0.5f;

        axiom::vertex_element2d ve = axiom::vertex_element2d(origin_ab, vec2(dist_ab * 0.5f, dist_ab * 0.25f), mat2(sep_ab, vec2(sep_ab.y, -sep_ab.x)));
        axiom::vertex_element2d vf = axiom::vertex_element2d(origin_cd, vec2(dist_cd * 0.5f, dist_cd * 0.25f), mat2(sep_cd, vec2(sep_cd.y, -sep_cd.x)));

        elements.push_back({ve, vf});
        positions.push_back(vec2(0.0f));
        orientations.push_back(glm::identity<mat2>());
        masses.push_back(5.0f);
    }
    
    //build_shape(vec2(-24.0f, -24.0f), glm::identity<mat2>(), positions, orientations, masses, elements);
    //build_shape(vec2(-24.0f, 24.0f), glm::identity<mat2>(), positions, orientations, masses, elements);
    //build_shape(vec2(24.0f, -24.0f), glm::identity<mat2>(), positions, orientations, masses, elements);
    //build_shape(vec2(24.0f, 24.0f), glm::identity<mat2>(), positions, orientations, masses, elements);
}

{
    std::vector<vec2> positions;
    std::vector<mat2> orientations;
    std::vector<float> masses;
    std::vector<std::vector<axiom::vertex_element2d>> elements;

    float inner_radius = 100.0f;
    float outer_radius = 105.0f;
    uint segments = 100;

    for(int i = 0; i < segments; ++i) {
        int a0 = i;
        int a1 = i + 1;

        float angle_0 = axiom::pi * 2.0f * a0 / segments;
        float angle_1 = axiom::pi * 2.0f * a1 / segments;

        vec2 dir_0 = vec2(cos(angle_0), sin(angle_0));
        vec2 dir_1 = vec2(cos(angle_1), sin(angle_1));
        axiom::vertex_element2d va = axiom::vertex_element2d(dir_0 * inner_radius);
        axiom::vertex_element2d vb = axiom::vertex_element2d(dir_1 * inner_radius);
        axiom::vertex_element2d vc = axiom::vertex_element2d(dir_0 * outer_radius);
        axiom::vertex_element2d vd = axiom::vertex_element2d(dir_1 * outer_radius);

        float dist_ab = length(va.center - vb.center);
        vec2 sep_ab = (va.center - vb.center) / dist_ab;
        vec2 origin_ab = (va.center + vb.center) * 0.5f;
        
        float dist_cd = length(vc.center - vd.center);
        vec2 sep_cd = (vc.center - vd.center) / dist_cd;
        vec2 origin_cd = (vc.center + vd.center) * 0.5f;

        axiom::vertex_element2d ve = axiom::vertex_element2d(origin_ab, vec2(dist_ab * 0.5f, dist_ab * 0.25f), mat2(sep_ab, vec2(sep_ab.y, -sep_ab.x)));
        axiom::vertex_element2d vf = axiom::vertex_element2d(origin_cd, vec2(dist_cd * 0.5f, dist_cd * 0.25f), mat2(sep_cd, vec2(sep_cd.y, -sep_cd.x)));

        elements.push_back({ve, vf});
        positions.push_back(vec2(0.0f));
        orientations.push_back(glm::identity<mat2>());
        masses.push_back(1.0f);
    }
    
    build_shape(vec2(0.0f, 0.0f), glm::identity<mat2>(), positions, orientations, masses, elements, true);
}

{
    std::vector<axiom::vertex_element2d> elements = {
        axiom::vertex_element2d(vec2(0.0f, 0.0f), vec2(0.75f, 0.5f), glm::identity<glm::mat2>(), {
            axiom::clipping_plane2d(vec2(0.0f, 0.25f), normalize(vec2(0.0f, -1.0f)))
        }),
        axiom::vertex_element2d(vec2(0.0f, 0.0f), vec2(0.75f, 0.5f), glm::identity<glm::mat2>(), {
            axiom::clipping_plane2d(vec2(0.0f, -0.25f), normalize(vec2(0.0f, 1.0f)))
        })
    };

    axiom::random32 rand(0x50015103);

    ivec2 array = ivec2(10, 10);
    vec2 sep = vec2(2.0f);

    for(int x = 0; x < array.x; ++x) {
        for(int y = 0; y < array.y; ++y) {
            vec2 pos = (vec2(x + 0.5f, y + 0.5f) - vec2(array) * 0.5f) * sep;
            build_shape(pos, glm::rotate(glm::identity<mat3>(), rand() * axiom::pi), 1.0f, elements);
        }
    }
}

//
//
//

std::function<void(axiom::render_target&)> render_func = [msystem, ui_system](axiom::render_target& f) {
    static axiom::vertices vertices;
    if(!vertices.initialized) {
        vertices.init();
    }

    //
    
    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)
    };

    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

    uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();

    axiom::transform2d& camera_transform = axiom::global_core.ecs->get_component<axiom::transform2d>(camera);
    axiom::camera2d& camera_cam = axiom::global_core.ecs->get_component<axiom::camera2d>(camera);

    camera_cam.aspect = vec2(f.size) / (float)glm::min(f.size.x, f.size.y);

    //

    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    axiom::shader& grid_shader = msystem->shaders["grid"];

    grid_shader.use();
    vertices.bind();

    glUniformMatrix4fv(0, 1, false, &view[0][0]);
    glUniformMatrix4fv(1, 1, false, &proj[0][0]);

    vertices.draw_vertices(GL_TRIANGLES);

    // render shape
    
    axiom::shader& color_shader = msystem->shaders["color"];
    auto& collector = axiom::global_core.ecs->collectors["color_mesh"];
    for(uint entity : collector.entities) {
        axiom::transform2d transform = axiom::global_core.ecs->get_component<axiom::transform2d>(entity);
        axiom::color_mesh2d mesh = axiom::global_core.ecs->get_component<axiom::color_mesh2d>(entity);

        //
        
        color_shader.use();

        mat4 model = axiom::get_model(transform);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);

        mesh.v_lines->draw_vertices(GL_LINES);  
        mesh.v_tris->draw_vertices(GL_TRIANGLES);   
    }
};
*/