#version 460 core

layout(binding = 0) uniform sampler2D color_tex;
layout(binding = 1) uniform sampler2D depth_tex;
layout(binding = 2) uniform sampler2D normal_tex;
layout(binding = 3) uniform sampler2D slope_tex;
layout(binding = 4) uniform sampler2D sun_tex;
layout(binding = 5) uniform sampler2D sun_slope_tex;
layout(binding = 6) uniform sampler3D density;

layout(location = 0) uniform mat4 proj;
layout(location = 1) uniform mat4 view;

layout(location = 2) uniform mat4 sun_proj;
layout(location = 3) uniform mat4 sun_view;
layout(location = 4) uniform vec3 rel_sun_pos;
layout(location = 5) uniform vec3 light_pos;

layout(location = 6) uniform vec3 sun_pos;
layout(location = 7) uniform mat4 rel_mat;
layout(location = 8) uniform vec2 radii;
layout(location = 9) uniform mat4 moon_mat;
layout(location = 10) uniform uint seed;

in vec2 tex_coord;
in vec3 light_dir;

out vec4 frag_color;

float base_radius = 300000.0;

float moon_size = base_radius * 0.45;

mat4 inverse_proj = inverse(proj);
mat4 inverse_view = inverse(view);
mat4 inverse_sun_proj = inverse(sun_proj);

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

    if(isinf(depth)) depth = 3.402823E+38;

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

    return min(slope, 5);
}

float plane_intersection(vec3 origin, vec3 direction) {
    vec3 normal = vec3(0, 0, 1);

    float height_origin = dot(normal, -origin);
    float direction_dot = dot(normal, direction);

    float t = height_origin / direction_dot;

    return t;
}

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

int range = 3;

vec4 max_size = inverse_sun_proj * vec4(1.0);

float sun_pixel_size = max_size.x / max_size.w * 2 / 2048 * sqrt(3.0);

float bias = max_size.x / max_size.w * 0.0002;
float slope_bias = max_size.x / max_size.w * 0.0002;

float threshold = pow(2, 60);
vec3 big_normalize(vec3 v) {
    if(abs(v.x) > threshold || abs(v.y) > threshold || abs(v.z) > threshold) {
        return normalize(v / threshold);
    } else return normalize(v);
}

void main() {
    ivec2 size = textureSize(depth_tex, 0);
    ivec2 texel = ivec2(size * tex_coord);

    vec4 color = texelFetch(depth_tex, texel, 0);

    vec3 view_dir = mat3(inverse_view) * vec3(0, 0, 1);

    vec4 nn = texelFetch(normal_tex, texel, 0);
    vec3 normal = nn.xyz;
    bool zero_normal = (normal == vec3(0, 0, 0));
    normal = normal * 2.0 - 1.0;
    float len_n = length(normal);
    bool no_normal = (len_n < 0.1);
    normal = normal / len_n;

    vec4 l_data = texelFetch(slope_tex, texel, 0);
    float shadow = l_data.y;
    //l_data.g = 1.0;

    float slope = get_slope(l_data.r * 2 - 1);

    float d = color.r;
    vec3 clip_space = vec3((vec2(texel) + 0.5) / size, d);

    vec3 world_pos = get_world_pos(clip_space);

    vec3 sun_dir = big_normalize(rel_sun_pos - world_pos);
    vec3 light_dir = big_normalize(light_pos - world_pos);

    vec4 sun_view_space = sun_view * vec4(world_pos - rel_sun_pos, 1.0);
    vec4 sun_clip_space = sun_proj * sun_view_space;
    sun_clip_space /= sun_clip_space.w;
    sun_clip_space = sun_clip_space * 0.5 + 0.5;
    
    vec3 ray_direction = big_normalize(sun_pos - world_pos);
    vec3 ray_origin = world_pos;
    
    float n = 1.0;
    float light_power = 0.75;
    //) && sun_view_space.z > -511 && sun_view_space.z < 512
    // REMOVE the false

    float s = 0.0;

    if(!zero_normal && floor(sun_clip_space.xyz) == vec3(0.0)) {
        vec4 sun_slope_t = texelFetch(sun_slope_tex, ivec2(sun_clip_space.xy * 4096), 0);
        float sun_slope = get_slope(sun_slope_t.r * 2 - 1);
        s = sun_slope;
        sun_view_space.z = max(sun_view_space.z, -max_size.z / max_size.w);

        float offset = bias + slope_bias * slope + sun_pixel_size * 1.0;
        //float offset = bias + slope_bias * slope + sun_pixel_size * sun_slope;

        vec4 sun_tex = texelFetch(sun_tex, ivec2(sun_clip_space.xy * 4096), 0);

        float sun_depth = sun_tex.r;
        float n_sun_depth = get_depth(sun_depth, inverse_sun_proj);
        n_sun_depth = n_sun_depth + (l_data.b * 10.0 - 5.0); //FOR BILLBOARD OFFSET
        //n_sun_depth = n_sun_depth - min(l_data.b, sun_slope_t.b);

        //offset += distance_bias * pow(2, log2(abs(sun_view_space.z)) - 23);

        float f = sun_view_space.z + offset - n_sun_depth;

        if(f < 0.0) n = 0.0;
        else if(!no_normal) n = pow(max(0, dot(normal, ray_direction)), light_power);
    } else if(!zero_normal && !no_normal) {
        n = pow(max(0, dot(normal, ray_direction)), light_power);
    }

    // REMOVE
    n = n * shadow + (1.0 - shadow);

    float occlusion = 0;

    mat3 r = rotate_z_to(normal);

    float radius = 0.2;
    float bias = 0.03;

    float depth = get_depth(d, inverse_proj);

    // ray

    // rings

    /*

    vec3 rorigin_ring = vec3(rel_mat * vec4(ray_origin, 1.0));
    vec3 rdirection_ring = mat3(rel_mat) * ray_direction;

    float t = plane_intersection(rorigin_ring, rdirection_ring);

    vec3 t_pos = rorigin_ring + rdirection_ring * t;
    float dist = length(t_pos);

    if(dist > radii.x && dist < radii.y && t > 0.0) {
        float center = (dist - radii.x) / (radii.y - radii.x);
        float density = max(0.0, texture(density, vec3(center, 0, 0) * 0.75 + vec3(hash_float(0x1E), hash_float(0x1F), hash_float(0x20))).r * 1.5 - 0.5);
        density = 1.0 - pow(density, 0.5);

        n = n * density;
    }

    // moon
    mat4 inv_mm = inverse(moon_mat);

    vec3 rorigin_moon = vec3(inv_mm * vec4(ray_origin, 1.0));
    vec3 rdirection_moon = mat3(inv_mm) * ray_direction;

    vec4 tt = sphere_intersection(rorigin_moon, rdirection_moon, moon_size, vec3(0, 0, 0));

    bool is_intersect = tt.a < moon_size && tt.y > 0.0;

    //if(is_intersect) n = 0.0;
    */

    // occlusion
    /*if(depth > -500 && d != 0.0) {
        for(int i = 0; i < 64; ++i) {
            vec3 v = sample_points[i];
            v *= radius;

            v = r * v;

            v += world_pos;

            vec4 screen_space = proj * view * vec4(v, 1.0);
            screen_space /= screen_space.w;


            ivec2 texel_2 = ivec2((screen_space.xy * 0.5 + 0.5) * size);

            float depth_2 = texelFetch(depth_tex, texel_2, 0).r;

            if(depth_2 != 0.0) {
                depth_2 = get_depth(depth_2, inverse_proj);

                float range_check = smoothstep(0.0, 1.0, radius / abs(depth_2 - depth));
                occlusion += (depth_2 >= depth + bias ? 1.0 : 0.0) * range_check;
            }
        }

        n = min(n, 1.0 - ((occlusion / 64) * (1.0 - l_data.g)));
    }*/

    /*if(floor(world_pos) == vec3(0, 0, 0)) {
        float shadow = 0.0;

        vec2 texel_size = 1.0 / textureSize(sun_tex, 0);
        for(int x = -range; x <= range; ++x) {
            for(int y = -range; y <= range; ++y) {
                float depth_sun = texture(sun_tex, world_pos.xy + texel_size * (vec2(x, y) + random_points[(x + range) * (y + range)])).r;
                shadow += int((world_pos.z > depth_sun) ? 1.0 : 0.0);
            }
        } 
        shadow /= (range * 2 + 1) * (range * 2 + 1);
        shadow = (shadow - 0.5) * 2;
        n = min(1.0, 1.0 - (shadow * 0.5 * (1 - max(max(abs(world_pos.x * 2 - 1), abs(world_pos.y * 2 - 1)), abs(world_pos.z * 2 - 1)))));
        //
    }*/

    /*
    float n = 1.0;
    if(floor(world_pos.xyz) == vec3(0, 0, 0)) {
        int count = 0;
        float accum = 0;
        for(int x = -range; x <= range; ++x) {
            for(int y = -range; y <= range; ++y) {
                ivec2 current_texel = texel + ivec2(x, y);

                if(current_texel.x >= 0 && current_texel.x < size.x && current_texel.y >= 0 && current_texel.y < size.y) {
                    ++count;

                    vec4 color = texelFetch(depth_tex, current_texel, 0);
                    vec3 clip_space = vec3(vec2(current_texel) / size, color.r);
                    vec3 sun_world_pos = get_sun_pos(clip_space);

                    if(floor(sun_world_pos) == vec3(0, 0, 0)) {
                        float depth_sun = texture(sun_tex, sun_world_pos.xy).r;
                        if(sun_world_pos.z < depth_sun) {
                            ++accum;
                        }
                    }
                }
            }
        }

        if(count != 0) {
            n = (accum / count) * 0.5 + 0.5;
        }
    }*/

    vec4 tex = texture(color_tex, tex_coord);

    frag_color = vec4(tex.xyz * n, tex.w);
}