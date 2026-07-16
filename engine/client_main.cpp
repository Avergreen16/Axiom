#include "core.hpp"
#include "render.hpp"
#include "input.hpp"
#include "billboard.hpp"
#include "voxel.hpp"
#include "building.hpp"
//#include "client_networking.hpp"

#include <chrono>
#include <windows.h>
using namespace std::chrono;

#include "stb_image.h"
#include "stb_image_write.h"

std::vector<ivec2> indices = {
    {0, 0},
    {1, 0},
    {1, 1},
    {0, 0},
    {1, 1},
    {0, 1}
};

lua_function lua_func = [](lua_State* state) {
    float x = lua_tonumber(state, 1);
    float y = lua_tonumber(state, 2);
    float z = lua_tonumber(state, 3);
    float r = lua_tonumber(state, 4);
    float g = lua_tonumber(state, 5);
    float b = lua_tonumber(state, 6);
    
    std::cout << "C++ func called from lua! parameters: " << x << " " << y << " " << z << " " << r << " " << g << " " << b << "\n";

    //
    Input_system& input_system = ecs.get_system<Input_system>();

    vec3 color = vec3(r, g, b);
    vec3 size = vec3(x, y, z);

    uint32_t camera = input_system.player_camera;
    Transform& camera_transform = ecs.get_component<Transform>(camera);
    
    Physics_system& ps = ecs.get_system<Physics_system>();

    Transform t;
    t.position = camera_transform.position;
    vec3 up = camera_transform.orientation * vec3(0, 0, -1);
    t.orientation = rotate_to(vec3(0.0, 0.0, 1.0), up);

    /*
    std::vector<vec3> vs = {
        vec3(-1.0f, -1.0f, -1.0f),
        vec3(1.0f, -1.0f, -1.0f),
        vec3(-1.0f, 1.0f, -1.0f),
        vec3(1.0f, 1.0f, -1.0f),
        vec3(-1.0f, -1.0f, 1.0f),
        vec3(1.0f, -1.0f, 1.0f),
        vec3(-1.0f, 1.0f, 1.0f),
        vec3(1.0f, 1.0f, 1.0f),
    };
    */
    std::vector<vec3> vs = {
        vec3(0.0f, 0.0f, -size.x * 0.5f + size.y),
        vec3(0.0f, 0.0f, size.x * 0.5f - size.y),
    };

    //for(vec3& v : vs) v *= size * 0.5f;
    
    Collider c;
    Collision_shape cs;
    cs.vertices = vs;
    cs.radius = size.y;
    cs.split_radius = vec3(size.y);
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

    uint32_t entity = ecs.insert_entity();

    ecs.insert_component(entity, m);
    ecs.insert_component(entity, t);
    ecs.insert_component(entity, c);

    return 0;
};

void start_ui() {
    auto& gui_system = ecs.get_system<GUI_system>();
    
    std::function<void()> func_lua = [&gui_system]() {
        uint32_t w = 160;
        
        gui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        Panel_Widget::insert(2.0f, false);

        gui_system.buffer(vec4(4.0f));
        gui_system.position(PM_TOP_LEFT);

        Column_Widget::insert();
        
        Grid_Widget::insert(3);
        
        gui_system.buffer(vec4(2.0f));

        gui_system.position(PM_CENTER_LEFT);
        //Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        Text_Widget::insert("Call Lua File", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        
        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        uint64_t n = Input_Box_Widget<std::string>::insert(vec2(40, 16), "", 
            [](Input_Box_Widget<std::string>& self) {
                Input_system& is = ecs.get_system<Input_system>();

                /*
                if(self.update) {
                    self.value = clamp(self.value, 1, 16);
                    is.num_cubes.x = self.value;
                } else {
                    self.value = is.num_cubes.x;
                }
                */
            }
        );
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        Button_Widget::insert(vec2(16, 16), color_lua, "", 
            [&gui_system, n](Button_Widget& self) {
                self.icon = vec4(27.0f, 49.0f, 6.0f, 6.0f);
                self.icon_size = vec2(6.0f);
                
                if(self.pressed) {
                    auto* widget = (Input_Box_Widget<std::string>*)gui_system.widgets[n].get();

                    std::string path = widget->value;

                    core.lua_wrapper.run_file(path);
                }
            }
        );

        gui_system.step();
    };

    std::function<void()> func_settings = [&gui_system]() {
        uint32_t w = 160;
        
        gui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        Panel_Widget::insert(2.0f, false);

        gui_system.buffer(vec4(4.0f));
        gui_system.position(PM_TOP_LEFT);

        Column_Widget::insert();
        
        Grid_Widget::insert(3);
        
        gui_system.buffer(vec4(2.0f));
        
        // camera mode
        // number base
        // gravity
        // iteration count
        // substeps
        // dampening
        // DOF
        // num cubes
        // object scale
        // number of links
        
        // number base
        // gravity
        // iteration count
        // substeps
        // 
        // camera mode
        // dampening
        // DOF
        // 
        // num cubes
        // summon cubes
        // 
        // number of links
        // summon chain

        // number base

        gui_system.position(PM_CENTER_LEFT);
        Text_Widget::insert("Number Base", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Drop_Widget::insert(vec2(160, 16), color_editor, 16, 16, 32, {"DECIMAL", "DOZENAL", "HEXADECIMAL", "a", "b", "c", "d"}, 0, 
            [](Drop_Widget& self) {
                if(self.selected == 0) core.number_base = 10;
                else if(self.selected == 1) core.number_base = 12;
                else if(self.selected == 2) core.number_base = 16;
            }
        );

        // gravity

        Text_Widget::insert("Gravity", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));
        
        Slider_Widget::insert(vec2(120, 16), 6.0f, color_physics, vec2(0.0f, 64.0f), 0.0f, 19.62f, "", [](Slider_Widget& self) {
            Physics_system& ps = ecs.get_system<Physics_system>();
            if(self.pressed) {
                ps.gravity = self.current_value;
            } else {
                self.current_value = ps.gravity;
            }
        });
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        Input_Box_Widget<float>::insert(vec2(40, 16), 4.0f, 
            [](Input_Box_Widget<float>& self) {
                Physics_system& ps = ecs.get_system<Physics_system>();
                if(self.update) ps.gravity = self.value;
                else self.value = ps.gravity;
            }
        );
        
        gui_system.step();

        // iterations

        Text_Widget::insert("Iteration Count", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        Slider_Widget::insert(vec2(120, 16), 6.0f, color_physics, vec2(1, 64), 1.0f, 4.0f, "", [](Slider_Widget& self) {
            Physics_system& ps = ecs.get_system<Physics_system>();
            if(self.pressed) {
                ps.iterations = self.current_value;
            } else {
                self.current_value = ps.iterations;
            }
        });
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        Input_Box_Widget<float>::insert(vec2(40, 16), 4.0f, 
            [](Input_Box_Widget<float>& self) {
                Physics_system& ps = ecs.get_system<Physics_system>();
                if(self.update) ps.iterations = self.value;
                else self.value = ps.iterations;
            }
        );

        gui_system.step();
        
        // substeps
        
        Text_Widget::insert("Substeps", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        
        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        Slider_Widget::insert(vec2(120, 16), 6.0f, color_physics, vec2(1, 64), 1.0f, 4.0f, "", [](Slider_Widget& self) {
            Physics_system& ps = ecs.get_system<Physics_system>();
            if(self.pressed) {
                float substeps = self.current_value;
                ps.substeps = substeps;
                ps.sub_dt = ps.physics_step / ps.substeps;
            } else {
                self.current_value = ps.substeps;
            }
        });
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        Input_Box_Widget<float>::insert(vec2(40, 16), 4.0f, 
            [](Input_Box_Widget<float>& self) {
                Physics_system& ps = ecs.get_system<Physics_system>();
                if(self.update) ps.substeps = self.value;
                else self.value = ps.substeps;
            }
        );

        gui_system.step();

        // spacer

        gui_system.step();
        Spacer_Widget::insert(vec2(0.0f, 8.0f), vec2(FLT_MAX, 8.0f), true);
        
        gui_system.buffer(vec4(4.0f));
        Grid_Widget::insert(3);

        // dampening
        
        gui_system.position(PM_CENTER_LEFT);
        Text_Widget::insert("Dampening", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Drop_Widget::insert(vec2(160, 16), color_debug, 16, 16, 160, {"ON", "OFF"}, 0, 
            [](Drop_Widget& self) {
                static uint32_t prev = 0;
                if(prev != self.selected) {
                    Physics_system& ps = ecs.get_system<Physics_system>();

                    prev = self.selected;
                    if(self.selected == 0) ps.do_dampening = true;
                    else if(self.selected == 1) ps.do_dampening = false;
                }
            }
        );
        
        // DOF
        
        gui_system.position(PM_CENTER_LEFT);
        Text_Widget::insert("DOF", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Drop_Widget::insert(vec2(160, 16), color_debug, 16, 16, 160, {"ON", "OFF"}, 0, 
            [](Drop_Widget& self) {
                static uint32_t prev = 0;
                if(prev != self.selected) {
                    Physics_system& ps = ecs.get_system<Physics_system>();

                    prev = self.selected;
                    if(self.selected == 0) ps.do_DOF = true;
                    else if(self.selected == 1) ps.do_DOF = false;
                }
            }
        );

        // spacer

        gui_system.step();
        Spacer_Widget::insert(vec2(0.0f, 8.0f), vec2(FLT_MAX, 8.0f), true);
        
        gui_system.buffer(vec4(4.0f));
        Grid_Widget::insert(3);

        // num cubes
        
        Text_Widget::insert("Number of Cubes", ALIGNMENT_LEFT, false);
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        uint64_t tw = Text_Widget::insert("X", ALIGNMENT_LEFT, false);
        Input_Box_Widget<int>::insert(vec2(40, 16), 4.0f, 
            [](Input_Box_Widget<int>& self) {
                Input_system& is = ecs.get_system<Input_system>();

                if(self.update) {
                    self.value = clamp(self.value, 1, 16);
                    is.num_cubes.x = self.value;
                } else {
                    self.value = is.num_cubes.x;
                }
            }
        );
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        Text_Widget::insert("Y", ALIGNMENT_LEFT, false);
        Input_Box_Widget<int>::insert(vec2(40, 16), 4.0f, 
            [](Input_Box_Widget<int>& self) {
                Input_system& is = ecs.get_system<Input_system>();

                if(self.update) {
                    self.value = clamp(self.value, 1, 16);
                    is.num_cubes.y = self.value;
                } else {
                    self.value = is.num_cubes.y;
                }
            }
        );
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        Text_Widget::insert("Z", ALIGNMENT_LEFT, false);
        Input_Box_Widget<int>::insert(vec2(40, 16), 4.0f, 
            [](Input_Box_Widget<int>& self) {
                Input_system& is = ecs.get_system<Input_system>();

                if(self.update) {
                    self.value = clamp(self.value, 1, 16);
                    is.num_cubes.z = self.value;
                } else {
                    self.value = is.num_cubes.z;
                }
            }
        );
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));
        
        gui_system.step();

        // create crates

        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        Button_Widget::insert(vec2(150, 16), color_debug, "Summon Blocks", 
            [](Button_Widget& self) {
                if(self.pressed) create_crates();
            }
        );
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));
        
        gui_system.step();

        // spacer

        gui_system.step();
        Spacer_Widget::insert(vec2(0.0f, 8.0f), vec2(FLT_MAX, 8.0f), true);
        
        gui_system.buffer(vec4(4.0f));
        Grid_Widget::insert(3);

        // number of links
    
        Text_Widget::insert("Number of Links", ALIGNMENT_LEFT, false);

        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        
        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(1.0f));

        Slider_Widget::insert(vec2(120, 16), 6.0f, color_debug, vec2(1, 16), 1.0f, 6.0f, "", [](Slider_Widget& self) {
            Input_system& is = ecs.get_system<Input_system>();
            if(self.pressed) {
                is.num_links = self.current_value;
            } else {
                self.current_value = is.num_links;
            }
        });
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        Input_Box_Widget<float>::insert(vec2(40, 16), 4.0f, 
            [](Input_Box_Widget<float>& self) {
                Input_system& is = ecs.get_system<Input_system>();

                if(self.update) {
                    is.num_links = self.value;
                } else {
                    self.value = is.num_links;
                }
            }
        );

        gui_system.step();

        // summon chain
        
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        Row_Widget::insert();
        gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        Button_Widget::insert(vec2(40, 16), color_debug, "Summon Chain", 
            [](Button_Widget& self) {
                if(self.pressed) create_chain();
            }
        );
        gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));
        
        gui_system.step();
    };

    /*
    
    // object scale

    Text_Widget::insert("Object Scale", ALIGNMENT_LEFT, false);
    Spacer_Widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

    Row_Widget::insert();
    gui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(1.0f));

    Slider_Widget::insert(vec2(120, 16), 6.0f, hsv_color(2.0f, 0.6f, 0.5), vec2(0.25f, 8.0f), 0.25f, 1.0f, "", [](Slider_Widget& self) {
        Input_system& is = ecs.get_system<Input_system>();
        if(self.pressed) {
            is.object_scale = self.current_value;
        } else {
            self.current_value = is.object_scale;
        }
    });
    gui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

    Input_Box_Widget<float>::insert(vec2(40, 16), 4.0f, 
        [](Input_Box_Widget<float>& self) {
            Input_system& is = ecs.get_system<Input_system>();

            if(self.update) {
                is.object_scale = self.value;
            } else {
                self.value = is.object_scale;
            }
        }
    );

    gui_system.step();
    */

    std::function<void()> func_selection = [&gui_system]() {
        gui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        Panel_Widget::insert(2.0f, false);
        
        gui_system.buffer(vec4(3.0f));
        gui_system.position(PM_TOP_LEFT);

        Column_Widget::insert(true);

        Text_Widget::insert("", ALIGNMENT_LEFT, true,
            [](std::string str) -> std::string {
                Input_system& is = ecs.get_system<Input_system>();

                if(is.object_target == NULL_ENTITY) return "no object selected";
                else {
                    Transform& transform = ecs.get_component<Transform>(is.object_target);
                    Collider& collider = ecs.get_component<Collider>(is.object_target);

                    std::string ret;

                    int base = core.number_base;

                    ret += "\\bPosition:\\r";
                    ret += "\n\\bX\\r = " + to_base(transform.position.x.sector, base) + " " + to_base(transform.position.x.fraction, base, 2);
                    ret += "\n\\bY\\r = " + to_base(transform.position.y.sector, base) + " " + to_base(transform.position.y.fraction, base, 2);
                    ret += "\n\\bZ\\r = " + to_base(transform.position.z.sector, base) + " " + to_base(transform.position.z.fraction, base, 2);
                    
                    ret += "\n";
                    ret += "\n\\bOrientation:\\r";
                    ret += "\n" + to_base(transform.orientation[0].x, base, 2) + " " + to_base(transform.orientation[1].x, base, 2) + " " + to_base(transform.orientation[2].x, base, 2);
                    ret += "\n" + to_base(transform.orientation[0].y, base, 2) + " " + to_base(transform.orientation[1].y, base, 2) + " " + to_base(transform.orientation[2].y, base, 2);
                    ret += "\n" + to_base(transform.orientation[0].z, base, 2) + " " + to_base(transform.orientation[1].z, base, 2) + " " + to_base(transform.orientation[2].z, base, 2);
                    ret += "\n\\bVector Lengths:\\r " + to_base(length(transform.orientation[0]), base, 2) + " " + to_base(length(transform.orientation[1]), base, 2) + " " + to_base(length(transform.orientation[2]), base, 2);
                    ret += "\n\\bVector Dots:\\r " + to_base(dot(transform.orientation[0], transform.orientation[1]), base, 2) + " " + to_base(dot(transform.orientation[0], transform.orientation[2]), base, 2) + " " + to_base(dot(transform.orientation[1], transform.orientation[2]), base, 2);


                    ret += "\n";
                    ret += "\n\\bNumber of Subshapes:\\r " + to_base((int32_t)collider.collision_shapes.size(), base);
                    ret += "\n\\bStatic:\\r " + std::string(collider.is_static ? "TRUE" : "FALSE");
                    ret += "\n\\bRotation Enabled:\\r " + std::string(collider.allow_rotation ? "TRUE" : "FALSE");
                    ret += "\n\\bGravity Enabled:\\r " + std::string(collider.allow_gravity ? "TRUE" : "FALSE");
                    ret += "\n\\bSleeping:\\r " + std::string(collider.sleeping ? "TRUE" : "FALSE");

                    ret += "\n";
                    ret += "\n\\bMass:\\r " + to_base(collider.mass, base, 2);
                    
                    ret += "\n";
                    ret += "\n\\bInertia Tensor:\\r";
                    ret += "\n" + to_base(collider.inertia_tensor[0].x, base, 6) + " " + to_base(collider.inertia_tensor[1].x, base, 6) + " " + to_base(collider.inertia_tensor[2].x, base, 6);
                    ret += "\n" + to_base(collider.inertia_tensor[0].y, base, 6) + " " + to_base(collider.inertia_tensor[1].y, base, 6) + " " + to_base(collider.inertia_tensor[2].y, base, 6);
                    ret += "\n" + to_base(collider.inertia_tensor[0].z, base, 6) + " " + to_base(collider.inertia_tensor[1].z, base, 6) + " " + to_base(collider.inertia_tensor[2].z, base, 6);
                    
                    ret += "\n";
                    ret += "\n\\bInverse Inertia Tensor:\\r";
                    ret += "\n" + to_base(collider.inverse_inertia_tensor[0].x, base, 6) + " " + to_base(collider.inverse_inertia_tensor[1].x, base, 6) + " " + to_base(collider.inverse_inertia_tensor[2].x, base, 6);
                    ret += "\n" + to_base(collider.inverse_inertia_tensor[0].y, base, 6) + " " + to_base(collider.inverse_inertia_tensor[1].y, base, 6) + " " + to_base(collider.inverse_inertia_tensor[2].y, base, 6);
                    ret += "\n" + to_base(collider.inverse_inertia_tensor[0].z, base, 6) + " " + to_base(collider.inverse_inertia_tensor[1].z, base, 6) + " " + to_base(collider.inverse_inertia_tensor[2].z, base, 6);
                    
                    ret += "\n";
                    ret += "\n\\bVelocity:\\r " + to_base(collider.velocity.x, base, 2) + " " + to_base(collider.velocity.y, base, 2) + " " + to_base(collider.velocity.z, base, 2);
                    
                    ret += "\n\\bAngular Momentum:\\r " + to_base(collider.angular_momentum.x, base, 2) + " " + to_base(collider.angular_momentum.y, base, 2) + " " + to_base(collider.angular_momentum.z, base, 2);

                    return ret;
                }
            }
        );
        
        gui_system.position(PM_TOP_CENTER);
        
        Button_Widget::insert(vec2(150, 16), color_editor, "", 
        [](Button_Widget& self) {
            Input_system& is = ecs.get_system<Input_system>();

            if(self.pressed) is.select_target = !is.select_target;

            std::string label;
            if(is.select_target) {
                is.object_target = NULL_ENTITY;

                label = "Click Object";
                float m = fmod(core.current_time, 2.0);
                if(m < 0.666) label += ".  ";
                else if(m < 1.333) label += ".. ";
                else label += "...";
            } else {
                label = "Click to Start Selection";
            }

            if(label != self.label) self.text_dirty = true;
            self.label = label;
        });
    };
    
    std::function<void()> func_global = [&gui_system]() {
        gui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        Panel_Widget::insert(2.0f, false);

        gui_system.buffer(vec4(4.0f));
        gui_system.position(PM_TOP_LEFT);
        Row_Widget::insert();
        Text_Widget::insert("", ALIGNMENT_LEFT, true,
            [](std::string str) -> std::string {
                int base = core.number_base;
                
                static double time = 0.0f;
                static double freq = 1.0f;
                static uint32_t num_frames = 0;

                time += core.delta_time;
                ++num_frames;

                if(time > freq || str == "") {
                    double fps = double(num_frames) / time;
                    time = 0.0;
                    num_frames = 0;

                    std::string string = "\\bFPS: \\r" + to_base(float(fps), core.number_base, 3) + "\n\n";

                    std::vector<std::string> order;
                    std::unordered_map<std::string, double> times;
                    double total_time = 0.0f;

                    for(Profiler_Frame& frame : core.profiler.prev_frames) {
                        for(Profiler_Entry& entry : frame.steps) {
                            if(times.contains(entry.name)) {
                                times[entry.name] += entry.time;
                            } else {
                                order.push_back(entry.name);
                                times[entry.name] = entry.time;
                            }
                            total_time += entry.time;
                        }
                    }

                    for(std::string name : order) {
                        double time = times[name];
                        double percent = time / total_time * (base * base);
                        double avg = time / double(core.profiler.num_frames) * 1000.0f;

                        std::string entry_str = "\\b" + name + ": \\r" + to_base(avg, core.number_base, 2) + "ms (" + to_base(percent, core.number_base, 2) + "%)\n";
                        
                        string += entry_str;
                    }

                    return string;
                } else {
                    return str;
                }
            }
        );
    };

    std::function<void()> func_physics = [&gui_system]() {
        gui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        Panel_Widget::insert(2.0f, false);

        gui_system.buffer(vec4(4.0f));
        gui_system.position(PM_TOP_LEFT);
        Row_Widget::insert();
        Text_Widget::insert("", ALIGNMENT_LEFT, true,
            [](std::string str) -> std::string {
                int base = core.number_base;

                static double time = 0.0f;
                static double freq = 1.0f;
                static uint32_t num_frames = 0;

                time += core.delta_time;
                ++num_frames;

                if(time > freq || str == "") {
                    Physics_system& physics_system = ecs.get_system<Physics_system>();

                    double fps = double(num_frames) / time;
                    time = 0.0;
                    num_frames = 0;

                    std::string string;

                    std::vector<std::string> order;
                    std::unordered_map<std::string, double> times;
                    double total_time = 0.0f;

                    for(Profiler_Frame& frame : physics_system.profiler.prev_frames) {
                        for(Profiler_Entry& entry : frame.steps) {
                            if(times.contains(entry.name)) {
                                times[entry.name] += entry.time;
                            } else {
                                order.push_back(entry.name);
                                times[entry.name] = entry.time;
                            }
                            total_time += entry.time;
                        }
                    }

                    for(std::string name : order) {
                        double time = times[name];
                        double percent = time / total_time * (base * base);
                        double avg = time / double(core.profiler.num_frames) * 1000.0f;

                        std::string entry_str = "\\b" + name + ": \\r" + to_base(avg, core.number_base, 2) + "ms (" + to_base(percent, core.number_base, 2) + "%)\n";
                        
                        string += entry_str;
                    }

                    return string;
                } else {
                    return str;
                }
            }
        );
    };

    // start

    gui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 0.0f));

    auto mfunc = [timer = 0.0](Menu_Widget& widget) {

    };

    std::shared_ptr<Menu_Node> node(new Menu_Node{
        "",
        {
            Menu_Node("OPTION A"),
            Menu_Node("OPTION B", {
                Menu_Node("OPTION E"),
                Menu_Node("OPTION F"),
                Menu_Node("OPTION G"),
                Menu_Node("OPTION H", {
                    Menu_Node("OPTION E"),
                    Menu_Node("OPTION F"),
                    Menu_Node("OPTION G"),
                    Menu_Node("OPTION H"),
                    Menu_Node("OPTION I"),
                }),
                Menu_Node("OPTION I"),
            }),
            Menu_Node("OPTION C"),
            Menu_Node("OPTION D", {
                Menu_Node("OPTION E"),
                Menu_Node("OPTION F"),
                Menu_Node("OPTION G"),
                Menu_Node("OPTION H"),
                Menu_Node("OPTION I"),
            }),
        }
    });
    
    auto menu_func = [&mfunc, w = NULL_WIDGET, node](Button_Widget& widget) mutable {
        auto& gui_system = ecs.get_system<GUI_system>();

        bool pressed_siblings = false;
        bool hovered_siblings = false;
        auto& parent = gui_system.widgets[widget.parent];
        for(uint64_t sibling : parent->children) {
            if(sibling != widget.self) {
                Button_Widget* button;
                if(button = dynamic_cast<Button_Widget*>(gui_system.widgets[sibling].get())) {
                    if(button->pressed) {
                        pressed_siblings = true;
                    }
                    
                    if(button->hovered) {
                        hovered_siblings = true;
                    }
                }
            }
        }

        bool click_off = false;
        if(widget.pressed) click_off = true;

        if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT) && w != NULL_WIDGET) {
            click_off = true;

            std::vector<uint64_t> cpath;
            uint64_t current = w;
            while(true) {
                cpath.push_back(current);

                auto& cw = ecs.get_system<GUI_system>().widgets[current];
                if(cw->children.size()) current = cw->children[0];
                else break;
            }
            
            for(uint64_t c : cpath) {
                auto* cw = (Menu_Widget*)ecs.get_system<GUI_system>().widgets[c].get();

                if(includes(core.cursor_pos, vec4(cw->position, cw->position + cw->size))) {
                    Menu_Node* rootn = cw->root.get();
                    for(uint32_t p : cw->path) {
                        rootn = &rootn->children[p];
                    }

                    if(rootn->children[cw->hovered].children.size()) {
                        click_off = false;
                        break;
                    }
                }
            }
        }

        bool is_null = w == NULL_WIDGET;
        if((widget.pressed || (pressed_siblings && widget.hovered)) && is_null) {
            ecs.get_system<GUI_system>().current_widget = widget.self;
            w = Menu_Widget::insert(widget.position + vec2(0.0f, -64.0f), 0.6f, color_editor * 0.9f, 160, 16, 64, node, {});
        }

        if((hovered_siblings || click_off) && !is_null) {
            if(w != NULL_WIDGET) {
                std::vector<uint64_t> erase;
                uint64_t current = w;
                while(true) {
                    erase.push_back(current);

                    auto& cw = ecs.get_system<GUI_system>().widgets[current];
                    if(cw->children.size()) current = cw->children[0];
                    else break;
                }

                for(uint64_t c : erase) {
                    ecs.get_system<GUI_system>().delete_buffer.push_back(c);
                }

                widget.children.clear();
                w = NULL_WIDGET;
            }
        }

        if(w != NULL_WIDGET) {
            widget.pressed = true;
        }
    };

    Screen_Widget::insert();

    Relative_Widget::insert(
        [&gui_system](Relative_Widget& widget) {
            Screen_Widget* parent = (Screen_Widget*)gui_system.widgets[widget.parent].get();
            widget.position = vec2(parent->header, parent->size.y - parent->header);

            for(auto child : widget.children) {
                auto& child_widget = gui_system.widgets[child];

                child_widget->position = widget.position;
                child_widget->size = vec2(parent->size.x - parent->header, parent->header);
            }
        }
    );
    gui_system.position(PM_CENTER_LEFT);
    Row_Widget::insert();
    Button_Widget::insert(vec2(30, 16), color_editor, "File", menu_func);

    Button_Widget::insert(vec2(30, 16), color_editor, "Edit", menu_func);

    Button_Widget::insert(vec2(60, 16), color_editor, "Selection", menu_func);

    Button_Widget::insert(vec2(30, 16), color_editor, "View", menu_func);

    gui_system.step();
    gui_system.step();

    gui_system.position(PM_TOP_LEFT);

    Split_Widget::insert(LM_COLUMN, {Panel_Constraint(1.0f / 16.0f, PANEL_MODE_SIZE), Panel_Constraint(15.0f / 16.0f)});
    
    gui_system.buffer(vec4(4.0f));
    Panel_Widget::insert(2.0f, false);
    Column_Widget::insert();
    /**/
    Text_Widget::insert("", ALIGNMENT_LEFT, true,
        [](std::string str) -> std::string {
            Input_system& system = ecs.get_system<Input_system>();
            Transform& cam_tf = ecs.get_component<Transform>(system.player_camera);

            std::string pos_x = "\\cE55";
            std::string pos_y = "\\c5E5";
            std::string pos_z = "\\c55E";
            std::string neg_x = "\\c5EE";
            std::string neg_y = "\\cE5E";
            std::string neg_z = "\\cEE5";
            std::string white = "\\cFFF";

            std::string a = (cam_tf.position.x.sector >= 0) ? pos_x : neg_x;
            std::string b = (cam_tf.position.y.sector >= 0) ? pos_y : neg_y;
            std::string c = (cam_tf.position.z.sector >= 0) ? pos_z : neg_z;

            str = "\\bPOSITION: \\r\\h" + a + to_base(cam_tf.position.x.sector, core.number_base) + " " + to_base(cam_tf.position.x.fraction, core.number_base, 2) + " " + b + to_base(cam_tf.position.y.sector, core.number_base) + " " + to_base(cam_tf.position.y.fraction, core.number_base, 2) + " " + c + to_base(cam_tf.position.z.sector, core.number_base) + " " + to_base(cam_tf.position.z.fraction, core.number_base, 2) + white;
            return str;
        }
    );
    Text_Widget::insert("", ALIGNMENT_LEFT, true,
        [](std::string str) -> std::string {
            Input_system& system = ecs.get_system<Input_system>();
            Transform& cam_tf = ecs.get_component<Transform>(system.player_camera);

            vec3 dir = cam_tf.orientation * vec3(0.0f, 0.0f, -1.0f);

            std::string pos_x = "\\cE55";
            std::string pos_y = "\\c5E5";
            std::string pos_z = "\\c55E";
            std::string neg_x = "\\c5EE";
            std::string neg_y = "\\cE5E";
            std::string neg_z = "\\cEE5";
            std::string white = "\\cFFF";

            std::string a = (abs(dir.x) > abs(dir.y) && abs(dir.x) > abs(dir.z)) ? ((dir.x > 0.0) ? pos_x : neg_x) : white;
            std::string b = (abs(dir.y) > abs(dir.x) && abs(dir.y) > abs(dir.z)) ? ((dir.y > 0.0) ? pos_y : neg_y) : white;
            std::string c = (abs(dir.z) > abs(dir.x) && abs(dir.z) > abs(dir.y)) ? ((dir.z > 0.0) ? pos_z : neg_z) : white;

            str = "\\bDIRECTION: \\r\\h" + a + to_base(dir.x, core.number_base, 2) + " " + b + to_base(dir.y, core.number_base, 2) + " " + c + to_base(dir.z, core.number_base, 2);
            return str;
        }
    );
    Text_Widget::insert("\\bFPS: ", ALIGNMENT_LEFT, true,
        [](std::string str) -> std::string {
            static double time = 0.0f;
            static double freq = 1.0f;
            static uint32_t num_frames = 0;

            time += core.delta_time;
            ++num_frames;

            if(time > freq) {
                double fps = double(num_frames) / time;
                time = 0.0;
                num_frames = 0;

                return "\\bFPS: \\r" + to_base(float(fps), core.number_base, 3);
            } else {
                return str;
            }
        }
    );

    gui_system.step();

    gui_system.position(PM_TOP_RIGHT);
    gui_system.buffer(vec4(4.0f));
    Row_Widget::insert();
    Button_Widget::insert(vec2(30, 30), color_physics, "", 
    [](Button_Widget& self) {
        Input_system& is = ecs.get_system<Input_system>();
        if(self.pressed) {
            if(is.camera_mode == CAMERA_FREECAM) {
                // set to player
                is.set_camera_mode(CAMERA_PLAYER);
            } else if(is.camera_mode == CAMERA_PLAYER) {
                // set to freecam
                is.set_camera_mode(CAMERA_FREECAM);
            }
        }

        if(is.camera_mode == CAMERA_PLAYER) self.icon = vec4(38.0f, 6.0f, 10.0f, 10.0f);
        else if(is.camera_mode == CAMERA_FREECAM) self.icon = vec4(28.0f, 6.0f, 10.0f, 10.0f);
        self.icon_size = vec2(10.0f);
    });
    Button_Widget::insert(vec2(30, 30), color_physics, "", 
    [](Button_Widget& self) {
        Physics_system& system = ecs.get_system<Physics_system>();
        if(self.pressed) system.sim_active = !system.sim_active;

        if(system.sim_active) {
            self.icon = vec4(54.0f, 24.0f, 10.0f, 10.0f);
        } else {
            self.icon = vec4(54.0f, 14.0f, 10.0f, 10.0f);
        }
        self.icon_size = vec2(10.0f);
    });
    Button_Widget::insert(vec2(30, 30), color_physics, "", 
    [](Button_Widget& self) {
        if(self.pressed) {
            Physics_system& system = ecs.get_system<Physics_system>();
            if(!system.sim_active) {
                system.physics_loop();
            }
        }

        self.icon = vec4(44.0f, 54.0f, 10.0f, 10.0f);
        self.icon_size = vec2(10.0f);
    });
    gui_system.step();
    gui_system.step();

    Split_Widget::insert(LM_ROW, {Panel_Constraint(0.5f, PANEL_MODE_SIZE), Panel_Constraint(0.5f)});

    Split_Widget::insert(LM_COLUMN, {Panel_Constraint(0.5f), Panel_Constraint(0.5f)});
    gui_system.buffer(vec4(0.0f));

    Panel_Widget::insert(2.0f, false);
    Tab_Widget::insert(20.0f, 0.0f, {
        Tab("Settings", 80.0f, color_editor, func_settings), 
        Tab("Selection", 80.0f, color_physics, func_selection), 
        Tab("Lua", 80.0f, color_lua, func_lua), 
    });
    gui_system.step();

    Panel_Widget::insert(2.0f, false);
    Tab_Widget::insert(20.0f, 0.0f, {
        Tab("Global", 80.0f, color_editor, func_global), 
        Tab("Physics", 80.0f, color_physics, func_physics), 
    });
    gui_system.step();
    gui_system.step();

    Panel_Widget::insert(2.0f, false);
    gui_system.buffer(vec4(0.0f));
    Render_Widget::insert();

    gui_system.step();
}

void Core::init() {
    start_time = get_time();
    prev_time = start_time;
    current_time = start_time;

    window = Window({start_x, start_y});
    window.init_callbacks();

    stbi_set_flip_vertically_on_load(true);
    shaders.emplace("chunk_shader", std::make_shared<Shader>(Shader("resources/shaders/chunk.vert", "resources/shaders/chunk.frag")));
    shaders.emplace("structure_shader", std::make_shared<Shader>(Shader("resources/shaders/structure_mesh.vert", "resources/shaders/structure_mesh.frag")));
    shaders.emplace("text_shader", std::make_shared<Shader>(Shader("resources/shaders/text.vert", "resources/shaders/text.frag")));
    shaders.emplace("screen_shader", std::make_shared<Shader>(Shader("resources/shaders/screen.vert", "resources/shaders/screen.frag")));
    shaders.emplace("screen_depth_shader", std::make_shared<Shader>(Shader("resources/shaders/screen_depth.vert", "resources/shaders/screen_depth.frag")));
    shaders.emplace("particle_shader", std::make_shared<Shader>(Shader("resources/shaders/particles.vert", "resources/shaders/particles.geom", "resources/shaders/particles.frag")));
    shaders.emplace("cloud_particle_shader", std::make_shared<Shader>(Shader("resources/shaders/cloud_particles.vert", "resources/shaders/cloud_particles.frag")));
    shaders.emplace("nebula_particle_shader", std::make_shared<Shader>(Shader("resources/shaders/nebula_particles.vert", "resources/shaders/nebula_particles.frag")));
    shaders.emplace("color_shader", std::make_shared<Shader>(Shader("resources/shaders/color.vert", "resources/shaders/color.frag")));
    shaders.emplace("color_vertex_shader", std::make_shared<Shader>(Shader("resources/shaders/color_vertex.vert", "resources/shaders/color_vertex.frag")));
    shaders.emplace("atmo_shader", std::make_shared<Shader>(Shader("resources/shaders/atmo.vert", "resources/shaders/atmo.frag")));
    shaders.emplace("black_hole_shader", std::make_shared<Shader>(Shader("resources/shaders/black_hole.vert", "resources/shaders/black_hole.frag")));
    shaders.emplace("galaxy_shader", std::make_shared<Shader>(Shader("resources/shaders/galaxy.vert", "resources/shaders/galaxy.frag")));
    shaders.emplace("galaxy_shader_elliptical", std::make_shared<Shader>(Shader("resources/shaders/galaxy2.vert", "resources/shaders/galaxy2.frag")));
    shaders.emplace("noise_clouds", std::make_shared<Shader>(Shader("resources/shaders/noise_clouds.comp")));
    shaders.emplace("model_rigged_shader", std::make_shared<Shader>(Shader("resources/shaders/model_rigged.vert", "resources/shaders/model_rigged.geom", "resources/shaders/model_rigged.frag")));
    shaders.emplace("model_shader", std::make_shared<Shader>(Shader("resources/shaders/model.vert", "resources/shaders/model.frag")));
    shaders.emplace("model_anim_shader", std::make_shared<Shader>(Shader("resources/shaders/model_anim.vert", "resources/shaders/model_anim.frag")));
    shaders.emplace("model_batch", std::make_shared<Shader>(Shader("resources/shaders/model_batch.vert", "resources/shaders/model_batch.frag")));
    shaders.emplace("gui_shader", std::make_shared<Shader>(Shader("resources/shaders/ui.vert", "resources/shaders/ui.frag")));
    shaders.emplace("torus_shader", std::make_shared<Shader>(Shader("resources/shaders/torus.vert", "resources/shaders/torus.frag")));
    shaders.emplace("atmo_shader_torus", std::make_shared<Shader>(Shader("resources/shaders/atmo_torus.vert", "resources/shaders/atmo_torus.frag")));
    shaders.emplace("ocean_shader", std::make_shared<Shader>(Shader("resources/shaders/ocean.vert", "resources/shaders/ocean.tesc", "resources/shaders/ocean.tese", "resources/shaders/ocean.geom", "resources/shaders/ocean.frag")));
    shaders.emplace("structure_model_shader", std::make_shared<Shader>(Shader("resources/shaders/structure_model.vert", "resources/shaders/structure_model.frag")));
    shaders.emplace("grid_shader", std::make_shared<Shader>(Shader("resources/shaders/grid.vert", "resources/shaders/grid.frag")));
    shaders.emplace("gas_giant_shader", std::make_shared<Shader>(Shader("resources/shaders/gas_giant.vert", "resources/shaders/gas_giant.frag")));
    shaders.emplace("sun_shader", std::make_shared<Shader>(Shader("resources/shaders/sun.vert", "resources/shaders/sun.frag")));
    shaders.emplace("ring_shader", std::make_shared<Shader>(Shader("resources/shaders/ring.vert", "resources/shaders/ring.frag")));
    shaders.emplace("ray_shader", std::make_shared<Shader>(Shader("resources/shaders/screen_ray.vert", "resources/shaders/screen_ray.frag")));
    shaders.emplace("color_map", std::make_shared<Shader>(Shader("resources/shaders/model_map.vert", "resources/shaders/model_map.frag")));

    textures.emplace("tilesheet", std::make_shared<Texture>(Texture("resources/textures/tilesheet.png", {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, 4)));
    textures.emplace("particles", std::make_shared<Texture>(Texture("resources/textures/particles.png", {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE})));
    textures.emplace("averie", std::make_shared<Texture>(Texture("resources/textures/averie.png", {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE})));
    textures.emplace("icons", std::make_shared<Texture>(Texture("resources/textures/ui.png", {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE})));
    
    uint32_t size = 64;

    textures.emplace("density_map_perlin", std::make_shared<Texture>(Texture({size, size, size}, GL_TEXTURE_3D, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE})));
    textures.emplace("density_map_voronoi", std::make_shared<Texture>(Texture({size, size, size}, GL_TEXTURE_3D, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE})));
    textures.emplace("density_map_voronoi2", std::make_shared<Texture>(Texture({size, size, size}, GL_TEXTURE_3D, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE})));

    Texture& texture_perlin = *textures["density_map_perlin"];
    Texture& texture_voronoi = *textures["density_map_voronoi"];
    Texture& texture_voronoi2 = *textures["density_map_voronoi2"];

    shaders["noise_clouds"]->use();

    //

    texture_perlin.bind_image(0, 0, GL_RGBA8);

    glUniform3f(0, 1.0f, 1.0f, 1.0f);
    glUniform3ui(1, 8, 8, 8);
    glUniform1f(2, 8.0f);
    glUniform1i(3, 0);
    
    Shader::dispatch_compute(glm::uvec3{size, size, size});

    glBindTexture(GL_TEXTURE_3D, textures["density_map_perlin"]->id);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LOD, 6);
    glGenerateMipmap(GL_TEXTURE_3D);

    //
    
    texture_voronoi.bind_image(0, 0, GL_RGBA8);

    glUniform3f(0, 1.0f, 1.0f, 1.0f);
    glUniform3ui(1, 8, 8, 8);
    glUniform1f(2, 8.0f);
    glUniform1i(3, 1);
    
    Shader::dispatch_compute(glm::uvec3{size, size, size});

    glBindTexture(GL_TEXTURE_3D, textures["density_map_perlin"]->id);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LOD, 6);
    glGenerateMipmap(GL_TEXTURE_3D);

    //
    
    texture_voronoi2.bind_image(0, 0, GL_RGBA8);

    glUniform3f(0, 1.0f, 1.0f, 1.0f);
    glUniform3ui(1, 8, 8, 8);
    glUniform1f(2, 8.0f);
    glUniform1i(3, 2);
    
    Shader::dispatch_compute(glm::uvec3{size, size, size});

    glBindTexture(GL_TEXTURE_3D, textures["density_map_perlin"]->id);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LOD, 6);
    glGenerateMipmap(GL_TEXTURE_3D);

    // lua
    core.lua_wrapper.expose(lua_func, "summon_cube");
}

struct Timer {
    steady_clock::time_point last_time;
    std::vector<double> timesteps;

    Timer();

    void get_elapsed_time(bool overwrite = false);

    void output();
};

Timer::Timer() {
    last_time = steady_clock::now();
}

void Timer::get_elapsed_time(bool overwrite) {
    steady_clock::time_point current_time = steady_clock::now();
    steady_clock::duration duration = current_time - last_time;

    if(overwrite) {
        last_time = current_time;
    }

    last_time = current_time;

    //timesteps.push_back(double(duration.count()) * steady_clock::period::num / steady_clock::period::den);
}

void Timer::output() {
    std::cout << "TIMER\n";

    for(int i = 0; i < timesteps.size(); ++i) {
        std::cout << i << ": " << timesteps[i] << "\n";
    }

    std::cout << "\n";
}

struct f_sort {
    bool operator()(std::pair<float, Particle_data> a, std::pair<float, Particle_data> b) {
        return a.first < b.first;
    }
};

/*
struct voronoi_cell {
    vec3 pos;
    int plate;
};

struct tectonic_plate {
    vec3 movement_dir;
    vec3 pos;
    vec3 offset;
    float elev;
    float weight;
};
*/

struct flow_edge {
    uint32_t a;
    uint32_t b;
};

struct flow_node {
    vec3 position;
    float elevation;
};

std::vector<mat3> matrices = {
    rotate_to(vec3(0, 0, 1), vec3(1, 0, 0)),
    rotate_to(vec3(0, 0, 1), vec3(-1, 0, 0)),
    rotate_to(vec3(0, 0, 1), vec3(0, 1, 0)),
    rotate_to(vec3(0, 0, 1), vec3(0, -1, 0)),
    rotate_to(vec3(0, 0, 1), vec3(0, 0, 1)),
    rotate_to(vec3(0, 0, 1), vec3(0, 0, -1)),
};

struct Color_texture {
    uint8_t* data;
    ivec3 size;

    Color_texture(std::string path) {
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load(path.data(), &size.x, &size.y, &size.z, 0);
    }

    ~Color_texture() {
        stbi_image_free(data);
    }

    vec3 sample(vec2 pos, bool nearest = true);
};

Color_texture land_biome_texture("resources/textures/biomes.png");
Color_texture ocean_biome_texture("resources/textures/biomes_ocean.png");

void create_platform(pvec3 position, mat3 orientation, vec3 size);
void create_sphere(pvec3 position, mat3 orientation, vec3 axes, vec3 color);
void create_planet(uint32_t seed, pvec3 position, mat3 orientation, vec3 dimensions, std::vector<crater_population> populations, std::vector<vec3> colors, float amplitude, float noise_freq, float noise_offset, float age_value, float ejecta_value, float blend_value);

int main() {
    if(glfwInit() == GLFW_FALSE) {
        std::cout << "ERROR: GLFW failed to load.\n";
        exit(-1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    core.init();
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    ecs.entity_manager.init();

    ecs.register_system<Physics_system>();
    ecs.register_system<GUI_system>();
    ecs.register_system<Input_system>();
    ecs.register_system<Voxel_system>();
    ecs.register_system<Building_system>();
    ecs.register_system<Particle_system>();
    ecs.register_system<Render_system>();

    start_ui();

    Render_system& render_system = ecs.get_system<Render_system>();
    Input_system& input_system = ecs.get_system<Input_system>();
    Physics_system& physics_system = ecs.get_system<Physics_system>();

    // gravity and light
    pnum light_year = pow(2, 48);
    pvec3 sun_pos = pvec3(light_year * 50000.0f, light_year * -30000.0f, light_year * 40000.0f);
    render_system.sun_pos = sun_pos;

    float sun_rad = 108 * 0x100000;
    vec3 dir = normalize(vec3(1.0f, 0.6f, 0.3f));

    pvec3 origin = pvec3(dir * (9288.0f * 0x100000));
    origin += sun_pos;

    vec3 uv = core.random.unit_vector();
    uv = core.random.unit_vector();

    mat3 orientation = identity<mat3>();

    vec3 sun_direction = render_system.light_direction;

    physics_system.gravity_center = origin;
    physics_system.gravity_orientation = identity<mat3>();
    physics_system.gravity_aspect = vec3(1.0f);

    render_system.atmo_center = origin;

    // planet

    Transform planet_transform;
    planet_transform.orientation = identity<mat3>();
    planet_transform.position = origin;
    
    Voxel_field planet_field;
    planet_field.planet_radius = 1.0477f * 0x100000;
    
    //planet_field.populations.push_back({0.05f, 0.175f, 4.0f, 20, 2});
    //planet_field.populations.push_back({0.015f, 0.05f, 4.0f, 2500, 75});
    //planet_field.populations.push_back({0.002f, 0.015f, 4.0f, 10000, 300});
    //planet_field.create_craters();

    uint32_t planet_entity = ecs.insert_entity();
    ecs.insert_component(planet_entity, planet_transform);
    ecs.insert_component_move(planet_entity, planet_field);

    // moon

    planet_field.planet_radius = 0.38f * 0x100000;
    planet_transform.position += pvec3(normalize(vec3(0.8f, 0.5f, -0.3f)) * 2400000.0f * 10.0f);

    //planet_entity = ecs.insert_entity();
    //ecs.insert_component(planet_entity, planet_transform);
    //ecs.insert_component_move(planet_entity, planet_field);
    

    // camera
    
    uint32_t entity = ecs.insert_entity();
    uint32_t camera_entity = entity;
    Camera camera;
    camera.fov = 90.0f;
    camera.near_plane = 0.01f;
    camera.framebuffer = 0;

    Transform tf;
    tf.orientation = identity<mat3>();

    tf.position = pvec3(pnum(54975581420389237, 152.502), pnum(-32985348814323974, 34.7989), pnum(43980465120518067, 190.529)); // on planet
    //tf.position = pvec3(pnum(0xC3500001E1E74D, 0xE3), pnum(-0x752FFFFEDE9C34, 0xA1), pnum(0x9C400000908FE7, 0xE4)); // in space

    render_system.player_camera = entity;
    input_system.player_camera = entity;

    ecs.insert_component(entity, camera);
    ecs.insert_component(entity, tf);

    vec3 forward = sun_direction;
    vec3 up = vec3(0, 0, 1);

    tf.orientation = create_rot_mat(up, forward);

    entity = ecs.insert_entity();
    uint32_t sun_entity = entity;
    render_system.sun_camera = entity;
    camera.fov = 256.0f;
    camera.near_plane = -256.0f;
    camera.far_plane = 256.0f;
    camera.orthogonal = true;
    camera.framebuffer = 2;
    ecs.insert_component(entity, camera);
    ecs.insert_component(entity, tf);
    
    render_system.sun_camera = sun_entity;

    std::shared_ptr<Mesh> mesh(new Mesh);
    std::vector<Mesh_vertex> mesh_vertices;
    std::vector<uint32_t> mesh_indices;
    int num_squares = 48;
    
    for(int i = 0; i < 6; ++i) {
        mat3 matrix = matrices[i];
        
        uint32_t start = mesh_vertices.size();

        for(int y = 0; y < num_squares + 1; ++y) {
            for(int x = 0; x < num_squares + 1; ++x) {
                vec3 pos = vec3(float(x) / num_squares * 2 - 1, float(y) / num_squares * 2 - 1, 1);
                pos = matrix * pos;

                Mesh_vertex v;
                v.position = normalize(pos);
                v.normal = v.position;

                mesh_vertices.push_back(v);
            }
        }
        
        for(int y = 0; y < num_squares; ++y) {
            for(int x = 0; x < num_squares; ++x) {
                ivec2 a = {x, y};
                ivec2 b = {x + 1, y};
                ivec2 c = {x, y + 1};
                ivec2 d = {x + 1, y + 1};

                uint32_t ia = a.x + a.y * (num_squares + 1);
                uint32_t ib = b.x + b.y * (num_squares + 1);
                uint32_t ic = c.x + c.y * (num_squares + 1);
                uint32_t id = d.x + d.y * (num_squares + 1);

                mesh_indices.push_back(start + ia);
                mesh_indices.push_back(start + ib);
                mesh_indices.push_back(start + id);
                mesh_indices.push_back(start + ia);
                mesh_indices.push_back(start + id);
                mesh_indices.push_back(start + ic);
            }
        }
    }

    mesh->add_vertices(mesh_vertices, mesh_indices);
    mesh->load_buffer();

    core.meshes.emplace("planet", mesh);

    GUI_system& gui_system = ecs.get_system<GUI_system>();

    /*
    gui_system.add_window(ivec2(20, 20), ivec2(240, 180), "Test Window");
    gui_system.add_text(fps_callback);
    gui_system.add_text(physics_callback);
    gui_system.add_text(position_callback);
    gui_system.add_text(mode_callback);
    gui_system.add_input(200, "02468");
    */

    glfwSetInputMode(core.window.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    //create_sphere(origin, identity<mat3>(), vec3(1200000.0f), vec3(1.0f, 0.45f, 0.75f));

    // planet
    float amplitude;
    vec3 color_a;
    vec3 color_b;
    vec3 color_c;
    vec3 color_d;
    vec3 dimensions;
    pvec3 position;
    mat3 planet_ori;
    uint32_t seed = 0;
    float blend_value = 0.25f;
    float age_value = 1.0f;
    float ejecta_value = 0.4f;
    float noise_offset = 0.0f;
    float noise_freq = 0.5f;

    // red - 0.0
    // yellow - 1.0
    // green - 2.0
    // cyan - 3.0
    // blue - 4.0
    // magenta - 5.0
    // red - 6.0

    std::vector<crater_population> populations;

    // blue moon
    populations.clear();
    populations.push_back({0.05f, 0.175f, 4.0f, 20, 2});
    populations.push_back({0.015f, 0.05f, 4.0f, 2500, 75});
    populations.push_back({0.002f, 0.015f, 4.0f, 50000, 300});
    std::vector<vec3> colors = {
        hsv_color(0.05, 0.45, 0.25),
        hsv_color(0.05, 0.45, 0.2),
        hsv_color(0.05, 0.45, 0.4),
        hsv_color(0.05, 0.45, 0.4)
        /*
        hsv_color(0.25, 0.65, 0.4),
        hsv_color(0.25, 0.65, 0.325),
        hsv_color(0.25, 0.65, 0.55),
        hsv_color(0.25, 0.65, 0.55)
        */
    };

    seed = 0xB3;
    position = origin + pvec3(normalize(vec3(0.8f, 0.5f, -0.3f)) * 2400000.0f * 10.0f);
    orientation = rotate_to(vec3(1.0f, 0.0f, 0.0f), normalize(origin - position)); 
    dimensions = vec3(base_radius * 0.15f);
    amplitude = 5000;
    noise_freq = 0.3f;
    noise_offset = 0.0f;
    age_value = 1.0f;
    ejecta_value = 0.25f;
    blend_value = 0.125f;

    //create_planet(seed, position, orientation, dimensions, populations, colors, amplitude, noise_freq, noise_offset, age_value, ejecta_value, blend_value);

    //create_sphere(origin, identity<mat3>(), vec3(base_radius), hex_color(0x0A2044));

    //

    //networking_core.connect("7F000001:EEEE");

    Time time;

    float fps = 15.0f;

    while(core.game_running) {
        core.profiler.start();

        //

        ecs.num_lookups = 0;

        //while(time.get_elapsed_time(false) < 1.0f / fps) {};

        float t = time.get_elapsed_time(true);
        

        core.random();
        
        core.delta_time = core.get_delta_time();
        
        glfwPollEvents();

        core.handle_events();

        double time = get_time();

        core.profiler.step("EVENTS");

        //networking_core.call();
        
        core.profiler.step("NETWORKING");

        for(std::size_t& code : ecs.system_manager.call_order) {
            auto& system = ecs.system_manager.systems[code];
            system->call();

            core.profiler.step(std::string(typeid(*system).name()));
        }

        core.events.clear();
        
        if(glfwWindowShouldClose(core.window.window)) {
            core.game_running = false;
        }

        core.profiler.loop();
    }

    glfwTerminate();
    core.thread_pool.stop = true;
    return 0;
}

vec3 Color_texture::sample(vec2 pos, bool nearest) {
    if(nearest) {
        ivec2 origin = floor(pos * vec2(size.xy()));
        ivec2 o0 = origin;
        o0 = clamp(o0, ivec2(0), size.xy() - 1);
        int i0 = o0.y * size.x + o0.x;

        vec3 c0 = vec3(data[i0 * size.z], data[i0 * size.z + 1], data[i0 * size.z + 2]) / 255.0f;
        return c0;
    } else {
        ivec2 origin = floor(pos * vec2(size.xy()) - 0.5f);
        vec2 blend = fract(pos * vec2(size.xy()) - 0.5f);

        ivec2 o0 = origin;
        ivec2 o1 = origin + ivec2(1, 0);
        ivec2 o2 = origin + ivec2(0, 1);
        ivec2 o3 = origin + ivec2(1, 1);
        o0 = clamp(o0, ivec2(0), size.xy() - 1);
        o1 = clamp(o1, ivec2(0), size.xy() - 1);
        o2 = clamp(o2, ivec2(0), size.xy() - 1);
        o3 = clamp(o3, ivec2(0), size.xy() - 1);

        int i0 = o0.y * size.x + o0.x;
        int i1 = o1.y * size.x + o1.x;
        int i2 = o2.y * size.x + o2.x;
        int i3 = o3.y * size.x + o3.x;

        vec3 c0 = vec3(data[i0 * size.z], data[i0 * size.z + 1], data[i0 * size.z + 2]) / 255.0f;
        vec3 c1 = vec3(data[i1 * size.z], data[i1 * size.z + 1], data[i1 * size.z + 2]) / 255.0f;
        vec3 c2 = vec3(data[i2 * size.z], data[i2 * size.z + 1], data[i2 * size.z + 2]) / 255.0f;
        vec3 c3 = vec3(data[i3 * size.z], data[i3 * size.z + 1], data[i3 * size.z + 2]) / 255.0f;

        vec3 c01 = mix(c0, c1, blend.x);
        vec3 c23 = mix(c2, c3, blend.x);
        vec3 result = mix(c01, c23, blend.y);

        return result;
    }
};

void create_platform(pvec3 position, mat3 orientation, vec3 size) {
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

    //collider.allow_rotation = true;
    //collider.allow_gravity = true;
    collider.is_static = true;
    collider.collision_shapes.clear();
    collider.collision_shapes.resize(1);
    collider.collision_shapes[0].collision_shape = std::make_shared<Collision_shape>(shape);
    Physics_system::initialize_collider(collider);

    ecs.insert_component(entity, tf);
    ecs.insert_component(entity, collider);

    vec3 color = vec3(0.35f);
    
    create_cube_mesh(entity, size * 0.5f, color, {0, 96, 32, 32});
}

void create_sphere(pvec3 position, mat3 orientation, vec3 axes, vec3 color) {
    auto get_normal = [&](vec3 pos, vec3 axes) {
        return normalize(vec3(pos.x / (axes.x * axes.x), pos.y / (axes.y * axes.y), pos.z / (axes.z * axes.z)));
    };

    auto project = [&](vec3 normal, vec3 axes) {
        return normalize(normal / axes) * axes;
    };

    auto sample = [&](vec3 v) {
        return Noise_gen::perlin_noise(v, 0.35f, 6, 0xB3);
    };
    
    std::cout << "finished setup\n";
    auto create_m = [&](float width, uint32_t tiles, vec3 origin, mat3 orientation, vec3 p_size, pvec3 position, uint32_t seed, mat3 planet_ori) {
        Collider cl;
        cl.allow_rotation = false;
        cl.allow_gravity = false;
        cl.is_static = true;

        std::vector<Mesh_vertex> mesh_vertices;
        std::vector<uint32_t> mesh_indices;

        std::vector<float> values;

        auto find_pos = [&](ivec2 pos) {
            vec3 mpos = origin + vec3(vec2(pos) / float(tiles) * width, 0);
            mpos = orientation * mpos;
            return normalize(mpos);
        };

        float diff = width / tiles * max(max(p_size.x, p_size.y), p_size.z);

        for(int y = 0; y < tiles + 1; ++y) {
            for(int x = 0; x < tiles + 1; ++x) {
                vec3 pos = find_pos({x, y});
                values.push_back(0.0f);
            }
        }

        float amplitude = axes.x * 0.01f;

        std::vector<vec3> mesh_colors;
        std::vector<vec3> mesh_normals;
        for(int y = 0; y < tiles + 1; ++y) {
            float multiplier_y = -1.0f;
            int y1 = y + 1;

            for(int x = 0; x < tiles + 1; ++x) {
                float multiplier_x = -1.0f;
                int x1 = x + 1;

                ivec2 vvi = ivec2(x, y);

                vec3 pos = find_pos(vvi);
                vec3 p_i = pos * p_size;

                mat3 ori = rotate_to(vec3(0, 0, 1), normalize(p_i));

                vec3 delta_x = ori * vec3(1, 0, 0);
                vec3 delta_y = ori * vec3(0, 1, 0);

                vec3 p_x = project(p_i + delta_x * diff, p_size);
                vec3 p_y = project(p_i + delta_y * diff, p_size);

                float vi = values[vvi.y * (tiles + 1) + vvi.x];

                vec3 pos_i = p_i + get_normal(p_i, p_size) * sample(normalize(p_i)) * amplitude;
                vec3 pos_x = p_x + get_normal(p_x, p_size) * sample(normalize(p_x)) * amplitude;
                vec3 pos_y = p_y + get_normal(p_y, p_size) * sample(normalize(p_y)) * amplitude;

                vec3 normal = cross(pos_x - pos_i, pos_y - pos_i);
                normal = normalize(normal);

                mesh_normals.push_back(normal);

                mesh_colors.push_back(color);
            }
        }
        
        for(int y = 0; y < tiles; ++y) {
            for(int x = 0; x < tiles; ++x) {
                ivec2 pos = {x, y};

                for(ivec2 v : indices) {
                    ivec2 new_pos = pos + v;

                    vec3 normal = mesh_normals[new_pos.y * (tiles + 1) + new_pos.x];
                    vec3 color = mesh_colors[new_pos.y * (tiles + 1) + new_pos.x];
                    float h = values[new_pos.y * (tiles + 1) + new_pos.x];

                    Mesh_vertex mv;
                    mv.position = find_pos(new_pos) * p_size;
                    mv.position += get_normal(mv.position, p_size) * sample(normalize(normalize(mv.position))) * amplitude;
                    mv.normal = normal;
                    //mv.position += get_normal(mv.position, p_size) * h;
                    
                    //mv.position = orientation * mv.position;
                    //mv.normal = orientation * normal;
                    mv.bone_weights = vec4(color, 0.0f);

                    mesh_indices.push_back(mesh_vertices.size());
                    mesh_vertices.push_back(mv);
                }
            }
        }

        // texture mesh
        vec3 mesh_avg = vec3(0.0f);
        for(int triangle = 0; triangle < mesh_indices.size() / 3; ++triangle) {
            Mesh_vertex& v0 = mesh_vertices[triangle * 3];
            Mesh_vertex& v1 = mesh_vertices[triangle * 3 + 1];
            Mesh_vertex& v2 = mesh_vertices[triangle * 3 + 2];

            mesh_avg += v0.position;
            mesh_avg += v1.position;
            mesh_avg += v2.position;

            vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));
            vec3 n = normal;

            normal = normalize(round(normal / max(abs(normal.x), max(abs(normal.y), abs(normal.z)))));

            vec3 tex_x = cross(normal, vec3(0, 1, 0));
            if(length(tex_x) == 0.0f) tex_x = cross(normal, vec3(0, 0, 1));
            tex_x = normalize(tex_x);
            vec3 tex_y = normalize(cross(normal, tex_x));

            vec3 avg_pos = v0.position + v1.position + v2.position;
            avg_pos /= 3.0f;

            float tc_multiplier = 0.5f;

            v0.tex_coords = vec2(dot(tex_x, v0.position) * tc_multiplier, dot(tex_y, v0.position) * tc_multiplier);
            v1.tex_coords = vec2(dot(tex_x, v1.position) * tc_multiplier, dot(tex_y, v1.position) * tc_multiplier);
            v2.tex_coords = vec2(dot(tex_x, v2.position) * tc_multiplier, dot(tex_y, v2.position) * tc_multiplier);

            // change
            //v0.normal = n;
            //v1.normal = n;
            //v2.normal = n;
        }
        if(mesh_indices.size()) mesh_avg /= mesh_indices.size();

        for(Mesh_vertex& v : mesh_vertices) {
            v.position -= mesh_avg;
        }
        
        for(int triangle = 0; triangle < mesh_indices.size() / 3; ++triangle) {
            Mesh_vertex& v0 = mesh_vertices[triangle * 3];
            Mesh_vertex& v1 = mesh_vertices[triangle * 3 + 1];
            Mesh_vertex& v2 = mesh_vertices[triangle * 3 + 2];

            Collision_shape shape;
            cl.mass = 0.0f;
            shape.mass = 0.0f;
            shape.radius = 0.0f;
            shape.vertices = {
                v0.position,
                v1.position,
                v2.position,
            };
            
            cl.collision_shapes.push_back(Convex_collider());
            cl.collision_shapes[cl.collision_shapes.size() - 1].collision_shape = std::make_shared<Collision_shape>(shape);
        }
        
        Transform t;
        t.position = mesh_avg;
        t.orientation = planet_ori;

        t.position = apply_matrix(planet_ori, t.position);
        t.position += position;

        Mesh_component mc;
        std::shared_ptr<Mesh> mesh(new Mesh);

        mesh->add_vertices(mesh_vertices, mesh_indices);
        mesh->load_buffer();
        mc.mesh = mesh;
        std::shared_ptr<Texture> texture = create_image_texture("resources/textures/tilesheet.png", ivec4(0, 96, 32, 32)); // ivec4(48, 128, 16, 16)
        
        mc.texture = texture;

        uint32_t entity = ecs.insert_entity();
        ecs.insert_component(entity, t);
        ecs.insert_component(entity, mc);
        ecs.insert_component(entity, cl);   

        Collider& ccl = ecs.get_component<Collider>(entity);
        Transform& tf = ecs.get_component<Transform>(entity);

        Physics_system& ps = ecs.get_system<Physics_system>();
        //ps.create_bounding_box(ccl, tf);
        //ccl.create_BVH(2, &tf);
    };

    uint32_t seed = core.random.next();

    
    double setup_time = get_time();

    int split = 8;
    float size = 1.0f;
    uint32_t tiles_per_split = 16;

    for(int i = 0; i < 6; ++i) {
        mat3 matrix = matrices[i];
        for(int x = 0; x < split; ++x) {
            for(int y = 0; y < split; ++y) {
                vec3 origin = vec3(size * 2.0f / split * x - size, size * 2.0f / split * y - size, size);
                //std::cout << i << " " << x << " " << y << "\n";
                
                //if(i == 0 && x < 16 && y < 16) 
                create_m(size * 2.0f / (float)split, tiles_per_split, origin, matrix, axes, position, seed, orientation);
            }
        }
    }
};

vec2 map_project(ivec2 position, ivec2 size) {
    vec2 coords = (vec2(position) + 0.5f) / vec2(size.xy());
    coords = coords * 2.0f - 1.0f;

    float theta = asin(coords.y);

    // lambda
    float longitude = (M_PI * coords.x) / cos(theta);

    // phi
    float latitude = asin((2.0f * theta + sin(2.0f * theta)) / M_PI);

    vec2 pos = vec2(longitude, latitude);
    pos.x = pos.x / M_PI * 0.5f + 0.5f;
    pos.y = pos.y / M_PI + 0.5f;

    return pos;
}

vec3 wrap(vec2 position) {
    float angle_y = (position.y - 0.5f) * M_PI;
    float angle_x = position.x * 2.0f * M_PI;

    float cos_y = cos(angle_y);
    vec3 pos = vec3(cos(angle_x) * cos_y, sin(angle_x) * cos_y, sin(angle_y));

    return pos;
}

void create_planet(uint32_t seed, pvec3 position, mat3 orientation, vec3 dimensions, std::vector<crater_population> populations, std::vector<vec3> colors, float amplitude, float noise_freq, float noise_offset, float age_value, float ejecta_value, float blend_value) {
    Physics_system& ps = ecs.get_system<Physics_system>();
    
    double start_time = get_time();

    Random random(seed);

    std::unordered_map<ivec3, std::vector<uint32_t>, Hash_coord> partition;
    int num_buckets = 12;

    float avg_dimension = (dimensions.x + dimensions.y + dimensions.z) / 3.0f;
    float max_dimension = max(max(dimensions.x, dimensions.y), dimensions.z);
    vec3 ratio = dimensions / avg_dimension;

    int max_bucket = floor((max_dimension / avg_dimension) * num_buckets);

    struct crater {
        vec3 position;
        float radius = 0.1f;
        float ejecta = 0.0f;
        float age = 0.0f;
        float height = 0.0f;
    };

    std::vector<crater> craters;

    auto smooth_min = [](float a, float b, float k) {
        if(k < 0.0f) {
            a = -a;
            b = -b;
            k = -k;
            
            float r = exp2(-a/k) + exp2(-b/k);
            return k*log2(r);
        } else {
            float r = exp2(-a/k) + exp2(-b/k);
            return -k*log2(r);
        }
    };

    auto crater_func = [&](float f, float depth, float steepness_inner, float steepness_outer, float rim_width) {
        float walls = (f * f - 1) * steepness_inner;
        float rim = (f - (1.0f + rim_width));
        rim = rim * rim * steepness_outer;

        float result = smooth_min(walls, rim, 0.05f);

        return result;
    };

    auto big_crater_func = [&](float f, float depth, float steepness_inner, float steepness_outer, float rim_width, float steepness_center, float width_center) {
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

        float peak = smooth_min(peak_pos, peak_neg, 0.05f);

        float result = smooth_min(walls, rim, 0.05f);
        if(steepness_center != 0.0f) result = smooth_min(result, peak, -0.05f);
        result = smooth_min(result, crater_floor, -0.05f);

        return result;
    };

    auto bias_func = [](float x, float bias) {
        float k = pow(1 - bias, 3);
        return x * k / (x * k - x + 1);
    };
    
    int num_prev = 0;
    for(int j = 0; j < populations.size(); ++j) {
        crater_population& pop = populations[j];

        for(int i = 0; i < pop.num_craters; ++i) {
            crater c;
            c.position = random.unit_vector() * ratio;
            float r = abs(random());
            r = bias_func(r, 0.6f);

            c.radius = r * (pop.max_size - pop.min_size) + pop.min_size;

            if(pop.num_craters - i < pop.num_ejecta) {
                c.ejecta = c.radius * (5.0f + 15.0f * abs(random()));
                //c.ejecta = c.radius * 9.0f;

                c.age = float(pop.num_craters - i) / pop.num_ejecta;
                c.age = pow(c.age, age_value);
                //c.age = pow(c.age, 4.0f);
            }

            craters.push_back(c);



            float max_rad = max(c.radius * 1.5f, c.ejecta);

            vec3 mmin = c.position - max_rad;
            vec3 mmax = c.position + max_rad;
            ivec3 rmin = floor(mmin * float(num_buckets));
            ivec3 rmax = floor(mmax * float(num_buckets));
            //rmin = clamp(rmin, ivec3(-max_bucket - 1), ivec3(max_bucket));
            //rmax = clamp(rmax, ivec3(-max_bucket - 1), ivec3(max_bucket));

            for(int z = rmin.z; z <= rmax.z; ++z) {
                for(int y = rmin.y; y <= rmax.y; ++y) {
                    for(int x = rmin.x; x <= rmax.x; ++x) {
                        ivec3 bucket = ivec3(x, y, z);

                        if(!partition.contains(bucket)) partition.emplace(bucket, std::vector<uint32_t>());

                        auto& p = partition[bucket];

                        p.push_back(i + num_prev);
                    }
                }
            }
        }

        num_prev += pop.num_craters;
    }

    auto sample_moon_noise = [](vec3 pos, float seed) {
        float n0 = Noise_gen::perlin_noise(pos, 0.45f, 3, seed);
        //n0 *= 0.65f;

        vec3 forward = vec3(1, 0, 0);
        float d = smoothstep(0.0f, 1.0f, dot(normalize(pos), forward));
        d -= 0.15f;
        n0 -= d * 0.7f;
        n0 += 0.5f;
        n0 *= 1.5f;

        return n0;
    };

    auto get_noise = [&](vec3 pos, float seed, float amplitude, int num_craters) {
        float n = sample_moon_noise(pos / avg_dimension, seed);
        n += Noise_gen::perlin_noise(pos / avg_dimension, 0.1f, 4.0f, seed) * 0.1f;

        //float nn = 1.0 - Noise_gen::ridged_perlin_noise(pos / avg_dimension, 0.035f, 4.0f, seed);
        //n += (pow(nn, 4.0f) * 0.5f * smoothstep((pow(abs(n), 0.5) * sign(n)) * 0.5f + 0.5f));
        //n = pow(abs(n), 0.f) * sign(n);
        //n += 0.0625f;
        //n += noise_offset;
        n *= amplitude;
        
        vec3 n_pos = pos / avg_dimension;
        ivec3 b = floor(n_pos * float(num_buckets));
        auto& bucket = partition[b];

        float crater_depth = n;
        
        for(int i : bucket) {
            if(i < num_craters) {
                crater& c = craters[i];
                
                vec3 rel = c.position - n_pos;
                float dist = length(rel);
                dist /= c.radius;

                float variation = 0.2f;

                if(dist < 1.5f + variation) {
                    float noise_v = Noise_gen::perlin_noise(rel / c.radius, 0.2f, 2.0f, seed) * 0.25f;
                    noise_v += Noise_gen::perlin_noise(rel / c.radius, 0.5f, 2.0f, seed);
                    dist += noise_v * variation;
                    dist = max(0.001f, dist);

                    if(dist < 1.5f) {
                        float crater_scale = c.radius * 0.075;
                        float height = crater_scale * avg_dimension;
                        float depth = height;//(0.0025f) * avg_dimension;
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
    
        return crater_depth;
    };

    auto get_color = [&](vec3 pos, float elevation) {
        vec3 p_i = pos * dimensions;

        float sep = blend_value * amplitude;
        float blend = (elevation + (sep * 0.5f)) / sep;
        blend = clamp(blend, 0.0f, 1.0f);
        
        vec3 n_pos = p_i / avg_dimension;
        ivec3 b = floor(n_pos * float(num_buckets));
        auto& bucket = partition[b];

        float ff = 0.0;
        
        for(int i : bucket) {
            crater& c = craters[i];

            vec3 rel_pos = c.position - n_pos;
            //std::vector<float> f = Noise_gen::perlin_noise(rel_pos, 0.25f, 2, i, ivec3(1, 1, 3), 1000.0f);
            //vec3 offset = {f[0], f[1], f[2]};
            //rel_pos += offset * 0.05f;

            float dist = length(rel_pos);

            if(c.ejecta != 0.0f) {
                //if(dist < c.ejecta) ff = 1.0;
                float dist2 = max(0.0, (dist - c.radius) / (c.ejecta - c.radius));
                if(dist2 < 1.0) {
                    vec3 flattened = normalize(rel_pos - c.position * dot(rel_pos, c.position));
                    float noise = (1.0 - Noise_gen::ridged_perlin_noise(flattened, 0.25f, 2, i));

                    dist2 = pow(dist2, 0.5f);

                    float fade = (dist + c.radius * noise * 0.25f - c.radius) / (c.radius * 0.5f);
                    fade = clamp(fade, 0.0f, 1.0f);
                    fade = smoothstep(0.0f, 1.0f, fade);
                    float fade2 = (1.0f - fade) * 0.75f;
                    fade = 0.75f + fade * 0.25f;

                    float f = (noise * (1.0f - ejecta_value) + ejecta_value) - dist2;
                    f = max(f * fade, fade2);

                    f = clamp(f, 0.0f, 1.0f);

                    ff = max(ff, f * c.age);
                }
            }
        }

        vec3 c0 = glm::mix(colors[1], colors[3], clamp(ff, 0.0f, 1.0f));
        vec3 c1 = glm::mix(colors[0], colors[2], clamp(ff, 0.0f, 1.0f));

        vec3 color = glm::mix(c0, c1, blend);

        return color;
    };
    
    for(int i = 0; i < craters.size(); ++i) {
        crater& c = craters[i];
        vec3 pos = c.position * avg_dimension;

        float height = get_noise(pos, seed, amplitude, i - 1);
        c.height = height;
    }
    
    auto get_normal = [&](vec3 pos, vec3 axes) {
        return normalize(vec3(pos.x / (axes.x * axes.x), pos.y / (axes.y * axes.y), pos.z / (axes.z * axes.z)));
    };

    auto project = [&](vec3 normal, vec3 axes) {
        return normalize(normal / axes) * axes;
    };
    
    std::cout << "finished setup\n";
    auto create_m = [&](float width, uint32_t tiles, vec3 origin, mat3 orientation, vec3 p_size, pvec3 position, uint32_t seed, mat3 planet_ori) {
        Collider cl;
        cl.allow_rotation = false;
        cl.allow_gravity = false;
        cl.is_static = true;

        std::vector<Mesh_vertex> mesh_vertices;
        std::vector<uint32_t> mesh_indices;

        mat3 inv = ps.gravity_orientation;

        std::vector<float> values;

        auto find_pos = [&](ivec2 pos) {
            vec3 mpos = origin + vec3(vec2(pos) / float(tiles) * width, 0);
            mpos = orientation * mpos;
            return normalize(mpos);
        };

        float diff = width / tiles * max(max(p_size.x, p_size.y), p_size.z);

        for(int y = 0; y < tiles + 1; ++y) {
            for(int x = 0; x < tiles + 1; ++x) {
                vec3 pos = find_pos({x, y});
                values.push_back(get_noise(pos * p_size, seed, amplitude, craters.size()));
            }
        }

        std::vector<vec3> mesh_colors;
        std::vector<vec3> mesh_normals;
        for(int y = 0; y < tiles + 1; ++y) {
            float multiplier_y = -1.0f;
            int y1 = y + 1;

            for(int x = 0; x < tiles + 1; ++x) {
                float multiplier_x = -1.0f;
                int x1 = x + 1;

                ivec2 vvi = ivec2(x, y);

                vec3 pos = find_pos(vvi);
                vec3 p_i = pos * p_size;

                mat3 ori = rotate_to(vec3(0, 0, 1), normalize(p_i));

                vec3 delta_x = ori * vec3(1, 0, 0);
                vec3 delta_y = ori * vec3(0, 1, 0);

                vec3 p_x = project(p_i + delta_x * diff, p_size);
                vec3 p_y = project(p_i + delta_y * diff, p_size);

                float vi = values[vvi.y * (tiles + 1) + vvi.x];
                float vx = get_noise(p_x, seed, amplitude, craters.size());
                float vy = get_noise(p_y, seed, amplitude, craters.size());

                vec3 pos_i = p_i + get_normal(p_i, p_size) * vi;
                vec3 pos_x = p_x + get_normal(p_x, p_size) * (vx);
                vec3 pos_y = p_y + get_normal(p_y, p_size) * (vy);

                vec3 normal = cross(pos_x - pos_i, pos_y - pos_i);
                normal = normalize(normal);

                mesh_normals.push_back(normal);

                vec3 color = get_color(pos, vi);
                mesh_colors.push_back(color);
            }
        }
        
        for(int y = 0; y < tiles; ++y) {
            for(int x = 0; x < tiles; ++x) {
                ivec2 pos = {x, y};

                for(ivec2 v : indices) {
                    ivec2 new_pos = pos + v;

                    vec3 normal = mesh_normals[new_pos.y * (tiles + 1) + new_pos.x];
                    vec3 color = mesh_colors[new_pos.y * (tiles + 1) + new_pos.x];
                    float h = values[new_pos.y * (tiles + 1) + new_pos.x];

                    Mesh_vertex mv;
                    mv.position = find_pos(new_pos) * p_size;
                    mv.position += get_normal(mv.position, p_size) * h;
                    
                    mv.position = ps.gravity_orientation * mv.position;
                    mv.normal = ps.gravity_orientation * normal;
                    mv.bone_weights = vec4(color, 0.0f);



                    mesh_indices.push_back(mesh_vertices.size());
                    mesh_vertices.push_back(mv);
                }
            }
        }

        // texture mesh
        vec3 mesh_avg = vec3(0.0f);
        for(int tt = 0; tt < mesh_indices.size() / 3; ++tt) {
            Mesh_vertex& v0 = mesh_vertices[tt * 3];
            Mesh_vertex& v1 = mesh_vertices[tt * 3 + 1];
            Mesh_vertex& v2 = mesh_vertices[tt * 3 + 2];

            mesh_avg += v0.position;
            mesh_avg += v1.position;
            mesh_avg += v2.position;

            vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));
            vec3 n = normal;

            normal = normalize(round(normal / max(abs(normal.x), max(abs(normal.y), abs(normal.z)))));

            vec3 tex_x = cross(normal, vec3(0, 1, 0));
            if(length(tex_x) == 0.0f) tex_x = cross(normal, vec3(0, 0, 1));
            tex_x = normalize(tex_x);
            vec3 tex_y = normalize(cross(normal, tex_x));

            vec3 avg_pos = v0.position + v1.position + v2.position;
            avg_pos /= 3.0f;

            v0.tex_coords = vec2(dot(tex_x, v0.position), dot(tex_y, v0.position));
            v1.tex_coords = vec2(dot(tex_x, v1.position), dot(tex_y, v1.position));
            v2.tex_coords = vec2(dot(tex_x, v2.position), dot(tex_y, v2.position));

            // change
            //v0.normal = n;
            //v1.normal = n;
            //v2.normal = n;
        }
        if(mesh_indices.size()) mesh_avg /= mesh_indices.size();

        for(Mesh_vertex& v : mesh_vertices) {
            v.position -= mesh_avg;
        }
        
        for(int tt = 0; tt < mesh_indices.size() / 3; ++tt) {
            Mesh_vertex& v0 = mesh_vertices[tt * 3];
            Mesh_vertex& v1 = mesh_vertices[tt * 3 + 1];
            Mesh_vertex& v2 = mesh_vertices[tt * 3 + 2];

            Collision_shape shape;
            cl.mass = 0.0f;
            shape.mass = 0.0f;
            shape.radius = 0.0f;
            shape.vertices = {
                v0.position,
                v1.position,
                v2.position,
            };

            vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));
            shape_face t = {{0, 1, 2}, normal};
            shape.faces = {t};
            
            cl.collision_shapes.push_back(Convex_collider());
            cl.collision_shapes[cl.collision_shapes.size() - 1].collision_shape = std::make_shared<Collision_shape>(shape);
        }
        
        Transform t;
        t.position = mesh_avg;
        t.orientation = planet_ori;

        t.position = apply_matrix(planet_ori, t.position);
        t.position += position;

        Mesh_component mc;
        std::shared_ptr<Mesh> mesh(new Mesh);

        mesh->add_vertices(mesh_vertices, mesh_indices);
        mesh->load_buffer();
        mc.mesh = mesh;
        std::shared_ptr<Texture> texture = create_image_texture("resources/textures/tilesheet.png", ivec4(48, 128, 16, 16));
        
        mc.texture = texture;

        uint32_t entity = ecs.insert_entity();
        ecs.insert_component(entity, t);
        ecs.insert_component(entity, mc);
        ecs.insert_component(entity, cl);   

        Collider& ccl = ecs.get_component<Collider>(entity);
        Transform& tf = ecs.get_component<Transform>(entity);

        Physics_system& ps = ecs.get_system<Physics_system>();
        //ps.create_bounding_box(ccl, tf);
        //ccl.create_BVH(2, &tf);
    };

    
    double setup_time = get_time();

    int split = 8;
    float size = 1.0f;
    uint32_t tiles_per_split = 32;

    for(int i = 0; i < 6; ++i) {
        mat3 matrix = matrices[i];
        for(int x = 0; x < split; ++x) {
            for(int y = 0; y < split; ++y) {
                vec3 origin = vec3(size * 2.0f / split * x - size, size * 2.0f / split * y - size, size);
                //std::cout << i << " " << x << " " << y << "\n";
                
                //if(i == 0 && x < 16 && y < 16) 
                create_m(size * 2.0f / split, tiles_per_split, origin, matrix, dimensions, position, seed, orientation);
            }
        }
    }
    
    // /profiler_planet.output();
    
    float width = size * 2.0f / split;
    float tiles = tiles_per_split;
    float diff = width / tiles * max(max(dimensions.x, dimensions.y), dimensions.z);
    
    double end_time = get_time();

    std::cout << "setup: " << setup_time - start_time << "\n";
    std::cout << "loop: " << end_time - setup_time << "\n";
    std::cout << "total: " << end_time - start_time << "\n\n";

    int image_size = 1024;
    std::vector<uint8_t> pixels(image_size * (image_size / 2) * 4);
    
    if(false) {
        for(int y = 0; y < image_size / 2; ++y) {
            for(int x = 0; x < image_size; ++x) {
                int i = y * image_size + x;

                vec2 p = map_project(ivec2(x, y), ivec2(image_size, image_size / 2));

                vec3 color = vec3(0.0f);
                if(p.x >= 0.0f && p.x < 1.0f && p.y >= 0.0f && p.y < 1.0f) {
                    mat3 r = rotate(float(M_PI), vec3(0.0f, 0.0f, 1.0f));
                    vec3 pos = r * wrap(p);
                    float elev = get_noise(pos * dimensions, seed, amplitude, craters.size());

                    color = get_color(pos, elev);

                    // normal

                    vec3 base_sun = normalize(vec3(1, 0, 1));

                    mat3 ori = rotate_to(vec3(0, 0, 1), pos);

                    vec3 delta_x = ori * vec3(1, 0, 0);
                    vec3 delta_y = ori * vec3(0, 1, 0);

                    vec3 p_i = pos * dimensions;
                    vec3 p_x = project(p_i + delta_x * diff, dimensions);
                    vec3 p_y = project(p_i + delta_y * diff, dimensions);

                    float vx = get_noise(p_x, seed, amplitude, craters.size());
                    float vy = get_noise(p_y, seed, amplitude, craters.size());

                    vec3 pos_i = p_i + get_normal(p_i, dimensions) * elev;
                    vec3 pos_x = p_x + get_normal(p_x, dimensions) * vx;
                    vec3 pos_y = p_y + get_normal(p_y, dimensions) * vy;

                    vec3 normal = cross(pos_x - pos_i, pos_y - pos_i);
                    normal = normalize(normal);

                    base_sun = ori * base_sun;
                    float factor = dot(normal, base_sun);
                    factor = (factor - 0.5f) * 2.0f + 0.5f;
                    factor = clamp(factor, 0.0f, 1.0f);
                    color *= factor;
                }

                pixels[i * 4] = color.x * 255;
                pixels[i * 4 + 1] = color.y * 255;
                pixels[i * 4 + 2] = color.z * 255;
                pixels[i * 4 + 3] = 255;
            }
        }

        std::string filename = "output/map_moon" + std::to_string(uint64_t(get_absolute_time() * 10)) + ".png";
        stbi_flip_vertically_on_write(true);
        stbi_write_png(filename.c_str(), image_size, image_size / 2, 4, pixels.data(), 4 * image_size);
    }
};