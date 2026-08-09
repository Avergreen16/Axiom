#version 460 core

layout(location = 0) out vec4 frag_color;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) in vec2 cs;
layout(location = 1) in mat4 inv_model;
layout(location = 5) in mat4 inv_view;
layout(location = 9) in mat4 inv_proj;

vec3 hex_color(uint i) {
    return vec3((i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF) / float(0xFF);
}

vec3 base_color = hex_color(0x1E1F2E);
vec3 line_color = (base_color + vec3(1.0)) * 0.5f;
//vec3 sector_color = hex_color(0xFF893D);

vec3 pos_x_color = hex_color(0xFF4040);
vec3 neg_x_color = hex_color(0x40FFFF);
vec3 pos_y_color = hex_color(0x40FF40);
vec3 neg_y_color = hex_color(0xFF40FF);

//

in vec2 tex_coord;
in mat4 inverse_proj;
in mat4 inverse_model;


float ray_plane(vec3 dir, vec3 pos, vec3 plane_pos, vec3 plane_normal) {
    float x = -dot(pos - plane_pos, plane_normal) / dot(plane_normal, dir);

    return x;
}

float map(float v, float min1, float max1, float min2, float max2) {
    float i = (v - min1) / (max1 - min1);

    return i * (max2 - min2) + min2;
}

vec4 get_color(vec2 pos, float dist, vec3 camera_pos, vec3 view_dir, mat3 norm) {
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

            /*
            p += fract(offset_minor1 / radius) * radius * 256.0;
            if(pp > 24) {
                float r = pow(2, pp - 24);
                p += fract(offset_minor0 * 256.0 / r) * r * pow(2, 24);
            }
            if(pp > 48) {
                float r = pow(2, pp - 48);
                p += fract(offset_major * 256.0 / r) * r * pow(2, 48);
            }
            */

            // 
            
            //p += (fract(offset_minor / r) * r + fract(offset_major * (4294967296.0 / r)) * r) * 256.0;

            float lw = 1.0 / 32.0;

            vec4 x_color = vec4(0.0);
            vec4 y_color = vec4(0.0);

            float fade1 = min(abs(camera_pos.z) / (radius * 0.125), 1.0);

            //

            vec2 tex_pos = p / radius;
            bool cont = dist > 0.0;
            
            vec2 dx = dFdx(tex_pos);
            vec2 dy = dFdy(tex_pos);

            //if(isnan(dx.x) || isnan(dy.y) || isinf(dx.x) || isinf(dy.y)) cont = false;

            if(cont) {
                vec2 ax = vec2(dx.x, dy.x);
                vec2 ay = vec2(dx.y, dy.y);
                float ddx = clamp(length(ax), 0.0, 1.0);
                float ddy = clamp(length(ay), 0.0, 1.0);

                float draw_width_x = ddx;//clamp(lw * fade1, ddx, 1.0);
                float draw_width_y = ddy;//clamp(lw * fade1, ddy, 1.0);
                
                bool axis = false;
                float x_width = abs(tex_pos.x) * 2.0;
                float y_width = abs(tex_pos.y) * 2.0;

                float x = pos.x;// + ((float(offset_major.x) * 16777216.0 + float(offset_minor0.x)) * 16777216.0 + float(offset_minor1.x)) * 256.0;
                float y = pos.y;// + ((float(offset_major.y) * 16777216.0 + float(offset_minor0.y)) * 16777216.0 + float(offset_minor1.y)) * 256.0;
                
                vec2 tex_pos_basis = vec2(x, y);
                
                //if(draw_width_x <= 0.001 || draw_width_y <= 0.001 || isnan(draw_width_x) || isnan(draw_width_y) || isinf(draw_width_x) || isinf(draw_width_y)) cont = false;

                
                if(cont) {
                    float x_d = abs(tex_pos_basis.x / radius) * 2.0 / draw_width_x;
                    float y_d = abs(tex_pos_basis.y / radius) * 2.0 / draw_width_y;

                    if(!isnan(draw_width_x) && !isnan(draw_width_y) && !isinf(draw_width_x) && !isinf(draw_width_y)) { 
                        if(x_d < 2.0) {
                            axis = true;
                            if(y_d < 2.0) {
                                if(abs(x_d) > abs(y_d)) {
                                    if(tex_pos_basis.x < 0.0) {
                                        x_color = vec4(neg_x_color, 1.0);
                                    } else {
                                        x_color = vec4(pos_x_color, 1.0);
                                    }
                                } else {
                                    if(tex_pos_basis.y < 0.0) {
                                        x_color = vec4(neg_y_color, 1.0);
                                    } else {
                                        x_color = vec4(pos_y_color, 1.0);
                                    }
                                }
                            } else {
                                if(tex_pos_basis.y < 0.0) {
                                    x_color = vec4(neg_y_color, 1.0);
                                } else {
                                    x_color = vec4(pos_y_color, 1.0);
                                }
                            }
                        } else if(y_d < 2.0) {
                            axis = true;

                            if(tex_pos_basis.x < 0.0) {
                                x_color = vec4(neg_x_color, 1.0);
                            } else {
                                x_color = vec4(pos_x_color, 1.0);
                            }
                        }
                    }
                    
                    draw_width_x = min(ddx, 1.0);
                    draw_width_y = min(ddy, 1.0);
                    x_d = abs(tex_pos_basis.x / radius) * 4.0 / draw_width_x;
                    y_d = abs(tex_pos_basis.y / radius) * 4.0 / draw_width_y;

                    if(!axis) {
                        x_width = abs(fract(tex_pos.x + 0.5) - 0.5) * 2.0;
                        y_width = abs(fract(tex_pos.y + 0.5) - 0.5) * 2.0;
                        if(x_width / draw_width_x <= 1.0) x_color = vec4(1.0, 1.0, 1.0, lw / draw_width_x + 0.3);
                        if(y_width / draw_width_y <= 1.0) y_color = vec4(1.0, 1.0, 1.0, lw / draw_width_y + 0.3);

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
                        color = vec4(max(color, min(vec4(line_color, 1.0), max(x_color, y_color))));
                    }
                }
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
    vec4 cc = vec4(cs, 0.5, 1.0);
    cc = inv_proj * cc;
    cc /= cc.w;
    cc = inv_view * cc;

    vec3 ray = normalize(cc.xyz);

    vec3 origin = vec3(model * vec4(0.0, 0.0, 0.0, 1.0));

    vec3 normal = vec3(0.0, 0.0, 1.0);

    float dist = dot(-origin, normal) / -dot(ray, normal);

    vec3 p = -origin + ray * dist;

    vec4 color = vec4(0.0);
    //if(dist > 0) {
        vec4 d = proj * (view * vec4((ray * dist), 1.0));
        d /= d.w;

        gl_FragDepth = d.z;

        vec2 plane_pos = p.xy;

        vec3 view_dir = ray;

        mat3 norm_mat = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);

        color = get_color(plane_pos, dist, -origin, ray, norm_mat);

        if(color.w == 0.0) color.w = 0.0;
    //}
    
    frag_color = vec4(base_color, 0.0) * (1.0 - color.w) + color * color.w;
    frag_color = min(frag_color, 1.0);
    //frag_normal = vec4(0.0);

    /*float x_a = floor(x) / 10;
    float x_b = floor(x / 10) / 10;
    float x_c = floor(x / 100) / 10;

    frag_color = vec4(x_a, x_b, x_c, 1);*/
}