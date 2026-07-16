#version 460 core

layout(binding = 1) uniform sampler2D tex;

///layout(rgba32f, binding = 0) uniform image3D kBuffer;

out vec4 frag_color;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex_coord;

void main() {
    vec4 tex_color = texture(tex, tex_coord / textureSize(tex, 0));
    vec4 col = tex_color * color;

    if(col.a == 0.0) discard;

    frag_color = col;
}