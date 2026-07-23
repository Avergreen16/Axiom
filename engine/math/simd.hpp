#include <array>
#include <vector>
#include <experimental/simd>

namespace stdx = std::experimental;

#include <xsimd/xsimd.hpp>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\orthonormalize.hpp"

#include <variant>
#include <unordered_set>
#include <string>

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;

using batch = xsimd::batch<float, xsimd::default_arch>;
using batch_int = xsimd::batch<int, xsimd::default_arch>;
constexpr std::size_t N = batch::size;

namespace axiom {

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

batch smoothstep(batch x);

simd_vec3 smoothstep(simd_vec3 v);

batch_int hash_coords(batch_int x, batch_int y, batch_int z);

}