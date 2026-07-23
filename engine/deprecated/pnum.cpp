#include "pnum.hpp";

void pnum::balance() {
    if(fraction < 0 || fraction >= sector_size) {
        int64_t change = glm::floor(fraction / sector_size);
        fraction -= change * sector_size;
        sector += change;
    }
}

pnum pnum::operator+(const pnum& a) const {
    pnum r;
    r.sector = sector + a.sector;
    r.fraction = fraction + a.fraction;
    r.balance();

    return r;
}

pnum pnum::operator-(const pnum& a) const {
    pnum r;
    r.sector = sector - a.sector;
    r.fraction = fraction - a.fraction;
    r.balance();

    return r;
}

pnum pnum::operator/(const pnum& a) const {
    double d0 = *this;
    double da = a;

    float r = pnum(d0 / da);

    return r;
}

pnum pnum::operator*(const pnum& a) const {
    double d0 = *this;
    double da = a;

    pnum r = pnum(d0 * da);

    return r;
}

pnum pnum::operator+(const float& a) const {
    pnum r;
    r.sector = sector;
    r.fraction = fraction + a;
    r.balance();

    return r;
}

pnum pnum::operator-(const float& a) const {
    pnum r;
    r.sector = sector;
    r.fraction = fraction - a;
    r.balance();

    return r;
}

pnum pnum::operator/(const float& a) const {
    pnum r = *this;

    int64_t sign = (sector < 0) ? -1 : 1;
    uint64_t absolute_value = abs(r.sector);
    
    double frac = 0;
    int64_t sec = 0;

    for(int i = 0; i < 3; ++i) {
        uint64_t s = absolute_value & (uint64_t(0xFFFFFF) << (i * 24));
        double s1 = double(int64_t(s) * sign) / a;

        int64_t s1_sec = glm::floor(s1);
        double s1_frac = (s1 - glm::floor(s1)) * 256.0;

        frac += s1_frac;
        sec += s1_sec;
    }

    r.fraction = frac + r.fraction / a;
    r.sector = sec;

    r.balance();

    return r;
}

/*pnum pnum::operator*(const float& a) const {
    pnum r = *this;

    int64_t sign = (sector < 0) ? -1 : 1;
    uint64_t absolute_value = abs(r.sector);
    
    double frac = 0;
    int64_t sec = 0;

    int64_t s = sector * floor(a);
    float sf = sector * fract(a);
    float sff = fract(sf) * sector_size;
    s += floor(sf);

    float n = floor(fraction) * a;
    float f = fract(fraction) * a;


    r.fraction = n + f + sff;
    r.sector = s;

    r.balance();

    return r;
}*/

pnum pnum::operator*(const float& a) const {
    pnum r = *this;

    
    float frac = 0;
    int64_t sec = 0;

    float floor_a = floor(a);
    float fract_a = a - floor_a;

    sec = int64_t(floor_a) * sector;
    frac += sector * fract_a;
    
    int64_t floor_b = floor(frac);
    sec += floor_b;
    frac -= floor_b;
    frac *= sector_size;
    frac += fraction * a;

    r.fraction = frac;
    r.sector = sec;

    r.balance();

    return r;
}

pnum& pnum::operator+=(const pnum& a) {
    *this = operator+(a);

    return *this;
}

pnum& pnum::operator-=(const pnum& a) {
    *this = operator-(a);

    return *this;
}

pnum& pnum::operator++() {
    ++fraction;
    balance();

    return *this;
}

pnum& pnum::operator--() {
    --fraction;
    balance();

    return *this;
}

bool pnum::operator>(const pnum& a) const {
    if(sector == a.sector) {
        return fraction > a.fraction;
    } else return sector > a.sector;
}

bool pnum::operator<(const pnum& a) const {
    if(sector == a.sector) {
        return fraction < a.fraction;
    } else return sector < a.sector;
}

bool pnum::operator>=(const pnum& a) const {
    return operator>(a) || operator==(a);
}

bool pnum::operator<=(const pnum& a) const {
    return operator<(a) || operator==(a);
}

bool pnum::operator==(const pnum& a) const {
    return (sector == a.sector && fraction == a.fraction);
}

pnum::pnum(int64_t s, float f) {
    sector = s;
    fraction = f;
}

pnum::pnum(float f) {
    sector = glm::floor(f / sector_size);
    fraction = f - (float(sector) * sector_size);
}

pnum::pnum(double d) {
    sector = glm::floor(d / sector_size);
    fraction = d - (double(sector) * sector_size);
}

pnum::pnum(int i) {
    sector = i / sector_size;
    fraction = i - (sector * sector_size);
}

pnum::pnum(long double d) {
    sector = glm::floor(d / sector_size);
    fraction = d - (sector * sector_size);
}

pnum::pnum(unsigned long long int i) {
    sector = i / sector_size;
    fraction = i - (sector * sector_size);
}

pnum::operator float() const {
    float d = float(sector) * sector_size;
    d += fraction;
    return d;
}

pnum::operator double() const {
    double d = double(sector) * sector_size;
    d += fraction;
    return d;
}

pnum::operator int() const {
    int i = sector * sector_size + glm::floor(fraction);
}

pnum floor(pnum a) {
    a.fraction = floor(a.fraction);

    return a;
}

pnum ceil(pnum a) {
    a.fraction = ceil(a.fraction);
    a.balance();

    return a;
}

pvec3 floor(pvec3 a) {
    return pvec3(floor(a.x), floor(a.y), floor(a.z));
}

pvec3 ceil(pvec3 a) {
    return pvec3(ceil(a.x), ceil(a.y), ceil(a.z));
}

pnum dot(pvec3 a, pvec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

pnum min(pnum a, pnum b) {
    if(a < b) return a;
    return b;
}

pnum max(pnum a, pnum b) {
    if(a > b) return a;
    return b;
}

pvec3 min(pvec3 a, pvec3 b) {
    pvec3 r;

    for(int i = 0; i < 3; ++i) {
        r[i] = min(a[i], b[i]);
    }

    return r;
}

pvec3 max(pvec3 a, pvec3 b) {
    pvec3 r;
    
    for(int i = 0; i < 3; ++i) {
        r[i] = max(a[i], b[i]);
    }

    return r;
}

vec3 normalize(pvec3 a) {
    dvec3 v = a;
    return normalize(v);
}

pnum operator "" p(long double d) {
    return pnum(d);
}

pnum operator "" p(unsigned long long int i) {
    return pnum(i);
}

std::ostream& operator<<(std::ostream& i, pnum p) {
    i << p.sector << " " << p.fraction;
    return i;
}

std::ostream& operator<<(std::ostream& i, pvec3 p) {
    i << p.x << " " << p.y << " " << p.z;
    return i;
}

pvec3 apply_matrix(mat3 m, pvec3 p) {
    vec3 x = m[0];
    vec3 y = m[1];
    vec3 z = m[2];

    pvec3 a;
    a.x = p.x * x.x + p.y * y.x + p.z * z.x;
    a.y = p.x * x.y + p.y * y.y + p.z * z.y;
    a.z = p.x * x.z + p.y * y.z + p.z * z.z;

    return a;
}