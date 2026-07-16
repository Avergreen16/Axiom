#version 460 core

layout(binding = 0) uniform sampler3D density;

layout(location = 3) uniform vec3 light_dir;
layout(location = 4) uniform vec3 apos;
layout(location = 5) uniform float darkness_value;
layout(location = 6) uniform double time;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 slope;

layout(location = 0) in vec2 p;
layout(location = 1) in vec3 pos;
layout(location = 2) in float f;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec2 size;

uint seed = uint(time / 4);

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

vec3 color1 = vec3(0.75, 0.8, 0.9) * 0.5;
vec3 color2 = vec3(0.9, 0.8, 0.5) * 0.5;

void main() {
    discard;
    
    float inner_rad = size.x;
    float outer_rad = size.y;
    float fade = 0.005;

    float dist = length(p);
    
    if(dist < inner_rad || dist > outer_rad) discard;

    float detail = 0x20000 * 8.5;

    float v = (dist - inner_rad) / (outer_rad - inner_rad);
    float fade_value = 1.0;
    if(v < fade) fade_value = v / fade;
    else if(1.0 - v < fade) fade_value = (1.0 - v) / fade;

    //v *= (outer_rad - inner_rad);


    vec3 offset = vec3(hash_float(0x111), hash_float(0x222), hash_float(0x444));
    float w = hash_float(0x333) * 0.4 + 0.4;
    v *= w;
    
    //v *= w / detail;

    float vv = (texture(density, vec3(0, 0, v) + offset).r - 0.5) * 4.0 + 0.75;
    vv += (texture(density, vec3(0, 0, v * 4.0) + offset).r - 0.5) * 0.5;
    vv *= fade_value;
    vv = clamp(vv, 0.0, 1.0);
    vv = smoothstep(0.0, 1.0, vv);
    float color_v = 0.0;
    
    if(vv < 0.5) discard;
    else {
        color_v = vv;
        vv = 1;
    }


    // y is shadow darkness
    vec3 normal_2 = normalize(cross(dFdx(pos), dFdy(pos)));
    float cosine = dot(normal_2, vec3(0, 0, -1));
    if(gl_FrontFacing) {
        if(cosine < 0) cosine = -cosine;
    } else {
        if(cosine > 0) cosine = -cosine;
    }
    
    slope = vec4(cosine * 0.5 + 0.5, 0.0, abs(f) * 2, 1.0);
    vec3 output_normal = normal * 0.5 + 0.5;

    output_normal = vec3(0.0);
    //

    vec3 color = mix(color1, color2, hash_float(0x8F153));
    color *= color_v;
    //frag_normal = vec4(output_normal, 1.0);
    frag_normal = vec4(output_normal, 1.0);
    frag_color = vec4(color, vv);
}