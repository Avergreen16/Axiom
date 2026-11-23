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
    mat2 rot = mat2(
        cos(0.25 * 3.14159265358979), -sin(0.25 * 3.14159265358979),
        sin(0.25 * 3.14159265358979), cos(0.25 * 3.14159265358979)
    );

    vec2 directions[3] = {
        vec2(1, 0), 
        vec2(cos(3.14159 * 2.0 / 6.0), sin(3.14159 * 2.0 / 6.0)),
        vec2(cos(-3.14159 * 2.0 / 6.0), sin(-3.14159 * 2.0 / 6.0))
    };

    //float l = float((-luminosity + 6.0) / 7.0 + 0.03125) / 1.03125;
    float a = 7.5;
    float b = -4.5;
    float l = log(luminosity) / (log(a) * b);
    l = float(l + 0.01) / 1.01;

    float f = l;    

    //
    
    vec2 d = tex;

    float p = pow(abs(d.x), power) + pow(abs(d.y), power);

    //
    if(p < f) {
        float pp = 0.2;
        
        float ff = (1.0 - p / f);
        ff = pow(ff, pp);
        ff = round(ff * 6.0) / 6.0;
        ff = pow(ff, 1.0 / pp);

        ff *= f;

        frag_color = vec4(color, ff);
    } else frag_color = vec4(0.0);

    frag_normal = vec4(1.0);
    frag_shading = vec4(1.0);
}