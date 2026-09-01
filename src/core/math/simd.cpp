#include "math/simd.hpp"
#include "math/random.hpp"

namespace axiom {

batch lerp(batch a, batch b, batch x) {
    return a + x * (b - a);
}

batch smoothstep(batch x) {
    batch xx = x * x;
    return xx * 3 - xx * x * 2;
}

simd_vec3 smoothstep(simd_vec3 v) {
    simd_vec3 ret;
    ret.x = smoothstep(v.x);
    ret.y = smoothstep(v.y);
    ret.z = smoothstep(v.z);
    return ret;
}

batch_int hash_coords(batch_int x, batch_int y, batch_int z) {
    batch_int hash = x * PRIME_X;
    hash ^= y * PRIME_Y;
    hash ^= z * PRIME_Z;

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return xsimd::abs(hash) & 0xF;
}

}