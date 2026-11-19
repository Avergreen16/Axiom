#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in float luminosity;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) out vec3 o_color;
layout(location = 1) out float o_luminosity;

void main() {
    gl_Position = proj * ((view * model) * vec4(normalize(position), 1.0));

    o_color = color;
    o_luminosity = luminosity;
}