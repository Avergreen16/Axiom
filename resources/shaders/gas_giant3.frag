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

mat2 get_rot(float angle) {
   return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

float map(float value, float min1, float max1, float min2, float max2) {
    float f = min2 + (value - min1) * (max2 - min2) / (max1 - min1);
    return min(max(0.0, f), 1.0);
}

vec3 calculate_flow(vec3 pos, float delta, vec2 aspect, float scale) {
    vec3 ddx = normalize(cross(pos, vec3(0, 0, 1)));
    vec3 ddy = normalize(cross(ddx, pos));

    float d0 = texture(density, pos * scale).g;
    float dx = texture(density, (pos * scale + ddx * delta)).g;
    float dy = texture(density, (pos * scale + ddy * delta)).g;

    float curl_x = (d0 - dx) / delta;
    float curl_y = (d0 - dy) / delta;

    return -ddx * curl_y * aspect.x + ddy * curl_x * aspect.y;
}

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