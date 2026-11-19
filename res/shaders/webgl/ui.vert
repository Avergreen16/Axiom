#version 300 es

precision highp float;
precision highp int;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec4 color;
layout(location = 3) in uint data;
layout(location = 4) in uint clip;

uniform mat3 view_mat;
uniform mat3 trans_mat;

out vec2 vert_tex;
out vec4 vert_color;
flat out uint vert_data;
flat out uint vert_clip;
out vec2 vert_pos;

void main() {
    vert_tex = tex;
    vert_color = color;
    vert_data = data;
    vert_clip = clip;
    vert_pos = pos.xy;

    gl_Position = vec4((view_mat * trans_mat * vec3(pos.xy, 1.0)).xy, pos.z, 1.0);
}