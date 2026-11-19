#version 460 core

layout(binding = 0) uniform sampler2D flat_tex;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_shading;

layout(location = 0) in vec4 color;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 position;
layout(location = 3) in vec2 texcoords;

layout(location = 3) uniform float contrast;

uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

void main() {
    frag_color = color;
    frag_shading = vec4(normal * 0.5 + 0.5, 1.0);//contrast);
    
    vec2 tt = (texcoords * 48.0) / vec2(textureSize(flat_tex, 0));

    vec4 tex = texture(flat_tex, tt);

    vec3 dx = dFdx(position);
    vec3 dy = dFdy(position);
    vec3 true_normal = normalize(cross(dx, dy));
    frag_normal = vec4((true_normal * 0.5 + 0.5) * 0xFE / 0xFF, 1.0);

    if(color.x > 1.0) frag_shading.w = 0.0;
    
    if(frag_color.w == 0.0) discard;
    
    frag_color.xyz *= tex.xyz;
}