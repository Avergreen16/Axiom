#version 460 core

vec2 points[6] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
};

layout(location = 0) out vec2 pos;

void main() {
    vec2 p = points[gl_VertexID];

    gl_Position = vec4(p * 2.0 - 1.0, 0.5, 1.0);

    pos = p;
}