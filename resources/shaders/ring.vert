#version 460 core

vec2 vertices[4] = {
    vec2(-1, -1),
    vec2(1, -1),
    vec2(-1, 1),
    vec2(1, 1)
};

int indices[12] = {
    0, 1, 3,
    0, 3, 2,
    0, 3, 1,
    0, 2, 3
};

layout(location = 0) uniform mat4 proj;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 model;
layout(location = 6) uniform double time;

layout(location = 0) out vec2 p;
layout(location = 1) out vec3 o_pos;
layout(location = 2) out float f;
layout(location = 3) out vec3 o_normal;
layout(location = 4) out vec2 o_size;

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

void main() {
    float size = 0x20000 * (8.5 * (1.25 + 3.75 * hash_float(0x8153)));
    float inner_size = max(size - (0x20000 * (8.5 * (0.125 + 3.875 * hash_float(0xFF44ED)))), 0x20000 * 8.5 * 1.25);
    if(hash(0x2931) % 3 == 0) size = 0.0;

    o_size = vec2(inner_size, size);

    vec2 v = vertices[indices[gl_VertexID]] * size;

    p = v;

    // normal
    vec3 normal;
    if(gl_VertexID < 6) normal = vec3(0, 0, 1);
    else normal = vec3(0, 0, -1);

    o_normal = mat3(model) * normal;
    //

    gl_Position = view * model * vec4(v, 0.0, 1.0);
    
    gl_Position = proj * gl_Position;
}