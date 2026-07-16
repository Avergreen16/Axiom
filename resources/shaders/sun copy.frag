#version 460 core

layout(binding = 0) uniform sampler3D density;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 pos;
layout(location = 2) in float f;
layout(location = 3) in vec3 tint;
layout(location = 4) in vec3 rel_pos;
layout(location = 5) in vec3 rel_normal;

layout(location = 3) uniform vec3 light_dir;
layout(location = 4) uniform vec3 apos;
layout(location = 5) uniform float darkness_value;
layout(location = 6) uniform double time;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 slope;

float pi = 3.14159265358979;

uint seed = uint(time / 2);

uint hash(uint x) {
    x = x ^ seed;
    
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float to_float(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat(m);       // Range [1:2]
    return f - 1.0;                // Range [0:1]
}

float hash_float(uint x) {
    return to_float(hash(x));
}

mat2 get_rot(float angle) {
   return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

mat3 get_rot_x(float angle) {
    vec3 vx = vec3(1.0, 0.0, 0.0);
    vec3 vy = vec3(0.0, cos(angle), sin(angle));
    vec3 vz = vec3(0.0, -sin(angle), cos(angle));

    return mat3(vx, vy, vz);
}

mat3 get_rot_y(float angle) {
    vec3 vx = vec3(cos(angle), 0.0, sin(angle));
    vec3 vy = vec3(0.0, 1.0, 0.0);
    vec3 vz = vec3(-sin(angle), 0.0, cos(angle));

    return mat3(vx, vy, vz);
}

mat3 get_rot_z(float angle) {
    vec3 vx = vec3(cos(angle), sin(angle), 0.0);
    vec3 vy = vec3(-sin(angle), cos(angle), 0.0);
    vec3 vz = vec3(0.0, 0.0, 1.0);

    return mat3(vx, vy, vz);
}

float map(float value, float min1, float max1, float min2, float max2) {
    float f = min2 + (value - min1) * (max2 - min2) / (max1 - min1);
    return min(max(0.0, f), 1.0);
}

float sample_texture(vec3 pos) {
    return texture(density, pos)[1];
}

vec3 calculate_flow(vec3 pos, float delta, vec2 aspect, float scale) {
    vec3 ddx = normalize(cross(pos, vec3(0, 0, 1)));
    vec3 ddy = normalize(cross(ddx, pos));

    float d0 = sample_texture(pos * scale);
    float dx = sample_texture((pos * scale + ddx * delta));
    float dy = sample_texture((pos * scale + ddy * delta));

    float curl_x = (d0 - dx) / delta;
    float curl_y = (d0 - dy) / delta;

    return -ddx * curl_y * aspect.x + ddy * curl_x * aspect.y;
}

vec3 get_color(float hue, float saturation, float value) {
    vec3 color;

    float f = fract(hue);

    if(hue < 1) {
        color = vec3(1.0, f, 0.0);
    } else if(hue < 2) {
        color = vec3(1.0 - f, 1.0, 0.0);
    } else if(hue < 3) {
        color = vec3(0.0, 1.0, f);
    } else if(hue < 4) {
        color = vec3(0.0, 1.0 - f, 1.0);
    } else if(hue < 5) {
        color = vec3(f, 0.0, 1.0);
    } else if(hue < 6) {
        color = vec3(1.0, 0.0, 1.0 - f);
    }

    color = color * saturation + (1.0 - saturation);
    color *= value;

    return color;
}

vec3 get_color(float hue) {
    vec3 color;

    float f = fract(hue);

    if(hue < 1) {
        color = vec3(1.0, f * 0.5, 0.0);
    } else if(hue < 2) {
        color = vec3(1.0, f * 0.5 + 0.5, f);
    } else if(hue < 3) {
        color = vec3(1.0 - f, 1.0 - f, 1.0);
    }

    //color = color * 0.75 + 0.25;

    return color;
}

float hue = 2.0;//hash_float(0x96371);

/* 0.0
vec3 col_d = get_color(hue);
vec3 col_e = get_color(hue + 0.5) * 1.5;
*/

/* 1.0
vec3 col_d = get_color(hue);
vec3 col_e = get_color(hue + 0.5) * 1.5;
*/

vec3 col_d = get_color(hue);
vec3 col_e = get_color(hue + 0.5) * 1.5;

// hue, sat, bright

void main() {
    vec3 rpos = normalize(rel_pos);

    vec3 offset = vec3(hash_float(0x111), hash_float(0x222), hash_float(0x444));
    //float aspect = hash_float(0x333) * 2.0 + 2.0;
    float aspect = hash_float(0x333) * 16.0 + 16.0;

    vec4 tex = texture(density, (rpos + offset) * aspect);
    frag_color = vec4(mix(col_d, col_e, pow(1.0 - tex.b, 0.5)), 1.0);
    float dot_n = dot(-normalize(pos), rel_normal);
    frag_color.xyz = mix(frag_color.xyz, vec3(1.0), (1.0 - dot_n) * 0.25);

    //

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

    // y is shadow darkness
    slope = vec4(cosine * 0.5 + 0.5, 0.0, abs(f) * 2, 1);
    vec3 output_normal = normal * 0.5 + 0.5;

    if(darkness_value == 1.0) frag_normal = vec4(0, 0, 0, 1.0);
    frag_normal = vec4(output_normal, 1.0);

    if(frag_color.w <= 2.0/255) discard;
    frag_color.w = 1.0;
}