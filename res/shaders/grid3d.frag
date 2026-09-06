#version 460 core

layout(binding = 0) uniform sampler2D viewport_depth_tex;

layout(location = 0) out vec4 frag_color;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;
layout(location = 3) uniform vec3 extents;

layout(location = 0) in vec2 cs;
layout(location = 1) in mat4 inv_model;
layout(location = 5) in mat4 inv_view;
layout(location = 9) in mat4 inv_proj;

vec3 hex_color(uint i) {
    return vec3((i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF) / float(0xFF);
}

vec3 base_color = hex_color(0x1E1F2E);
vec3 line_color = hex_color(0xFFFFFF);
//vec3 sector_color = hex_color(0xFF893D);

vec3 pos_x_color = hex_color(0xFF4040);
vec3 neg_x_color = hex_color(0x40FFFF);
vec3 pos_y_color = hex_color(0x40FF40);
vec3 neg_y_color = hex_color(0xFF40FF);
vec3 pos_z_color = hex_color(0x4040FF);
vec3 neg_z_color = hex_color(0xFFFF40);

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

vec3 cycle_color(float angle, float saturation, float value) {
    float x = mod(angle, 1.0) * 6.0;
    float fracx = fract(x);

    if(x < 1.0) return vec3(1.0, fracx, 0.0);
    if(x < 2.0) return vec3(1.0 - fracx, 1.0, 0.0);
    if(x < 3.0) return vec3(0.0, 1.0, fracx);
    if(x < 4.0) return vec3(0.0, 1.0 - fracx, 1.0);
    if(x < 5.0) return vec3(fracx, 0.0, 1.0);
    return vec3(1.0, 0.0, 1.0 - fracx);
}

vec4 blend(vec4 dst, vec4 src) {
    return dst * (1.0 - src.w) + src * src.w;
}

float flt_max = 1.0 / 0.0;
float depth = 0.0;
vec4 lcolor = vec4(0.0);

void render_line(vec3 line_origin, vec3 line_direction, vec3 ray_origin, vec3 ray_direction, mat3 view_mat, vec3 color, float min_len, float max_len) {
    vec3 W = line_origin - ray_origin;
    float a = dot(W, line_direction);
    float b = dot(W, ray_direction);
    float c = dot(line_direction, ray_direction);
    float d = dot(line_direction, line_direction);
    float e = dot(ray_direction, ray_direction);

    float denom = (d * e - c * c);
    float s = (b * c - a * e) / denom;
    float t = (b * d - c * a) / denom;

    vec3 ps = line_origin + line_direction * s;
    vec3 pt = ray_origin + ray_direction * t;

    float dist = length(ps - pt);

    float between_dist_x = dot(view_mat[0], ps - pt);
    float between_dist_y = dot(view_mat[1], ps - pt);

    float dx = dFdx(between_dist_x);
    float dy = dFdy(between_dist_y);
    float dl = length(vec2(dx, dy));

    if(length(ps - pt) / dl < 1.5 && t > 0.0 && s > min_len && s < max_len) {
        vec4 d = proj * (view * vec4((ray_direction * t), 1.0));
        d /= d.w;

        if(d.z > depth) {
            lcolor = max(lcolor, vec4(color, clamp(1.5 - dist / dl, 0.0, 1.0)));
            gl_FragDepth = d.z;
        }
    }
}


float filtered_grid(vec2 p, vec2 dpdx, vec2 dpdy ) {
    const vec2 N = max(vec2(10.0), 1.0 / (max(abs(dpdx), abs(dpdy)) * 1.5));
    vec2 w = max(abs(dpdx), abs(dpdy));
    vec2 a = p + 0.5 * w;                        
    vec2 b = p - 0.5 * w;           
    vec2 i = (floor(a) + min(fract(a) * N, 1.0) - floor(b) - min(fract(b) * N, 1.0)) / (N * w);
    return (1.0 - i.x) * (1.0 - i.y);
}

vec4 get_color(vec2 p, float dist, vec3 camera_pos, vec3 view_dir, mat3 norm, float h) {
    uint num_scales = 4;
    uint scale_p = 2;
    uint scale = uint(pow(2, scale_p));
    float width = 1.0;

    float ss = log(abs(camera_pos.z)) / log(scale);
    int starting_scale = max(0, int(floor(log(abs(camera_pos.z)) / log(scale))));

    float b = ss - starting_scale;

    float bounds = min(max(extents.x, extents.y), pow(2, 71));
    
    int min_s = 0;
    int max_s = int(floor(log(bounds) / log(16)));


    vec4 color = vec4(0.0);
    
    float ddistx = dFdx(dist);
    float ddisty = dFdx(dist);
    

    //

    /*
    for(int i = -2; i <= 2; ++i) {
        int ii = i + starting_scale;
        if(ii >= min_s && ii <= max_s) {
            uint pp = ii * scale_p;

            float radius = pow(2, pp);

            //

            vec2 pos = p / radius;
            
            vec2 dx = dFdx(pos);
            vec2 dy = dFdy(pos);
            vec2 ax = vec2(dx.x, dy.x);
            vec2 ay = vec2(dx.y, dy.y);
            float ddx = length(ax);
            float ddy = length(ay);

            float dhdx = dFdx(h);
            float dhdy = dFdy(h);

            float lw = 1.0 / 32.0;

            vec4 x_color = vec4(0.0);
            vec4 y_color = vec4(0.0);

            float fade1 = min(min(abs(camera_pos.z), bounds) / (radius * 0.125), 1.0);

            bool axis = false;

            float x_d = abs(pos.x) / ddx;
            float y_d = abs(pos.y) / ddy;

            x_d = abs(pos.x) * 4.0 / ddx;
            y_d = abs(pos.y) * 4.0 / ddy;

            if(!axis) {
                float x_width = abs(fract(pos.x + 0.5) - 0.5) * 2.0;
                float y_width = abs(fract(pos.y + 0.5) - 0.5) * 2.0;
                if(x_width / ddx <= 1.0) x_color = vec4(1.0, 1.0, 1.0, lw / ddx + 0.3);
                if(y_width / ddy <= 1.0) y_color = vec4(1.0, 1.0, 1.0, lw / ddy + 0.3);
            }

            vec2 dd = vec2(ddx, ddy);

            vec2 c = ceil(abs(pos) - dd * 0.5) * radius;
            if(c.x > bounds || c.y > bounds) {
                x_color = vec4(0.0);
                y_color = vec4(0.0);
            }

            float opacity = 0.75;

            color = vec4(max(color, min(vec4(line_color, 1.0), max(x_color, y_color))));
        }
    }
    */

    /*
    for(int i = -1; i <= 1; ++i) {
        int ii = i + starting_scale;
        if(ii >= min_s && ii <= max_s) {
            uint pp = ii * scale_p;

            float radius = pow(2, pp);

            //

            vec2 pos = p / radius;
            
            vec2 dx = dFdx(pos);
            vec2 dy = dFdy(pos);
            vec2 ax = vec2(dx.x, dy.x);
            vec2 ay = vec2(dx.y, dy.y);
            float ddx = length(ax);
            float ddy = length(ay);

            float dist = length(vec3(pos * radius, 0.0) - camera_pos) / radius;
            float f = clamp(exp(-max(dist * 0.01, 0.0)), 0.0, 1.0);
            float f2 = 1.0 - clamp(exp(-max(dist * pow(2, scale_p) * 0.01, 0.0)), 0.0, 1.0);
            f = min(f, f2);
            
            float distance_to_line = min(abs(pos.x - round(pos.x)) / ddx, abs(pos.y - round(pos.y)) / ddy);
            
            color = vec4(line_color, max(color.w, smoothstep(1.0, 0.0, distance_to_line / 2.0) * f));
        }
    }
    */

    if(dot(vec3(p, 0.0) - camera_pos, view_dir) < 0) return vec4(0.0);

    vec2 pos = p / pow(2, (starting_scale - 1) * scale_p);


            
    vec2 dx = dFdx(pos);
    vec2 dy = dFdy(pos);
    vec2 ax = vec2(dx.x, dy.x);
    vec2 ay = vec2(dx.y, dy.y);
    float ddx = length(ax);
    float ddy = length(ay);

    return vec4(line_color, 1.0 - filtered_grid(pos, dx, dy));

    if(dist < 0.0) return vec4(0.0);

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
    
    float denom = dot(ray, vec3(0.0, 0.0, 1.0));

    vec3 origin = vec3(model * vec4(0.0, 0.0, 0.0, 1.0));

    vec3 normal = vec3(0.0, 0.0, 1.0);

    float dist = dot(-origin, normal) / -dot(ray, normal);

    vec3 p = -origin + ray * dist;

    vec4 color = vec4(0.0);
    //if(dist > 0) {
        vec4 d = proj * (view * vec4((ray * dist), 1.0));
        d /= d.w;

        if(d.z > depth) gl_FragDepth = d.z;

        vec2 plane_pos = p.xy;

        vec3 view_dir = ray;

        mat3 norm_mat = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);

        color = get_color(plane_pos, dist, -origin, ray, norm_mat, denom);

        if(color.w == 0.0) color.w = 0.0;
    //}

    depth = texture(viewport_depth_tex, cs.xy * 0.5 + 0.5).r;
    
    vec4 fcolor = color * color.w;
    fcolor = min(fcolor, 1.0);

    render_line(origin, vec3(1.0, 0.0, 0.0), vec3(0.0), ray, mat3(inv_view), pos_x_color, 0.0, min(extents.x, flt_max));
    render_line(origin, vec3(0.0, 1.0, 0.0), vec3(0.0), ray, mat3(inv_view), pos_y_color, 0.0, min(extents.y, flt_max));
    render_line(origin, vec3(0.0, 0.0, 1.0), vec3(0.0), ray, mat3(inv_view), pos_z_color, 0.0, min(extents.z, flt_max));
    render_line(origin, vec3(1.0, 0.0, 0.0), vec3(0.0), ray, mat3(inv_view), neg_x_color, -min(extents.x, flt_max), 0.0);
    render_line(origin, vec3(0.0, 1.0, 0.0), vec3(0.0), ray, mat3(inv_view), neg_y_color, -min(extents.y, flt_max), 0.0);
    render_line(origin, vec3(0.0, 0.0, 1.0), vec3(0.0), ray, mat3(inv_view), neg_z_color, -min(extents.z, flt_max), 0.0);

    if(lcolor.w != 0.0) fcolor = blend(fcolor, lcolor);

    if(depth == 0.0) frag_color = vec4(base_color, 1.0);
    else frag_color = vec4(0.0);

    frag_color = vec4(frag_color.xyz * (1.0 - fcolor.w), frag_color.w) + vec4(fcolor.xyz * fcolor.w, fcolor.w);

    //frag_normal = vec4(0.0);

    /*float x_a = floor(x) / 10;
    float x_b = floor(x / 10) / 10;
    float x_c = floor(x / 100) / 10;

    frag_color = vec4(x_a, x_b, x_c, 1);*/
}