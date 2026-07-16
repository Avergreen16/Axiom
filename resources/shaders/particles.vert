#version 460 core

/*
struct Particle_data {
    vec3 position;
    vec4 color;
    vec2 size;
    vec2 tex_coord;
    vec2 tex_size;
    vec3 orient;
    uint settings;
};

layout(std140, binding = 0) buffer Particle_buffer {
    Particle_data particle_data[];
};
*/


layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 size;
layout(location = 3) in vec2 tex_coord;
layout(location = 4) in vec2 tex_size;
layout(location = 5) in vec3 orient;
layout(location = 6) in uint settings;

//

layout(location = 0) uniform mat4 view_mat;
layout(location = 1) uniform mat4 proj_mat;
layout(location = 2) uniform vec3 particle_up;
layout(location = 3) uniform ivec2 screen_size;

layout(location = 0) out vec3 o_position;
layout(location = 1) out vec4 o_color;
layout(location = 2) out vec2 o_size;
layout(location = 3) out vec2 o_tex_coord;
layout(location = 4) out vec2 o_tex_size;
layout(location = 5) out vec3 o_orient;
layout(location = 6) flat out uint o_settings;

float far_plane = pow(2, 37) * 1000000;

mat3 inv = inverse(mat3(view_mat));

void main() {
    o_position = position;
    o_color = color;
    o_size = size;
    o_tex_coord = tex_coord;
    o_tex_size = tex_size;
    o_orient = orient;
    o_settings = settings;

    gl_Position = vec4(position, 1.0);
}