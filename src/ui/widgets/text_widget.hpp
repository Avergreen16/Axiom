#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct text_widget : widget {
    std::function<std::string(std::string)> callback;

    axiom::text text;

    void handle_inputs();
    void mesh();
    void get_y();
    void set_str(std::string str);
    void init();
    
    axiom::capture_data handle_capture();
    
    std::vector<axiom::text*> get_text();

    static ulong insert(std::string str, axiom::text_alignment alg, bool wrap = true, std::function<std::string(std::string)> callback = [](std::string str) {return str;});
};

}