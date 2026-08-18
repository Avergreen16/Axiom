#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <deque>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>
#include <shared_mutex>
#include <iostream>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

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
using ulong = uint64_t;
using byte = uint8_t;

std::ostream& operator<<(std::ostream& stream, const vec2& v);
std::ostream& operator<<(std::ostream& stream, const vec3& v);
std::ostream& operator<<(std::ostream& stream, const vec4& v);

std::ostream& operator<<(std::ostream& stream, const ivec2& v);
std::ostream& operator<<(std::ostream& stream, const ivec3& v);
std::ostream& operator<<(std::ostream& stream, const ivec4& v);

namespace axiom {

double get_time();
double get_absolute_time();
ulong get_timestamp();

vec3 hex_color(uint color);
vec3 hsv_color(float hue, float saturation, float value);

std::u32string convert_string(std::string str);
std::string convert_string(std::u32string str);

std::string get_text_from_file(std::string path);
std::vector<byte> get_bytes_from_file(std::string path);

void write_text_to_file(std::string path, std::string data);
void write_bytes_to_file(std::string path, std::vector<byte> data);

auto get_date_time(ulong timestamp);

std::string get_date_time_string(ulong timestamp);

struct profiler_entry {
    std::string name;
    ulong start;
    ulong end;
};

struct frame {
    ulong start;
    ulong end;

    std::deque<profiler_entry> entries;
};

struct profiler {
    uint32_t num_frames = 2000;

    uint current_depth = 0;
    
    frame current_frame;
    std::deque<frame> frames;

    void insert(std::string name, ulong start, ulong end);

    void start_frame();

    void end_frame();

    void clear();
};

struct profile_scope {
    profiler* prof;
    std::string name;
    ulong start;

    profile_scope(profiler* prof_, std::string name_) {
        start = axiom::get_timestamp();

        prof = prof_;
        name = name_;
    }

    ~profile_scope() {
        ulong end = axiom::get_timestamp();

        prof->insert(name, start, end);
    }
};

struct Thread_pool {
    std::queue<std::unique_ptr<std::function<void()>>> tasks;
    std::shared_mutex task_mutex;
    std::vector<std::thread> threads;

    bool stop = false;

    Thread_pool(int num_threads);

    ~Thread_pool();

    void add_task(std::function<void()> f);
};

struct Time {
    std::chrono::steady_clock::time_point last_time;

    void overwrite();

    Time();

    double get_elapsed_time(bool overwrite = false);
};

extern profiler prof;

#define PROFILE_SCOPE(name) \
    profile_scope profile_scope_##__LINE__(&prof, name)

}