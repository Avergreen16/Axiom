#version 300 es

precision highp float;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec4 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec4 color0;
out vec2 texture0;

void main() {
    gl_Position = proj * (view * model * vec4(position, 0.0, 1.0));

    color0 = color;
    texture0 = tex;
}