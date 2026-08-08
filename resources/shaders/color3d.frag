#version 460 core


layout(location = 3) uniform vec3 light;

layout(location = 0) out vec4 frag_color;
//layout(location = 1) out vec4 frag_normal;

layout(location = 0) in vec3 color;
layout(location = 1) in vec3 normal;

void main() {
    float l = clamp(dot(normal, light), 0.0, 1.0);
    l = l * 0.75 + 0.25;

    frag_color = vec4(color * l, 1);
    //frag_normal = vec4(0.0, 0.0, 0.0, 1.0);
}