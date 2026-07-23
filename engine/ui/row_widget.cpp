#include <ui/row_widget.hpp>
#include <ui/ui_system.hpp>

#include <GLFW/glfw3.h>

namespace axiom {
    
uint64_t row_widget::insert(bool fill) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    row_widget widget;
    widget.layout_mode = axiom::layout_mode::ROW;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;
    widget.fill = fill;

    widget.max_width = FLT_MAX;
    widget.min_width = 0.0f;

    return ui_system.insert_widget(widget, true);
}

void row_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    columns.resize(children.size(), 0.0f);
    row = 0.0f;

    //
    widget_constraint c;
    c.func = [this]() {
        columns.resize(children.size(), 0.0f);
        column_buffers.resize(children.size() - 1, 0.0f);
        row = 0.0f;
    };
    before.push_back(c);

    // constrain ratios
    c.func = [this, ui_system]() {
        for(int i = 0; i < 16; ++i) {
            // distribute space
            float available_space = size.x;
            float total = 0.0f;

            float prev = 0.0f;
            
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];

                available_space -= c0->size.x;
                if(i != children.size() - 1) {
                    float max_buffer = glm::max(c0->buffer.x, prev);
                    available_space -= max_buffer;
                }
                prev = c0->buffer.z;

                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width) total += c0->weight_width;
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width) c0->size.x = glm::clamp(c0->size.x + available_space * c0->weight_width / total, c0->min_width, c0->max_width);
            }

            // constrain x
            
            float max_x = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];

                if(c0->min_width != c0->max_width) {
                    float x = (c0->size.x - c0->min_width) / (c0->weight_width / total);
                    max_x = glm::max(max_x, x);
                }
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width) {
                    c0->size.x = glm::clamp(c0->min_width + max_x * c0->weight_width / total, c0->min_width, c0->max_width);
                }
            }
        }
    };
    before.push_back(c);

    // distribute y
    c.func = [this, ui_system]() {
        float available_space = size.y;
        
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];

            if(c0->weight_height != 0.0f) {
                c0->size.y = glm::clamp(available_space, c0->min_height, c0->max_height);
            }
        }
    };
    before.push_back(c);

    // column and row derivation
    c.func = [this, ui_system]() {
        float row_target = 0.0f;

        float prev = 0.0f;

        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];

            columns[i] = c0->size.x;

            if(i != children.size() - 1) column_buffers[i] = glm::max(c0->buffer.x, prev);
            prev = c0->buffer.z;

            row_target = glm::max(row_target, c0->size.y);
        
        }

        if(fill) {
            row = size.y;
        } else {
            row = row_target;
        }
    };
    before.push_back(c);

    // position self
    c.func = [this]() {
        vec4 area = {position, size};
        vec2 true_size = vec2(0.0f, row);

        for(int i = 0; i < columns.size(); ++i) {
            true_size.x += columns[i];
            
            if(i != children.size() - 1) true_size.x += column_buffers[i];
        }

        switch(position_mode) {
            case axiom::position_mode::BOTTOM_LEFT: {
                // do nothing
                break;
            }
            case axiom::position_mode::BOTTOM_CENTER: {
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case axiom::position_mode::BOTTOM_RIGHT: {
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case axiom::position_mode::CENTER_LEFT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                break;
            }
            case axiom::position_mode::CENTER: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case axiom::position_mode::CENTER_RIGHT: {
                position.y = position.y + (size.y - true_size.y) * 0.5f;
                position.x = position.x + (size.x - true_size.x);
                break;
            }
            case axiom::position_mode::TOP_LEFT: {
                position.y = position.y + (size.y - true_size.y);
                break;
            }
            case axiom::position_mode::TOP_CENTER: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x) * 0.5f;
                break;
            }
            case axiom::position_mode::TOP_RIGHT: {
                position.y = position.y + (size.y - true_size.y);
                position.x = position.x + (size.x - true_size.x);
                break;
            }
        }
    };
    before.push_back(c);

    // target and weights for x and y
    c.func = [this, ui_system]() {
        float target_min = 0.0f;
        float target_max = 0.0f;
        float target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];

            if(c0->min_width != c0->max_width) {
                target_min += c0->min_width;
                target_max += c0->max_width;
                target_weight += c0->weight_width;
            } else {
                target_min += c0->size.x;
                target_max += c0->size.x;
            }
        }

        min_width = glm::max(min_width, target_min);
        max_width = glm::min(max_width, target_max);
        
        if(weight_width == 2.0f || flag) {
            flag = true;
        }
        weight_width = target_weight;


        //

        target_min = 0.0f;
        target_max = 0.0f;
        target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];

            if(/*c0->min_height != c0->max_height && */c0->weight_height != 0) {
                target_min = glm::max(target_min, c0->min_height);
                target_max = glm::max(target_max, c0->max_height);
                target_weight = glm::max(target_weight, c0->weight_height);
            } else {
                target_min = glm::max(target_min, c0->size.y);
                target_max = glm::max(target_max, c0->size.y);
            }
        }

        min_height = target_min;
        max_height = target_max;
        weight_height = target_weight;

        if(weight_height == 0.0f) size.y = min_height;
    };
    before.push_back(c);

    // positioning
    c.func = [this, ui_system]() {
        float x_pos = position.x;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];

            // y positioning
            c0->position.x = x_pos;
            
            x_pos += columns[i];
            if(i != children.size() - 1) x_pos += column_buffers[i];

            // y positioning
            // x positioning
            switch(c0->position_mode) {
                case axiom::position_mode::BOTTOM_LEFT:
                case axiom::position_mode::BOTTOM_CENTER:
                case axiom::position_mode::BOTTOM_RIGHT: {
                    c0->position.y = position.y;

                    break;
                }
                case axiom::position_mode::CENTER_LEFT:
                case axiom::position_mode::CENTER:
                case axiom::position_mode::CENTER_RIGHT: {
                    c0->position.y = position.y + (row - c0->size.y) * 0.5f;

                    break;
                }

                case axiom::position_mode::TOP_LEFT:
                case axiom::position_mode::TOP_CENTER:
                case axiom::position_mode::TOP_RIGHT: {
                    c0->position.y = position.y + (row - c0->size.y);

                    break;
                }
            }
            
        }
    };
    before.push_back(c);
}

}