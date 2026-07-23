#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct render_target;

struct relative_widget : widget {
    std::function<void(relative_widget&)> callback;
    
    void init();
    
    static uint64_t insert(std::function<void(relative_widget&)> callback = [](relative_widget& self) {});
};

}