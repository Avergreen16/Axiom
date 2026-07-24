#include <ui/column_widget.hpp>
#include <ui/ui_system.hpp>

#include <GLFW/glfw3.h>

namespace axiom {

uint64_t column_widget::insert(bool fill) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    column_widget widget;
    widget.layout_mode = axiom::layout_mode::COLUMN;
    widget.buffer = ui_system.input_state.active_buffer;
    widget.position_mode = ui_system.input_state.active_position;
    widget.fill = fill;

    return ui_system.insert_widget(widget, true);
}

void column_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this]() {
        rows.resize(children.size(), 0.0f);
        row_buffers.resize(children.size() - 1, 0.0f);
        column = 0.0f;
    };
    before.push_back(c);

    // constrain ratios
    c.func = [this, ui_system]() {
        for(int i = 0; i < 16; ++i) {
            // distribute space
            float available_space = size.y;
            float total = 0.0f;

            float prev = 0.0f;
            
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

                available_space -= c0->size.y;
                if(i != children.size() - 1) {
                    float max_buffer = glm::max(c0->buffer.y, prev);
                    available_space -= max_buffer;
                }
                prev = c0->buffer.w;

                if(c0->min_height != c0->max_height) total += c0->weight_height;
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];
                if(c0->min_height != c0->max_height) c0->size.y = glm::clamp(c0->size.y + available_space * c0->weight_height / total, c0->min_height, c0->max_height);
            }

            // constrain x
            
            float max_x = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

                if(c0->min_height != c0->max_height) {
                    float x = (c0->size.y - c0->min_height) / (c0->weight_height / total);
                    max_x = glm::max(max_x, x);
                }
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];
                if(c0->min_height != c0->max_height) {
                    c0->size.y = glm::clamp(c0->min_height + max_x * c0->weight_height / total, c0->min_height, c0->max_height);
                } else c0->size.y = c0->min_height;
            }
        }
    };
    before.push_back(c);

    // distribute x
    c.func = [this, ui_system]() {
        float available_space = size.x;
        
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

            if(c0->weight_width != 0.0f) {
                float C = c0->size.x - glm::clamp(available_space, c0->min_width, c0->max_width);
                c0->size.x -= C;
            }
        }
    };
    before.push_back(c);

    // column and row derivation
    c.func = [this, ui_system]() {
        float column_target = 0.0f;

        float prev = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

            float C0 = rows[i] - c0->size.y;
            rows[i] -= C0;

            if(i != children.size() - 1) {
                float max_buffer = glm::max(c0->buffer.y, prev);
                row_buffers[i] = max_buffer;
            }
            prev = c0->buffer.w;

            column_target = glm::max(column_target, c0->size.x);
        }
        
        if(fill) {
            column = size.x;
        } else {
            column = column_target;
        }
    };
    after.push_back(c);

    // target and weights for x and y
    c.func = [this, ui_system]() {
        float target_min = 0.0f;
        float target_max = 0.0f;
        float target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

            if(c0->min_height != c0->max_height) {
                target_min += c0->min_height;
                target_max += c0->max_height;
                target_weight += c0->weight_height;
            } else {
                target_min += c0->size.y;
                target_max += c0->size.y;
                target_weight = glm::max(target_weight, 1.0f);
            }
        }

        min_height = target_min;
        max_height = target_max;
        weight_height = target_weight;

        //

        target_min = 0.0f;
        target_max = 0.0f;
        target_weight = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

            if(c0->min_width != c0->max_width) {
                target_min = glm::max(target_min, c0->min_width);
                target_max = glm::max(target_max, c0->max_width);
                target_weight = glm::max(target_weight, c0->weight_width);
            } else {
                target_min = glm::max(target_min, c0->size.x);
                target_max = glm::max(target_max, c0->size.x);
                target_weight = glm::max(target_weight, 1.0f);
            }
        }

        min_width = target_min;
        max_width = target_max;
        weight_width = target_weight;
    };
    after.push_back(c);

    // position self
    c.func = [this]() {
        vec4 area = {position, size};
        vec2 true_size = vec2(column, 0.0f);

        for(int i = 0; i < rows.size(); ++i) {
            true_size.y += rows[i];
            if(i != rows.size() - 1) true_size.y += row_buffers[i];
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

        size = true_size;
    };
    after.push_back(c);

    // positioning
    c.func = [this, ui_system]() {
        float y_pos = position.y;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[(int)children.size() - i - 1]];

            // y positioning
            c0->position.y = y_pos;
            
            y_pos += rows[i];
            if(i != children.size() - 1) y_pos += row_buffers[i];

            // x positioning
            switch(c0->position_mode) {
                case axiom::position_mode::BOTTOM_LEFT:
                case axiom::position_mode::CENTER_LEFT:
                case axiom::position_mode::TOP_LEFT: {
                    c0->position.x = position.x;

                    break;
                }
                case axiom::position_mode::BOTTOM_CENTER:
                case axiom::position_mode::CENTER:
                case axiom::position_mode::TOP_CENTER: {
                    c0->position.x = position.x + (column - c0->size.x) * 0.5f;

                    break;
                }
                case axiom::position_mode::BOTTOM_RIGHT:
                case axiom::position_mode::CENTER_RIGHT:
                case axiom::position_mode::TOP_RIGHT: {
                    c0->position.x = position.x + (column - c0->size.x);

                    break;
                }
            }
        }
    };
    after.push_back(c);
}

}