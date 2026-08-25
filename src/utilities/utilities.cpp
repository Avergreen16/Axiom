#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>

#include "utilities.hpp"


std::ostream& operator<<(std::ostream& stream, const vec2& v) {
    return stream << v.x << " " << v.y;
}

std::ostream& operator<<(std::ostream& stream, const vec3& v) {
    return stream << v.x << " " << v.y << " " << v.z;
}

std::ostream& operator<<(std::ostream& stream, const vec4& v) {
    return stream << v.x << " " << v.y << " " << v.z << " " << v.w;
}

std::ostream& operator<<(std::ostream& stream, const ivec2& v) {
    return stream << v.x << " " << v.y;
}

std::ostream& operator<<(std::ostream& stream, const ivec3& v) {
    return stream << v.x << " " << v.y << " " << v.z;
}

std::ostream& operator<<(std::ostream& stream, const ivec4& v) {
    return stream << v.x << " " << v.y << " " << v.z << " " << v.w;
}

namespace axiom {

double get_time() {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1000000;
}

double get_absolute_time() {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000000;
}

ulong get_timestamp() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

auto get_date_time(ulong timestamp) {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::time_point(std::chrono::microseconds(timestamp));
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm local = *std::localtime(&t);
    return local;
}

//

void profiler::insert(std::string name, ulong start, ulong end) {
    profiler_entry entry;
    entry.name = name;
    entry.start = start;
    entry.end = end;

    current_frame.entries.push_back(entry);
}

void profiler::start_frame() {
    frame current;
    current.start = axiom::get_timestamp();
    
    current_frame = current;
}

void profiler::end_frame() {
    current_frame.end = axiom::get_timestamp();
    frames.push_back(current_frame);

    while(frames.size() > num_frames) frames.pop_front();
}

void profiler::clear() {
    frames.clear();
}

/*
std::string m;
switch(month) {
    case 1: {
        m = "January";
        break;
    }
    case 2: {
        m = "February";
        break;
    }
    case 3: {
        m = "March";
        break;
    }
    case 4: {
        m = "April";
        break;
    }
    case 5: {
        m = "May";
        break;
    }
    case 6: {
        m = "June";
        break;
    }
    case 7: {
        m = "July";
        break;
    }
    case 8: {
        m = "August";
        break;
    }
    case 9: {
        m = "September";
        break;
    }
    case 10: {
        m = "October";
        break;
    }
    case 11: {
        m = "November";
        break;
    }
    case 12: {
        m = "December";
        break;
    }
}
*/

std::string get_date_time_string(ulong timestamp) {
    auto time_data = get_date_time(timestamp);
    std::string date;

    uint month = time_data.tm_mon + 1;
    
    date += std::to_string(month) + "/" + std::to_string(time_data.tm_mday) + "/" + std::to_string(time_data.tm_year + 1900)  + " ";

    std::string time;
    int hour = time_data.tm_hour;
    if(hour > 11) {
        time = "PM";
        hour -= 12;
    } else time = "AM";
    if(hour == 0) hour = 12;
    std::string minute = std::to_string(time_data.tm_min);
    if(minute.length() == 1) minute = "0" + minute;
    time = std::to_string(hour) + ":" + minute + " " + time;

    date += time;

    return date;
}

vec3 hex_color(uint color) {
    vec3 col = vec3((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    col /= 0xFF;

    return col;
}

vec3 hsv_color(float hue, float saturation, float value) {
    vec3 color;

    hue = glm::fract(hue / 6.0f) * 6.0f;
    float f = glm::fract(hue);

    if(hue < 1) {
        color = vec3(1.0, f, 0.0);
    } else if(hue < 2) {
        color = vec3(1.0 - f, 1.0, 0.0);
    } else if(hue < 3) {
        color = vec3(0.0, 1.0, f);
    } else if(hue < 4) {
        color = vec3(0.0, 1.0 - f, 1.0);
    } else if(hue < 5) {
        color = vec3(f, 0.0, 1.0);
    } else if(hue < 6) {
        color = vec3(1.0, 0.0, 1.0 - f);
    }

    color = color * saturation + (1.0f - saturation);
    color *= value;

    return color;
}

std::u32string convert_string(std::string str) {
    std::u32string ret;

    uint32_t index = 0;
    while(true) {
        if(index >= str.size()) break;

        byte c = static_cast<byte>(str[index]);

        uint32_t character = 0;

        if((c & 0x80) == 0x00) {
            uint32_t cc = c & 0x7F;
            character |= cc;
        } else if((c & 0xE0) == 0xC0) {
            uint32_t cc = c & 0x1F;
            character |= cc << 6;

            ++index;
            c = static_cast<byte>(str[index]);
            cc = c & 0x3F;
            character |= cc;
        } else if((c & 0xF0) == 0xE0) {
            uint32_t cc = c & 0xF;
            character |= cc << 12;

            ++index;
            c = static_cast<byte>(str[index]);
            cc = c & 0x3F;
            character |= cc << 6;
            
            ++index;
            c = static_cast<byte>(str[index]);
            cc = c & 0x3F;
            character |= cc;
        } else if((c & 0xF8) == 0xF0) {
            uint32_t cc = c & 0x7;
            character |= cc << 18;

            ++index;
            c = static_cast<byte>(str[index]);
            cc = c & 0x3F;
            character |= cc << 12;
            
            ++index;
            c = static_cast<byte>(str[index]);
            cc = c & 0x3F;
            character |= cc << 6;
            
            ++index;
            c = static_cast<byte>(str[index]);
            cc = c & 0x3F;
            character |= cc;
        }

        ret += character;
        
        ++index;
    }

    return ret;
}

std::string convert_string(std::u32string str) {
    std::string ret;

    uint32_t index = 0;
    while(true) {
        if(index >= str.size()) break;
        
        uint32_t c = str[index];
        std::string encoding;

        if(c >= 0x10000) {
            byte a = 0x80 | (0x3F & c);
            byte b = 0x80 | (0x3F & (c >> 6));
            byte c = 0x80 | (0x3F & (c >> 12));
            byte d = 0x80 | (0x7 & (c >> 18));

            encoding += d;
            encoding += c;
            encoding += b;
            encoding += a;
        } else if(c >= 0x800) {
            byte a = 0x80 | (0x3F & c);
            byte b = 0x80 | (0x3F & (c >> 6));
            byte c = 0xE0 | (0xF & (c >> 12));

            encoding += c;
            encoding += b;
            encoding += a;
        } else if(c >= 0x7F) {
            byte a = 0x80 | (0x3F & c);
            byte b = 0xC0 | (0x1F & (c >> 6));

            encoding += b;
            encoding += a;
        } else {
            encoding += byte(c);
        }

        ret += encoding;
        
        ++index;
    }

    return ret;
}

std::string get_text_from_file(std::string path) {
    std::ifstream file;
    file.open(path);

    if(file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();

        file.close();
        return ss.str();
    } else {
        std::cout << "failed to open file " << path << std::endl;
        file.close();

        return "";
    }
}

std::vector<byte> get_bytes_from_file(std::string path) {
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);

    if(file.is_open()) {
        std::vector<byte> ret;

        byte byte;
        while(file.read((char*)&byte, 1)) {
            ret.push_back(byte);
        }

        file.close();
        return ret;
    } else {
        std::cout << "failed to open file " << path << std::endl;
        file.close();

        return {};
    }
}

void write_text_to_file(std::string path, std::string data) {
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::trunc);

    file.write(data.data(), data.size());
    
    file.close();
}

void write_bytes_to_file(std::string path, std::vector<byte> data) {
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::trunc | std::ios::binary);

    file.write((const char*)data.data(), data.size());

    file.close();
}


Thread_pool::Thread_pool(int num_threads) {
    for(int i = 0; i < num_threads; ++i) {
        threads.emplace_back(std::thread([this, i]() {
            while(!stop) {
                std::function<void()> task;

                while(!tasks.size()) {
                    if(stop) goto stop_o;
                    std::this_thread::yield();
                }

                task_mutex.lock_shared();
                if(tasks.size()) {
                    task_mutex.unlock_shared();
                    task_mutex.lock();
                    if(tasks.size()) {
                        task = std::move(*tasks.front().get());
                        tasks.pop();
                        task_mutex.unlock();

                        task();
                    } else task_mutex.unlock();
                } else task_mutex.unlock_shared();

                stop_o:
            }
        }));
    }
}

Thread_pool::~Thread_pool() {
    stop = true;

    for(std::thread& t : threads) {
        t.join();
    }
}

void Thread_pool::add_task(std::function<void()> f) {
    task_mutex.lock();
    tasks.emplace(std::make_unique<std::function<void()>>(std::move(f)));
    task_mutex.unlock();
}

void Time::overwrite() {
    last_time = std::chrono::steady_clock::now();
}

Time::Time() {
    overwrite();
}

double Time::get_elapsed_time(bool overwrite) {
    std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration duration = current_time - last_time;

    if(overwrite) {
        last_time = current_time;
    }

    return double(duration.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
}

profiler prof;

}