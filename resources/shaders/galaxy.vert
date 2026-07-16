#version 460 core

vec3 grid_plane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, 1, 0), vec3(-1, -1, 0),
    vec3(-1, -1, 0), vec3(1, -1, 0), vec3(1, 1, 0)
);

layout(location = 0) out vec2 screen_coord;

void main() {
    screen_coord = grid_plane[gl_VertexID].xy;
    gl_Position = vec4(grid_plane[gl_VertexID].xyz, 1.0);
}