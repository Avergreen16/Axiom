#version 460 core

layout(binding = 0) uniform sampler2D depth_tex;
layout(binding = 1) uniform sampler3D density_tex;

layout(location = 0) uniform mat4 proj;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 model;

layout(location = 3) uniform vec3 axes;
layout(location = 4) uniform float light;
layout(location = 5) uniform vec3 color2;
layout(location = 6) uniform vec3 core_size;

layout(location = 7) uniform mat4 models[8];

float light_year = pow(2, 48);
vec3 color = vec3(0x80, 0xA0, 0xff) / float(0xFF);

layout(location = 0) in vec2 screen_coord;

mat4 inv_proj = inverse(proj);
mat4 inv_view = inverse(view);
mat4 inv_model = inverse(model);

out vec4 frag_color;

float falloff = 1.0;

float pi = 3.14159265358979;

mat3 z_rot(float r) {
    vec3 x = vec3(cos(r), sin(r), 0);
    vec3 y = vec3(-sin(r), cos(r), 0);
    vec3 z = vec3(0, 0, 1);

    return mat3(x, y, z);
}

uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

uint hash(uvec2 v) { 
    return hash(v.x ^ hash(v.y)); 
}

uint hash(uvec3 v) { 
    return hash(v.x ^ hash(v.y) ^ hash(v.z)); 
}

uint hash(uvec4 v) {
    return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w)); 
}

float to_float(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat(m);       // Range [1:2]
    return f - 1.0;                // Range [0:1]
}

float get_depth(float d, mat4 inv_proj) {
    vec4 dv = vec4(0, 0, d, 1.0);
    dv = inv_proj * dv;
    dv /= dv.w;

    return dv.z;
}

vec3 get_pos(vec4 p) {
    vec4 a = inv_proj * p;
    a /= a.w;
    a = inv_view * a;

    return vec3(a);
}

float ray_plane(vec3 origin, vec3 direction, vec3 normal) {
    float f0 = dot(origin, normal);
    float f1 = dot(direction, normal);

    return -f0 / f1;
}

bool ray_cube(vec3 origin, vec3 direction, vec3 axes, out float i0, out float i1) {
    float enter = 3.402823466E+38;
    float exit = 3.402823466E+38;

    vec3 positions[6] = {
        vec3(axes.x, 0, 0),
        vec3(0, axes.y, 0),
        vec3(0, 0, axes.z),
        vec3(-axes.x, 0, 0),
        vec3(0, -axes.y, 0),
        vec3(0, 0, -axes.z)
    };

    vec3 normals[6] = {
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(-1, 0, 0),
        vec3(0, -1, 0),
        vec3(0, 0, -1)
    };

    int n[6] = {
        0, 1, 2, 0, 1, 2
    };

    for(int i = 0; i < 6; ++i) {
        float a = dot(origin - positions[i], normals[i]);
        float b = dot(direction, normals[i]);
        float c = -a / b;
        if(c > 0) {
            vec3 v = origin + direction * c;

            int ni = n[i];

            int n0 = int(mod(ni - 1, 3));
            int n1 = int(mod(ni + 1, 3));

            if(abs(v[n0]) < axes[n0] && abs(v[n1]) < axes[n1]) {
                if(b > 0) {
                    if(c < exit) exit = c;
                } else {
                    if(c < enter) enter = c;
                }
            }
        }
    }

    if(enter == 3.402823466E+38) enter = 0;
    
    vec3 v = origin + direction * (enter + exit) * 0.5;

    v = abs(v);

    if(v.x < axes.x && v.y < axes.y && v.z < axes.z) {
        i0 = enter;
        i1 = exit;

        return true;
    } else {
        return false;
    }
}

vec3 sphere_raycast(vec3 center, float radius, vec3 origin, vec3 dir) {
    vec3 rel_center = center - origin;

    float closest_pos = dot(rel_center, dir);

    vec3 c = -rel_center + dir * closest_pos;

    float dist = length(c);

    if(dist > radius) return vec3(0, closest_pos, 0);

    float t = sqrt(radius * radius - dist * dist);

    float t0 = closest_pos - t;
    float t1 = closest_pos + t;
    //t0 = max(t0, 0);

    return vec3(t0, closest_pos, t1);
}

vec3 ellipsoid_raycast(vec3 center, vec3 radius, vec3 origin, vec3 dir, mat3 orientation) {
    mat3 inv_ori = transpose(orientation);
    vec3 new_origin = origin - center;

    vec3 new_dir = inv_ori * dir;
    new_origin = inv_ori * new_origin;

    vec3 rotated_dir = new_dir;

    new_dir = normalize(new_dir / radius);
    new_origin = new_origin / radius;

    vec3 r = sphere_raycast(vec3(0.0), 1, new_origin, new_dir);

    vec3 t0 = new_dir * r.x;
    vec3 tc = new_dir * r.y;
    vec3 t1 = new_dir * r.z;

    //vec3 new_o = new_origin * radius;

    vec3 t;
    t.x = dot(t0 * radius, rotated_dir);
    t.y = dot(tc * radius, rotated_dir);
    t.z = dot(t1 * radius, rotated_dir);

    return t;
}

float sample_octaves(vec3 v, int octaves, float roughness) {
    vec3 a = v / 2.0;

    float tex = 0.0;
    for(int i = 0; i < octaves; ++i) {
        float tex_sample = texture(density_tex, a).r * 2.0 - 1.0;
        tex += tex_sample * pow(roughness, i);
        a *= 2.0;
    }
    
    return tex;
}

float radius = 1.0;
float thickness = 0.4;

vec2 sample_texture(vec3 v, float bar) {
    uint hash_v = hash(uvec3(color * 4096));
    float xv = to_float(hash_v);
    hash_v = hash(hash_v);
    float yv = to_float(hash_v);
    hash_v = hash(hash_v);
    float zv = to_float(hash_v);

    vec3 vvv = vec3(xv, yv, zv) * -5.0;
    //vvv = round(vvv * 4) / 4;
    //vvv = vec3(0.0);

    vec3 ff = v;

    vec2 f = ff.xy;

    vec3 vv = vec3(ff.xy, 0.0);

    vv *= 1.0;

    int num_arms = 3;

    float tex = 0.0;

    float twist = -2.75;

    float r = exp(bar / twist);

    float dust = 0.0;
    
    float tex_sample = pow(1.0 - length(v), 0.5);

    for(int i = 0; i < num_arms; ++i) {
        float f = float(i) / num_arms;
        mat3 rot = z_rot((f + sample_octaves(vv * 1.0, 3, 0.65) * 0.04) * pi * 2.0);// 

        vec3 nv = rot * vv;
        nv /= radius;
        //nv -= vec3(0.33, 0, 0);

        vec3 prev = nv;

        float len = length(nv);
        float t = (log(len)) * twist;
        mat3 zr = z_rot(t);
        nv = zr * nv;

        float bulge_arm_falloff = 1.0;
        float fade = 0.1;
        //if(prev.y < 0.0) bulge_arm_falloff = 0.0;

        //nv = rot * nv;

        if(nv.x > 0.0) tex = max(tex, pow(1.0 - abs(nv.y) / len, 2.0) * tex_sample);// ;
        //if(nv.y > 0.0) dust += max(0.0, pow(1.0 - abs(nv.x) / len, 6.0)) * tex_sample;
    }
    //vv *= 0.5;
    
    vec3 nv = vv;
    float len = length(nv.xy);
    float t = (log(len)) * twist;
    mat3 zr = z_rot(t * 0.45);
    nv = zr * nv;

    vec3 p = nv;
    float len2 = length(p);
    vec2 xy = p.xy;
    vec3 pos = vec3(log(len2), normalize(xy));
    
    float v0 = 1.0;//sample_octaves(pos, 1, 0.5);

    tex_sample *= (v0 * 0.5 + 0.5) * 0.65;
    tex_sample *= tex;
    //tex_sample += sample_octaves(ff * 4.0, 3, 0.65) * 0.5 * tex;
    tex_sample *= 1.5;

    dust *= sample_octaves(vv * 4.0, 3, 0.65) * 1.0 + 0.75;
    dust *= smoothstep(1.0, 0.0, abs(ff.z) / 0.1);
    dust = max(0.0, dust);

    float core_factor = clamp(length(v / (core_size / axes)) - 0.5, 0.0, 1.0);
    /*tex_sample /= 0.55;
    tex_sample = clamp(0.0, 1.0, tex_sample);
    tex_sample = pow(tex_sample, 2);*/

    return vec2(tex_sample * core_factor, dust * core_factor * 2.0);
}

float plane_intersection(vec3 origin, vec3 direction) {
    vec3 normal = vec3(0, 0, 1);

    float height_origin = dot(normal, origin);
    float direction_dot = dot(normal, direction);

    float t = -height_origin / direction_dot;

    return t;
}

float density_value = 2.0;

bool ray_plane(vec3 plane_origin, vec3 plane_normal, vec3 origin, vec3 direction, vec2 axes, out float i0) {
    vec3 rel_origin = origin - plane_origin;

    float height = dot(plane_normal, rel_origin);
    float dir = dot(plane_normal, direction);

    float t = -height / dir;

    vec3 v = rel_origin + direction * t;

    if(v.x < axes.x && v.x > -axes.x && v.y < axes.y && v.y > -axes.y) {
        i0 = t;
        return true;
    }
    return false;
}

struct galaxy_data {
    vec3 position;
    mat3 orientation;
    vec3 size;
    vec3 core_size;
    vec3 color;
    float period;
    vec3 noise_offset;
};

vec3 galaxy_ray(vec3 ray_origin, vec3 ray_direction, float depth, galaxy_data data) {
    mat3 inv = transpose(data.orientation);
    vec3 new_origin = inv * (ray_origin - data.position);
    vec3 new_direction = inv * ray_direction;

    vec3 size = max(data.size, data.core_size);

    float i0;
    float i1;

    //ray_plane(vec3(0.0), vec3(0.0, 0.0, 1.0), new_origin, new_direction, size.xy, i0);

    vec3 accum_color = vec3(0, 0, 0);
    float transmittance = 0;
    int num_steps = 32;

    vec3 far = data.color;
    vec3 near = vec3(1.0, 0.75, 0.5);

    bool intersection = ray_cube(new_origin, new_direction, size, i0, i1);
    i0 = min(max(i0, 0.0), depth);
    i1 = min(i1, depth);
    float thickness = i1 - i0;

    float minimum = min(min(size.x, size.y), size.z);
    
    if(intersection && thickness > 0.0) {
        float multiplier = 25.0 / num_steps * (thickness / minimum);

        if(i0 >= 0.0) {
            for(int i = 0; i < num_steps; ++i) {
                vec3 pos = new_origin + new_direction * (i0 + thickness * ((float(i) + to_float(hash(uvec3(ray_direction * 5000.0 + 5000.0)))) / num_steps));

                vec3 pos2 = pos / data.size;

                vec3 vv = vec3(pos2.xy, 0);
                
                // twist
                float len = length(vv.xy);
                float len2 = length(pos.xy);
                vec3 pos3 = pos2;

                radius = 1.0;

                vec3 b = data.core_size / data.size;
                float bar_length = max(data.core_size.x, data.core_size.y);

                float bar_falloff = smoothstep(-bar_length * 0.66, 0.0, len2 - bar_length);

                vec2 d = sample_texture(pos3, max(b.x, b.y)); //  * data.period + data.noise_offset
                float density = d.x;
                float dust = d.y;

                falloff = smoothstep(0.0, 1.0, falloff);
                density *= falloff * bar_falloff;

                density = max(density, 0.0);
                if(length(data.size) == 0.0) {
                    len = 0.0;
                    density = 0.0;
                }

                float l0 = len;//pow(l, 0.5);

                vec3 c = mix(near, far, l0);
                

                float core_factor = length(pos / data.core_size);
                core_factor = abs(core_factor);
                core_factor = exp(-core_factor * 6.0) * (1.0 - core_factor);
                core_factor = max(core_factor, 0.0);

                vec3 closest_pt = vec3(0.0);
                float dist = length(vv - closest_pt);
                dist = pow(dist, 0.5);

                float x = dist;
                float y = abs(pos2.z * 5);
                float gradient = max(0.0, (1.0 - x)) * max(0.0, (1.0 - y)) * length(1.0 - vec2(x, y));
                gradient = pow(gradient, 0.75);

                gradient = max(0.0, gradient);
                density = max(0.0, density);
                density *= gradient;
                dust *= gradient;
                gradient = pow(gradient, 1.5);
                density += gradient * 0.2;

                density *= (1.0 - core_factor);

                float transmittance_factor = 1.0;

                transmittance += density * multiplier;
                //transmittance += dust * multiplier * 2;
                accum_color += c * density * multiplier * exp(-transmittance * transmittance_factor);
                //accum_color += vec3(0.2, 0.15, 0.1) * dust * multiplier * exp(-transmittance * transmittance_factor);

                core_factor *= 0.3;
                density = core_factor;
                c = vec3(1.0, 0.75, 0.5);
                
                accum_color += c * density * multiplier * exp(-transmittance * transmittance_factor);
            }
        }
    } else accum_color = vec3(0, 0, 0);

    return accum_color;
}

void main() {
    discard;
    // get depth
    ivec2 size = textureSize(depth_tex, 0);
    ivec2 texel = ivec2((screen_coord * 0.5 + 0.5) * size);

    vec4 c = texelFetch(depth_tex, texel, 0);

    float depth = c.r;

    depth = -get_depth(depth, inv_proj);
    
    vec4 raw_dir = inv_proj * vec4(screen_coord, 0.5, 1.0);
    raw_dir /= raw_dir.w;

    vec3 frag_dir = raw_dir.xyz;
    frag_dir = normalize(frag_dir);

    float factor = dot(vec3(0, 0, 1), frag_dir);

    depth /= factor;

    depth = -depth;

    //

    frag_dir = mat3(inv_view) * frag_dir;

    vec3 center = vec3(model * vec4(0.0, 0.0, 0.0, 1.0));

    float pixel_size = axes.z / 24;
    vec3 n_frag_dir = (mat3(view) * frag_dir);
    n_frag_dir /= abs(n_frag_dir.z);
    float center_dist = length(center);

    vec4 ps = proj * vec4(pixel_size, 0, -center_dist, 1.0);
    ps /= ps.w;
    pixel_size = ps.x;

    vec4 center_pos = proj * vec4(center, 1.0);
    center_pos /= center_pos.w;
    vec2 center_offset = center_pos.xy;

    pixel_size = min(pixel_size, 1.0 / 100);

    //n_frag_dir = vec3(round((n_frag_dir.xy - center_offset) / pixel_size) * pixel_size + center_offset, n_frag_dir.z);
    n_frag_dir = normalize(n_frag_dir);
    
    vec3 prev_frag_dir = frag_dir;

    frag_dir = mat3(inv_view) * n_frag_dir;
    vec3 r_origin = vec3(0.0);

    vec3 facing = vec3(inv_view * vec4(0, 0, -1, 1));

    ivec2 fc = ivec2(floor(gl_FragCoord.xy));

    galaxy_data data;
    data.position = vec3(models[0] * vec4(0.0, 0.0, 0.0, 1.0));
    data.orientation = mat3(models[0]);
    data.size = vec3(light_year * 30000.0, light_year * 30000.0, light_year * 6000.0);
    data.core_size = vec3(light_year * 10000.0, light_year * 4000.0, light_year * 4000.0);
    data.color = color;
    data.period = 1.25;
    data.noise_offset = vec3(0.0);

    vec3 accum_color = galaxy_ray(r_origin, frag_dir, depth, data);

    /*
    data.position = vec3(models[1] * vec4(0.0, 0.0, 0.0, 1.0));
    data.orientation = mat3(models[1]);
    data.size = vec3(light_year * 12000.0, light_year * 12000.0, light_year * 2400.0);
    data.core_size = vec3(light_year * 4000.0);
    data.color = vec3(0x6E, 0x53, 0xF2) / float(0xFF);
    data.period = 1.0;
    data.noise_offset = vec3(0.25, 0.75, 0.5);

    accum_color += galaxy_ray(r_origin, frag_dir, depth, data);

    data.position = vec3(models[2] * vec4(0.0, 0.0, 0.0, 1.0));
    data.orientation = mat3(models[2]);
    data.size = vec3(0.0);
    data.core_size = vec3(light_year * 15000.0, light_year * 25000.0, light_year * 15000.0);
    data.color = vec3(1.0);//vec3(0x6E, 0x53, 0xF2) / float(0xFF);
    data.period = 0.65;
    data.noise_offset = vec3(0.25, 0.25, 0.25);

    accum_color += galaxy_ray(r_origin, frag_dir, depth, data);
    */

    float transmittance = 1.0;//exp(-transmittance * 2.5);

    frag_color = vec4(accum_color * clamp(1.0 - light * 4.0, 0.0, 1.0) * 0.5, 0.0);
    frag_color.a = (1.0 - transmittance);
}

/*

float sample_texture(vec3 v) {
    uint hash_v = hash(uvec3(color * 4096));
    float xv = to_float(hash_v);
    hash_v = hash(hash_v);
    float yv = to_float(hash_v);
    hash_v = hash(hash_v);
    float zv = to_float(hash_v);

    vec3 vvv = vec3(0.0);//vec3(xv, yv, zv) * -5.0;
    //vvv = round(vvv * 4) / 4;
    //vvv = vec3(0.0);

    vec3 ff = v;

    vec2 f = ff.xy;

    vec3 vv = vec3(ff.xy, 0.0);

    vv *= 1.0;

    int num_arms = 3;

    float tex = 0.0;

    for(int i = 0; i < num_arms; ++i) {
        float f = float(i) / num_arms;
        mat3 rot = z_rot(f * pi * 2.0);

        vec3 nv = rot * vv;

        nv /= radius;

        if(abs(nv.x) < thickness * pow(1.0 - nv.y, 0.5) && nv.y > 0.0) tex = max(tex, 1.0 - abs(nv.x) / (thickness * pow(1.0 - nv.y, 0.5)));
    }
    //vv *= 0.5;

    float tex_sample = sample_octaves((vv + vvv), 1, 1.0);
    tex_sample += (1.0 - length(v)) * 0.25;
    tex_sample += tex;

    float core_factor = clamp(length(v / (core_size / axes)) - 0.5, 0.0, 1.0);

    return tex_sample;// * core_factor;
}
*/