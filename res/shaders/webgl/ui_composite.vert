#version 300 es

precision highp float;

vec2 points[6] = vec2[6](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

out vec2 vert_pos;

void main() {
    vec2 p = points[gl_VertexID];

    gl_Position = vec4(p * 2.0 - 1.0, 0.5, 1.0);

    vert_pos = p;
}