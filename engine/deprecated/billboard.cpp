#include "billboard.hpp"

void Billboard_animation::set_animation(uint32_t id) {
    if(current_animation != id) {
        current_animation = id;
        current_time = 0.0f;
    }
}