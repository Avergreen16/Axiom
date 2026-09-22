#version 460 core

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 3) uniform float contrast;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_shading;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex;
layout(location = 2) flat in vec4 tex_range;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 position;

void main() {
    vec2 t = mod(tex, tex_range.zw) + tex_range.xy;
    vec4 tex_col = texture(texture_sampler, t / vec2(textureSize(texture_sampler, 0)));
    
    frag_color = vec4(tex_col.xyz * color.xyz, tex_col.w * color.w);
    frag_shading = vec4(normal * 0.5 + 0.5, 1.0);//contrast);

    vec3 dx = dFdx(position);
    vec3 dy = dFdy(position);
    vec3 true_normal = normalize(cross(dx, dy));
    frag_normal = vec4((true_normal * 0.5 + 0.5) * 0xFE / 0xFF, 1.0);

    if(frag_color.w == 0.0) discard;
}