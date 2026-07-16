#include "utility.hpp"
#include "random.hpp"

#include <unordered_set>

using namespace std::chrono;

double get_time() {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1000000;
}

double get_absolute_time() {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000000;
}


std::u32string convert_string(std::string str) {
    std::u32string ret;

    uint32_t index = 0;
    while(true) {
        if(index >= str.size()) break;

        uint8_t c = static_cast<uint8_t>(str[index]);

        uint32_t character = 0;

        if((c & 0x80) == 0x00) {
            uint32_t cc = c & 0x7F;
            character |= cc;
        } else if((c & 0xE0) == 0xC0) {
            uint32_t cc = c & 0x1F;
            character |= cc << 6;

            ++index;
            c = static_cast<uint8_t>(str[index]);
            cc = c & 0x3F;
            character |= cc;
        } else if((c & 0xF0) == 0xE0) {
            uint32_t cc = c & 0xF;
            character |= cc << 12;

            ++index;
            c = static_cast<uint8_t>(str[index]);
            cc = c & 0x3F;
            character |= cc << 6;
            
            ++index;
            c = static_cast<uint8_t>(str[index]);
            cc = c & 0x3F;
            character |= cc;
        } else if((c & 0xF8) == 0xF0) {
            uint32_t cc = c & 0x7;
            character |= cc << 18;

            ++index;
            c = static_cast<uint8_t>(str[index]);
            cc = c & 0x3F;
            character |= cc << 12;
            
            ++index;
            c = static_cast<uint8_t>(str[index]);
            cc = c & 0x3F;
            character |= cc << 6;
            
            ++index;
            c = static_cast<uint8_t>(str[index]);
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
            uint8_t a = 0x80 | (0x3F & c);
            uint8_t b = 0x80 | (0x3F & (c >> 6));
            uint8_t c = 0x80 | (0x3F & (c >> 12));
            uint8_t d = 0x80 | (0x7 & (c >> 18));

            encoding += d;
            encoding += c;
            encoding += b;
            encoding += a;
        } else if(c >= 0x800) {
            uint8_t a = 0x80 | (0x3F & c);
            uint8_t b = 0x80 | (0x3F & (c >> 6));
            uint8_t c = 0xE0 | (0xF & (c >> 12));

            encoding += c;
            encoding += b;
            encoding += a;
        } else if(c >= 0x7F) {
            uint8_t a = 0x80 | (0x3F & c);
            uint8_t b = 0xC0 | (0x1F & (c >> 6));

            encoding += b;
            encoding += a;
        } else {
            encoding += uint8_t(c);
        }

        ret += encoding;
        
        ++index;
    }

    return ret;
}

std::size_t Hash_coord::operator()(const ivec2& v) const {
    int hash = v.x * PRIME_X;
    hash ^= v.y * PRIME_Y;

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t Hash_coord::operator()(const ivec3& v) const {
    int hash = v.x * PRIME_X;
    hash ^= v.y * PRIME_Y;
    hash ^= v.z * PRIME_Z;

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t Hash_coord::operator()(const ivec4& v) const {
    int hash = v.x * PRIME_X;
    hash ^= v.y * PRIME_Y;
    hash ^= v.z * PRIME_Z;
    hash ^= v.y * PRIME_X;

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}


std::size_t Hash_coord::operator()(const vec2& v) const {
    int hash = v.x * PRIME_X;
    hash ^= int(v.y * PRIME_Y);

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t Hash_coord::operator()(const vec3& v) const {
    int hash = v.x * PRIME_X;
    hash ^= int(v.y * PRIME_Y);
    hash ^= int(v.z * PRIME_Z);

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::size_t Hash_coord::operator()(const vec4& v) const {
    int hash = v.x * PRIME_X;
    hash ^= int(v.y * PRIME_Y);
    hash ^= int(v.z * PRIME_Z);
    hash ^= int(v.y * PRIME_X);

    hash ^= (hash >> 13);
    hash = hash * 60493 + 19990303;
    return hash;
}

std::vector<std::shared_ptr<Octree_cell>> compute_octree(int power, int min_power, vec3 rel_pos, float split_factor, int max_power) {
    if(max_power == -1) max_power = power;

    std::vector<std::shared_ptr<Octree_cell>> ret;
    std::vector<std::shared_ptr<Octree_cell>> current_cells = {std::make_shared<Octree_cell>(Octree_cell({0, 0, 0, power}))};
    std::vector<std::shared_ptr<Octree_cell>> new_cells;

    float offset = pow(2.0f, power) * 0.5f;

    while(true) {
        for(std::shared_ptr<Octree_cell>& cell : current_cells) {
            int p2 = cell->id.w;
            if(p2 == min_power) continue;

            float side_length = pow(2.0f, p2);
            vec3 center = vec3(cell->id.xyz()) * side_length - offset + side_length * 0.5f;

            vec3 rel = center - rel_pos;
            float max_coord = max(max(abs(rel.x), abs(rel.y)), abs(rel.z));
            rel /= max_coord;

            if(length(rel) * max_coord <= side_length * split_factor || p2 > max_power) {
                ivec4 base = ivec4(cell->id.xyz() * 2, cell->id.w - 1);

                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base, cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 1, 0), cell)));

                cell->is_leaf = false;

                new_cells.insert(new_cells.end(), cell->children.begin(), cell->children.end());
            }
        }

        ret.insert(ret.end(), current_cells.begin(), current_cells.end());

        if(new_cells.size() == 0) break;
        current_cells = new_cells;
        new_cells.clear();
    }

    return ret;
}

std::vector<ivec4> get_children(ivec4 cell) {
    ivec4 base = ivec4(cell.xyz() * 2, cell.w - 1);

    return {
        base,
        base + ivec4(1, 0, 0, 0),
        base + ivec4(0, 1, 0, 0),
        base + ivec4(1, 1, 0, 0),
        base + ivec4(0, 0, 1, 0),
        base + ivec4(1, 0, 1, 0),
        base + ivec4(0, 1, 1, 0),
        base + ivec4(1, 1, 1, 0),
    };
}


std::vector<ivec4> get_parents(ivec4 cell) {
    std::vector<ivec4> ret;
    ivec4 base = cell;

    while(!(base.x == 0 && base.y == 0 && base.z == 0)) {
        base.x /= 2;
        base.y /= 2;
        base.z /= 2;
        base.w += 1;
        ret.push_back(base);
    }

    return ret;
}

void octree_insert_cell(std::vector<std::shared_ptr<Octree_cell>>& octree, std::unordered_set<ivec4, Hash_coord>& set, ivec4 insert, int power) {
    auto split_leaf = [&](std::shared_ptr<Octree_cell> cell) {
        ivec4 base = ivec4(cell->id.xyz() * 2, cell->id.w - 1);

        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base, cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 0, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 0, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 0, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 0, 1, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 1, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 1, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 1, 0), cell)));
        
        set.insert(base);
        set.insert(base + ivec4(1, 0, 0, 0));
        set.insert(base + ivec4(0, 1, 0, 0));
        set.insert(base + ivec4(1, 1, 0, 0));
        set.insert(base + ivec4(0, 0, 1, 0));
        set.insert(base + ivec4(1, 0, 1, 0));
        set.insert(base + ivec4(0, 1, 1, 0));
        set.insert(base + ivec4(1, 1, 1, 0));

        octree.insert(octree.end(), cell->children.begin(), cell->children.end());

        cell->is_leaf = false;
    };

    std::vector<ivec4> path(power - insert.w + 1);
    
    ivec4 c = insert;
    for(int i = insert.w; i <= power; ++i) {
        path[power - i] = c;
        ivec3 cc = c.xyz() / 2;
        c.x = cc.x;
        c.y = cc.y;
        c.z = cc.z;
        c.w = i;
    }

    std::shared_ptr<Octree_cell> current_cell = octree[0];
    for(int i = power - 1; i > insert.w; --i) {
        if(current_cell->is_leaf) split_leaf(current_cell);

        ivec4 c = path[power - i];
        ivec3 base = ivec3(current_cell->id.xyz() * 2);
        base = c.xyz() - base;

        uint32_t j = base.x + base.y * 2 + base.z * 4;

        current_cell = current_cell->children[j];
    }
}

std::vector<std::shared_ptr<Octree_cell>> compute_octree_with_neighbors(int power, int min_power, vec3 rel_pos, float split_factor, float split_add, int max_power) {
    if(max_power == -1) max_power = power;

    std::unordered_set<ivec4, Hash_coord> set;

    std::vector<std::shared_ptr<Octree_cell>> octree;
    std::vector<std::shared_ptr<Octree_cell>> current_cells = {std::make_shared<Octree_cell>(Octree_cell({0, 0, 0, power}))};
    std::vector<std::shared_ptr<Octree_cell>> new_cells;

    float offset = pow(2.0f, power) * 0.5f;

    while(true) {
        for(std::shared_ptr<Octree_cell>& cell : current_cells) {
            int p2 = cell->id.w;
            if(p2 == min_power) continue;

            float side_length = pow(2.0f, p2);
            vec3 center = vec3(cell->id.xyz()) * side_length - offset + side_length * 0.5f;

            vec3 rel = center - rel_pos;
            float max_coord = max(max(abs(rel.x), abs(rel.y)), abs(rel.z));
            rel /= max_coord;

            if(length(rel) * max_coord <= side_length * split_factor + split_add || p2 > max_power) {
                ivec4 base = ivec4(cell->id.xyz() * 2, cell->id.w - 1);

                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base, cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 1, 0), cell)));
                
                set.insert(base);
                set.insert(base + ivec4(1, 0, 0, 0));
                set.insert(base + ivec4(0, 1, 0, 0));
                set.insert(base + ivec4(1, 1, 0, 0));
                set.insert(base + ivec4(0, 0, 1, 0));
                set.insert(base + ivec4(1, 0, 1, 0));
                set.insert(base + ivec4(0, 1, 1, 0));
                set.insert(base + ivec4(1, 1, 1, 0));

                cell->is_leaf = false;

                new_cells.insert(new_cells.end(), cell->children.begin(), cell->children.end());
            }
        }

        octree.insert(octree.end(), current_cells.begin(), current_cells.end());

        if(new_cells.size() == 0) break;
        current_cells = new_cells;
        new_cells.clear();
    }

    std::vector<ivec4> neighbors = {
        ivec4(-1, 0, 0, 0),
        ivec4(1, 0, 0, 0),
        ivec4(0, -1, 0, 0),
        ivec4(0, 1, 0, 0),
        ivec4(0, 0, -1, 0),
        ivec4(0, 0, 1, 0),
    };

    /*
    std::vector<ivec4> neighbors = {
        ivec4(1, 1, 1, 0),
        ivec4(0, 1, 1, 0),
        ivec4(-1, 1, 1, 0),
        ivec4(1, 0, 1, 0),
        ivec4(0, 0, 1, 0),
        ivec4(-1, 0, 1, 0),
        ivec4(1, -1, 1, 0),
        ivec4(0, -1, 1, 0),
        ivec4(-1, -1, 1, 0),
        
        ivec4(1, 1, 0, 0),
        ivec4(0, 1, 0, 0),
        ivec4(-1, 1, 0, 0),
        ivec4(1, 0, 0, 0),
        //ivec4(0, 0, 0, 0), self
        ivec4(-1, 0, 0, 0),
        ivec4(1, -1, 0, 0),
        ivec4(0, -1, 0, 0),
        ivec4(-1, -1, 0, 0),
        
        ivec4(1, 1, -1, 0),
        ivec4(0, 1, -1, 0),
        ivec4(-1, 1, -1, 0),
        ivec4(1, 0, -1, 0),
        ivec4(0, 0, -1, 0),
        ivec4(-1, 0, -1, 0),
        ivec4(1, -1, -1, 0),
        ivec4(0, -1, -1, 0),
        ivec4(-1, -1, -1, 0),
    };
    */

    for(int i = 0; i < octree.size(); ++i) {
        std::shared_ptr<Octree_cell> cell = octree[i];

        if(cell->id.w < power && cell->is_leaf) {
            uint32_t m = uint32_t(1) << (power - cell->id.w);
            
            for(ivec4 c : neighbors) {
                ivec4 n = cell->id + c;

                if(!(n.x < 0 || n.y < 0 || n.z < 0 || n.x >= m || n.y >= m || n.z >= m)) {
                    ivec4 n2 = ivec4(n.xyz() / 2, n.w + 1);

                    if(!set.contains(n2)) octree_insert_cell(octree, set, n2, power);
                }
            }
        }
    }

    return octree;
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
    last_time = steady_clock::now();
}

Time::Time() {
    overwrite();
}

double Time::get_elapsed_time(bool overwrite) {
    steady_clock::time_point current_time = steady_clock::now();
    steady_clock::duration duration = current_time - last_time;

    if(overwrite) {
        last_time = current_time;
    }

    return double(duration.count()) * steady_clock::period::num / steady_clock::period::den;
}


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
        integer = integers[clamp(i % base, 0, 15)] + integer;
        i /= base;
    }
    
    if(integer.size() == 0) integer = "0";

    int f_count = 0;
    while(f != 0.0) {
        f *= base;
        floating += integers[clamp((int)floor(f), 0, 15)];
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

Profiler profiler;
Profiler profiler3;