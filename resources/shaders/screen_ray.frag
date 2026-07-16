#version 460 core

layout(binding = 0) uniform sampler2D depth_tex;
layout(binding = 1) uniform sampler3D perlin;
layout(binding = 1) uniform sampler3D voronoi;

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
layout(location = 10) uniform vec3 planet_radii;

layout(location = 0) in vec2 screen_coord;

out vec4 frag_color;

uint seed = seed_;

float base_radius = planet_radii.x;
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

float get_depth(float d, mat4 inv_p) {
    //d = (1 - d) * 2 - 1;
    vec4 dv = vec4(0.0, 0.0, d, 1.0);

    dv = inv_p * dv;
    dv /= dv.w;
    float depth = dv.z;

    if(isinf(depth)) depth = -3.402823E+38;

    return depth;
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

vec4 blend(vec4 dst, vec4 src) {
    return vec4(dst.xyz * (1.0 - src.w) + src.xyz * src.w, max(dst.w, src.w));
}

float fbm(vec3 pos, float freq, int octaves, float gain, float lacunarity, sampler3D sampler) {
    float accum = 0.0;

    float l = 1.0;
    float g = 1.0;
    for(int i = 0; i < octaves; ++i) {
        vec3 p = pos * freq / 4.0;
        accum += (texture(sampler, p).r * 2.0 - 1.0) * g;
        
        pos *= lacunarity;
        g *= gain;
    }

    return accum;
}

vec3 calculate_flow(vec3 pos, float delta, vec2 aspect, vec2 scale) {
    vec3 ddx = vec3(0.0, 0.0, 1.0);//normalize(cross(pos, vec3(0, 0, 1)));
    vec3 ddy = vec3(0.0, 1.0, 0.0);//normalize(cross(ddx, pos));

    vec3 sc = vec3(scale.x, scale.x, scale.y);

    float d0 = fbm(pos * sc, 8.0, 2, 0.5, 2.0, perlin);
    float dx = fbm(pos * sc + ddx * delta, 8.0, 2, 0.5, 2.0, perlin);
    float dy = fbm(pos * sc + ddy * delta, 8.0, 2, 0.5, 2.0, perlin);

    float curl_x = (d0 - dx) / delta;
    float curl_y = (d0 - dy) / delta;

    return -ddx * curl_y * aspect.x + ddy * curl_x * aspect.y;
}

float loop(double v, double period) {
    return float(v * period);
}

/*
int num_steps = 0;
float delta = 0.01;
//pos += vec3(loop(time, 0.01), loop(time, 0), loop(time, 0.006));
for(int i = 0; i < num_steps; ++i) {
    vec3 flow = calculate_flow(pos, 0.02, vec2(1.0, 1.0) * delta / num_steps, vec2(1.0, 1.0) * 0.33);
    
    pos += flow;
    o += flow * 0.1;
}
*/

float sample_cloud_texture(vec3 pos, vec3 rand, bool f, bool ray) {
    float height = length(pos);
    vec3 prev_pos = pos;
    
    vec3 o = pos;

    int num_steps = 2;
    float delta = 0.01;
    //pos += vec3(loop(time, 0.01), loop(time, 0), loop(time, 0.006));
    for(int i = 0; i < num_steps; ++i) {
        vec3 flow = calculate_flow(pos * 0.2, 0.01, vec2(1.0, 1.0) * (delta / num_steps), vec2(1.0, 1.0) * 0.33);
        
        pos += flow;
        o += flow * 0.5;
    }

    float thick_weight = fbm(pos, 1.0, 2, 0.5, 2.0, perlin) - 0.05;
    float thin_weight = fbm(pos + vec3(0.5), 1.0, 2, 0.5, 2.0, perlin);

    float f0 = fbm(pos, 8.0, 2, 0.5, 2.0, perlin);
    float f1 = fbm(o, 8.0, 2, 0.5, 2.0, perlin);
    float f2 = fbm(o, 64.0, 4, 0.5, 2.0, voronoi);

    float thick = fbm(pos, 2.0, 4, 0.5, 2.0, perlin) + f1 * 0.5 + f2 * 0.115 - 0.15;
    float thin = f0 + f2 * 0.115 - 0.25;

    float ret = max((thick_weight + thick) + max(thin_weight + thin, 0.0), 0.0);

    float falloff = 1.0;
    if(f) {
        if(height > 1.001) falloff = 0.0;
    } else if(ray) {
        falloff = clamp((ret - 0.125) * 2.0, 0.0, 1.0);
    }

    return ret * falloff; 
}

vec3 sample_gradient(vec3 pos, vec3 rand, float delta) {
    float d = 0.001;
    float a = sample_cloud_texture(pos, rand, false, false);
    float x = sample_cloud_texture(pos + vec3(d, 0, 0), rand, false, false);
    float y = sample_cloud_texture(pos + vec3(0, d, 0), rand, false, false);
    float z = sample_cloud_texture(pos + vec3(0, 0, d), rand, false, false);

    return vec3(x - a, y - a, z - a) / d;
}

float sample_lighting(vec3 pos, vec3 rand, vec3 light_dir, bool s) {
    float light = 0.0;
    float step_size = 0.2;
    vec3 p = pos;
    if(!s) step_size = 0.01;

    int num_steps = 4;
    float pp = rand.x * (step_size / num_steps);
    for(int i = 0; i < num_steps; ++i) {
        pp += (step_size / num_steps);
        p = pos + light_dir * pp;

        light += (1.0 / num_steps) * (sample_cloud_texture(p, rand, s, true));
    }

    return exp(-light * 1.5);
}

vec3 sunset_clouds = vec3(1.0, 0.5, 0.3);

// x = front lighting color
// y = back lighting color
// z = sunset color
vec3 sample_atmosphere(vec3 pos, vec3 ray_dir, vec3 sun_pos, float planet_radius, float atmo_width) {
    vec3 ret = vec3(0.0);
    
    vec3 center_pos = pos;

    float ii = 1.0 - (length(center_pos) - planet_radius) / atmo_width;
    ii = clamp(ii, 0.0, 1.0);

    vec3 normal = normalize(center_pos);

    vec3 sun_direction = normalize(sun_pos - center_pos);

    float ndotl = dot(normal, sun_direction);
    float wrap = 0.15;
    float ndotl_wrap = clamp(((ndotl + wrap) / (1.0 + wrap)) * 1.5, 0.0, 1.0);
    ndotl_wrap = min(ndotl_wrap * 1.5, 1.0);

    float closest_dist = dot(ray_dir, -pos);
    vec3 closest_point = pos + ray_dir * max(0.0, closest_dist);
    float dist_from_center = length(closest_point);
    float radius = planet_radius + atmo_width;
    float intersection = sqrt(planet_radius * planet_radius + dist_from_center * dist_from_center);
    vec3 point = pos + ray_dir * max(0.0, closest_dist - intersection);

    float sun_factor = max(0.0, dot(ray_dir, sun_direction));
    float horizon_factor = 1.0 - abs(dot(normalize(point), ray_dir));
    float sun_horizon = 1.0 - min(1.0, clamp(dot(normalize(point), sun_direction), 0.0, 1.0) * 2.25);

    ret.x = 1.0;
    ret.y = 1.0;

    float extinction_factor = ndotl_wrap * ii;

    float scatter = sun_factor * horizon_factor * sun_horizon;

    ret.x = mix(ret.x, 0, scatter);
    ret.y = mix(0.0, ret.y, scatter);
    ret.z = ndotl_wrap;

    ret.xy *= extinction_factor;

    return ret;
}

float ef = 0.0;

vec3 sunset_col = vec3(1.0, 0.5, 0.2);
vec3 front_lighting_color = vec3(0.325, 0.5, 0.9);

void main() {
    // get depth
    ivec2 size = textureSize(depth_tex, 0);
    ivec2 texel = ivec2((screen_coord * 0.5 + 0.5) * size);

    vec4 color = texelFetch(depth_tex, texel, 0);

    float depth = color.r;

    depth = -get_depth(depth, inverse_proj);
    
    vec4 raw_dir = inverse_proj * vec4(screen_coord, 0.5, 1.0);
    raw_dir /= raw_dir.w;

    vec3 pixel_dir = -raw_dir.xyz;
    pixel_dir = normalize(pixel_dir);

    float factor = dot(vec3(0, 0, 1), pixel_dir);

    depth /= factor;

    pixel_dir = mat3(inverse_view) * (-pixel_dir); // inverse_view flips the direction

    //
    
    vec3 main_ray_origin = vec3(earth_mat * vec4(0, 0, 0, 1));
    vec3 main_ray_dir = mat3(earth_mat) * pixel_dir;
    vec3 main_sun_pos = vec3(earth_mat * vec4(sun_pos, 1.0));

    float atmo_width = 30000;

    //

    float radius = base_radius * 1.0 + atmo_width * 0.4;

    seed = 0;

    vec4 output_col = vec4(0.0);
    vec3 sunset_color = vec3(1.0, 0.5, 0.3);
    float falloff_start = atmo_width;
    float falloff_thickness = atmo_width * 0.5;

    uint i = 1;

    //
    
    float r = to_float(hash((texel.x * 0x7feb352dU) ^ (texel.y * 0x846ca68bU)));

    // front
    {
        vec4 cloud_raycast = sphere_intersection(main_ray_origin, main_ray_dir, radius, vec3(0, 0, 0));

        vec3 rand = vec3(hash_float(i), hash_float(i * 2), hash_float(i * 3));

        vec3 pos = main_ray_origin + main_ray_dir * cloud_raycast.z;
        vec3 dir = normalize(main_sun_pos - pos);

        vec3 weights = sample_atmosphere(pos / radius, main_ray_dir, main_sun_pos, base_radius * 0.25, atmo_width);

        float density = sample_cloud_texture(pos / radius, rand, false, false);

        density = min(1.0, density * 40);
        density = smoothstep(0.0, 1.0, density);
        
        vec3 direction = sample_gradient(pos / radius, rand, 0.00005);
        float d = clamp(dot(direction, dir), 0.0, 1.0);
        float factor = mix(0.85, 1.0, d);

        vec3 light = mix(vec3(1.0), sunset_clouds, 1.0 - weights.z);
        
        light *= sample_lighting(pos / radius, vec3(r), dir, false);
        light *= factor;
        light *= weights.z;

        light = light * 0.99 + 0.01;

        vec4 new_color = vec4(light, density);
        if(cloud_raycast.w < radius && cloud_raycast.z > 0.0 && cloud_raycast.z < depth) output_col = blend(output_col, new_color);
    }

    // back
    {
        vec4 cloud_raycast = sphere_intersection(main_ray_origin, main_ray_dir, radius, vec3(0, 0, 0));
        
        vec3 rand = vec3(hash_float(i), hash_float(i * 2), hash_float(i * 3));

        //
        vec3 pos = main_ray_origin + main_ray_dir * cloud_raycast.x;
        vec3 dir = normalize(main_sun_pos - pos);

        vec3 weights = sample_atmosphere(pos / radius, main_ray_dir, main_sun_pos, base_radius * 0.25, atmo_width);

        float density = sample_cloud_texture(pos / radius, rand, false, false);

        float dd = density;

        density = min(1.0, density * 40);
        density = smoothstep(0.0, 1.0, density);

        vec3 direction = sample_gradient(pos / radius, rand, 0.00005);
        float d = clamp(dot(normalize(-direction), dir), 0.0, 1.0);
        float factor = mix(0.85, 1.0, d);

        vec3 light = mix(vec3(1.0), sunset_clouds, 1.0 - weights.z);
        
        light *= sample_lighting(pos / radius, vec3(r), dir, true);//change
        light *= factor;
        light *= weights.z;
        
        light = light * 0.9975 + 0.0025;

        vec4 new_color = vec4(light, density);
        if(dd == 0.1) new_color = vec4(1.0, 0.0, 0.0, new_color.w);
        if(cloud_raycast.w < radius && cloud_raycast.x > 0.0 && cloud_raycast.x < depth) output_col = blend(output_col, new_color);
    }

    //

    if(output_col.w != 0.0) output_col.xyz /= output_col.w;

    // atmosphere

    float planet_radius = base_radius * 1.0;

    vec4 atmo_raycast = sphere_intersection(main_ray_origin, main_ray_dir, planet_radius + atmo_width, vec3(0.0));

    if(atmo_raycast.a < planet_radius + atmo_width) {
        vec3 center_pos = main_ray_origin + main_ray_dir * min(depth, max(0.0, atmo_raycast.y));

        vec3 weights = sample_atmosphere(center_pos, main_ray_dir, main_sun_pos, planet_radius, atmo_width);

        vec3 vcolor = weights.x * front_lighting_color + weights.y * sunset_col;

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
}