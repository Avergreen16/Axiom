#pragma once

#include <include/core.hpp>

#include <render/target.hpp>
#include <render/shadow/shadow.hpp>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

#ifdef AXIOM_WEBGL

#elifdef AXIOM_OPENGL
#define AXIOM_TAKE_SCREENSHOT

#endif

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using dvec2 = glm::vec<2, double>;
using dvec3 = glm::vec<3, double>;
using dvec4 = glm::vec<4, double>;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;

namespace axiom {

struct context;
struct texture;
struct shader;
struct vertices;
    
struct render_system : axiom::system {
    axiom::context* ctx;
    std::function<void()> draw_func;

    std::string resource_root = "external/axiom/res";
    std::string output_root = "output";

    std::unordered_map<std::string, axiom::texture> textures;
    std::unordered_map<std::string, axiom::shader> shaders;

    std::vector<std::unique_ptr<axiom::render_target>> targets;
    std::vector<std::unique_ptr<axiom::shadow_renderer>> shadow_renderers;

    // shadow parameters
    float light_altitude = 50.0f;
    float light_azimuth = 35.0f;
    float light_contrast = 0.98f;

    render_system(axiom::context* ctx_);

    void call();

    void process_inputs();

    #ifdef AXIOM_TAKE_SCREENSHOT
    void take_screenshot(std::string filepath);
    #endif
};

void render_init(axiom::context* ctx);

axiom::shader& get_shader(std::string id);
axiom::texture& get_texture(std::string id);

}