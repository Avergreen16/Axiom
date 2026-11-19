#version 460 core

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec3 o_position;

void main() {
    gl_Position = vec4(position, 1.0);

    o_color = vec4(1.0);
    o_position = position;
}