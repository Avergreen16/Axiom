#include <thread>

#include "noise.hpp"
#include "random.hpp"
#include "simd.hpp"

namespace axiom {

std::array<vec3, 16> perlin_vectors = {
    vec3(0, -1, -1),
    vec3(0, -1, 1),
    vec3(0, 1, 1),
    vec3(0, 1, -1), 
    vec3(-1, 0, -1),
    vec3(1, 0, 1),
    vec3(-1, 0, 1),
    vec3(1, 0, -1),
    vec3(-1, 0, -1),
    vec3(1, 0, 1),
    vec3(-1, 0, 1),
    vec3(1, 0, -1), 
    vec3(-1, -1, 0),
    vec3(1, -1, 0),
    vec3(-1, 1, 0),
    vec3(1, 1, 0),
};

std::array<vec3, 16> noise_gen::perlin_vectors = perlin_vectors;

vec3 get_vec(int i) {
    return perlin_vectors[i];
}

std::array<vec3, 256> gen_voronoi_vectors() {
    std::array<vec3, 256> ret;

    random random_(8376436);

    for(int i = 0; i < 156; ++i) {
        vec3 v = {random_(), random_(), random_()};

        v = v * 0.5f + 0.5f;

        ret[i] = v;
    }

    return ret;
}

std::array<vec3, 256> noise_gen::voronoi_vectors = gen_voronoi_vectors();

std::array<int, 256> noise_gen::hash_table = {
    151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
    140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
    247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
    57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
    74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
    60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
    65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
    200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
    52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
    207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
    119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
    129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
    218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
    81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
    184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
    222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180
};

uint8_t noise_gen::hash_with_table(uvec3 i) {
    return hash_table[uint8_t(hash_table[uint8_t(hash_table[uint8_t(i.x)] + i.y)] + i.z)];
}


float noise_gen::voronoi_noise(glm::vec3 position, float period, uint seed) {
    random32 random_(seed);

    float dist = __FLT_MAX__;

    glm::vec3 pos = position / period;

    ivec3 cell_pos = glm::floor(pos);

    for(int z = -1; z <= 1; ++z) {
        for(int y = -1; y <= 1; ++y) {
            for(int x = -1; x <= 1; ++x) {
                ivec3 cell_point = cell_pos + ivec3(x, y, z);
                glm::vec3 pt = voronoi_vectors[hash_with_table(cell_point)] * 0.5f + 0.5f;

                float dist_pt = length((pos - vec3(cell_point)) - pt);

                dist = glm::min(dist_pt, dist);
            }
        }
    }

    return glm::clamp(dist, 0.0f, 1.0f);
}

float noise_gen::perlin_noise(glm::vec3 position, float period, uint octaves, uint seed, float persistance) {
    uint h = hash(seed);

    float sum = 0.0;
    float maximum = 0.0;
    float power = 1;

    position = position / period;

    for(int i = 0; i < octaves; ++i) {
        glm::ivec3 corner_id = glm::ivec3(floor(position));
        glm::vec3 fractional = glm::fract(position);
        glm::vec3 vector_origins[8] = {
            corner_id,
            corner_id + glm::ivec3(1, 0, 0),
            corner_id + glm::ivec3(0, 1, 0),
            corner_id + glm::ivec3(1, 1, 0),
            corner_id + glm::ivec3(0, 0, 1),
            corner_id + glm::ivec3(1, 0, 1),
            corner_id + glm::ivec3(0, 1, 1),
            corner_id + glm::ivec3(1, 1, 1)
        };
        
        glm::vec3 vecs[8] = {
            get_vec(hash(vector_origins[0], seed)),
            get_vec(hash(vector_origins[1], seed)),
            get_vec(hash(vector_origins[2], seed)),
            get_vec(hash(vector_origins[3], seed)),
            get_vec(hash(vector_origins[4], seed)),
            get_vec(hash(vector_origins[5], seed)),
            get_vec(hash(vector_origins[6], seed)),
            get_vec(hash(vector_origins[7], seed)),
        };

        float values[8];

        for(int j = 0; j < 8; ++j) {
            vec3 origin = vector_origins[j];
            vec3 vector = vecs[j];

            vec3 offset = position - origin;

            values[j] = dot(offset, vector);
        }

        fractional = glm::vec3(glm::smoothstep(0.0f, 1.0f, fractional.x), glm::smoothstep(0.0f, 1.0f, fractional.y), glm::smoothstep(0.0f, 1.0f, fractional.z));

        float v = glm::mix(
            glm::mix(
            glm::mix(values[0], values[1], fractional.x), 
            glm::mix(values[2], values[3], fractional.x), fractional.y),
            glm::mix(
            glm::mix(values[4], values[5], fractional.x), 
            glm::mix(values[6], values[7], fractional.x), fractional.y), fractional.z);
        
        sum += v * power;
        maximum += power;

        power *= persistance;

        position *= 2.0f;
    }

    sum /= maximum;

    return sum;
}

float noise_gen::ridged_perlin_noise(glm::vec3 position, float period, uint octaves, uint seed, float persistance) {
    uint h = hash(seed);

    float sum = 0.0;
    float maximum = 0.0;
    float power = 1;

    position = position / period;

    for(int i = 0; i < octaves; ++i) {
        glm::ivec3 corner_id = glm::ivec3(floor(position));
        glm::vec3 fractional = glm::fract(position);
        glm::vec3 vector_origins[8] = {
            corner_id,
            corner_id + glm::ivec3(1, 0, 0),
            corner_id + glm::ivec3(0, 1, 0),
            corner_id + glm::ivec3(1, 1, 0),
            corner_id + glm::ivec3(0, 0, 1),
            corner_id + glm::ivec3(1, 0, 1),
            corner_id + glm::ivec3(0, 1, 1),
            corner_id + glm::ivec3(1, 1, 1)
        };
        
        glm::vec3 vecs[8] = {
            perlin_vectors[(hash_with_table(vector_origins[0]) ^ h) & 0xF],
            perlin_vectors[(hash_with_table(vector_origins[1]) ^ h) & 0xF],
            perlin_vectors[(hash_with_table(vector_origins[2]) ^ h) & 0xF],
            perlin_vectors[(hash_with_table(vector_origins[3]) ^ h) & 0xF],
            perlin_vectors[(hash_with_table(vector_origins[4]) ^ h) & 0xF],
            perlin_vectors[(hash_with_table(vector_origins[5]) ^ h) & 0xF],
            perlin_vectors[(hash_with_table(vector_origins[6]) ^ h) & 0xF],
            perlin_vectors[(hash_with_table(vector_origins[7]) ^ h) & 0xF],
        };

        float values[8];

        for(int j = 0; j < 8; ++j) {
            vec3 origin = vector_origins[j];
            vec3 vector = vecs[j];

            vec3 offset = position - origin;

            values[j] = dot(offset, vector);
        }

        fractional = glm::vec3(glm::smoothstep(0.0f, 1.0f, fractional.x), glm::smoothstep(0.0f, 1.0f, fractional.y), glm::smoothstep(0.0f, 1.0f, fractional.z));

        float v = glm::mix(
            glm::mix(
            glm::mix(values[0], values[1], fractional.x), 
            glm::mix(values[2], values[3], fractional.x), fractional.y),
            glm::mix(
            glm::mix(values[4], values[5], fractional.x), 
            glm::mix(values[6], values[7], fractional.x), fractional.y), fractional.z);

        v = 1.0f - abs(v);
        v *= v;
        
        sum += v * power;
        maximum += power;

        power *= persistance;

        position *= 2.0f;
    }

    sum /= maximum;

    return sum;
}

std::vector<float> noise_gen::perlin_noise(vec3 pos, float period, uint octaves, uint seed, ivec3 size, float diff, float persistance) {
    std::vector<float> ret;
    ret.resize(size.x * size.y * size.z + N);

    int num_target = size.x * size.y * size.z;

    float sx = 1.0f / size.x + 0.00001f;
    float sy = 1.0f / size.y + 0.00001f;
    float ss = sx * sy;
    float frequency = 1.0f / period;

    alignas(32) float iota_v[N];
    for(int i = 0; i < N; ++i) {
        iota_v[i] = i;
    }
    batch iota = xsimd::load_aligned(iota_v);

    int num_threads = 1;
    std::vector<std::thread> threads(num_threads);

    int section = ceil(float(num_target) / num_threads);

    for(int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread([&, i]() {
            int num_current = section * i;
            int end_pos = glm::min(num_target, section * (i + 1));

            while(num_current < end_pos) {
                simd_vec3 position;

                batch bb = iota + float(num_current);

                batch xv = floor(bb / size.x);
                batch ssv = floor(bb / (size.x * size.y));

                position.x = bb - xv * size.x;
                position.y = xv - ssv * size.y;
                position.z = ssv;

                position *= diff;
                position += pos;
                position *= frequency;

                alignas(32) float v[N];
                for(int i = 0; i < N; ++i) v[i] = 0;

                batch sum = xsimd::load_aligned(v);

                float maximum = 0.0;
                float power = 1.0f;

                for(int i = 0; i < octaves; ++i) {
                    simd_vec3 corner = position.floor();
                    simd_vec3 fractional = position - corner;
                    simd_vec3 origins[8] = {
                        corner,
                        corner + vec3(1, 0, 0),
                        corner + vec3(0, 1, 0),
                        corner + vec3(1, 1, 0),
                        corner + vec3(0, 0, 1),
                        corner + vec3(1, 0, 1),
                        corner + vec3(0, 1, 1),
                        corner + vec3(1, 1, 1),
                    };

                    batch values[8];
                    simd_vec3 gradient;
                    
                    int ii = 0;
                    for(simd_vec3& v : origins) {
                        batch_int x = xsimd::batch_cast<int>(v.x);
                        batch_int y = xsimd::batch_cast<int>(v.y);
                        batch_int z = xsimd::batch_cast<int>(v.z);
                        
                        batch_int index = hash_coords(x, y, z);

                        batch_int batch_A = ((index & 1) << 1) - 1; // -1 and 1
                        batch_int batch_B = ((index & 2)) - 1; // -1 and 1
                        batch_int batch_C = (((index + 1) & 2)) - 1; // -1 and 1
                        batch_int batch_0 = index & 0; // zero
                        auto batch_1 = index > 3;
                        auto batch_2 = index < 4 || index > 11;
                        auto batch_3 = index < 12;

                        gradient.x = xsimd::batch_cast<float>(xsimd::select(batch_1, batch_A, batch_0));
                        gradient.y = xsimd::batch_cast<float>(xsimd::select(batch_2, batch_B, batch_0));
                        gradient.z = xsimd::batch_cast<float>(xsimd::select(batch_3, batch_C, batch_0));

                        values[ii] = (position - v).dot(gradient);
                        ++ii;
                    }
                    
                    fractional = smoothstep(fractional);

                    batch A = lerp(values[0], values[1], fractional.x);
                    batch B = lerp(values[2], values[3], fractional.x);
                    batch C = lerp(values[4], values[5], fractional.x);
                    batch D = lerp(values[6], values[7], fractional.x);

                    A = lerp(A, B, fractional.y);
                    B = lerp(C, D, fractional.y);
                    
                    A = lerp(A, B, fractional.z);
                    
                    sum += A * power;
                    maximum += power;
                    power *= persistance;
                    position *= 2.0f;
                }

                float m = 1.0f / maximum;

                sum *= m;

                alignas(32) float r[N];
                sum.store_aligned(r);
                //memcpy(&ret[i], &sum, N);

                for(int i = 0; i < N; ++i) {
                    ret[i + num_current] = r[i];
                }

                num_current += N;
            }
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    return ret;
}


std::vector<float> noise_gen::ridged_perlin_noise(vec3 pos, float period, uint octaves, uint seed, ivec3 size, float diff, float persistance) {
    uint h = hash(seed);

    std::vector<float> ret;
    ret.reserve(size.x * size.y * size.z);

    //stdx::simd_size<float> size_var;
    //int size_v = size_var.value;
    int size_v = 16;

    int num_current = 0;
    int num_target = size.x * size.y * size.z;

    while(num_current < num_target) {
        stdx::fixed_size_simd<float, 16> a([num_current, pos, period, octaves, seed, size, diff, persistance, h](int i) {
            uint i2 = i + num_current;
            ivec3 chunk_pos = {i2 % size.x, i2 / size.x % size.y, i2 / (size.x * size.y)};

            vec3 position = (vec3)chunk_pos * diff;
            position += pos;
            position /= period;

            float sum = 0.0;
            float maximum = 0.0;
            float power = 1;

            //position = position / period;

            for(int i = 0; i < octaves; ++i) {
                glm::ivec3 corner_id = glm::ivec3(floor(position));
                glm::vec3 fractional = glm::fract(position);
                glm::vec3 vector_origins[8] = {
                    corner_id,
                    corner_id + glm::ivec3(1, 0, 0),
                    corner_id + glm::ivec3(0, 1, 0),
                    corner_id + glm::ivec3(1, 1, 0),
                    corner_id + glm::ivec3(0, 0, 1),
                    corner_id + glm::ivec3(1, 0, 1),
                    corner_id + glm::ivec3(0, 1, 1),
                    corner_id + glm::ivec3(1, 1, 1)
                };
                
                glm::vec3 vecs[8] = {
                    perlin_vectors[(hash_with_table(vector_origins[0]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[1]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[2]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[3]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[4]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[5]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[6]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[7]) ^ h) & 0xF],
                };

                float values[8];

                for(int j = 0; j < 8; ++j) {
                    vec3 origin = vector_origins[j];
                    vec3 vector = vecs[j];

                    vec3 offset = position - origin;

                    values[j] = dot(offset, vector);
                }

                fractional = glm::vec3(glm::smoothstep(0.0f, 1.0f, fractional.x), glm::smoothstep(0.0f, 1.0f, fractional.y), glm::smoothstep(0.0f, 1.0f, fractional.z));

                float v = glm::mix(
                    glm::mix(
                    glm::mix(values[0], values[1], fractional.x), 
                    glm::mix(values[2], values[3], fractional.x), fractional.y),
                    glm::mix(
                    glm::mix(values[4], values[5], fractional.x), 
                    glm::mix(values[6], values[7], fractional.x), fractional.y), fractional.z);

                v = abs(v);
                
                sum += v * power;
                maximum += power;

                power *= persistance;

                position *= 2.0f;
            }

            sum /= maximum;

            return sum;
        });

        for(int i = 0; i < size_v; ++i) {
            ret.push_back(a[i]);
        }

        num_current += size_v;
    }

    ret.resize(size.x * size.y * size.z);
    return ret;
}


std::vector<float> noise_gen::perlin_noise_normalized(vec3 pos, float period, uint octaves, uint seed, ivec3 size, float diff, float persistance) {
    uint h = hash(seed);

    std::vector<float> ret;
    ret.reserve(size.x * size.y * size.z);

    //stdx::simd_size<float> size_var;
    //int size_v = size_var.value;
    int size_v = 16;

    int num_current = 0;
    int num_target = size.x * size.y * size.z;


    while(num_current < num_target) {
        stdx::fixed_size_simd<float, 16> a([num_current, pos, period, octaves, seed, size, diff, persistance, h](int i) {
            uint i2 = i + num_current;
            ivec3 chunk_pos = {i2 % size.x, i2 / size.x % size.y, i2 / (size.x * size.y)};

            vec3 position = (vec3)chunk_pos * diff;
            position += pos;
            position = normalize(position);
            position /= period;

            float sum = 0.0;
            float maximum = 0.0;
            float power = 1;

            //position = position / period;

            for(int i = 0; i < octaves; ++i) {
                vec3 norm_pos = normalize(position);

                glm::ivec3 corner_id = glm::ivec3(floor(position));
                glm::vec3 fractional = glm::fract(position);
                glm::vec3 vector_origins[8] = {
                    corner_id,
                    corner_id + glm::ivec3(1, 0, 0),
                    corner_id + glm::ivec3(0, 1, 0),
                    corner_id + glm::ivec3(1, 1, 0),
                    corner_id + glm::ivec3(0, 0, 1),
                    corner_id + glm::ivec3(1, 0, 1),
                    corner_id + glm::ivec3(0, 1, 1),
                    corner_id + glm::ivec3(1, 1, 1)
                };
                
                glm::vec3 vecs[8] = {
                    perlin_vectors[(hash_with_table(vector_origins[0]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[1]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[2]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[3]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[4]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[5]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[6]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[7]) ^ h) & 0xF],
                };

                float values[8];

                for(int j = 0; j < 8; ++j) {
                    vec3 origin = vector_origins[j];
                    vec3 vector = vecs[j];

                    vec3 offset = position - origin;

                    values[j] = dot(offset, vector);
                }

                fractional = glm::vec3(glm::smoothstep(0.0f, 1.0f, fractional.x), glm::smoothstep(0.0f, 1.0f, fractional.y), glm::smoothstep(0.0f, 1.0f, fractional.z));

                float v = glm::mix(
                    glm::mix(
                    glm::mix(values[0], values[1], fractional.x), 
                    glm::mix(values[2], values[3], fractional.x), fractional.y),
                    glm::mix(
                    glm::mix(values[4], values[5], fractional.x), 
                    glm::mix(values[6], values[7], fractional.x), fractional.y), fractional.z);
                
                sum += v * power;
                maximum += power;

                power *= persistance;

                position *= 2.0f;
            }

            sum /= maximum;

            return sum;
        });

        for(int i = 0; i < size_v; ++i) {
            ret.push_back(a[i]);
        }

        num_current += size_v;
    }

    //ret.resize(size.x * size.y * size.z);
    return ret;
}


std::vector<float> noise_gen::ridged_perlin_noise_normalized(vec3 pos, float period, uint octaves, uint seed, ivec3 size, float diff, float persistance) {
    uint h = hash(seed);

    std::vector<float> ret;
    ret.reserve(size.x * size.y * size.z);

    //stdx::simd_size<float> size_var;
    //int size_v = size_var.value;
    int size_v = 16;

    int num_current = 0;
    int num_target = size.x * size.y * size.z;

    while(num_current < num_target) {
        stdx::fixed_size_simd<float, 16> a([num_current, pos, period, octaves, seed, size, diff, persistance, h](int i) {
            uint i2 = i + num_current;
            ivec3 chunk_pos = {i2 % size.x, i2 / size.x % size.y, i2 / (size.x * size.y)};

            vec3 position = (vec3)chunk_pos * diff;
            position += pos;
            position = normalize(position);
            position /= period;

            float sum = 0.0;
            float maximum = 0.0;
            float power = 1;

            //position = position / period;

            for(int i = 0; i < octaves; ++i) {
                glm::ivec3 corner_id = glm::ivec3(floor(position));
                glm::vec3 fractional = glm::fract(position);
                glm::vec3 vector_origins[8] = {
                    corner_id,
                    corner_id + glm::ivec3(1, 0, 0),
                    corner_id + glm::ivec3(0, 1, 0),
                    corner_id + glm::ivec3(1, 1, 0),
                    corner_id + glm::ivec3(0, 0, 1),
                    corner_id + glm::ivec3(1, 0, 1),
                    corner_id + glm::ivec3(0, 1, 1),
                    corner_id + glm::ivec3(1, 1, 1)
                };
                
                glm::vec3 vecs[8] = {
                    perlin_vectors[(hash_with_table(vector_origins[0]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[1]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[2]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[3]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[4]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[5]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[6]) ^ h) & 0xF],
                    perlin_vectors[(hash_with_table(vector_origins[7]) ^ h) & 0xF],
                };

                float values[8];

                for(int j = 0; j < 8; ++j) {
                    vec3 origin = vector_origins[j];
                    vec3 vector = vecs[j];

                    vec3 offset = position - origin;

                    values[j] = dot(offset, vector);
                }

                fractional = glm::vec3(glm::smoothstep(0.0f, 1.0f, fractional.x), glm::smoothstep(0.0f, 1.0f, fractional.y), glm::smoothstep(0.0f, 1.0f, fractional.z));

                float v = glm::mix(
                    glm::mix(
                    glm::mix(values[0], values[1], fractional.x), 
                    glm::mix(values[2], values[3], fractional.x), fractional.y),
                    glm::mix(
                    glm::mix(values[4], values[5], fractional.x), 
                    glm::mix(values[6], values[7], fractional.x), fractional.y), fractional.z);

                v = abs(v);
                
                sum += v * power;
                maximum += power;

                power *= persistance;

                position *= 2.0f;
            }

            sum /= maximum;

            return sum;
        });

        for(int i = 0; i < size_v; ++i) {
            ret.push_back(a[i]);
        }

        num_current += size_v;
    }

    ret.resize(size.x * size.y * size.z);
    return ret;
}

}