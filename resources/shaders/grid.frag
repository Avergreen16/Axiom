#version 460 core

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;
layout(location = 3) uniform ivec2 offset_major;
layout(location = 4) uniform uvec2 offset_minor0;
layout(location = 5) uniform uvec2 offset_minor1;

layout(binding = 0) uniform sampler2D texture;

in vec2 tex_coord;
in mat4 inverse_proj;
in mat4 inverse_model;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;

float ray_plane(vec3 dir, vec3 pos, vec3 plane_pos, vec3 plane_normal) {
    float x = -dot(pos - plane_pos, plane_normal) / dot(plane_normal, dir);

    return x;
}

float map(float v, float min1, float max1, float min2, float max2) {
    float i = (v - min1) / (max1 - min1);

    return i * (max2 - min2) + min2;
}

vec4 get_color(vec2 pos, vec3 camera_pos, vec3 view_dir, mat3 norm) {
    uint num_scales = 4;
    uint scale_p = 4;
    uint scale = uint(pow(2, scale_p));
    float width = 1.0;
    int starting_scale = max(0, int(floor(log(abs(camera_pos.z)) / log(scale))));

    float bounds = pow(2, 71);
    
    int min_s = 0;
    int max_s = int(floor(log(bounds) / log(16)));

    vec4 color = vec4(0.0);

    for(int i = -2; i <= 2; ++i) {
        int ii = i + starting_scale;
        if(ii >= min_s && ii <= max_s) {
            uint pp = ii * scale_p;

            float radius = pow(2, pp);
            vec2 p = pos;

            p += fract(offset_minor1 / radius) * radius * 256.0;
            if(pp > 24) {
                float r = pow(2, pp - 24);
                p += fract(offset_minor0 * 256.0 / r) * r * pow(2, 24);
            }
            if(pp > 48) {
                float r = pow(2, pp - 48);
                p += fract(offset_major * 256.0 / r) * r * pow(2, 48);
            }

            // 
            
            //p += (fract(offset_minor / r) * r + fract(offset_major * (4294967296.0 / r)) * r) * 256.0;

            float lw = 1.0 / 32;

            vec4 x_color = vec4(0.0);
            vec4 y_color = vec4(0.0);

            float fade1 = min(abs(camera_pos.z) / (radius * 0.125), 1.0);

            vec2 tex_pos = p / radius;
            vec2 dx = dFdx(tex_pos);
            vec2 dy = dFdy(tex_pos);
            vec2 ax = vec2(dx.x, dy.x);
            vec2 ay = vec2(dx.y, dy.y);
            float ddx = length(ax);
            float ddy = length(ay);

            float draw_width_x = min(ddx, 1.0);//clamp(lw * fade1, ddx, 1.0);
            float draw_width_y = min(ddy, 1.0);//clamp(lw * fade1, ddy, 1.0);
            
            bool axis = false;
            float x_width = abs(tex_pos.x) * 2.0;
            float y_width = abs(tex_pos.y) * 2.0;

            float x = pos.x + ((float(offset_major.x) * 16777216.0 + float(offset_minor0.x)) * 16777216.0 + float(offset_minor1.x)) * 256.0;
            float y = pos.y + ((float(offset_major.y) * 16777216.0 + float(offset_minor0.y)) * 16777216.0 + float(offset_minor1.y)) * 256.0;
            
            vec2 tex_pos_basis = vec2(x, y);

            float x_d = abs(tex_pos_basis.x / radius) * 2.0 / draw_width_x;
            float y_d = abs(tex_pos_basis.y / radius) * 2.0 / draw_width_y;

            if(x_d < 2.0) {
                axis = true;
                if(y_d < 2.0) {
                    if(abs(x_d) > abs(y_d)) {
                        if(tex_pos_basis.x < 0.0) {
                            x_color = vec4(0.25, 1.0, 1.0, 1.0);
                        } else {
                            x_color = vec4(1.0, 0.25, 0.25, 1.0);
                        }
                    } else {
                        if(tex_pos_basis.y < 0.0) {
                            x_color = vec4(1.0, 0.25, 1.0, 1.0);
                        } else {
                            x_color = vec4(0.25, 1.0, 0.25, 1.0);
                        }
                    }
                } else {
                    if(tex_pos_basis.y < 0.0) {
                        x_color = vec4(1.0, 0.25, 1.0, 1.0);
                    } else {
                        x_color = vec4(0.25, 1.0, 0.25, 1.0);
                    }
                }
            } else if(y_d < 2.0) {
                axis = true;

                if(tex_pos_basis.x < 0.0) {
                    x_color = vec4(0.25, 1.0, 1.0, 1.0);
                } else {
                    x_color = vec4(1.0, 0.25, 0.25, 1.0);
                }
            }

            if(!axis) {
                x_width = abs(fract(tex_pos.x + 0.5) - 0.5) * 2.0;
                y_width = abs(fract(tex_pos.y + 0.5) - 0.5) * 2.0;
                if(x_width / draw_width_x <= 1.0) x_color = vec4(1.0, 1.0, 1.0, lw / draw_width_x);
                if(y_width / draw_width_y <= 1.0) y_color = vec4(1.0, 1.0, 1.0, lw / draw_width_y);

                float start_v = 0.5;
                float end_v = 1.0;

                if(ddx / radius > start_v) x_color = mix(x_color, vec4(1.0, 1.0, 1.0, lw * fade1), (ddx / radius - start_v) / (end_v - start_v));
                if(ddy / radius > start_v) y_color = mix(y_color, vec4(1.0, 1.0, 1.0, lw * fade1), (ddy / radius - start_v) / (end_v - start_v));
            }

            vec2 dd = vec2(draw_width_x, draw_width_y);

            vec2 c = ceil(abs(tex_pos) - dd * 0.5) * radius;
            if(c.x > bounds || c.y > bounds) {
                x_color = vec4(0.0);
                y_color = vec4(0.0);
            }

            float opacity = 0.75;

            if(axis) {
                color = x_color;
            } else {
                color = vec4(max(color, min(vec4(1.0), x_color + y_color)));
            }
        }
    }

    return color;
}

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

mat3 y_norm = mat3(
    vec3(1, 0, 0),
    vec3(0, 0, 1),
    vec3(0, 1, 0)
);

mat3 x_norm = mat3(
    vec3(0, 0, 1),
    vec3(0, 1, 0),
    vec3(1, 0, 0)
);

mat3 z_norm = mat3(
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
);

void main() {
    mat3 norm_mat = z_norm;

    vec4 v = vec4(tex_coord, 0.5, 1.0);
    v = inverse_proj * v;
    v /= v.w;
    v = vec4(normalize(v.xyz), 1.0);
    vec3 rel_v = v.xyz;

    v = transpose(view) * v;
    vec3 pos = vec3(inverse_model * vec4(0.0, 0.0, 0.0, 1.0));

    float x = ray_plane(v.xyz, pos, vec3(0.0), norm_mat * vec3(0.0, 0.0, 1.0));

    float d = dot(rel_v, vec3(0, 0, 1));

    if(x <= 0) discard;

    
    vec4 p = vec4(0.0, 0.0, x * d, 1.0);

    p = proj * p;

    p /= p.w;

    gl_FragDepth = p.z;

    vec3 plane_pos = (pos + v.xyz * x);
    vec3 view_dir = vec3(transpose(view) * vec4(0, 0, -1, 1));


    plane_pos = norm_mat * plane_pos;
    view_dir = norm_mat * view_dir;
    pos = norm_mat * pos;

    vec4 color = get_color(plane_pos.xy, pos, view_dir, norm_mat);
    
    /*uint value = uint((tex_coord.x + 1.0) * 1000000 + (tex_coord.y + 1.0) * 1000);
    float rand_v = to_float(hash(value)) * 0.5 + 0.5;
    if(rand_v > color.w) discard;
    frag_color = color;*///vec4(color.xyz, 1.0);

    if(color.w == 0.0) discard;

    frag_color = color;
    frag_color = min(frag_color, 1.0);
    frag_normal = vec4(0.0);

    frag_color.w *= 0.2;

    /*float x_a = floor(x) / 10;
    float x_b = floor(x / 10) / 10;
    float x_c = floor(x / 100) / 10;

    frag_color = vec4(x_a, x_b, x_c, 1);*/
}