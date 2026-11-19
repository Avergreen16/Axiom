#version 300 es

precision highp float;

layout(location = 0) out vec4 frag_color;

uniform sampler2D texture_sampler;

in vec4 color0;
in vec2 texture0;

void main() {
    vec4 tex_col = texture(texture_sampler, texture0);
    frag_color = vec4(tex_col.xyz * color0.xyz, tex_col.w * color0.w);

    if(frag_color.w == 0.0) discard;
}