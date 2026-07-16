#version 460 core

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 pos;
layout(location = 3) in float f;
layout(location = 4) in vec3 tint;
layout(location = 5) in flat ivec4 bone_ids;
layout(location = 6) in vec3 world_pos;

layout(location = 3) uniform vec3 light_dir;
layout(location = 4) uniform vec3 apos;
layout(location = 5) uniform float darkness_value;
layout(location = 6) uniform vec3 global_tint;

// planet data
layout(location = 7) uniform vec3 center_pos;
layout(location = 8) uniform vec3 sun_dir;
layout(location = 9) uniform vec3 planet_radii;
layout(location = 10) uniform float atmosphere_thickness;

// offset
layout(location = 11) uniform float depth_offset;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 slope;

float get_scattering(vec3 rel_position, float atmo_thickness, float planet_radius, vec3 sun_direction) {
    float ii = 1.0 - (length(rel_position) - planet_radius) / atmo_thickness;
    ii = clamp(ii, 0.0, 1.0);

    vec3 normal = normalize(rel_position);

    float ndotl = dot(normal, sun_direction);
    float wrap = 0.15;
    float ndotl_wrap = clamp((ndotl + wrap) / (1.0 + wrap), 0.0, 1.0);
    ndotl_wrap = min(ndotl_wrap * 1.5, 1.0);

    float extinction_factor = ndotl_wrap * ii;

    return extinction_factor;
}

void main() {
    float d = max(darkness_value, 0.1);
    float light = max(dot(light_dir, normal), 0.0) * (1 - d) + d;
    
    vec4 color;
    if(bone_ids.x >= 0) {
        vec2 dx = dFdx(tex_coord);
        vec2 dy = dFdy(tex_coord);

        vec2 v = textureSize(tex, 0);

        vec2 t_c = mod(tex_coord * v, vec2(bone_ids.zw)) + vec2(bone_ids.xy);
        t_c /= v;

        color = textureGrad(tex, t_c, dx, dy);
    } else {
        color = texture(tex, tex_coord);
    }


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

    float dv = get_scattering(world_pos - center_pos, atmosphere_thickness, planet_radii.x, sun_dir);
    dv = 1.0 - dv;
    dv = smoothstep(0.0, 1.0, dv) * 0.5 + 0.5;

    // y is shadow darkness
    slope = vec4(cosine * 0.5 + 0.5, dv, (depth_offset + 5.0) / 10.0, 1);
    vec3 output_normal = normal * 0.5 + 0.5;

    if(darkness_value == 1.0) frag_normal = vec4(0, 0, 0, 1.0);
    frag_normal = vec4(output_normal, 1.0);

    if(frag_color.w <= 2.0/255) discard;
    frag_color.w = 1.0;
    frag_color.xyz *= tint;
    frag_color.xyz *= global_tint;
    //if(frag_color.w < 128.0 / 255) discard;
    //else frag_color.w = 1.0;
}