#version 460 core

layout(binding = 0) uniform sampler3D density;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 pos;
layout(location = 2) in float f;
layout(location = 3) in vec3 tint;
layout(location = 4) in vec3 rel_pos;

layout(location = 3) uniform vec3 light_dir;
layout(location = 4) uniform vec3 apos;
layout(location = 5) uniform float darkness_value;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 slope;

float pi = 3.14159265358979;

uint hash(uint x) {
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
    float val1 = value - min1;
    val1 = val1 / (max1 - min1);
    val1 *= (max2 - min2);
    val1 += min2;
    return min(max(0.0, val1), 1.0);
}

float get_texture(vec3 pos) {
    float t1 = texture(density, pos).g;
    return t1;
}

vec3 calculate_flow(vec3 pos, float delta, vec2 aspect, vec2 scale) {
    vec3 ddx = normalize(cross(pos, vec3(0, 0, 1)));
    vec3 ddy = normalize(cross(ddx, pos));

    vec3 vscale = vec3(scale.x, scale.x, scale.y);

    float d0 = get_texture(pos * vscale);
    float dx = get_texture(pos * vscale + ddx * delta);
    float dy = get_texture(pos * vscale + ddy * delta);

    float curl_x = (d0 - dx) / delta;
    float curl_y = (d0 - dy) / delta;

    return ddx * curl_y * aspect.x - ddy * curl_x * aspect.y;
}

vec3 get_color(float hue, float saturation, float brightness) {
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
    color *= brightness;

    return color;
}
/*
float freq_b = 2;
//freq_b = freq_b * base_freq;

rpos_a = rpos;
for(int i = 0; i < num_iterations; ++i) {
    vec3 flow = calculate_flow(rpos_a, freq_b * 0.001, vec2(1.0 / freq_b * 0.01 / num_iterations), vec2(1.0 * freq_b)); // last number larger means smaller swirls
    
    rpos_a += flow;
    rpos_a = normalize(rpos_a);
}
rpos += rpos_a - rpos_b;
*/

float base_freq = 16;

void main() {
    float d = max(darkness_value, 0.1);
    float light = max(dot(light_dir, normal), 0.0) * (1 - d) + d;
    
    float sample_delta = 0.01;
    vec2 texture_scale = vec2(1.0, 0.6) * 0.3;
    vec2 step_delta = vec2(1.0, 0.4) * 0.005;

    vec3 rpos = normalize(rel_pos);
    vec3 rpos_a = rpos;
    vec3 rpos_b = rpos;
    
    float dd = texture(density, rpos * vec3(0.01, 0.01, 0.5)).r * 2;

    float spot = 0.0;
    float spot_2 = 0.0;


    // swirl
    vec3 storm = vec3(1, 0, 0);
    float radius = 0.4;

    rpos_a *= vec3(1.0, 1.0, 3.0);
    vec3 rel = rpos_a - storm;
    float len = length(rel);
    float r = clamp(len / radius, 0.0, 1.0);
    //r = pow(r, 0.5);
    if(len < radius) {
        mat2 ori = get_rot((len) * -20);

        rpos_a = vec3(rpos_a.x, ori * rpos_a.yz);
        rpos_a = normalize(rpos_a);

        spot = map(1.0 - r, 0.2, 0.4, 0.0, 1.0);
        spot_2 = map(1.0 - r, 0.1, 0.3, 0.0, 1.0);
    }
    rpos_a /= vec3(1.0, 1.0, 3.0);
    rpos = mix(rpos_a, rpos, r);


    // flow
    float delta = 0.01;
    vec2 aspect = vec2(1.0, 0.2) * 0.0005;

    int num_iterations = 20;
    
    rpos_a = rpos;
    for(int i = 0; i < num_iterations; ++i) {
        vec3 flow = calculate_flow(rpos_a, sample_delta * 0.1, step_delta * 0.1, 1.0 / texture_scale * 0.25); // last number larger means smaller swirls
        
        rpos_a += flow;
        rpos_a = normalize(rpos_a);
    }
    rpos += rpos_a - rpos_b;

    
    
    vec3 aspect2 = vec3(0.03, 0.03, 0.6) * 1.0;
    vec3 rpos2 = (rpos * 0.5 + 0.5);
    float dtex = texture(density, rpos2 * aspect2 * 1.0).r;
    dtex += (texture(density, rpos2 * (12) * aspect2).r - (0.5)) * (0.2);
    dtex += (texture(density, rpos2 * 2).r) * (0.0 + spot_2 * 0.8);

    float base = dtex;
    base = mix(base, 1.0 + base, spot);

    //

    vec3 normal_2 = normalize(cross(dFdx(pos), dFdy(pos)));
    float cosine = dot(normal_2, vec3(0, 0, -1));
    if(gl_FrontFacing) {
        if(cosine < 0) cosine = -cosine;
    } else {
        if(cosine > 0) cosine = -cosine;
    }

    vec3 col_a = get_color(0.3, 0.1, 1.0);
    vec3 col_b = get_color(0.4, 0.8, 0.65);
    
    float d2 = dot(normalize(apos), light_dir);
    float wrap = 0.35;
    float d2_wrap = 0.95;

    // y is shadow darkness
    slope = vec4(cosine * 0.5 + 0.5, 0.0, abs(f) * 2, 1);
    vec3 output_normal = normal * 0.5 + 0.5;

    frag_color = vec4(mix(col_a, col_b, base), 1.0);

    if(darkness_value == 1.0) frag_normal = vec4(0, 0, 0, 1.0);
    frag_normal = vec4(output_normal, 1.0);

    //frag_color.g = fract((rpos2 * aspect2).z * 16);

    if(frag_color.w <= 2.0/255) discard;
    frag_color.w = 1.0;
}

/*
void main() {
    float d = max(darkness_value, 0.1);
    float light = max(dot(light_dir, normal), 0.0) * (1 - d) + d;
    

    // color
    vec3 rpos = normalize(rel_pos);
    vec3 rpos_a = rpos;
    vec3 rpos_b = rpos;
    vec3 rpos1 = rpos * 0.5 + 0.5;

    float delta = 0.01;
    vec2 aspect = vec2(1.0, 0.2) * 0.0005;
    int num_iterations = 20;

    vec3 storm = vec3(1, 0, 0);
    float radius = 0.4;

    // swirl
    rpos_a *= vec3(1.0, 1.0, 3.0);
    vec3 rel = rpos_a - storm;
    float len = length(rel);
    float r = clamp(len / radius, 0.0, 1.0);
    //r = pow(r, 0.5);
    if(len < radius) {
        mat2 ori = get_rot(len * -30);

        rpos_a = vec3(rpos_a.x, ori * rpos_a.yz);
        rpos_a = normalize(rpos_a);
    }
    rpos_a /= vec3(1.0, 1.0, 3.0);
    rpos = mix(rpos_a, rpos, r);

    // flow
    rpos_a = rpos_b;
    for(int i = 0; i < num_iterations; ++i) {
        vec3 flow = calculate_flow(rpos_a, delta, aspect, 0.4 + (1.0 - r) * 0.3); // last number larger means smaller swirls
        
        rpos_a += flow;
        rpos_a = normalize(rpos_a);
    }
    rpos += rpos_a - rpos_b;
    
    vec3 aspect2 = vec3(0.01, 0.01, 0.5);
    vec3 rpos2 = (rpos * 0.5 + 0.5);
    float dtex = (texture(density, rpos2 * aspect2).r * 6.0 - 2.0);

    frag_color = vec4(dtex.r, dtex.r, dtex.r, 1.0);

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

    vec3 col_a = vec3(1.0, 0.3, 0.1) * 0.1;
    vec3 col_b = vec3(1.0, 0.9, 0.25) * 0.6;
    
    float d2 = dot(normalize(apos), light_dir);
    float wrap = 0.35;
    float d2_wrap = 0.95;

    // y is shadow darkness
    slope = vec4(cosine * 0.5 + 0.5, 0.0, abs(f) * 2, 1);
    vec3 output_normal = normal * 0.5 + 0.5;

    frag_color = vec4(mix(col_a, col_b, dtex.r), 1.0);

    if(darkness_value == 1.0) frag_normal = vec4(0, 0, 0, 1.0);
    frag_normal = vec4(output_normal, 1.0);

    if(frag_color.w <= 2.0/255) discard;
    frag_color.w = 1.0;
}
*/