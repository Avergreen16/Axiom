#include <window.hpp>
#include <math.hpp>
#include <ecs.hpp>
#include <render.hpp>
#include <ui.hpp>
#include <platform.hpp>
#include <utilities.hpp>
#include <scene.hpp>
#include <physics-2d.hpp>

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
            axiom::physics_system& physics_system = axiom::global_core.ecs->get_system<axiom::physics_system>();

            physics_system.sim_active = !physics_system.sim_active;
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

    std::function<void()> lipsum_func = [ui_system]() {
        ui_system->position(axiom::position_mode::TOP_LEFT);

        ui_system->input_reset();
        ui_system->input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("WINDOW", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
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

    std::function<void()> render_func = [ui_system]() {
        ui_system->position(axiom::position_mode::TOP_LEFT);

        ui_system->input_reset();
        ui_system->input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("WINDOW", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
        axiom::panel_widget::insert();
        ui_system->input_z(0.0f);
    };

    
    std::shared_ptr<axiom::menu_node> node(new axiom::menu_node{
        "",
        {
            axiom::menu_node("Debug Windows", {
                axiom::menu_node("Lipsum", {}, lipsum_func),
                axiom::menu_node("Render", {}, render_func),
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
    
    axiom::text_asset color_vert = axiom::text_asset::load("resources/shaders/color.vert");
    axiom::text_asset color_frag = axiom::text_asset::load("resources/shaders/color.frag");
    msystem->shaders.emplace("color", std::move(axiom::shader(color_vert, color_frag)));
    
    ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 0.0f));
    msystem->targets.push_back(axiom::render_target());

    {
        std::function<void(axiom::render_target&)> render_func = [msystem, ui_system](axiom::render_target& f) {
            static vec2 cursor_pos = vec2(0.0f);
            static bool capture = false;
            static uint constraint = 0xFFFFFFFF;
            
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

            cursor_pos = ui_system->window->cursor_pos;
            cursor_pos = (cursor_pos - (vec2)f.position - (0.5f * (vec2)f.size)) / (0.5f * (vec2)f.size);

            camera_cam.aspect = vec2(f.size) / (float)glm::min(f.size.x, f.size.y);

            //

            mat4 view = axiom::get_view(camera_cam, camera_transform);
            mat4 proj = axiom::get_proj(camera_cam);

            mat4 inv_view = glm::inverse(view);
            mat4 inv_proj = glm::inverse(proj);

            cursor_pos = inv_view * inv_proj * vec4(cursor_pos, 0.0f, 1.0f);
            

            axiom::shader& grid_shader = msystem->shaders["grid"];

            grid_shader.use();
            vertices.bind();

            glUniformMatrix4fv(0, 1, false, &view[0][0]);
            glUniformMatrix4fv(1, 1, false, &proj[0][0]);
            glUniform2i(2, f.size.x, f.size.y);
            glUniform2f(3, cursor_pos.x, cursor_pos.y);

            vertices.draw_vertices(GL_TRIANGLES);

            //

            bool includes = axiom::includes(ui_system->window->cursor_pos, ivec4(f.position, f.position + f.size));

            if(constraint != 0xFFFFFFFF) capture = false;

            std::cout << constraint << "\n";
            
            if(includes) {
                if(ui_system->window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
                    capture = true;
                }
            }
            if(!ui_system->window->input_map[axiom::input_code::MOUSE_LEFT]) capture = false;

            if(capture) {
                vec2 delta = ui_system->window->cursor_delta / (0.5f * (vec2)f.size);

                vec2 world_delta = mat4(mat3(inv_view)) * inv_proj * vec4(delta, 0.0f, 1.0f);

                camera_transform.position -= world_delta;
            }
            if(includes) {
                float zoom_delta = glm::pow(1.25f, ui_system->window->scroll_delta);
                if(zoom_delta != 1.0f) {
                    vec2 offset = camera_transform.position - cursor_pos;
                    offset /= zoom_delta;

                    camera_transform.position = offset + cursor_pos;
                    
                    camera_cam.zoom *= zoom_delta;
                }
            }

            // render shape
            
            axiom::shader& color_shader = msystem->shaders["color"];
            auto& collector = axiom::global_core.ecs->collectors["color_mesh"];
            for(uint entity : collector.entities) {
                axiom::transform2d transform = axiom::global_core.ecs->get_component<axiom::transform2d>(entity);
                axiom::color_mesh mesh = axiom::global_core.ecs->get_component<axiom::color_mesh>(entity);

                //
                
                color_shader.use();

                mat4 model = axiom::get_model(transform);

                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                mesh.v_lines->draw_vertices(GL_LINES);  
                mesh.v_tris->draw_vertices(GL_TRIANGLES);   
            }

            //

            axiom::physics_system& physics = axiom::global_core.ecs->get_system<axiom::physics_system>();

            // update constraint
            if(constraint != 0xFFFFFFFF) {
                axiom::constraint& cc = physics.constraints[constraint];
                cc.pos[0].b = cursor_pos;
                
                if(!ui_system->window->input_map[axiom::input_code::MOUSE_LEFT]) {
                    physics.constraints.erase(physics.constraints.begin() + constraint);
                    constraint = 0xFFFFFFFF;
                }
            }

            if(ui_system->window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && ui_system->window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
                if(constraint == 0xFFFFFFFF) {
                    for(uint32_t entity : physics.collectors[0].entities) {
                        axiom::transform2d& transform = axiom::global_core.ecs->get_component<axiom::transform2d>(entity);
                        axiom::collider2d& collider = axiom::global_core.ecs->get_component<axiom::collider2d>(entity);

                        vec2 rel_point = glm::transpose(transform.orientation) * (cursor_pos - transform.position);

                        bool collide = false;

                        for(axiom::collision_shape& cs : collider.shapes) {
                            vec2 rel_point2 = transpose(cs.orientation) * (rel_point - cs.position);

                            collide |= axiom::physics_system::collision_point(cs.vertices, rel_point2);

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
        };
        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0};

        msystem->targets[0] = axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0, 0), formats, attachments);
    }

    axiom::render_widget::insert(&msystem->targets[0], 0);

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

        axiom::transform2d& transform = axiom::global_core.ecs->get_component<axiom::transform2d>(camera);
        //axiom::camera2d& cam = axiom::global_core.ecs->get_component<axiom::camera2d>(camera);
        
        return "position: " + axiom::to_base(transform.position.x, 16, 3) + " " + axiom::to_base(transform.position.y, 16, 3);
    };

    axiom::text_widget::insert("position: ", axiom::text_alignment::LEFT, true, position_func);

    /*
    std::function<std::string(std::string)> widget_func = [ui_system](std::string prev) {
        return "widgets: " + std::to_string(ui_system->widgets.size());
    };

    axiom::text_widget::insert("widgets: ", axiom::text_alignment::LEFT, true, widget_func);
    */
};

void build_shape(vec2 pos, mat2 ori, float mass, std::vector<axiom::vertex_element> elements, bool is_static = false) {
    axiom::color_mesh mesh;
    axiom::collider2d collider;

    axiom::collision_shape shape;
    shape.vertices = elements;
    shape.mass = mass;
    collider.shapes.push_back(shape);

    collider.is_static = is_static;
    axiom::physics_system::calculate_inertia(collider);

    //

    std::vector<axiom::color_vertex> perimeter_vertices;
    std::vector<axiom::color_vertex> area_vertices;

    std::vector<vec2> perimeter;
    std::vector<vec2> area;
    axiom::create_mesh(collider, &perimeter, &area);

    for(vec2 v : perimeter) {
        perimeter_vertices.push_back(axiom::color_vertex(v, vec4(1.0f)));
    }
    
    for(vec2 v : area) {
        area_vertices.push_back(axiom::color_vertex(v, vec4(1.0f, 1.0f, 1.0f, 0.125f)));
    }

    mesh.v_lines = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_lines->init();
    mesh.v_lines->vertex_buffer_data(perimeter_vertices.data(), perimeter_vertices.size(), sizeof(axiom::color_vertex), GL_STATIC_DRAW);
    mesh.v_lines->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex), 0);
    mesh.v_lines->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex), sizeof(float) * 2);
    
    mesh.v_tris = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_tris->init();
    mesh.v_tris->vertex_buffer_data(area_vertices.data(), area_vertices.size(), sizeof(axiom::color_vertex), GL_STATIC_DRAW);
    mesh.v_tris->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex), 0);
    mesh.v_tris->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex), sizeof(float) * 2);

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

    axiom::physics_system psystem;
    ecs.register_system(psystem);

    main_system bsystem(&win);
    bsystem.textures.emplace("font_axiom_default", std::move(font_tex));
    ecs.register_system(bsystem);
    
    axiom::signature sig = axiom::global_core.ecs->update_signature<axiom::transform2d>();
    axiom::global_core.ecs->update_signature<axiom::camera2d>(sig);
    axiom::collector col(sig);
    axiom::global_core.ecs->create_collector("camera", col);
    
    sig = axiom::global_core.ecs->update_signature<axiom::transform2d>();
    axiom::global_core.ecs->update_signature<axiom::color_mesh>(sig);
    col = axiom::collector(sig);
    axiom::global_core.ecs->create_collector("color_mesh", col);

    uint camera_entity = axiom::global_core.ecs->insert_entity();
    axiom::camera2d cam;
    axiom::transform2d tf;

    tf.position = vec2(0.5f, 0.25f);
    tf.orientation = glm::identity<mat2>(); //glm::rotate(glm::identity<mat3>(), axiom::pi * 0.125f);

    axiom::global_core.ecs->insert_component(camera_entity, cam);
    axiom::global_core.ecs->insert_component(camera_entity, tf);

    //
    
    build_shape(vec2(0.0f, 0.0f), glm::identity<mat2>(), 1.0f, {
        axiom::vertex_element(vec2(-8.0f, -0.5f)),
        axiom::vertex_element(vec2(8.0f, -0.5f)),
        axiom::vertex_element(vec2(-8.0f, 0.5f)),
        axiom::vertex_element(vec2(8.0f, 0.5f)),
    }, true);

    build_shape(vec2(-8.0f, 8.0f), glm::identity<mat2>(), 1.0f, {
        axiom::vertex_element(vec2(0.0f, 1.0f), vec2(1.0f, 0.25f), glm::identity<glm::mat2>()),
        axiom::vertex_element(vec2(0.0f, -1.0f), vec2(1.0f, 0.5f), glm::identity<glm::mat2>()),
    });
    
    build_shape(vec2(8.0f, 8.0f), glm::identity<mat2>(), 1.0f, {
        axiom::vertex_element(vec2(-1.5f, 0.0f), vec2(0.333f, 1.0f), glm::rotate(glm::identity<glm::mat3>(), axiom::pi * 0.175f)),
        axiom::vertex_element(vec2(1.5f, 0.0f), vec2(0.333f, 1.0f), glm::rotate(glm::identity<glm::mat3>(), axiom::pi * -0.175f)),
    });

    //

    create_ui();

    while(!win.should_close) {
        win.poll_events();

        ecs.do_frame();
    }
}

/*
double current_time = axiom::get_time();
double delta_time = current_time - prev_time;
prev_time = current_time;

if(win.input_map[axiom::input_code::KEY_F1]) {
    double sleep_for = (1.0f / frame_rate) - delta_time;
    prev_time += sleep_for;
    if(sleep_for > 0.0f) std::this_thread::sleep_for(std::chrono::microseconds(int(sleep_for * 1000000.0f)));
}
*/