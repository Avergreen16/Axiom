#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in ivec4 bone_ids;
layout(location = 4) in vec4 bone_weights;

layout(location = 0) uniform mat4 proj;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 model;
layout(location = 6) uniform double time;
layout(location = 7) uniform uint seed2;

layout(location = 0) out vec3 o_normal;
layout(location = 1) out vec3 o_pos;
layout(location = 2) out float f;
layout(location = 3) out vec3 tint;
layout(location = 4) out vec3 o_rel_pos;
layout(location = 5) out vec3 o_rel_normal;

uint seed = seed2;

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

float min_size = 3;
float max_size = 12;

void main() {
    vec4 pos = vec4(position, 1.0);

    o_rel_pos = position;

    mat3 m = mat3(model);

    m = mat3(normalize(m[0]), normalize(m[1]), normalize(m[2]));
    mat3 vv = mat3(view);

    o_normal = m * normal;
    o_rel_normal = vv * m * normal;
    if(normal == vec3(0.0)) o_normal = vec3(0.0);

    o_normal = normalize(o_normal);

    gl_Position = (view * model * pos);

    o_pos = gl_Position.xyz;

    gl_Position = proj * gl_Position;

    f = bone_weights.w;
    tint = bone_weights.xyz;
}