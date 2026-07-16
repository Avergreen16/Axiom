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
layout(location = 7) uniform uint seed2;
layout(location = 8) uniform double time2;
layout(location = 9) uniform uint type;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 slope;

uint seed = seed2 + 160;

float pi = 3.14159265358979;

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
    return to_float(hash(x) ^ seed);
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

float fbm(vec3 pos, float freq, int octaves, float gain, float lacunarity, int n) {
    float accum = 0.0;
    for(int i = 0; i < octaves; ++i) {
        vec3 p = pos * pow(lacunarity, i) * (freq / 4.0);
        accum += (texture(density, p)[n] * 2.0 - 1.0) * pow(gain, i);
    }

    return accum;
}

float map(float value, float min1, float max1, float min2, float max2) {
    float f = min2 + (value - min1) * (max2 - min2) / (max1 - min1);
    return f;
}

vec3 calculate_flow(vec3 pos, float delta, vec2 aspect, vec2 scale) {
    vec3 ddx = normalize(cross(pos, vec3(0, 0, 1)));
    vec3 ddy = normalize(cross(ddx, pos));

    vec3 sc = vec3(scale.x, scale.x, scale.y);

    float d0 = fbm(pos * sc, 8.0, 3, 0.75, 1.0, 0);
    float dx = fbm(pos * sc + ddx * delta, 8.0, 3, 0.75, 1.0, 0);
    float dy = fbm(pos * sc + ddy * delta, 8.0, 3, 0.75, 1.0, 0);

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

float base_hue = hash_float(0x971314) * 6.0;
float base_sat = pow(hash_float(0xFFEEF), 0.5);
vec3 col_a;
vec3 col_b;
vec3 col_c;

uint num_storms = 0;
float min_storm = 0.0;
float max_storm = 0.0;

float flow_radius = 2.0;

vec3 col_d = get_color(0.6, 0.5, 0.85);
vec3 col_e = get_color(0.6, 0.45, 1.0);

// hue, sat, bright

float get_value(vec3 pos) {
    vec3 offset = vec3(hash_float(0x666), hash_float(0xFFF), hash_float(0xEEE));
    vec3 aspect2 = vec3(0.125, 0.125, 0.125);
    float dtex_a = fbm(pos * aspect2 + offset, 16.0, 1, 0.5, 2.0, 0);
    //dtex_a = dtex_a * 2.5 - 1.0;

    return dtex_a;
}

vec3 get_color(vec3 pos) {    
    vec3 offset = vec3(hash_float(0x111), hash_float(0x222), hash_float(0x333));
    vec3 aspect2 = vec3(0.005, 0.005, 0.25 + hash_float(0x444) * 0.25) * 0.75;
    //pos = (pos * 0.5 + 0.5);

    float dtex_a = fbm(pos * aspect2 + offset, 12.0, 2, 0.5, 2.0, 0) * 2.0;
    dtex_a += fbm(pos * aspect2 + offset, 60.0, 1, 0.5, 2.0, 0) * 0.375;

    //dtex_a = dtex_a * 2.0 - 1.0;
    //dtex_a = pow(abs(dtex_a), 1.0) * sign(dtex_a);
    dtex_a = dtex_a * 0.5 + 0.5;
    
    vec3 dtex = mix(col_a, col_b, clamp(dtex_a, 0.0, 1.0));

    return dtex;
}

const uint max_storms = 16;

vec4 storms[max_storms];
vec3 storm_colors[max_storms];

float loop(double v, double period) {
    return float(v * period);
}

struct storm_positions {
    vec3 positions[max_storms];
};

uint num_sites = 64;



storm_positions get_storms() {
    int shuffle[max_storms];

    for(int i = 0; i < max_storms; ++i) {
        shuffle[i] = i;
    }
    for(int i = int(max_storms - 1); i >= 1; --i) {
        uint j = hash(i ^ seed) % (i + 1);

        int temp = shuffle[j];
        shuffle[j] = shuffle[i];
        shuffle[i] = temp;
    }

    storm_positions s;

    float area = 4 * pi;
    float avg_sep = sqrt(area / max_storms);

    float golden_ratio = (1 + sqrt(5.0)) / 2.0;
    for(int i = 0; i < int(max_storms); ++i) {
        int ii = shuffle[i] + 1;
        float theta = 2.0 * pi * ii * golden_ratio;
        float phi = 1.0 - 2.0 * ii / (max_storms + 2);
        phi = acos(phi);

        vec3 pos = {cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi)};
        pos = normalize(pos + vec3(hash_float((i + 2000) ^ seed), hash_float((i + 8236) ^ seed), hash_float((i + 29137) ^ seed)) * avg_sep * 0.25f);

        s.positions[i] = pos;
    }

    return s;
}

storm_positions storm_pos = get_storms();

void main() {
    if(type == 0) {
        col_a = get_color(mod(base_hue + 0.1, 6.0), 0.3 * base_sat, 0.5);
        col_b = get_color(mod(base_hue - 0.05, 6.0), 0.3 * base_sat, 0.45);
        
        float albedo = hash_float(0xAFEC);
        if(albedo < 0.5) {
            albedo *= 2;
            col_a *= albedo * 0.85 + 0.15;
            col_b *= albedo * 0.85 + 0.15;
        } else {
            albedo = (albedo - 0.5) * 2.0;

            col_a = col_a * (1.0 - albedo) + albedo;
            col_b = col_b * (1.0 - albedo) + albedo;
        }
        
        flow_radius = 0.375 + 0.375 * hash_float(0xEF5276);
    } else if(type == 1) {
        col_a = get_color(mod(base_hue + 0.1, 6.0), 0.65 * base_sat, 0.5);
        col_b = get_color(mod(base_hue - 0.05, 6.0), 0.75 * base_sat, 0.25);

        float albedo = hash_float(0xAFEC) * 0.75f;
        if(albedo < 0.5) {
            //albedo *= 2;
            col_a *= albedo * 0.25 + 0.75;
            col_b *= albedo * 0.85 + 0.15;
        } else {
            //albedo = (albedo - 0.5) * 2.0;
            float albedo_a = albedo;
            float albedo_b = albedo * 0.35;

            col_a = col_a * (1.0 - albedo_a) + albedo_a;
            col_b = col_b * (1.0 - albedo_b) + albedo_b;
        }

        num_storms = hash(0x82136) % 2 + 10;
        min_storm = 0.025;
        max_storm = min_storm + mix(0.025, 0.075, hash_float(0xF4E4));

        flow_radius = 0.375 + 0.375 * hash_float(0xEF5276);
    } else if(type == 2) {
        col_a = get_color(mod(base_hue - 0.1, 6.0), 0.2 * base_sat, 0.9);
        col_b = get_color(mod(base_hue + 0.2, 6.0), 0.75 * base_sat, 0.4);

        float albedo = hash_float(0xAFEC);
        col_b *= albedo * 0.25 + 0.15;

        num_storms = hash(0x82136) % 5 + 5;
        min_storm = 0.025;
        max_storm = min_storm + mix(0.025, 0.075, hash_float(0xF4E4));

        flow_radius = 0.375 + 0.375 * hash_float(0xEF5276);
    }

    for(int i = 0; i < num_storms; ++i) {
        float len_xy = length(storm_pos.positions[i].xy);
        vec2 fxy = storm_pos.positions[i].xy / len_xy;
        vec2 fz = vec2(len_xy, storm_pos.positions[i].z);

        float z_angle = atan(fz.y / fz.x);
        float y_angle = atan(fxy.y, fxy.x);
        float scale = 1.25 + hash_float(i * 4 + 2) * 1.25;
        float radius = min_storm + (max_storm - min_storm) * hash_float(i * 4 + 3);

        storms[i] = vec4(z_angle, y_angle, scale, radius);

        //vec3 pos = get_rot_y(y_angle) * get_rot_z(z_angle) * vec3(1, 0, 0);
        vec3 p = get_rot_z(z_angle) * get_rot_y(y_angle) * vec3(1, 0, 0);

        vec3 color = get_color(p * 0.5 + 0.5);
        storm_colors[i] = color;
    }



    float d = max(darkness_value, 0.1);
    float light = max(dot(light_dir, normal), 0.0) * (1 - d) + d;
    
    uint num_pixels = 512;
    vec3 ppos = normalize(rel_pos);

    float abs_x = abs(ppos.x);
    float abs_y = abs(ppos.y);
    float abs_z = abs(ppos.z);
    float abs_max = max(max(abs_x, abs_y), abs_z);
    ppos /= abs_max;
    if(abs_max == abs_x) ppos.x = sign(ppos.x);
    else if(abs_max == abs_y) ppos.y = sign(ppos.y);
    else if(abs_max == abs_z) ppos.z = sign(ppos.z);

    ppos *= float(num_pixels) * 0.5;
    //ppos = floor(ppos);
    ppos = normalize(ppos);

    // color
    vec3 qpos = ppos;
    vec3 rpos = ppos;
    vec3 rpos_a = rpos;
    vec3 rpos_b = rpos;

    float delta = 0.01;
    vec2 aspect = vec2(1.0, 0.2) * 0.0005;
    int num_iterations = 20;

    float radius = 0.4;

    float spot = 0.0;
    float spot2 = 0.0;
    float spot3 = 0.0;

    vec3 rpos_c = rpos;

    float swirl_tex = 0.0;
    vec3 color = vec3(0.0);

    // swirl

    for(int i = 0; i < num_storms; ++i) {
        vec4 storm = storms[i];

        float z_angle = storm.x;
        float y_angle = storm.y;
        float scale = storm.z;
        float radius = storm.w;

        mat3 z_mat = get_rot_z(z_angle);
        mat3 y_mat = get_rot_y(y_angle);
        
        vec3 storm_pos = z_mat * y_mat * vec3(1, 0, 0);
        
        rpos_a = transpose(y_mat) * transpose(z_mat) * rpos_a;
        rpos_a *= vec3(1.0, 1.0, scale);
        vec3 rel = rpos_a - vec3(1, 0, 0);
        float len = length(rel);
        


        float displacement = len - radius;
        if(len > radius) {
            vec3 tangent_dir = rel;

            float displacement2 = exp(-(displacement / radius) * 0.5);

            rpos_a += -tangent_dir * displacement2;
        }
        
        mat2 ori = get_rot(exp(-((displacement - radius) / radius) * 1.0));
        rpos_a = vec3(rpos_a.x, ori * rpos_a.yz);


        rpos_a /= vec3(1.0, 1.0, scale);
        rpos_a = z_mat * y_mat * rpos_a;
        rpos_a = normalize(rpos_a);


        
        radius = radius * 1.5;
        rpos_c = transpose(y_mat) * transpose(z_mat) * rpos_c;
        rpos_c *= vec3(1.0, 1.0, scale);
        rel = rpos_c - vec3(1, 0, 0);
        len = length(rel);

        if(len < radius) {
            int ss = int(sign(hash_float(0x55FFE + i) - 0.5));
            if(ss == 0) ss = 1;
            float r1 = loop(time, 0.05) * pi;
            
            ori = get_rot((1.0 - len / radius) * pi * (0.75 + 0.5 * hash_float(0xFFEEBB + i)) * ss + r1);
            rpos_c = vec3(rpos_c.x, ori * rpos_c.yz);

            float tex = fbm(rpos_c / radius * 0.15, 8.0, 1, 0.5, 2.0, 0);
            swirl_tex = max(tex * 0.5 + 0.5, swirl_tex);

            color = storm_colors[i];
        }
        spot = max(spot, map(len / radius, 0.667, 1.0, 1.0, 0.0));
        //spot2 = max(spot2, map(len / radius, 0.0, 1.2, 1.0, 0.0));
        //spot3 = max(spot3, map(len / radius, 1.0, 2.0, 1.0, 0.0));

        rpos_c /= vec3(1.0, 1.0, scale);
        rpos_c = z_mat * y_mat * rpos_c;
        rpos_c = normalize(rpos_c);

        //spot = max(spot, r);
    }
    //vec3 rpos_d = rpos_a;

    // flow
    //rpos_a = rpos_b;
    float flow_intensity = get_value(rpos_a);
    flow_intensity = 0.5 + flow_intensity * 0.15;
    vec3 rpos1 = rpos_a;
    vec3 offset = vec3(0.0);//vec3(loop(time2, 0.001), loop(time2, 0.0015), 0.0);
    for(int i = 0; i < num_iterations; ++i) {
        vec3 flow1 = calculate_flow(rpos1 + offset, delta, aspect * flow_radius * flow_intensity, 3.0 * flow_radius / vec2(1.0, 0.4)); // last number larger means smaller swirls
        //vec3 flow2 = calculate_flow(rpos2, delta, aspect * flow_radius * 6 * flow_intensity, 0.2 / flow_radius / vec2(1.0, 0.4)); 
        //vec3 flow2 = calculate_flow(rpos_a, delta, aspect * flow_radius, (0.6 + spot2 * 0.2) / flow_radius); // last number larger means smaller swirls

        rpos1 += flow1;
        //rpos2 += flow2;

        rpos1 = normalize(rpos1);
        //rpos2 = normalize(rpos2);
    }
    rpos = rpos1;

    color = mix(color, col_b, 0.25);
    
    vec3 dtex = get_color(rpos * 0.5 + 0.5);
    vec3 dtex2 = mix(color, color * 0.95, clamp(swirl_tex * 3.0 - 1.0, 0.0, 1.0));
    dtex = mix(dtex, dtex2, clamp(spot + max(swirl_tex * 4.0 - 1.0, 0.0) * clamp(spot, 0.0, 1.0), 0.0, 1.0));

    frag_color = vec4(dtex, 1.0);

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