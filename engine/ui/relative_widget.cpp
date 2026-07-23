#include <ui/relative_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void relative_widget::init() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    //
    widget_constraint c;
    c.func = [this]() {
        callback(*this);
    };
    after.push_back(c);
}

uint64_t relative_widget::insert(std::function<void(relative_widget&)> callback = [](relative_widget& self) {}) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    relative_widget widget;
    widget.callback = callback;
    widget.weight_width = 0.0f;
    widget.weight_height = 0.0f;

    return ui_system.insert_widget(widget, true);
}

}