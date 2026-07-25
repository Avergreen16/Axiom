#include <window.hpp>
#include <math.hpp>
#include <ecs.hpp>
#include <render.hpp>
#include <ui.hpp>
#include <platform.hpp>
#include <nlohmann/json.hpp>

#include <iostream>

using json = nlohmann::json;

struct message {
    std::string sender;
    std::string message;
    uint64_t timestamp;
};

struct basic_system : axiom::system {
    axiom::window* win;
    axiom::shader shad;
    axiom::vertices vertices;
    axiom::texture tex;

    axiom::texture ui_texture;
    axiom::texture* font_texture;

    basic_system(axiom::window* win_, axiom::texture* texture) {
        win = win_;

        axiom::text_asset vert = axiom::text_asset::load("resources/shaders/ui.vert");
        axiom::text_asset frag = axiom::text_asset::load("resources/shaders/ui.frag");
        
        shad = std::move(axiom::shader(vert, frag));
        
        axiom::texture_asset texasset = axiom::texture_asset::load("resources/textures/ui.png");

        tex = std::move(axiom::texture(texasset, axiom::RGBA8));
        font_texture = texture;

        /*
        struct color_vertex {
            vec3 position;
            vec3 color;
            vec2 tex_coord;
        };

        std::vector<color_vertex> vs = {
            color_vertex({-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}),
            color_vertex({0.0f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}),
            color_vertex({0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f})
        };
        */

        vertices.init();

        /*
        vertices.vertex_buffer_data(vs.data(), 3, sizeof(color_vertex), GL_STATIC_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex), 0);
        vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 6);
        */
    }

    void call() {
        if(win->pressed_buttons.contains(axiom::input_code::KEY_F11)) {
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

        shad.use();
        font_texture->bind(0);
        tex.bind(1);

        axiom::ui_system& ui_system = ecs->get_system<axiom::ui_system>();

        //std::cout << win->is_fullscreen() << " " << win->screen_size.x << " " << win->screen_size.y << " " << win->viewport_size.x << " " << win->viewport_size.y << "\n";

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

int main(int argc, char* argv[]) {
    float param = 0.0f;

    axiom::window win = axiom::window(ivec2(64, 64), ivec2(512, 512), 6, "axiom test", false);
    win.hide_cursor();

    axiom::ecs ecs;
    ecs.make_active();

    axiom::font_asset default_font = axiom::font_asset::load("resources/fonts/axiom_default.bdf"); //

    axiom::texture font_tex = std::move(axiom::texture(default_font.texture, axiom::texture_format::RGBA8));
    
    axiom::ui_system ui_system(&win);
    ui_system.font_assets = {&default_font};

    ecs.register_system(ui_system);

    std::function<void()> lipsum_func = [&ui_system]() {
        ui_system.position(axiom::position_mode::TOP_LEFT);

        ui_system.input_reset();
        ui_system.input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("WINDOW", window_size, (vec2(ui_system.window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
        ui_system.buffer(vec4(0.0f));
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);
        ui_system.buffer(vec4(2.0f));

        axiom::column_widget::insert();

        std::string lipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum neque mi, tincidunt vitae efficitur in, porta eget erat. Nam vitae leo nec ligula imperdiet lacinia. Praesent sed elit vitae diam finibus convallis at a leo. Duis finibus dolor nisl, vitae tristique lectus egestas a. Nullam quam lectus, fringilla a iaculis vel, suscipit sit amet lectus. Nulla rutrum dapibus enim et tincidunt. Suspendisse et lacus ac dui tristique bibendum et a orci. Donec maximus nulla quis scelerisque placerat. Sed sagittis quam est, vestibulum condimentum sem feugiat ac. Cras non est at nisl fringilla interdum. Vestibulum ut neque sagittis, dictum ligula non, gravida mi. Quisque a nunc lorem. Nam libero libero, aliquet eu tincidunt sit amet, sollicitudin suscipit ex. Curabitur lacinia magna augue, vitae laoreet nisl placerat a. Donec convallis nulla sed nulla lacinia, sed tempus orci volutpat. Quisque vitae turpis eu nisl cursus dignissim.

    Aliquam interdum lectus risus, id efficitur ipsum bibendum vitae. Donec nulla ante, pretium in ullamcorper nec, bibendum ac nunc. Vivamus metus nisl, suscipit ac commodo at, viverra at enim. Pellentesque egestas facilisis sagittis. Duis vel sodales augue. Aliquam erat volutpat. Nam vel lectus at dui congue tincidunt. Phasellus placerat aliquet urna eu congue. Quisque turpis mauris, accumsan at tincidunt ut, dapibus sit amet erat. Nullam enim felis, facilisis nec vulputate eu, congue et nunc. Nunc eros turpis, placerat ac sem eget, pulvinar ultrices arcu. Etiam placerat dui eros, eget commodo metus tempus et. Maecenas volutpat lacinia nisi, eu laoreet sapien ultrices nec.)";

        axiom::text_widget::insert(lipsum, axiom::text_alignment::LEFT, true);
    };

    std::function<void()> render_func = [&ui_system]() {
        ui_system.position(axiom::position_mode::TOP_LEFT);

        ui_system.input_reset();
        ui_system.input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("WINDOW", window_size, (vec2(ui_system.window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
        axiom::panel_widget::insert();
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

    /*
    std::string lipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna. Mauris ultrices, metus vel consectetur aliquet, urna tellus eleifend tellus, in iaculis sem ligula at eros. Nulla pretium sed eros id vestibulum. Cras sodales ligula vitae leo vulputate tincidunt eget a nibh. Fusce augue nunc, condimentum non vulputate quis, laoreet vel augue. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nullam efficitur metus eget diam molestie sollicitudin. Mauris hendrerit, ligula a scelerisque viverra, nisi ex venenatis ex, nec hendrerit ante tortor eu ante. Nullam sapien arcu, porta in dictum ac, dignissim ac ex. Nam euismod fermentum molestie.

    Aenean cursus a odio in luctus. Integer mollis lacus et nisl vulputate, quis faucibus nisl tristique. Maecenas at sodales elit. Nam ut ex mollis ipsum ultrices aliquet in ut odio. Phasellus pharetra ipsum euismod cursus rutrum. Fusce at ligula iaculis, sodales mauris quis, convallis ex. Pellentesque a iaculis nunc, commodo egestas felis. Quisque pharetra volutpat justo, eu sollicitudin enim vulputate ullamcorper. Nunc dapibus aliquam lacus id sollicitudin. Proin vestibulum feugiat imperdiet. Morbi non nunc at orci tristique lacinia. Pellentesque euismod vestibulum eros ut lacinia. Donec hendrerit est eget metus pharetra semper. Vivamus volutpat leo eu sem porttitor tristique. Suspendisse potenti. Fusce commodo odio vestibulum, accumsan augue sed, egestas arcu.

    Suspendisse sit amet consectetur tellus. Nunc ornare scelerisque magna sed gravida. Proin eu condimentum felis. Nunc eu dui quis enim suscipit ornare. Mauris sed nisi enim. Aliquam malesuada interdum lorem, sit amet sodales nisl. In elementum euismod elit non euismod. Nunc tempor erat lacus, quis condimentum mauris aliquam eget. Morbi non hendrerit ex. Duis pharetra commodo lacus ac facilisis. Cras elementum vehicula ante. Duis sodales elementum erat, quis venenatis erat.

    Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus non justo consequat, luctus lectus eu, porta est. Phasellus tincidunt ligula a fringilla semper. Nunc quis diam in dui hendrerit porttitor. Aenean blandit vitae quam feugiat malesuada. Sed non ipsum diam. Sed tincidunt velit non bibendum tincidunt. Morbi et purus metus. Aenean fermentum, elit sed pretium venenatis, nisl leo molestie tellus, ac facilisis metus elit eget orci.

    Sed vel augue eu leo gravida dictum. Nullam pharetra turpis sem, sed rhoncus massa hendrerit at. Pellentesque molestie tincidunt mollis. Fusce ipsum mauris, sodales dictum ipsum vel, vulputate pretium tellus. Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.)";
    */

    std::function<void()> func_chat = [&ui_system]() {
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        ui_system.position(axiom::position_mode::TOP_LEFT);

        axiom::panel_widget::insert();
        ui_system.buffer(vec4(0.0f));
        axiom::column_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);
        ui_system.buffer(vec4(0.0f));

        float buffer = 8.0f;

        ui_system.buffer(vec4(4.0f));
        axiom::column_widget::insert();

        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f), false);

        // messages

        // addie
        axiom::text_widget::insert("> Addie", axiom::text_alignment::LEFT);
        axiom::message_widget::insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_red, vec2(buffer), axiom::get_timestamp(), 1);
        // averie

        ui_system.position(axiom::position_mode::TOP_RIGHT);
        axiom::text_widget::insert("Averie <", axiom::text_alignment::LEFT);
        axiom::message_widget::insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_green, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_green, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_green, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_green, vec2(buffer), axiom::get_timestamp());
        axiom::message_widget::insert("Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna.", axiom::text_alignment::LEFT, vec2(160, 10000), axiom::color_green, vec2(buffer), axiom::get_timestamp(), 2);

        //
        ui_system.input_step(2);
        axiom::spacer_widget::insert(vec2(0.0f), vec2(FLT_MAX), false, true);
        ui_system.buffer(vec4(8.0f));
        ui_system.position(axiom::position_mode::BOTTOM_LEFT);
        axiom::column_widget::insert();
        axiom::text_box_widget::insert(FLT_MAX, vec2(8.0f, 8.0f), "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        ui_system.set_attrib(vec2(160, 16), vec2(FLT_MAX, 16), vec2(1.0f));
    };

    std::function<void()> func_settings = [&ui_system, &node, &param]() {
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        axiom::panel_widget::insert();

        ui_system.buffer(vec4(4.0f));
        ui_system.position(axiom::position_mode::TOP_LEFT);

        axiom::column_widget::insert();
        axiom::grid_widget::insert(3);

        //
        
        ui_system.position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("slider", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(FLT_MAX, 0), false);
        axiom::row_widget::insert();
        ui_system.set_attrib(vec2(160, 16), vec2(160, 16), vec2(1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6, axiom::color_blue, vec2(-16.0f, 16.0f), 0.0f, 0.0f, "",
            [&param](axiom::slider_widget& self) {
                if(!self.pressed) self.current_value = param;
                else param = self.current_value;
            }
        );
        ui_system.set_attrib(vec2(0, 16), vec2(FLT_MAX, 16), vec2(1.0f));

        axiom::input_box_widget<float>::insert(vec2(40, 16), 0.0f, 
            [&param](axiom::input_box_widget<float>& self) {
                if(self.update) {
                    param = self.value;
                } else {
                    self.value = param;
                }
            }
        );

        ui_system.input_step();
    };

    axiom::screen_widget::insert("axiom text", axiom::color_blue);
    axiom::relative_widget::insert(
        [&ui_system](axiom::relative_widget& widget) {
            axiom::screen_widget* parent = (axiom::screen_widget*)ui_system.widgets[widget.parent].get();
            widget.position = vec2(parent->header, parent->size.y - parent->header);

            for(auto child : widget.children) {
                auto& child_widget = ui_system.widgets[child];

                child_widget->position = widget.position;
                child_widget->size = vec2(parent->size.x - parent->header, parent->header);
            }
        }
    );

    ui_system.position(axiom::position_mode::CENTER_LEFT);
    axiom::row_widget::insert();

    axiom::button_widget::insert(vec2(40.0f, 16.0f), axiom::color_blue, "TEST", 
        [&node, &ui_system](axiom::button_widget& self) {
            if(self.pressed) {
                ui_system.position(axiom::position_mode::TOP_LEFT);

                ui_system.input_reset();
                axiom::menu_widget::insert(self.position, 0.001, axiom::color_blue, 200, 16, FLT_MAX, node, {});
            }
        }
    );
    
    ui_system.input_root(1);
    ui_system.position(axiom::position_mode::TOP_LEFT);

    axiom::split_widget::insert(axiom::layout_mode::ROW, {{1.0f, axiom::panel_mode::SCALE}, {1.0f, axiom::panel_mode::SCALE}});
    axiom::panel_widget::insert();
    
    ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
    axiom::tab_widget::insert(24.0f, 2.0f, {
        axiom::tab("Settings", 80.0f, axiom::color_blue, func_settings),
        axiom::tab("Chat", 80.0f, axiom::color_blue, func_chat),
    });

    //
    //
    //

    ui_system.input_root(2);
    
    axiom::panel_widget::insert();

    // json test

    json root;
    root["messages"] = json::array();
    
    json message;
    message["sender"] = "Averie";
    message["text"] = "Hello addie!";
    message["timestamp"] = axiom::get_timestamp();

    root["messages"].push_back(message);

    //axiom::write_text_to_file("output/message" + std::to_string(axiom::get_timestamp()) + ".json", root.dump(4));

    //

    basic_system bsystem(&win, &font_tex);
    ecs.register_system(bsystem);

    while(!win.should_close) {
        win.poll_events();

        ecs.do_frame();
    }
}

/*
ui_system.position(axiom::position_mode::CENTER_LEFT);
axiom::text_widget::insert("button 0", axiom::text_alignment::LEFT, false);
axiom::spacer_widget::insert(vec2(0, 0), vec2(FLT_MAX, 0), false);
axiom::row_widget::insert();
ui_system.set_attrib(vec2(160, 16), vec2(160, 16), vec2(1.0f));
axiom::button_widget::insert(vec2(120, 16), axiom::color_blue, "BUTTON", 
    [](axiom::button_widget& self) {
        if(self.pressed) std::cout << "PRESSED" << "\n";
    }
);
ui_system.set_attrib(vec2(0, 16), vec2(FLT_MAX, 16), vec2(1.0f));
axiom::input_box_widget<float>::insert(vec2(40, 16), 0.0f);
ui_system.input_step();

ui_system.position(axiom::position_mode::TOP_LEFT);
axiom::spacer_widget::insert(vec2(0, 16), vec2(FLT_MAX, 16), true);
*/