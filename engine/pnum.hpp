#pragma once
#include <cstdint>
#include <iostream>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"

using namespace glm;

const uint32_t sector_size = 256;

struct pnum {
    int64_t sector;
    float fraction;

    pnum() = default;
    pnum(pnum&& a) = default;
    pnum& operator=(pnum&& a) = default;
    pnum(const pnum& a) = default;
    pnum& operator=(const pnum& a) = default;

    pnum(float f);
    pnum(double d);
    pnum(int i);
    pnum(long double d);
    pnum(unsigned long long int i);
    pnum(int64_t s, float f);

    operator float() const;
    operator double() const;
    operator int() const;

    void balance();

    pnum operator+(const pnum& a) const;
    pnum operator-(const pnum& a) const;
    pnum operator/(const pnum& a) const;
    pnum operator*(const pnum& a) const;
    pnum operator+(const float& a) const;
    pnum operator-(const float& a) const;
    pnum operator/(const float& a) const;
    pnum operator*(const float& a) const;
    pnum& operator+=(const pnum& a);
    pnum& operator-=(const pnum& a);
    pnum& operator++();
    pnum& operator--();

    bool operator>(const pnum& a) const;
    bool operator<(const pnum& a) const;
    bool operator==(const pnum& a) const;
    bool operator<=(const pnum& a) const;
    bool operator>=(const pnum& a) const;
};

using pvec2 = glm::vec<2, pnum>;
using pvec3 = glm::vec<3, pnum>;

const float frac_epsilon = pow(2, -(23 - floor(log2(float(sector_size)))));
const pnum MAX_PNUM = pnum(0x7FFFFFFFFFFFFFFF, sector_size - frac_epsilon);
const pnum MIN_PNUM = pnum(-0x8000000000000000, 0.0f);

pnum floor(pnum a);

pnum ceil(pnum a);

pvec3 floor(pvec3 a);

pvec3 ceil(pvec3 a);

pnum dot(pvec3 a, pvec3 b);

vec3 normalize(pvec3 a);

pnum min(pnum a, pnum b);

pnum max(pnum a, pnum b);

pvec3 min(pvec3 a, pvec3 b);

pvec3 max(pvec3 a, pvec3 b);

pnum operator "" p(long double d);

pnum operator "" p(unsigned long long int i);

std::ostream& operator<<(std::ostream& i, pnum p);

std::ostream& operator<<(std::ostream& i, pvec3 p);

pvec3 apply_matrix(mat3 m, pvec3 p);