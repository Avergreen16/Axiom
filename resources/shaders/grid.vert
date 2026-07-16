#version 460 core

vec3 grid_plane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, 1, 0), vec3(-1, -1, 0),
    vec3(-1, -1, 0), vec3(1, -1, 0), vec3(1, 1, 0)
);

layout(location = 0) uniform mat4 model;
layout(location = 2) uniform mat4 proj;

out vec2 tex_coord;

out mat4 inverse_proj;
out mat4 inverse_model;

void main() {
    tex_coord = grid_plane[gl_VertexID].xy;
    gl_Position = vec4(grid_plane[gl_VertexID].xyz, 1.0);

    inverse_proj = inverse(proj);
    inverse_model = -model;
}