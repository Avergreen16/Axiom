#version 460 core

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_shading;

layout(location = 0) in vec3 color;
layout(location = 1) in float luminosity;
layout(location = 2) in vec2 tex;

void main() {
    float power = 0.5;
    vec2 dist = abs(tex);

    float p = pow(dist.x, power) + pow(dist.y, power);

    //float l = float((-luminosity + 6.0) / 7.0 + 0.03125) / 1.03125;
    float a = 10;
    float b = -4.5;
    float l = log(luminosity) / (log(a) * b);
    l = float(l + 0.01) / 1.01;

    float f = l;

    if(p < l) frag_color = vec4(color, round((1.0 - p / l) * 5) / 5 * f);
    else frag_color = vec4(0.0);

    frag_normal = vec4(1.0);
    frag_shading = vec4(1.0);
}