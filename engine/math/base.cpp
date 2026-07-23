#include "base.hpp"

namespace axiom {
    
std::string to_base(int32_t num, int base, bool use_i2) {
    std::string ret;

    bool neg = (num < 0);

    num = abs(num);
    while(num > 0) {
        if(use_i2) ret = integers_letters[num % base] + ret;
        else ret = integers[num % base] + ret;
        num /= base;
    }

    if(ret.size() == 0) ret = "0";
    
    if(neg) ret = "-" + ret;

    return ret;
}

std::string to_base(int64_t num, int base, bool use_i2) {
    std::string ret;

    bool neg = (num < 0);

    num = abs(num);
    while(num > 0) {
        if(use_i2) ret = integers_letters[num % base] + ret;
        else ret = integers[num % base] + ret;
        num /= base;
    }

    if(ret.size() == 0) ret = "0";
    if(neg) ret = "-" + ret;

    return ret;
}

std::string to_base(float num, int base, int max_float, bool use_i2) {
    if(std::isinf(num)) {
        return "INFINITY";
    }

    if(std::isnan(num)) {
        return "NAN";
    }

    std::string integer;
    std::string floating;

    bool neg = (num < 0);
    num = abs(num);

    int i = floor(num);
    float f = num - i;

    while(i > 0) {
        integer = integers[glm::clamp(i % base, 0, 15)] + integer;
        i /= base;
    }
    
    if(integer.size() == 0) integer = "0";

    int f_count = 0;
    while(f != 0.0) {
        f *= base;
        floating += integers[glm::clamp((int)floor(f), 0, 15)];
        f -= floor(f);

        ++f_count;
        if(f_count >= max_float) break;
    }

    if(floating.size() == 0) floating = "0";

    std::string ret;
    if(neg) ret = "-";

    ret += integer + '.' + floating;
    

    return ret;
}

int from_base(std::string num, int base) {
    int ret = 0;

    bool neg = false;
    if(num[0] == '-') {
        neg = true;
        num.erase(0);
    }

    for(uint8_t c : num) {
        ret *= base;
        if(c >= '0' && c <= '9') ret += c - '0';
        else if(c >= 0x80 && c <= 0x85) ret += c - 0x76;
        else if(c >= 'A' && c <= 'F') ret += c - 'A' + 0xA;
    }

    if(neg) ret *= -1;

    return ret;
}

}