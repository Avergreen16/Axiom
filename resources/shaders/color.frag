#version 460 core

layout(location = 3) uniform vec4 color;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;

void main() {
    frag_color = vec4(color);
    frag_normal = vec4(0.0, 0.0, 0.0, 1.0);

    if(color.w == 0.0) {
        frag_color.w = 1.0;
        gl_FragDepth = 0.0;
    }
}