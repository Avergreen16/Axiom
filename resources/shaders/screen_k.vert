#version 460 core

vec3 grid_plane[6] = vec3[](
    vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, -1, 1),
    vec3(-1, -1, 1), vec3(1, -1, 1), vec3(1, 1, 1)
);

void main() {
    gl_Position = vec4(grid_plane[gl_VertexID].xyz, 1.0);
}