#version 460 core

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_shading;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

layout(location = 3) uniform float contrast;

void main() {
    vec4 tex_col = texture(texture_sampler, tex / vec2(textureSize(texture_sampler, 0)));
    
    frag_color = vec4(tex_col.xyz * color.xyz, tex_col.w * color.w);
    frag_normal = vec4(normal * 0.5 + 0.5, 1.0);
    frag_shading = vec4(normal * 0.5 + 0.5, contrast);
    if(frag_color.w == 0.0) discard;
}