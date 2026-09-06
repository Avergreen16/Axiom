#pragma once

#include <include/core.hpp>

#include <render/window/window.hpp>
#include <render/target.hpp>
#include <render/wrapper.hpp>

namespace axiom {
    
struct render_system : axiom::system {
    axiom::window* win;
    axiom::vertices vertices;
    std::string resource_root = "external/axiom/res";
    std::string output_root = "output";

    std::unordered_map<std::string, axiom::texture> textures;
    std::unordered_map<std::string, axiom::shader> shaders;

    std::vector<axiom::render_target> targets;

    // shadow parameters
    float light_altitude = 50.0f;
    float light_azimuth = 35.0f;
    float light_contrast = 0.98f;

    render_system(axiom::window* win_);

    void call();

    void process_inputs();

    void take_screenshot(std::string filepath);
};

void render_init(axiom::window* window);

axiom::shader& get_shader(std::string id);
axiom::texture& get_texture(std::string id);
axiom::vertices& get_vertices();

}