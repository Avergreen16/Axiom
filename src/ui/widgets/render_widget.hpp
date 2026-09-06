#pragma once

#include <ui/widget_base.hpp>

namespace axiom {

struct render_target;

struct render_widget : widget {
    axiom::render_target* target;
    std::function<void(axiom::render_widget*)> callback = [](axiom::render_widget*) {};
    uint texture;

    void mesh();
    void init();
    void handle_inputs();
    axiom::capture_data handle_capture();

    static ulong insert(axiom::render_target* target, uint texture, std::function<void(axiom::render_widget*)> callback = [](axiom::render_widget*) {});
};

}