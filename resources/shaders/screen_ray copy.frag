#version 460 core

layout(binding = 0) uniform sampler2D depth_tex;
layout(binding = 1) uniform sampler3D density;

layout(location = 0) uniform mat4 proj;
layout(location = 1) uniform mat4 view;

layout(location = 2) uniform vec3 sun_pos;

layout(location = 3) uniform mat4 rel_mat;
layout(location = 4) uniform vec2 radii;
layout(location = 5) uniform mat4 moon_mat;
layout(location = 6) uniform uint seed_;
layout(location = 7) uniform double time2;
layout(location = 8) uniform mat4 torus_mat;
layout(location = 9) uniform mat4 earth_mat;

layout(location = 0) in vec2 screen_coord;

out vec4 frag_color;

uint seed = seed_;

float base_radius = 600.0 * 2000;
vec3 torus_radii = vec3(20.0, 8.0, 4.5) * base_radius;
float atmosphere_thickness = base_radius * 0.035;

//

float moon_size = base_radius * 0.45;

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

vec3 color1 = vec3(0.75, 0.8, 0.9) * 0.5;
vec3 color2 = vec3(0.9, 0.8, 0.5) * 0.5;

//

mat4 inverse_proj = inverse(proj);
mat4 inverse_view = inverse(view);

vec3 light_dir = normalize(mat3(inverse_view) * vec3(0, 0, 1));

mat3 rotate_z_to(vec3 b) {
    vec3 z = b;
    vec3 x = normalize(cross(z, vec3(0, 0, 1)));
    vec3 y = normalize(cross(z, x));

    return mat3(x, y, z);
}

float get_depth(float d, mat4 inv_p) {
    //d = (1 - d) * 2 - 1;
    vec4 dv = vec4(0.0, 0.0, d, 1.0);

    dv = inv_p * dv;
    dv /= dv.w;
    float depth = dv.z;

    if(isinf(depth)) depth = -3.402823E+38;

    return depth;
}

vec3 get_world_pos(vec3 clip_space) {
    clip_space.x = clip_space.x * 2 - 1;
    clip_space.y = clip_space.y * 2 - 1;

    vec4 world_pos = inverse_proj * (vec4(clip_space, 1.0));
    world_pos /= world_pos.w;
    world_pos = inverse_view * vec4(world_pos.xyz, 1.0);
    
    if(isinf(world_pos.z)) world_pos.z = 3.402823E+38;

    return world_pos.xyz;
}

float get_slope(float cosine) {
    float sine = sqrt(1 - cosine * cosine);

    float slope = sine / cosine;

    return clamp(slope, -1, 1);
}

float plane_intersection(vec3 origin, vec3 direction) {
    vec3 normal = vec3(0, 0, 1);

    float height_origin = dot(normal, -origin);
    float direction_dot = dot(normal, direction);

    float t = height_origin / direction_dot;

    return t;
}

// x = entry step
// y = closest step
// z = exit step
// w = closest dist to center
vec4 sphere_intersection(vec3 origin, vec3 direction, float radius, vec3 position) {
    vec3 sphere_pos = position;

    vec3 rel_origin = origin - sphere_pos;

    float t = dot(-rel_origin, direction);

    float c = radius;
    float a = length(rel_origin + direction * t);

    float b = sqrt(c * c - a * a);

    vec4 tt = vec4(t - b, t, t + b, a);

    return tt;
}

float sd_superellipse(vec2 pos, vec2 radii, float n) {
    vec2 p = abs(pos) / radii;
    float m = pow(pow(p.x, n) + pow(p.y, n), 1.0/n);
    float d = m - 1.0;
    // Scale back to world space
    float k = min(radii.x, radii.y);
    return d * k;
}

float torus_SDF(vec3 pos, vec3 radius) {
    vec2 q = vec2(length(pos.xy) - radius.x, pos.z);
    return sd_superellipse(q, radius.yz, 2.0);
}

vec3 get_gradient(vec3 pos, vec3 radii) {
    float delta = radii.z * 0.0001;

    float d0 = torus_SDF(pos, radii);
    float dx = torus_SDF(pos + vec3(delta, 0, 0), radii);
    float dy = torus_SDF(pos + vec3(0, delta, 0), radii);
    float dz = torus_SDF(pos + vec3(0, 0, delta), radii);

    vec3 gradient = vec3(dx - d0, dy - d0, dz - d0) / delta;

    return gradient;
}

vec4 torus_intersection(vec3 origin, vec3 direction, vec3 radius, out int num_roots, vec2 range) {
    vec3 position = origin;

    float total_dist = range.x;
    float start_dist = torus_SDF(position + direction * range.x, radius);
    float prev_dist = start_dist;
    float dist = start_dist;

    int counter = 0;
    int max_iter = 256;

    vec4 roots = vec4(-1);
    int num = 0;
    float sign_ = sign(start_dist);
    if(sign_ == 0) sign_ = 1;

    // ratio between (the difference between previous dist and current dist) / the previous dist

    while(num < 4 && counter < max_iter && total_dist <= range.y) {
        position = origin + direction * total_dist;

        float new_dist = torus_SDF(position, radius) * sign_;

        if(new_dist < 0.0) {
            vec2 range2 = vec2(total_dist - dist, total_dist);
            float rr = 0.5;
            float delta = 0.5;
            float rsign = sign_;
            // backtrack
            
            int max_steps = 32;
            for(int i = 0; i < max_steps; ++i) {
                float dd = range2.x + (range2.y - range2.x) * rr;
                vec3 current_pos = origin + direction * dd;

                if(sign(torus_SDF(current_pos, radius)) * sign_ == rsign) {
                    rsign *= -1.0;
                }
                rr += -delta * rsign;
                delta *= 0.5;
            }

            sign_ *= -1.0;
            roots[num] = range2.x + (range2.y - range2.x) * rr;
            ++num;
        }

        new_dist = abs(new_dist);

        dist = clamp(new_dist, radius.y * 0.05, radius.x * 2.0);
        total_dist += dist;

        ++counter;
    }

    num_roots = num;
    return roots;
}

float torus_approach(vec3 origin, vec3 direction, vec3 radius, vec2 range) {
    float current_pos = 0.5;
    float delta = 0.5;
    float phi = (1.0 + sqrt(5.0)) * 0.5;
    
    float r = range.y - range.x;
    vec2 current_range = range;

    float c = (current_range.y - current_range.x) / phi;
    float v = current_range.y - c;
    float fv = torus_SDF(origin + direction * v, radius);

    uint num_steps = 16;

    for(int i = 0; i < num_steps; ++i) {
        float c = (current_range.y - current_range.x) / phi;
        float a;
        float b;
        float fa;
        float fb;

        if(v - current_range.x < current_range.y - v) {
            a = v;
            fa = fv;

            b = current_range.x + c;
            fb = torus_SDF(origin + direction * b, radius);
        } else {
            a = current_range.y - c;
            fa = torus_SDF(origin + direction * a, radius);

            b = v;
            fb = fv;
        }

        if(fa > fb) {
            current_range = vec2(a, current_range.y);
            v = b;
            fv = fb;
        } else {
            current_range = vec2(current_range.x, b);
            v = a;
            fv = fa;
        }
    }

    return v;
}

float torus_step_approach(vec3 origin, vec3 direction, vec3 radius, vec2 range) {
    uint num_steps = 6;
    float r = range.y - range.x;

    float prev_d = range.x;
    float prev_sdf = torus_SDF(origin + direction * range.x, radius);
    float current_d = range.x;
    float current_sdf = prev_sdf;

    float min_d = range.x;
    float min_sdf = prev_sdf;

    for(int i = 0; i < num_steps; ++i) {
        float v = float(i + 1) / float(num_steps - 1);
        float next_d = range.x + r * v; 

        vec3 pos = origin + direction * next_d;
        float next_sdf = torus_SDF(pos, radius);

        if(current_sdf <= prev_sdf && current_sdf <= next_sdf) {
            float new_d = torus_approach(origin, direction, radius, vec2(prev_d, next_d));

            float new_sdf = torus_SDF(origin + direction * new_d, radius);

            if(new_sdf < min_sdf) {
                min_sdf = new_sdf;
                min_d = new_d;
            }
        }

        prev_d = current_d;
        prev_sdf = current_sdf;
        current_d = next_d;
        current_sdf = next_sdf;
    }

    return min_d;
}

vec4 blend(vec4 dst, vec4 src) {
    return vec4(dst.xyz * (1.0 - src.w) + src.xyz * src.w, max(dst.w, src.w));
}

float sample_texture(vec3 pos) {
    return textureGrad(density, pos, vec3(0.0001), vec3(0.0001))[1];
}

float fbm(vec3 pos, float freq, int octaves, float gain, float lacunarity, int n) {
    float accum = 0.0;
    for(int i = 0; i < octaves; ++i) {
        vec3 p = pos * pow(lacunarity, i) * (freq / 8.0);
        accum += (texture(density, p)[n] * 2.0 - 1.0) * pow(gain, i);
    }

    return accum;
}

vec3 calculate_flow(vec3 pos, float delta, vec2 aspect, vec2 scale) {
    vec3 ddx = normalize(cross(pos, vec3(0, 0, 1)));
    vec3 ddy = normalize(cross(ddx, pos));

    vec3 sc = vec3(scale.x, scale.x, scale.y);

    float d0 = fbm(pos * sc, 8.0, 1, 0.5, 2.0, 0);
    float dx = fbm(pos * sc + ddx * delta, 8.0, 1, 0.5, 2.0, 0);
    float dy = fbm(pos * sc + ddy * delta, 8.0, 1, 0.5, 2.0, 0);

    float curl_x = (d0 - dx) / delta;
    float curl_y = (d0 - dy) / delta;

    return -ddx * curl_y * aspect.x + ddy * curl_x * aspect.y;
}

float loop(double v, double period) {
    return float(v * period);
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

vec3 norm_torus(vec3 pos, vec3 radii) {
    vec2 n = normalize(pos.xy);
    vec3 closest_point = vec3(n * radii.x, 0);

    vec3 rel_pos = pos - closest_point;

    vec3 r = vec3(rel_pos.xy / radii.y, rel_pos.z / radii.z);
    r = normalize(r);
    r = vec3(r.xy * radii.y, r.z * radii.z);
    return closest_point + r;
}

vec4 sample_torus(vec3 pos) {

    return vec4(1.0, 0.25, 0.25, 0.5);
}

float sample_cloud_texture(vec3 pos, vec3 rand, bool f, bool ff) {
    float height = length(pos);
    vec3 prev_pos = pos;

    //double time = time2 * 0.033;

    // flow
    int num_steps = 3;
    float delta = 0.002;
    //pos += vec3(loop(time, 0.01), loop(time, 0.005), loop(time, 0.006));
    pos = normalize(pos);
    for(int i = 0; i < num_steps; ++i) {
        //vec3 flow = calculate_flow(current_pos, 0.01, vec2(1.0, 1.0) * 0.01, vec2(1.0, 1.0) * 0.075);
        vec3 flow = calculate_flow(pos, 0.01, vec2(1.0, 1.0) * delta / num_steps, vec2(1.0, 1.0) * 0.33);
        
        pos = normalize(pos + flow);
    }

    float coverage = 0.1;
    //
    float mask_scale = 1.38;
    float mask_contrast = 2.51;
    float mask_bias = 0.333;
    //
    float thin_scale = 19.62;
    float thin_cutoff = 0.574;
    float thin_softness = 0.522;
    float thin_weight = 0.519;
    //
    float puffy_scale = 2.5;
    int puffy_octaves = 4;
    float puffy_gain = 0.5;
    float puffy_lacunarity = 2.796;
    float puffy_sharpness = 1.0;
    float puffy_weight = 0.519;
    // 

    float thin_height = 0.005;
    float puffy_height = 0.1;
    if(ff) thin_height = 0.1;

    vec3 rand_thin = vec3(-1.344, 6.397, 2.996);// + vec3(0, 0, loop(time, 0.01));
    vec3 rand_puffy = vec3(2.023, -7.239, 1.552);// + vec3(0, loop(time, 0.02), 0);
    vec3 rand_mask = vec3(6.778, -1.345, -6.967);// + vec3(loop(time, 0.005), 0, 0);

    //
    float thin = fbm(pos + rand_thin, thin_scale, 2, 0.5, 2.0, 0);
    thin += fbm((pos + rand_thin) * 4.0, thin_scale, 2, 0.5, 2.0, 1) * 0.25;
    thin = smoothstep(thin_cutoff - thin_softness, thin_cutoff + thin_softness, thin);
    thin *= thin_weight;
    thin *= clamp(fbm(pos + rand_mask, 2.5, 2, 0.5, 2.0, 0) * 5, 0.0, 1.0);
    
    float h = clamp((height - 1.0 - thin_height) / -thin_height, 0.0, 1.0);
    thin *= h;
    thin = max(thin, 0.0);

    // 
    float puffy = fbm(pos + rand_puffy, puffy_scale, 1, 0.5, 2.0, 0);
    puffy -= fbm(pos + rand_puffy, puffy_scale * 2.0, puffy_octaves, puffy_gain, puffy_lacunarity, 1) * 0.5;
    puffy = 1.0 - pow(abs(puffy), puffy_sharpness) * sign(puffy);
    puffy *= puffy_weight;
    
    h = clamp((height - 1.0 - puffy_height) / -puffy_height, 0.0, 1.0);
    puffy *= h;
    //
    float mask = fbm(pos + rand_mask * 0.1, mask_scale, 1, 0.75, 2.0, 0);
    mask = pow(clamp(mask + mask_bias, 0.0, 1.0), mask_contrast);
    mask = 0.5;
    float ret = mix(thin, puffy, mask);
    //ret = puffy;//clamp((ret - coverage) / (1.0 - coverage), 0.0, 1.0);

    float falloff = 1.0;
    if(f) {
        //if(height > 1.0001) falloff = 0.0;
    }

    //float falloff = clamp(-(length(prev) - 1.0 - 0.01) / 0.01, 0.0, 1.0);

    return ret * falloff; 
}

vec3 sample_gradient(vec3 pos, vec3 rand, float delta) {
    float a = sample_cloud_texture(pos, rand, false, true);
    float x = sample_cloud_texture(pos + vec3(delta, 0, 0), rand, false, true);
    float y = sample_cloud_texture(pos + vec3(0, delta, 0), rand, false, true);
    float z = sample_cloud_texture(pos + vec3(0, 0, delta), rand, false, true);

    return vec3(x - a, y - a, z - a) / delta;
}

float sample_lighting(vec3 pos, vec3 rand, vec3 light_dir, bool s) {
    float light = 0.0;
    float step_size = 0.2;
    vec3 p = pos;
    if(!s) step_size = 0.01;

    int num_steps = 3;
    float pp = (hash_float(hash(uint((pos.x + 1.0) * 982173)) ^ hash(uint((pos.y + 1.0) * 2398409)) + hash(uint((pos.z + 1.0) * 38047009))) + 0.5) * (step_size / num_steps);
    for(int i = 0; i < num_steps; ++i) {
        pp += (step_size / num_steps);
        p = pos + light_dir * pp;

        light += (1.0 / num_steps) * (sample_cloud_texture(p, rand, s, false));
    }

    return exp(-light * 5);
}

// torus clouds

/*
float height = torus_SDF(pos, radii);
    vec3 prev_pos = pos;
    float div_value = min(radii.y, radii.z) * 0.5;

    //double time = time2 * 0.033;

    // flow
    int num_steps = 5;
    float delta = 0.035;
    //pos += vec3(loop(time, 0.01), loop(time, 0.005), loop(time, 0.006));
    for(int i = 0; i < num_steps; ++i) {
        //vec3 flow = calculate_flow(current_pos, 0.01, vec2(1.0, 1.0) * 0.01, vec2(1.0, 1.0) * 0.075);
        vec3 flow = calculate_flow(pos / div_value, 0.01, vec2(1.0, 1.0) * delta / num_steps, vec2(1.0, 1.0) * 0.03);
        
        pos = norm_torus(pos + flow * div_value, radii);
    }

    float coverage = 0.1;
    //
    float mask_scale = 1.38;
    float mask_contrast = 2.51;
    float mask_bias = 0.333;
    //
    float thin_scale = 19.62;
    float thin_cutoff = 0.574;
    float thin_softness = 0.522;
    float thin_weight = 0.519;
    //
    float puffy_scale = 2.5;
    int puffy_octaves = 4;
    float puffy_gain = 0.5;
    float puffy_lacunarity = 2.796;
    float puffy_sharpness = 1.0;
    float puffy_weight = 0.519;
    // 

    float thin_height = 0.005 * div_value;
    float puffy_height = 0.1 * div_value;
    if(ff) thin_height = 0.1 * div_value;

    vec3 rand_thin = vec3(-1.344, 6.397, 2.996);// + vec3(0, 0, loop(time, 0.01));
    vec3 rand_puffy = vec3(2.023, -7.239, 1.552);// + vec3(0, loop(time, 0.02), 0);
    vec3 rand_mask = vec3(6.778, -1.345, -6.967);// + vec3(loop(time, 0.005), 0, 0);

    //
    float thin = fbm(pos / div_value + rand_thin, thin_scale, 2, 0.5, 2.0, 0);
    thin += fbm((pos / div_value + rand_thin) * 4.0, thin_scale, 2, 0.5, 2.0, 1) * 0.25;
    thin = smoothstep(thin_cutoff - thin_softness, thin_cutoff + thin_softness, thin);
    thin *= thin_weight;
    thin *= clamp(fbm(pos / div_value + rand_mask, 2.5, 2, 0.5, 2.0, 0) * 5, 0.0, 1.0);
    
    float h = clamp((height - thin_height) / -thin_height, 0.0, 1.0);
    //thin *= h;
    //thin = max(thin, 0.0);

    // 
    float puffy = fbm(pos / div_value + rand_puffy, puffy_scale, 1, 0.5, 2.0, 0);
    puffy -= fbm(pos / div_value + rand_puffy, puffy_scale * 2.0, puffy_octaves, puffy_gain, puffy_lacunarity, 1) * 0.5;
    puffy = 1.0 - pow(abs(puffy), puffy_sharpness) * sign(puffy);
    puffy *= puffy_weight;
    
    h = clamp((height - puffy_height) / -puffy_height, 0.0, 1.0);
    //puffy *= h;
    //
    float mask = fbm(pos / (div_value * 10) + rand_mask, mask_scale, 5, 0.75, 2.0, 0);
    mask = pow(clamp(mask + mask_bias, 0.0, 1.0), mask_contrast);
    float ret = mask;//mix(thin, puffy, mask);
    //ret = clamp((ret - coverage) / (1.0 - coverage), 0.0, 1.0);

    float falloff = 1.0;
    if(f) {
        //if(height > 0.0001 / div_value) falloff = 0.0;
    }

    //float falloff = clamp(-(length(prev) - 1.0 - 0.01) / 0.01, 0.0, 1.0);

    return ret * falloff; 
}
*/

float sample_cloud_texture_torus(vec3 pos, vec3 radii, vec3 rand, bool f, bool ff) {
    float div_value = base_radius * 4.0;//min(radii.y, radii.z) * 0.5;
    float mask_scale = 1.38;
    float mask = fbm(pos / (div_value * 10), mask_scale, 5, 0.75, 2.0, 0);
    float ret = mask;

    return ret; 
}

vec3 sample_gradient_torus(vec3 pos, vec3 radii, vec3 rand, float delta) {
    float div_value = min(radii.y, radii.z);

    float a = sample_cloud_texture_torus(pos, radii, rand, false, true);
    float x = sample_cloud_texture_torus(pos + vec3(delta, 0, 0) * div_value, radii, rand, false, true);
    float y = sample_cloud_texture_torus(pos + vec3(0, delta, 0) * div_value, radii, rand, false, true);
    float z = sample_cloud_texture_torus(pos + vec3(0, 0, delta) * div_value, radii, rand, false, true);

    return vec3(x - a, y - a, z - a);
}

float sample_lighting_torus(vec3 pos, vec3 radii, vec3 rand, vec3 light_dir, bool s) {
    float div_value = min(radii.y, radii.z);

    float light = 0.0;
    float step_size = 0.2 * div_value;
    vec3 p = pos;
    if(!s) step_size = 0.01 * div_value;

    int num_steps = 3;
    float pp = (hash_float(hash(uint((pos.x + 1.0) * 982173)) ^ hash(uint((pos.y + 1.0) * 2398409)) + hash(uint((pos.z + 1.0) * 38047009))) + 0.5) * (step_size / num_steps);
    for(int i = 0; i < num_steps; ++i) {
        pp += (step_size / num_steps);
        p = pos + light_dir * pp;

        light += (1.0 / num_steps) * (sample_cloud_texture_torus(p, radii, rand, s, false));
    }

    return exp(-light * 5);
}

vec3 sunset_clouds = vec3(1.0, 0.5, 0.3);

// x = front lighting color
// y = back lighting color
// z = sunset color
vec4 sample_atmosphere(vec3 pos, vec3 ray_dir, vec3 sun_pos, float planet_radius, float atmo_width) {
    vec4 ret = vec4(0.0);
    
    vec3 center_pos = pos;

    float ii = 1.0 - (length(center_pos) - planet_radius) / atmo_width;
    ii = clamp(ii, 0.0, 1.0);

    vec3 normal = normalize(center_pos);

    vec3 sun_direction = normalize(sun_pos - center_pos);

    float ndotl = dot(normal, sun_direction);
    float wrap = 0.15;
    float ndotl_wrap = clamp((ndotl + wrap) / (1.0 + wrap), 0.0, 1.0);

    wrap = 0.15;
    float ndotl_wrap2 = clamp((ndotl + wrap) / (1.0 + wrap), 0.0, 1.0);

    float rdots = max(0.0, dot(ray_dir, sun_direction));
    float sun_amount = rdots;

    //vec3 back_lighting_color = vec3(1.0, 0.0, 0.0);
    //vec3 front_lighting_color = vec3(0.2, 0.5, 1.0);

    ret.x = ndotl_wrap;
    ret.y = 1.0 - ndotl_wrap;
    ret.w = ndotl_wrap2;
    
    //vec3 fog_color = mix(back_lighting_color, front_lighting_color, ndotl_wrap);

    float extinction_factor = ndotl_wrap * ii;

    float scatter_width = 0.38;
    float scatter = smoothstep(0.0, scatter_width, ndotl_wrap) * smoothstep(scatter_width * 2.0, scatter_width, ndotl_wrap);
    scatter *= sun_amount;

    ret.x = mix(ret.x, 0, scatter);
    ret.y = mix(ret.y, 0, scatter);
    ret.z = scatter;

    ret.xyz *= extinction_factor;

    return ret;
}


vec4 sample_atmosphere_torus(vec3 pos, vec3 ray_dir, vec3 sun_pos, vec3 normal) {
    vec4 ret = vec4(0.0);
    
    vec3 center_pos = pos;

    vec3 sun_direction = normalize(sun_pos - center_pos);

    float ndotl = dot(normal, sun_direction);
    float wrap = 0.15;
    float ndotl_wrap = clamp((ndotl + wrap) / (1.0 + wrap), 0.0, 1.0);

    wrap = 0.15;
    float ndotl_wrap2 = clamp((ndotl + wrap) / (1.0 + wrap), 0.0, 1.0);

    float rdots = max(0.0, dot(ray_dir, sun_direction));
    float sun_amount = rdots;

    ret.x = ndotl_wrap;
    ret.y = 1.0 - ndotl_wrap;
    ret.w = ndotl_wrap2;

    float extinction_factor = ndotl_wrap;

    float scatter_width = 0.38;
    float scatter = smoothstep(0.0, scatter_width, ndotl_wrap) * smoothstep(scatter_width * 2.0, scatter_width, ndotl_wrap);
    scatter *= sun_amount;

    ret.x = mix(ret.x, 0, scatter);
    ret.y = mix(ret.y, 0, scatter);
    ret.z = scatter;

    ret.xyz *= extinction_factor;

    return ret;
}

vec3 sunset_col = vec3(1.0, 0.5, 0.2);
vec3 back_lighting_color = vec3(1.0, 0.5, 0.2);
vec3 front_lighting_color = vec3(0.2, 0.5, 1.0);

void main() {

    // get depth
    ivec2 size = textureSize(depth_tex, 0);
    ivec2 texel = ivec2((screen_coord * 0.5 + 0.5) * size);

    ivec2 ftexel = ivec2(floor(gl_FragCoord.xy));
    vec4 color = texelFetch(depth_tex, ftexel, 0);

    float depth = color.r;

    depth = -get_depth(depth, inverse_proj);
    
    vec4 raw_dir = inverse_proj * vec4(screen_coord, 0.5, 1.0);
    raw_dir /= raw_dir.w;

    vec3 pixel_dir = -raw_dir.xyz;
    pixel_dir = normalize(pixel_dir);

    float factor = dot(vec3(0, 0, 1), pixel_dir);

    depth /= factor;

    pixel_dir = mat3(inverse_view) * (-pixel_dir); // inverse_view flips the direction
    //pixel_dir.z = -pixel_dir.z;

    // ray trace

    vec4 ori = vec4(0, 0, 0, 1);
    ori = rel_mat * ori;

    vec3 ray_dir = mat3(rel_mat) * pixel_dir;
    vec3 ray_origin = ori.xyz;

    float t = plane_intersection(ray_origin, ray_dir);

    vec3 rel_pos = vec3(inverse_view * vec4(0, 0, 0, 1));

    // compare

    float pixel_size = base_radius / 512 * 6.5;

    vec3 position = ray_origin + ray_dir * t;
    float dist = length(position);

    vec3 sun_rel = vec3(rel_mat * vec4(sun_pos, 1.0));
    vec3 sun_dir = normalize(sun_rel);

    vec3 moon_pos = vec3(rel_mat * (moon_mat * vec4(0.0, 0.0, 0.0, 1.0)));

    vec4 tt0 = sphere_intersection(position, sun_dir, base_radius * 6.5, vec3(0, 0, 0));
    vec4 tt1 = sphere_intersection(position, sun_dir, moon_size, moon_pos);
    bool is_intersect0 = tt0.a < (base_radius * 6.5) && tt0.y > 0.0;
    bool is_intersect1 = tt1.a < moon_size && tt1.y > 0.0;

    bool is_intersect = is_intersect0 || is_intersect1;

    // torus

    vec4 torus_color = vec4(0.0);

    vec3 main_ray_origin = vec3(torus_mat * vec4(0, 0, 0, 1));
    vec3 main_ray_dir = mat3(torus_mat) * pixel_dir;
    vec3 main_sun_pos = vec3(torus_mat * vec4(sun_pos, 1.0));
    
    vec3 radii = torus_radii + vec3(0.0, atmosphere_thickness, atmosphere_thickness);
    float sphere_radius = radii.x + max(radii.y, radii.z);

    int num_roots;
    vec4 bounding_sphere = sphere_intersection(main_ray_origin, main_ray_dir, sphere_radius, vec3(0.0));
    vec2 range = vec2(bounding_sphere.x, bounding_sphere.z);
    float epsilon = radii.y * 0.1;
    range.x -= epsilon;
    range.y += epsilon;

    if(bounding_sphere.w < sphere_radius && bounding_sphere.z > 0.0) {

        vec4 ttt = torus_intersection(main_ray_origin, main_ray_dir, radii, num_roots, range);
        ttt = max(ttt, vec4(0.0));

        vec3 light_dir = normalize(vec3(1));

        if(num_roots != 0) {
            float thickness0 = 0.0;
            float fade0 = 0.0;
            float thickness1 = 0.0;
            float fade1 = 0.0;
            vec3 normal0 = vec3(0.0);
            vec3 normal1 = vec3(0.0);
            float v0;
            float v1;

            /*
            */

            float atmo_density = 0.15;

            v0 = torus_step_approach(main_ray_origin, main_ray_dir, radii, vec2(ttt[0], ttt[1]));
            float sdf0 = torus_SDF(main_ray_origin + main_ray_dir * clamp(v0, 0.0, depth), radii);
            float n0 = (sdf0 + atmosphere_thickness) / (atmosphere_thickness);
            n0 = 1.0 - clamp(n0, 0.0, 1.0);
            if(depth < ttt[1]) {
                float thickness = clamp(ttt[1], 0.0, depth) - clamp(ttt[0], 0.0, depth);

                thickness0 = thickness;
            } else {
                fade0 = n0;
            }
            float d0 = clamp(v0, 0.0, depth);
            normal0 = get_gradient(main_ray_origin + main_ray_dir * clamp(v0, 0.0, depth), radii);
            
            if(num_roots > 2) {
                v1 = torus_step_approach(main_ray_origin, main_ray_dir, radii, vec2(ttt[2], ttt[3]));
                float sdf1 = torus_SDF(main_ray_origin + main_ray_dir * clamp(v1, 0.0, depth), radii);
                float n1 = (sdf1 + atmosphere_thickness) / (atmosphere_thickness);
                n1 = 1.0 - clamp(n1, 0.0, 1.0);
                if(depth < ttt[3]) {
                    float thickness = clamp(ttt[3], 0.0, depth) - clamp(ttt[2], 0.0, depth);
                    thickness1 = thickness;
                } else {
                    fade1 = n1;
                }
                float d1 = clamp(v1, 0.0, depth);
                normal1 = get_gradient(main_ray_origin + main_ray_dir * clamp(v1, 0.0, depth), radii);
            }
            
            vec3 center_pos;
            vec3 weights;
            vec3 vcolor;
            float transmittance;
            vec4 col;
            float total_fade = fade0 + fade1;
            float max_fade = (max(fade0, fade1) + total_fade) * 0.5;

            if(ttt[1] > 0.0) {
                center_pos = main_ray_origin + main_ray_dir * clamp(v0, 0.0, depth);
                weights = sample_atmosphere_torus(center_pos, main_ray_dir, main_sun_pos, normal0).xyz;
                vcolor = weights.x * front_lighting_color + weights.y * back_lighting_color + weights.z * sunset_col;

                transmittance = 1.0 - exp(-(thickness0 + thickness1) / atmosphere_thickness * atmo_density);
                col = vec4(vcolor * transmittance, transmittance);
                
                torus_color = blend(torus_color, col);
                if(total_fade != 0) torus_color += vec4(vcolor * (fade0 / total_fade * max_fade), 0.0); 
            } else {
                total_fade = fade1;
                max_fade = fade1;
            }

            if(num_roots > 2 && ttt[3] > 0.0) {
                center_pos = main_ray_origin + main_ray_dir * clamp(v1, 0.0, depth);
                vec3 weights = sample_atmosphere_torus(center_pos, main_ray_dir, main_sun_pos, normal1).xyz;
                vec3 vcolor = weights.x * front_lighting_color + weights.y * back_lighting_color + weights.z * sunset_col;
            
                transmittance = 1.0 - exp(-thickness1 / atmosphere_thickness * atmo_density);
                col = vec4(vcolor * transmittance, transmittance);

                torus_color = blend(torus_color, col);
                if(total_fade != 0) torus_color += vec4(vcolor * (fade1 / total_fade * max_fade), 0.0);
            }
        }
    }

    // torus clouds
    radii = torus_radii + vec3(0.0, atmosphere_thickness * 0.4, atmosphere_thickness * 0.4);
    
    vec4 raycast = torus_intersection(main_ray_origin, main_ray_dir, radii, num_roots, range);

    vec4 cloud_color = vec4(0.0);

    for(int i = num_roots - 1; i >= 0; --i) {
        float dist = raycast[i];

        if(dist >= 0.0 && dist < depth) {
            vec3 pos = main_ray_origin + main_ray_dir * dist;
            vec3 dir = normalize(main_sun_pos - pos);

            vec3 normal = normalize(get_gradient(pos, radii));

            //

            bool is_inside = (i % 2 == 0);

            if(i == 0) {
                vec3 rand = vec3(0.0);

                vec4 weights = sample_atmosphere_torus(pos, main_ray_dir, main_sun_pos, normal);

                float density = sample_cloud_texture_torus(pos, radii, rand, false, false);

                density = min(1.0, density * 4);
                density = smoothstep(0.0, 1.0, density);
                
                vec3 direction = sample_gradient_torus(pos, radii, rand, 0.005);
                float dd = clamp(dot(normalize(-direction), dir), 0.0, 1.0);
                float factor = mix(0.85, 1.0, dd);

                vec3 light = mix(vec3(1.0), sunset_clouds, 1.0 - weights.w);
                
                light *= sample_lighting_torus(pos, radii, rand, dir, is_inside);
                light *= weights.w;
                light *= factor;
                
                float ddd = length(pos - main_ray_origin);
                float f = ddd / (20.0 * base_radius) * 2;
                vec3 c = get_color(f, 1.0, 1.0);

                //vec4 new_color = vec4(light * c, density);
                vec4 new_color = vec4(c, density);
                cloud_color = blend(cloud_color, new_color);
            }
        }
    }

    if(cloud_color.w != 0.0) {
        cloud_color.xyz /= cloud_color.w;
        torus_color = blend(torus_color, cloud_color);
    }
    

    // rings
    
    if(t > 0.0 && t < depth) {
        float shadow = 1.0;
        if(is_intersect) shadow = 0.0;

        //vec3 pixel = floor(position / pixel_size) * pixel_size;
        vec3 pixel = position;
        dist = length(pixel);
        
        if(dist > radii.x && dist < radii.y) {
            vec3 color3 = color1 + (color2 - color1) * hash_float(0x1743);
            float center = (dist - radii.x) / (radii.y - radii.x);
            float density = max(0.0, texture(density, vec3(center, 0, 0) * 0.75 + vec3(hash_float(0x1E), hash_float(0x1F), hash_float(0x20))).r * 1.5 - 0.5);
            density = pow(density, 0.5);
        
            frag_color = vec4(color3 * shadow * density, density);
        }
    }
    


    main_ray_origin = vec3(earth_mat * vec4(0, 0, 0, 1));
    main_ray_dir = mat3(earth_mat) * pixel_dir;
    main_sun_pos = vec3(earth_mat * vec4(sun_pos, 1.0));

    float atmo_width = 8000;

    uint num_shells = 1;
    float shell_dist = atmo_width * 0.25 / num_shells;
    float shell_start = base_radius * 1.0 + atmo_width * 0.4;

    seed = 0;

    vec4 output_col = vec4(0.0);
    vec3 sunset_color = vec3(1.0, 0.5, 0.3);
    float falloff_start = atmo_width;
    float falloff_thickness = atmo_width * 0.5;

    for(int i = int(num_shells) - 1; i >= 0; --i) {
        float radius = shell_start + shell_dist * i;

        vec4 cloud_raycast = sphere_intersection(main_ray_origin, main_ray_dir, radius, vec3(0, 0, 0));

        vec3 rand = vec3(0.0);//vec3(hash_float(i), hash_float(i * 2), hash_float(i * 3));

        if(cloud_raycast.w < radius) {
            // inside
            if(cloud_raycast.z > 0.0) {
                if(cloud_raycast.z < depth) {
                    vec3 pos = main_ray_origin + main_ray_dir * cloud_raycast.z;
                    vec3 dir = normalize(main_sun_pos - pos);

                    vec4 weights = sample_atmosphere(pos / radius, main_ray_dir, main_sun_pos, base_radius * 0.25, atmo_width);

                    float density = sample_cloud_texture(pos / radius, rand, false, false);

                    density = min(1.0, density * 4);
                    density = smoothstep(0.0, 1.0, density);
                    
                    vec3 direction = sample_gradient(pos / radius, rand, 0.005);
                    float d = clamp(dot(normalize(-direction), dir), 0.0, 1.0);
                    float factor = mix(0.85, 1.0, d);

                    vec3 light = mix(vec3(1.0), sunset_clouds, 1.0 - weights.w);
                    
                    light *= sample_lighting(pos / radius, rand, dir, false);
                    light *= weights.w;
                    light *= factor;

                    vec4 new_color = vec4(light, density);
                    //vec4 new_color = vec4(0.0, 1.0, 0.0, density);
                    output_col = blend(output_col, new_color);
                }
            }
        } else {
            //output_col = vec4(1.0, 0.0, 0.0, 1.0);
        }
    }

    for(int i = 0; i < num_shells; ++i) {
        float radius = shell_start + shell_dist * i;

        vec4 cloud_raycast = sphere_intersection(main_ray_origin, main_ray_dir, radius, vec3(0, 0, 0));
        
        vec3 rand = vec3(hash_float(i), hash_float(i * 2), hash_float(i * 3));

        if(cloud_raycast.w < radius) {
            // outside
            if(cloud_raycast.x > 0.0 && cloud_raycast.x < depth) {
                vec3 pos = main_ray_origin + main_ray_dir * cloud_raycast.x;
                vec3 dir = normalize(main_sun_pos - pos);

                vec4 weights = sample_atmosphere(pos / radius, main_ray_dir, main_sun_pos, base_radius * 0.25, atmo_width);

                float density = sample_cloud_texture(pos / radius, rand, false, false);

                density = min(1.0, density * 4);
                density = smoothstep(0.0, 1.0, density);

                vec3 direction = sample_gradient(pos / radius, rand, 0.005);
                float d = clamp(dot(normalize(-direction), dir), 0.0, 1.0);
                float factor = mix(0.85, 1.0, d);

                vec3 light = mix(vec3(1.0), sunset_clouds, 1.0 - weights.w);
                
                light *= sample_lighting(pos / radius, rand, dir, true);//change
                light *= weights.w;
                light *= factor;

                vec4 new_color = vec4(light, density);
                output_col = blend(output_col, new_color);
            }

        }
    }

    if(output_col.w != 0.0) output_col.xyz /= output_col.w;

    // atmosphere
    
    frag_color = torus_color;

    float planet_radius = base_radius * 1.0;

    vec4 atmo_raycast = sphere_intersection(main_ray_origin, main_ray_dir, planet_radius + atmo_width, vec3(0.0));

    if(atmo_raycast.a < planet_radius + atmo_width) {
        vec3 center_pos = main_ray_origin + main_ray_dir * min(depth, max(0.0, atmo_raycast.y));

        vec4 weights = sample_atmosphere(center_pos, main_ray_dir, main_sun_pos, planet_radius, atmo_width);

        vec3 vcolor = weights.x * front_lighting_color + weights.y * back_lighting_color + weights.z * sunset_col;

        float start = atmo_raycast.x;
        float atmo_thickness = min(depth, atmo_raycast.z) - max(0.0, atmo_raycast.x);

        if(depth < atmo_raycast.z) {
            float atmo_density = 0.15;

            float thickness = max(atmo_thickness, 0.0);

            float transmittance = 1.0 - exp(-thickness / atmo_width * atmo_density);
            //transmittance *= 2.0;
            vec4 col = vec4(vcolor * transmittance, transmittance);

            frag_color = blend(frag_color, col);
        } else {
            frag_color += vec4(vcolor, 0.0);
        }
    }
    
    frag_color = blend(frag_color, output_col);

    /*if(depth < 60) {
        frag_color = vec4(get_color(depth / 10, 1.0, 1.0), 0.5);
    }*/
}

/*

    for(int i = 0; i < num_storms; ++i) {
        vec4 storm = storms[i];

        float z_angle = storm.x;
        float y_angle = storm.y;
        float scale = storm.z;
        float radius = storm.w;

        mat3 z_mat = get_rot_z(z_angle);
        mat3 y_mat = get_rot_y(y_angle);
        
        vec3 storm_pos = z_mat * y_mat * vec3(1, 0, 0);
        
        rpos = transpose(y_mat) * transpose(z_mat) * rpos;
        rpos *= vec3(1.0, 1.0, scale);
        vec3 rel = rpos - vec3(1, 0, 0);
        float len = length(rel);
        


        float displacement = len - radius;
        if(len > radius) {
            vec3 tangent_dir = rel;

            float displacement2 = exp(-(displacement / radius) * 0.5);

            rpos += -tangent_dir * displacement2;
        }
        
        mat2 ori = get_rot(exp(-((displacement - radius) / radius) * 1.0));
        rpos = vec3(rpos.x, ori * rpos.yz);


        rpos /= vec3(1.0, 1.0, scale);
        rpos = z_mat * y_mat * rpos;
        rpos = normalize(rpos);


        
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
    */