#pragma once;
#include <shared_mutex>
#include <queue>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include <thread>
#include <iostream>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm\glm.hpp"
#include "glm\gtx\matrix_transform_2d.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtx\quaternion.hpp"

using namespace glm;

const std::vector<std::string> integers = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "\xC2\x80", "\xC2\x81", "\xC2\x82", "\xC2\x83", "\xC2\x84", "\xC2\x85"};
const std::string integers_letters = "0123456789ABCDEF";

const double hexond_ratio = 86400.0 / 65536.0;

constexpr int PRIME_X = 73856093;
constexpr int PRIME_Y = 19349663;
constexpr int PRIME_Z = 83492791;

double get_time();
double get_absolute_time();

std::u32string convert_string(std::string str);
std::string convert_string(std::u32string str);

struct Hash_coord {
    std::size_t operator()(const ivec2& v) const;
    std::size_t operator()(const ivec3& v) const;
    std::size_t operator()(const ivec4& v) const;

    std::size_t operator()(const vec2& v) const;
    std::size_t operator()(const vec3& v) const;
    std::size_t operator()(const vec4& v) const;
};

struct Octree_cell {
    ivec4 id;
    std::shared_ptr<Octree_cell> parent = nullptr;
    std::vector<std::shared_ptr<Octree_cell>> children;

    bool is_leaf = true;
};

std::vector<std::shared_ptr<Octree_cell>> compute_octree(int power, int min_power, vec3 rel_pos, float split_factor, int max_power = -1);
//void octree_insert_cell(std::vector<std::shared_ptr<Octree_cell>>& octree, ivec4 insert, int power);
std::vector<std::shared_ptr<Octree_cell>> compute_octree_with_neighbors(int power, int min_power, vec3 rel_pos, float split_factor, float split_add = 0.0f, int max_power = -1);
std::vector<ivec4> get_children(ivec4 cell);
std::vector<ivec4> get_parents(ivec4 cell);

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

//

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

//

std::string to_base(int32_t num, int base, bool use_i2 = false);
std::string to_base(int64_t num, int base, bool use_i2 = false);
std::string to_base(float num, int base, int max_float, bool use_i2 = false);
int from_base(std::string num, int base);