#version 460 core

layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2D map_tex;

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 pos;
layout(location = 3) in float f;
layout(location = 4) in vec3 tint;
layout(location = 5) in vec3 map_pos;

layout(location = 3) uniform vec3 light_dir;
layout(location = 4) uniform vec3 apos;
layout(location = 5) uniform float darkness_value;
layout(location = 6) uniform vec3 texture_center;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 slope;

float pi = 3.14159265358979;

void main() {
    float d = max(darkness_value, 0.1);
    float light = max(dot(light_dir, normal), 0.0) * (1 - d) + d;
    vec4 color = texture(tex, tex_coord);

    frag_color = vec4(color.xyz, color.w);

    vec3 normal_2 = normalize(cross(dFdx(pos), dFdy(pos)));
    float cosine = dot(normal_2, vec3(0, 0, -1));
    if(gl_FrontFacing) {
        if(cosine < 0) cosine = -cosine;
    } else {
        if(cosine > 0) cosine = -cosine;
    }

    float min_shadow = 0.01;
    float max_shadow = min_shadow + 0.74 * darkness_value;
    
    float d2 = dot(normalize(apos), light_dir);
    float wrap = 0.35;
    float d2_wrap = 0.95;
    
    /*if(apos == vec3(0.0) || light_dir == vec3(0.0)) d2_wrap = 1.0;
    else {
        d2_wrap = clamp((d2 + wrap) / (1.0 + wrap), 0.0, 1.0);
        d2_wrap = d2_wrap * (max_shadow - min_shadow) + min_shadow;
    }*/

    // y is shadow darkness
    slope = vec4(cosine * 0.5 + 0.5, 0.0, abs(f) * 2, 1);
    vec3 output_normal = normal * 0.5 + 0.5;

    if(darkness_value == 1.0) frag_normal = vec4(0, 0, 0, 1.0);
    frag_normal = vec4(output_normal, 1.0);

    if(frag_color.w <= 2.0/255) discard;
    frag_color.w = 1.0;
    //frag_color.xyz *= tint;

    vec3 map_tint = vec3(1.0);
    vec3 direction_normal = normalize(map_pos + texture_center);
    float y_angle = asin(direction_normal.z);
    vec2 xy = normalize(direction_normal.xy);
    float x_angle = atan(xy.y, xy.x);
    vec2 coords = vec2(x_angle / (2.0 * pi) + 0.5, y_angle / pi + 0.5);
    map_tint = texture(map_tex, coords).xyz;
    frag_color.xyz *= map_tint;
    //float len_y = length(direction_normal.xy);

    //if(frag_color.w < 128.0 / 255) discard;
    //else frag_color.w = 1.0;
}