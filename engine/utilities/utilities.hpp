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

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"
#include "glm\gtx\orthonormalize.hpp"

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

struct Profiler_Entry {
    double time;
    std::string name;
};

struct Profiler_Frame {
    std::vector<Profiler_Entry> steps;
};

struct Profiler {
    uint32_t num_frames = 20;
    std::deque<Profiler_Frame> prev_frames;
    Profiler_Frame current_frame;

    double prev_time;
    uint32_t iterations = 0;

    void start() {
        prev_time = get_time();
    }

    void step(std::string name = "") {
        double current_time = get_time();
        double diff = current_time - prev_time;
        prev_time = current_time;

        Profiler_Entry entry;
        entry.name = name;
        entry.time = diff;

        current_frame.steps.push_back(entry);
    }

    void loop() {
        prev_frames.push_back(current_frame);
        current_frame.steps.clear();

        while(prev_frames.size() > num_frames) prev_frames.pop_front();
    }

    void clear() {
        current_frame.steps.clear();
        prev_frames.clear();
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

}