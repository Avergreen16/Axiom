#version 460 core

layout(binding = 0) uniform sampler2D viewport_normal_tex;
layout(binding = 1) uniform sampler2D viewport_shading_tex;
layout(binding = 2) uniform sampler2D viewport_depth_tex;
layout(binding = 3) uniform sampler2D shadow_normal_tex[5];
layout(binding = 8) uniform sampler2D shadow_depth_tex[5];

//
layout(location = 0) in vec2 coords;
layout(location = 1) in mat4 inv_viewport_model;
layout(location = 5) in mat4 inv_viewport_view;
layout(location = 9) in mat4 inv_viewport_proj;

layout(location = 3) uniform mat4 shadow_model[5];
layout(location = 8) uniform mat4 shadow_view[5];
layout(location = 13) uniform mat4 shadow_proj[5];

layout(location = 19) uniform float texture_size;
layout(location = 20) uniform float pixel_size;
layout(location = 21) uniform float texture_growth;

layout(location = 0) out vec4 frag_color;

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
    return f * 2.0 - 3.0;                // Range [-1:1]
}

//

float pi = 3.141592653;

vec3 hsv_color(float hue, float saturation, float value) {
    float x = mod(hue, 1.0) * 6.0;
    float frac = fract(x);

    if(x < 1.0) return vec3(1.0, frac, 0.0);
    if(x < 2.0) return vec3(1.0 - frac, 1.0, 0.0);
    if(x < 3.0) return vec3(0.0, 1.0, frac);
    if(x < 4.0) return vec3(0.0, 1.0 - frac, 1.0);
    if(x < 5.0) return vec3(frac, 0.0, 1.0);
    return vec3(1.0, 0.0, 1.0 - frac);
}

vec4 blend(vec4 dst, vec4 src) {
    return dst * (1.0 - src.w) + src * src.w;
}

mat4 get_transform(mat4 m) {
    return mat4(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        m[3][0], m[3][1], m[3][2], 1
    );
}

void create_offsets(out vec2[16] v, ivec2 pos) {
    for(int i = 0; i < 16; ++i) {
        vec2 vv = normalize(vec2(to_float(hash(uint((pos.x * 3 * pos.y) * 255 + i))), to_float(hash(uint((pos.x * 2 * pos.y * 5) * 511 + i))))) * (float(i + 1) / 16);
        v[i] = vv;
    }
}

vec2 offsets[16];

void main() {
    frag_color = vec4(0.0);
    
    ivec2 ptexel = ivec2(gl_FragCoord.xy);

    ivec2 size = textureSize(shadow_depth_tex[0], 0);

    create_offsets(offsets, ptexel);

    float depth = texelFetch(viewport_depth_tex, ptexel, 0).r;
    vec3 normal = texelFetch(viewport_normal_tex, ptexel, 0).rgb * 2.0 - 1.0;

    vec4 shading = texelFetch(viewport_shading_tex, ptexel, 0);
    shading.xyz = shading.xyz * 2.0 - 1.0;

    vec4 pos = vec4(vec2(gl_FragCoord.xy) / vec2(textureSize(viewport_depth_tex, 0)) * 2.0 - 1.0, depth, 1.0);

    pos = inv_viewport_proj * pos;
    pos /= pos.w;
    
    pos = inv_viewport_view * pos;


    uint include = 0xFFFFFFFF;
    float sd = 0.0;
    vec3 light_dir;
    float max_depth = 3e34;

    light_dir = normalize(mat3(transpose(shadow_view[0])) * vec3(0.0, 0.0, 1.0));
    vec3 ppos = light_dir * -1e10;

    bool x = false;

    //for(int i = 4; i >= 0; --i) {
    for(int i = 0; i < 5; ++i) {
        float texel_size = pixel_size * pow(texture_growth, i);
        vec4 spos = pos;

        mat4 smodel = shadow_model[i];// * inv_viewport_model);
        mat4 sview = shadow_view[i];
        mat4 sproj = shadow_proj[i];

        spos = sview * spos;

        float pdepth = spos.z;

        spos = sproj * spos;
        spos /= spos.w;

        if(include == 0xFFFFFFFF && spos.x < 1.0 && spos.x > -1.0 && spos.y < 1.0 && spos.y > -1.0 && spos.z >= 0.0 && spos.z < 1.0) {
            vec2 texel_f = (spos.xy * 0.5 + 0.5) * vec2(textureSize(shadow_depth_tex[i], 0));
            ivec2 stexel = ivec2(floor(texel_f));

            vec3 snormal = texelFetch(shadow_normal_tex[i], stexel, 0).rgb * 2.0 - 1.0;
            float sdepth = texelFetch(shadow_depth_tex[i], stexel, 0).r;

            vec3 vx = vec3(1.0, 0.0, 0.0);
            vec3 vy = vec3(0.0, 1.0, 0.0);
            vec3 snormal2 = mat3(sview) * snormal;
            float slope_x = dot(snormal2, vx);
            float slope_y = dot(snormal2, vy);

            spos = vec4(spos.xy, sdepth, 1.0);
            spos = inverse(sproj) * spos;
            spos = inverse(sview) * spos;

            float psdepth = sdepth;

            sdepth = dot(vec3(spos), light_dir);
            pdepth = dot(vec3(pos), light_dir);
            float rdepth = dot(ppos, light_dir);

            vec4 pp = vec4(ppos, 1.0);
            pp = sview * pp;
            pp = sproj * pp;

            if((pp.x < 1.0 && pp.x > -1.0 && pp.y < 1.0 && pp.y > -1.0 && pp.z >= 0.0 && pp.z < 1.0 || sdepth > rdepth) && x == false) {
                sd = sdepth;

                float bias = texel_size * 2.0;

                float cos_theta = clamp(dot(snormal, light_dir), 0.1, 1.0);
                float slope = sqrt(1.0 - cos_theta * cos_theta) / cos_theta;
                bias += slope * texel_size;

                float target_depth = pdepth;// + bias;
                target_depth = (sproj * sview * pos).z;

                float frac = 0.0;//float(target_depth < psdepth);
                float null_pixels = 0;

                for(int j = 0; j < 16; ++j) {
                    vec2 screen_pos = texel_f;
                    screen_pos += offsets[j];

                    ivec2 ntexel = ivec2(floor(screen_pos));

                    if(ntexel.x < 0 || ntexel.y < 0 || ntexel.x >= size.x || ntexel.y >= size.y) ++null_pixels;
                    else {
                        float sdepth2 = texelFetch(shadow_depth_tex[i], ntexel, 0).r;

                        if(target_depth < sdepth2) ++frac;
                    }
                }
                
                frag_color = vec4(0.0, 0.0, 0.0, frac / (16.0 - null_pixels));

                x = true;
            }
        }
    }
    
    if(!(normal.x == -1.0 && normal.y == -1.0 && normal.z == -1.0)) frag_color = vec4(0.0, 0.0, 0.0, mix((1.0 - clamp(dot(light_dir, normal), 0.0, 1.0)) * 1.0, 1.0, frag_color.w)); 
    frag_color.w *= shading.w;

    //
    
    //frag_color = vec4(normal, 0.75);
}