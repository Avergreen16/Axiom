#version 460 core

layout(binding = 0) uniform sampler2D depth_tex;
layout(binding = 1) uniform sampler3D density_tex;

layout(location = 0) uniform mat4 proj;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 model;

layout(location = 3) uniform vec3 axes;
layout(location = 4) uniform float light_factor;
layout(location = 5) uniform vec3 color2;
layout(location = 6) uniform vec3 bulge_size;

vec3 color = vec3(142, 173, 245) / 255.0;

layout(location = 0) in vec2 screen_coord;

mat4 inv_proj = inverse(proj);
mat4 inv_view = inverse(view);
mat4 inv_model = inverse(model);

out vec4 frag_color;

float falloff = 1.0;

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

mat3 z_rot(float r) {
    vec3 x = vec3(cos(r), sin(r), 0);
    vec3 y = vec3(-sin(r), cos(r), 0);
    vec3 z = vec3(0, 0, 1);

    return mat3(x, y, z);
}

float sample_octaves(vec3 v, int octaves, float roughness) {
    vec3 a = v / 2.0;

    float tex = 0.0;
    for(int i = 0; i < octaves; ++i) {
        float tex_sample = texture(density_tex, a).r * 2.0 - 1.0;
        tex += tex_sample * pow(roughness, i);
        a /= 2;
    }
    
    return tex;
}

float sample_texture(vec3 v) {
    uint hash_v = hash(uvec3(color * 4096));
    float xv = to_float(hash_v);
    hash_v = hash(hash_v);
    float yv = to_float(hash_v);
    hash_v = hash(hash_v);
    float zv = to_float(hash_v);

    vec3 vvv = vec3(xv, yv, zv);
    vvv = round(vvv * 4) / 4;
    //vvv = vec3(0.0);


    float l = 1.0 - length(v.xy);
    float t = 4; // amount of twisting

    mat3 zr = z_rot(l * t);

    vec3 ff = v;

    vec2 f = ff.xy / l;

    vec3 vv = vec3(ff.xy, 0.0);

    vv *= 1.0;
    //vv *= 0.5;

    float ss_scale = 8.0; // scale of distortion

    int num_fluid_iterations = 10;
    
    vv = zr * vv;

    for(int i = 0; i < num_fluid_iterations; ++i) {
        float scale_sample = sample_octaves((vv + vec3(0.8, 0.2, 0.3)) * ss_scale + vvv, 1, 0.8);
        float scale_sample_x = sample_octaves((vv + vec3(0.8, 0.2, 0.3)) * ss_scale + vvv + vec3(0.01, 0.0, 0.0), 1, 0.8);
        float scale_sample_y = sample_octaves((vv + vec3(0.8, 0.2, 0.3)) * ss_scale + vvv + vec3(0.0, 0.01, 0.0), 1, 0.8);
        //float scale_sample_z = sample_octaves((v + vec3(0.1, 0.6, 0.5)) * ss_scale + vvv, 2, 0.8);
        float sx = (scale_sample_x - scale_sample) / 0.01;
        float sy = (scale_sample_y - scale_sample) / 0.01;

        vec3 curl = vec3(sy, -sx, 0.0);

        float mix_v = 0.001; // amount of distortion

        vv = vv + curl * mix_v;
    }

    float tex_sample = sample_octaves((vv + vvv), 1, 1.0);
    tex_sample += (1.0 - length(v)) * 0.25;

    float bulge_factor = clamp(length(v / (bulge_size / axes)) - 0.5, 0.0, 1.0);
    /*tex_sample /= 0.55;
    tex_sample = clamp(0.0, 1.0, tex_sample);
    tex_sample = pow(tex_sample, 2);*/

    return tex_sample * bulge_factor;
}

float plane_intersection(vec3 origin, vec3 direction) {
    vec3 normal = vec3(0, 0, 1);

    float height_origin = dot(normal, origin);
    float direction_dot = dot(normal, direction);

    float t = height_origin / direction_dot;

    return t;
}

float density_value = 2.0;

bool ray_plane(vec3 origin, vec3 direction, vec2 axes, out float i0) {
    float f = plane_intersection(origin, direction);

    vec3 v = origin + direction * f;

    if(v.x < axes.x && v.x > -axes.x && v.y < axes.y && v.y > -axes.y) {
        i0 = f;
        return true;
    }
    return false;
}

void main() {
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


    vec3 r_origin = -center;

    vec3 facing = vec3(inv_view * vec4(0, 0, -1, 1));

    ivec2 fc = ivec2(floor(gl_FragCoord.xy));


    frag_dir = mat3(inv_model) * frag_dir;
    r_origin = mat3(inv_model) * r_origin;

    vec3 axes_v = axes;
    //axes_v.z = pow(2, 44);



    float i0;
    float i1;

    mat3 matv;
    matv = mat3(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

    //float t = plane_intersection(r_origin, frag_dir);

    //vec3 pos = r_origin + frag_dir * t;

    //frag_color = vec4(1.0, 0.0, 0.0, 1.0);

    bool intersection = ray_cube(r_origin, frag_dir, axes_v, i0, i1);

    float enter = min(max(i0, 0.0), depth);
    float exit = min(i1, depth);

    float transmittance = 0;
    vec3 accum_color = vec3(0, 0, 0);

    vec3 far = color;
    vec3 near = vec3(1.0, 0.75, 0.5);

    if(intersection) {
        int steps_through = 50;
        float thickness = exit - enter;

        for(int i = 0; i < steps_through; ++i) {
            float r = to_float(hash(uvec3(ivec3((frag_dir + float(i)) * 500) - 0x3FFFFFFF)));
            float dist = enter + thickness * ((float(i + r * 0.75)) / (steps_through));
            float width = 100 / steps_through;

            if(dist >= 0) {
                vec3 pos = r_origin + frag_dir * dist;

                vec3 pos2 = pos / axes_v;

                vec3 vv = vec3(pos2.xy, 0);

                float density = sample_texture(pos2);
                float l = length(vv);
                density = max(density, 0.0);
                float l0 = l;//pow(l, 0.5);

                vec3 c = mix(near, far, l0);

                float bulge_factor = length(pos2 * axes_v / bulge_size);
                bulge_factor = abs(bulge_factor);
                bulge_factor = exp(-bulge_factor * 6.0) * (1.0 - bulge_factor);
                bulge_factor = max(bulge_factor, 0.0);

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
                gradient = pow(gradient, 1.5);
                density += gradient * 0.2;


                density *= (1.0 - bulge_factor);

                float transmittance_factor = 1.0;

                transmittance += density * (width);
                accum_color += c * density * (width) * exp(-transmittance * transmittance_factor);

                bulge_factor *= 0.3;
                density = bulge_factor;
                c = vec3(1.0, 0.75, 0.5);
                
                //transmittance += density * (width);
                accum_color += c * density * (width) * exp(-transmittance * transmittance_factor);
            }
        }
    } else accum_color = vec3(0, 0, 0);

    transmittance = exp(-transmittance * 2.5);

    frag_color = vec4(accum_color * light_factor * 0.5, 0.0);
    frag_color.a = (1.0 - transmittance);
}