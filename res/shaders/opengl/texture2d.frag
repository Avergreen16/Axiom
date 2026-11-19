#version 460 core

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;

void main() {
    vec4 tex_col = texture(texture_sampler, tex / vec2(textureSize(texture_sampler, 0)));
    frag_color = vec4(tex_col.xyz * color.xyz, tex_col.w * color.w);
    
    frag_normal = vec4(0.0, 0.0, 0.0, 1.0);
}