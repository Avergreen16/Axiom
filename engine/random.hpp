#pragma once;
#include "pnum.hpp"
#include "wrapper.hpp"

#include <array>
#include <vector>
#include <experimental/simd>

namespace stdx = std::experimental;

#include <xsimd/xsimd.hpp>

using batch = xsimd::batch<float, xsimd::default_arch>;
using batch_int = xsimd::batch<int, xsimd::default_arch>;
constexpr std::size_t N = batch::size;

vec3 hex_color(uint32_t color);
vec3 hsv_color(float hue, float saturation, float value);

batch_int hash_coords(batch_int x, batch_int y, batch_int z);
int hash_coord(ivec3 v);

struct simd_vec3 {
    batch x;
    batch y;
    batch z;

    simd_vec3 operator+(const simd_vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x + a.x;
        ret_v.y = y + a.y;
        ret_v.z = z + a.z;
        return ret_v;
    }

    simd_vec3 operator-(const simd_vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x - a.x;
        ret_v.y = y - a.y;
        ret_v.z = z - a.z;
        return ret_v;
    }

    simd_vec3 operator*(const simd_vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x * a.x;
        ret_v.y = y * a.y;
        ret_v.z = z * a.z;
        return ret_v;
    }

    simd_vec3 operator/(const simd_vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x / a.x;
        ret_v.y = y / a.y;
        ret_v.z = z / a.z;
        return ret_v;
    }

    simd_vec3 operator+=(const simd_vec3& a) {
        x = x + a.x;
        y = y + a.y;
        z = z + a.z;

        return *this;
    }

    simd_vec3 operator-=(const simd_vec3& a) {
        x = x - a.x;
        y = y - a.y;
        z = z - a.z;

        return *this;
    }

    simd_vec3 operator*=(const simd_vec3& a) {
        x = x * a.x;
        y = y * a.y;
        z = z * a.z;

        return *this;
    }

    simd_vec3 operator/=(const simd_vec3& a) {
        x = x / a.x;
        y = y / a.y;
        z = z / a.z;

        return *this;
    }

    simd_vec3 operator=(const simd_vec3& a) {
        x = a.x;
        y = a.y;
        z = a.z;

        return *this;
    }

    simd_vec3 operator+(const vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x + a.x;
        ret_v.y = y + a.y;
        ret_v.z = z + a.z;
        return ret_v;
    }

    simd_vec3 operator-(const vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x - a.x;
        ret_v.y = y - a.y;
        ret_v.z = z - a.z;
        return ret_v;
    }

    simd_vec3 operator*(const vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x * a.x;
        ret_v.y = y * a.y;
        ret_v.z = z * a.z;
        return ret_v;
    }

    simd_vec3 operator/(const vec3& a) {
        simd_vec3 ret_v;
        ret_v.x = x / a.x;
        ret_v.y = y / a.y;
        ret_v.z = z / a.z;
        return ret_v;
    }

    simd_vec3 operator+=(const vec3& a) {
        x = x + a.x;
        y = y + a.y;
        z = z + a.z;

        return *this;
    }

    simd_vec3 operator-=(const vec3& a) {
        x = x - a.x;
        y = y - a.y;
        z = z - a.z;

        return *this;
    }

    simd_vec3 operator*=(const vec3& a) {
        x = x * a.x;
        y = y * a.y;
        z = z * a.z;

        return *this;
    }

    simd_vec3 operator/=(const vec3& a) {
        x = x / a.x;
        y = y / a.y;
        z = z / a.z;

        return *this;
    }

    simd_vec3 operator+(const float& a) {
        simd_vec3 ret_v;
        ret_v.x = x + a;
        ret_v.y = y + a;
        ret_v.z = z + a;
        return ret_v;
    }

    simd_vec3 operator-(const float& a) {
        simd_vec3 ret_v;
        ret_v.x = x - a;
        ret_v.y = y - a;
        ret_v.z = z - a;
        return ret_v;
    }

    simd_vec3 operator*(const float& a) {
        simd_vec3 ret_v;
        ret_v.x = x * a;
        ret_v.y = y * a;
        ret_v.z = z * a;
        return ret_v;
    }

    simd_vec3 operator/(const float& a) {
        simd_vec3 ret_v;
        ret_v.x = x / a;
        ret_v.y = y / a;
        ret_v.z = z / a;
        return ret_v;
    }

    simd_vec3 operator+=(const float& a) {
        x = x + a;
        y = y + a;
        z = z + a;

        return *this;
    }

    simd_vec3 operator-=(const float& a) {
        x = x - a;
        y = y - a;
        z = z - a;

        return *this;
    }

    simd_vec3 operator*=(const float& a) {
        x = x * a;
        y = y * a;
        z = z * a;

        return *this;
    }

    simd_vec3 operator/=(const float& a) {
        x = x / a;
        y = y / a;
        z = z / a;

        return *this;
    }
    
    batch dot(const simd_vec3& a) {
        return x * a.x + y * a.y + z * a.z;
    }

    void lambda(std::function<vec3(int)> func) {
        alignas(32) float temp_x[N];
        alignas(32) float temp_y[N];
        alignas(32) float temp_z[N];

        for(int j = 0; j < N; ++j) {
            vec3 v = func(j);

            temp_x[j] = v.x;
            temp_y[j] = v.y;
            temp_z[j] = v.z;
        }

        x = batch::load_aligned(temp_x);
        y = batch::load_aligned(temp_y);
        z = batch::load_aligned(temp_z);
    }

    simd_vec3 floor() {
        simd_vec3 v;

        v.x = xsimd::floor(x);
        v.y = xsimd::floor(y);
        v.z = xsimd::floor(z);

        return v;
    }
};

struct simd_ivec3 {
    batch_int x;
    batch_int y;
    batch_int z;

    simd_ivec3 operator+(const simd_ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x + a.x;
        ret_v.y = y + a.y;
        ret_v.z = z + a.z;
        return ret_v;
    }

    simd_ivec3 operator-(const simd_ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x - a.x;
        ret_v.y = y - a.y;
        ret_v.z = z - a.z;
        return ret_v;
    }

    simd_ivec3 operator*(const simd_ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x * a.x;
        ret_v.y = y * a.y;
        ret_v.z = z * a.z;
        return ret_v;
    }

    simd_ivec3 operator/(const simd_ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x / a.x;
        ret_v.y = y / a.y;
        ret_v.z = z / a.z;
        return ret_v;
    }

    simd_ivec3 operator+=(const simd_ivec3& a) {
        x = x + a.x;
        y = y + a.y;
        z = z + a.z;

        return *this;
    }

    simd_ivec3 operator-=(const simd_ivec3& a) {
        x = x - a.x;
        y = y - a.y;
        z = z - a.z;

        return *this;
    }

    simd_ivec3 operator*=(const simd_ivec3& a) {
        x = x * a.x;
        y = y * a.y;
        z = z * a.z;

        return *this;
    }

    simd_ivec3 operator/=(const simd_ivec3& a) {
        x = x / a.x;
        y = y / a.y;
        z = z / a.z;

        return *this;
    }

    simd_ivec3 operator=(const simd_ivec3& a) {
        x = a.x;
        y = a.y;
        z = a.z;

        return *this;
    }

    simd_ivec3 operator+(const ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x + a.x;
        ret_v.y = y + a.y;
        ret_v.z = z + a.z;
        return ret_v;
    }

    simd_ivec3 operator-(const ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x - a.x;
        ret_v.y = y - a.y;
        ret_v.z = z - a.z;
        return ret_v;
    }

    simd_ivec3 operator*(const ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x * a.x;
        ret_v.y = y * a.y;
        ret_v.z = z * a.z;
        return ret_v;
    }

    simd_ivec3 operator/(const ivec3& a) {
        simd_ivec3 ret_v;
        ret_v.x = x / a.x;
        ret_v.y = y / a.y;
        ret_v.z = z / a.z;
        return ret_v;
    }

    simd_ivec3 operator+=(const ivec3& a) {
        x = x + a.x;
        y = y + a.y;
        z = z + a.z;

        return *this;
    }

    simd_ivec3 operator-=(const ivec3& a) {
        x = x - a.x;
        y = y - a.y;
        z = z - a.z;

        return *this;
    }

    simd_ivec3 operator*=(const ivec3& a) {
        x = x * a.x;
        y = y * a.y;
        z = z * a.z;

        return *this;
    }

    simd_ivec3 operator/=(const ivec3& a) {
        x = x / a.x;
        y = y / a.y;
        z = z / a.z;

        return *this;
    }

    simd_ivec3 operator+(const int& a) {
        simd_ivec3 ret_v;
        ret_v.x = x + a;
        ret_v.y = y + a;
        ret_v.z = z + a;
        return ret_v;
    }

    simd_ivec3 operator-(const int& a) {
        simd_ivec3 ret_v;
        ret_v.x = x - a;
        ret_v.y = y - a;
        ret_v.z = z - a;
        return ret_v;
    }

    simd_ivec3 operator*(const int& a) {
        simd_ivec3 ret_v;
        ret_v.x = x * a;
        ret_v.y = y * a;
        ret_v.z = z * a;
        return ret_v;
    }

    simd_ivec3 operator/(const float& a) {
        simd_ivec3 ret_v;
        ret_v.x = x / a;
        ret_v.y = y / a;
        ret_v.z = z / a;
        return ret_v;
    }

    simd_ivec3 operator+=(const int& a) {
        x = x + a;
        y = y + a;
        z = z + a;

        return *this;
    }

    simd_ivec3 operator-=(const int& a) {
        x = x - a;
        y = y - a;
        z = z - a;

        return *this;
    }

    simd_ivec3 operator*=(const int& a) {
        x = x * a;
        y = y * a;
        z = z * a;

        return *this;
    }

    simd_ivec3 operator/=(const int& a) {
        x = x / a;
        y = y / a;
        z = z / a;

        return *this;
    }

    void from_float(simd_vec3 v) {
        x = xsimd::batch_cast<int>(v.x);
        y = xsimd::batch_cast<int>(v.y);
        z = xsimd::batch_cast<int>(v.z);
    }
};

batch lerp(batch a, batch b, batch x);

uint32_t hash(uint32_t x);

uint32_t hash(glm::uvec2 v);

uint32_t hash(glm::uvec3 v);

uint32_t hash(glm::uvec4 v);

float to_float(uint32_t m);

pnum to_pnum(uint32_t m);


uint64_t hash(uint64_t x);

uint64_t hash(vec<2, uint64_t> v);

uint64_t hash(vec<3, uint64_t> v);

uint64_t hash(vec<4, uint64_t> v);

struct Random {
    uint64_t seed;
    uint64_t value;
    
    Random(uint64_t init_seed);

    Random(const Random& r) = default;
    Random& operator=(const Random& r) = default;
    Random(Random&& r) = default;
    Random& operator=(Random&& r) = default;

    float operator()();
    uint64_t next();
    vec3 unit_vector();

    float operator()(glm::vec<3, uint64_t> i);
    uint64_t hash_i(glm::vec<3, uint64_t> i);
    vec3 unit_vector(glm::vec<3, uint64_t> i);
    vec3 cube_vector(glm::vec<3, uint64_t> i);
};

struct Random32 {
    uint32_t seed;
    uint32_t value;

    Random32(uint32_t init_seed);

    Random32(const Random32& r) = default;
    Random32& operator=(const Random32& r) = default;
    Random32(Random32&& r) = default;
    Random32& operator=(Random32&& r) = default;

    float operator()();
    uint32_t next();
    vec3 unit_vector();

    float operator()(uvec3 i);
    uint32_t hash_i(uvec3 i);
    vec3 unit_vector(uvec3 i);
    vec3 cube_vector(uvec3 i);
};

std::array<vec3, 256> gen_voronoi_vectors();

struct Noise_gen {
    static std::array<int, 256> hash_table;
    static std::array<vec3, 16> perlin_vectors;
    static std::array<vec3, 256> voronoi_vectors;

    static uint8_t hash_with_table(uvec3 i);

    static float voronoi_noise(glm::vec3 position, float period, uint32_t seed);

    static float perlin_noise(glm::vec3 position, float period, uint32_t octaves, uint32_t seed, float persistance = 0.5f);

    static float ridged_perlin_noise(glm::vec3 position, float period, uint32_t octaves, uint32_t seed, float persistance = 0.5f);

    static std::vector<float> perlin_noise(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    static std::vector<float> ridged_perlin_noise(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    static std::vector<float> perlin_noise_normalized(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    static std::vector<float> ridged_perlin_noise_normalized(vec3 pos, float period, uint32_t octaves, uint32_t seed, ivec3 size, float diff, float persistance = 0.5f);
    
    void generate_noise(glm::ivec4 index, float* ptr);
};

extern std::array<vec3, 16> perlin_vectors;