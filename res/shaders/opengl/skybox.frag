#version 460 core

layout(binding = 0) uniform sampler2D viewport_color_tex;
layout(binding = 1) uniform sampler2D viewport_depth_tex;

layout(location = 0) out vec4 frag_color;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;
layout(location = 3) uniform vec3 extents;

layout(location = 0) in vec2 cs;
layout(location = 1) flat in mat4 inv_model;
layout(location = 5) flat in mat4 inv_view;
layout(location = 9) flat in mat4 inv_proj;

vec3 hex_color(uint i) {
    return vec3((i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF) / float(0xFF);
}

void main() {
    vec4 cc = vec4(cs, 0.5, 1.0);
    cc = inv_proj * cc;
    cc /= cc.w;
    cc = inv_view * cc;

    vec3 ray = normalize(cc.xyz);

    float fade = 1.0;

    float depth = texture(viewport_depth_tex, cs.xy * 0.5 + 0.5).r;
    vec3 color = texture(viewport_color_tex, cs.xy * 0.5 + 0.5).xyz;

    vec4 dd = vec4(0.0, 0.0, depth, 1.0);
    dd = inv_proj * dd;
    dd /= dd.w;

    if(depth != 0.0) fade = mix(1.0 - exp(dd.z * 0.001), 0.0, clamp(color.x + color.y + color.z, 0.0, 1.0));
    
    //
    frag_color = vec4(mix(hex_color(0x457edc), hex_color(0x2e5ca9), ray.z) * (1.0 - exp(dd.z * 0.001)), fade);
}