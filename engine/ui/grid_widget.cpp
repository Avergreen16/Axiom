#include <numeric>

#include <ui/grid_widget.hpp>
#include <ui/ui_system.hpp>

#include <GLFW/glfw3.h>

namespace axiom {

ulong grid_widget::insert(uint num_columns) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    grid_widget widget;
    widget.layout_mode = axiom::layout_mode::GRID;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    widget.num_columns = num_columns;
 
    return ui_system.insert_widget(widget, true);
}

void grid_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();
    //
    widget_constraint c;
    c.func = [this]() {
        int num_rows = ceil(float(children.size()) / num_columns);

        rows.resize(num_rows, 0.0f);
        row_buffers.resize(num_rows - 1, buffer.x);
        columns.resize(num_columns, 0.0f);
        column_buffers.resize(num_columns - 1, buffer.y);
    };
    before.push_back(c);

    // constrain ratios
    c.func = [this, ui_system]() {
        for(int i = 0; i < 16; ++i) {
            // distribute space
            float available_space = size.x;

            std::vector<float> total(num_columns, 0.0f);
            std::vector<float> space(num_columns, 0.0f);
            //float total = 0.0f;

            float prev = 0.0f;
            
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];

                

                int column = i % num_columns;

                if(column != num_columns - 1) {
                    space[column] = glm::max(space[column], c0->size.x + buffer.x);
                } else {
                    space[column] = glm::max(space[column], c0->size.x);
                }

                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width) total[column] = glm::max(total[column], c0->weight_width);
            }

            float total_accum = 0.0f;
            for(float t : total) total_accum += t;
            for(float f : space) available_space -= f;
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width) c0->size.x = glm::clamp(c0->size.x + available_space * c0->weight_width / total_accum, c0->min_width, c0->max_width);
            }

            // constrain x
            
            float max_x = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];

                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width && total_accum) {
                    float x = (c0->size.x - c0->min_width) / (c0->weight_width / total_accum);
                    max_x = glm::max(max_x, x);
                }
            }
                
            for(int i = 0; i < children.size(); ++i) {
                auto& c0 = ui_system->widgets[children[i]];
                if(c0->min_width != c0->max_width && c0->size.x != c0->max_width && total_accum) {
                    c0->size.x = glm::clamp(c0->min_width + max_x * c0->weight_width / total_accum, c0->min_width, c0->max_width);
                }
            }
        }
    };
    before.push_back(c);

    // column and row derivation
    c.func = [this, ui_system]() {
        float column_target = 0.0f;
        std::fill(rows.begin(), rows.end(), 0.0f);
        std::fill(columns.begin(), columns.end(), 0.0f);

        int num_rows = ceil(float(children.size()) / num_columns);

        float prev = 0.0f;
        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];
            
            uint row = floor(float(i) / num_columns);
            uint column = i % num_columns;

            rows[row] = glm::max(rows[row], c0->size.y);
            columns[column] = glm::max(columns[column], c0->size.x);
        }

        for(int i = 0; i < num_columns - 1; ++i) {
            column_buffers[i] = buffer.x;
        }
        for(int i = 0; i < num_rows - 1; ++i) {
            row_buffers[i] = buffer.y;
        }

        float size_x = 0.0f;
        for(float f : columns) size_x += f;
        for(float f : column_buffers) size_x += f;
    };
    before.push_back(c);

    // target and weights for x and y
    c.func = [this, ui_system]() {
        std::vector<float> target_min = std::vector<float>(num_columns, 0.0f);
        std::vector<float> target_max = std::vector<float>(num_columns, 0.0f);
        std::vector<float> target_weight = std::vector<float>(num_columns, 0.0f);
        
        int num_rows = ceil(float(children.size()) / num_columns);

        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];

            ivec2 index = {i % num_columns, num_rows - floor(float(i) / num_columns) - 1};

            float buf = 0.0f;
            if(index.x != num_columns - 1) buf = buffer.x;

            if(c0->min_width != c0->max_width) {
                target_min[index.x] = glm::max(target_min[index.x], c0->min_width + buf);
                target_max[index.x] = glm::max(target_max[index.x], c0->max_width + buf);
                target_weight[index.x] = glm::max(target_weight[index.x], c0->weight_width);
            } else {
                target_min[index.x] = glm::max(target_min[index.x], c0->size.x + buf);
                target_max[index.x] = glm::max(target_max[index.x], c0->size.x + buf);
            }
        }

        min_width = std::accumulate(target_min.begin(), target_min.end(), 0.0f);
        max_width = std::accumulate(target_max.begin(), target_max.end(), 0.0f);
        weight_width = std::accumulate(target_weight.begin(), target_weight.end(), 0.0f);

        //

        target_min = std::vector<float>(num_rows, 0.0f);
        target_max = std::vector<float>(num_rows, 0.0f);
        target_weight = std::vector<float>(num_rows, 0.0f);

        for(int i = 0; i < children.size(); ++i) {
            auto& c0 = ui_system->widgets[children[i]];

            ivec2 index = {i % num_columns, num_rows - floor(float(i) / num_columns) - 1};
            
            float buf = 0.0f;
            if(index.y != num_rows - 1) buf = buffer.y;

            if(c0->min_height != c0->max_height) {
                target_min[index.y] = glm::max(target_min[index.y], c0->min_height + buf);
                target_max[index.y] = glm::max(target_max[index.y], c0->max_height + buf);
                target_weight[index.y] = glm::max(target_weight[index.y], c0->weight_height);
            } else {
                target_min[index.y] = glm::max(target_min[index.y], c0->size.y + buf);
                target_max[index.y] = glm::max(target_max[index.y], c0->size.y + buf);
            }
        }

        min_height = std::accumulate(target_min.begin(), target_min.end(), 0.0f);
        max_height = std::accumulate(target_max.begin(), target_max.end(), 0.0f);
        weight_height = std::accumulate(target_weight.begin(), target_weight.end(), 0.0f);
    };
    before.push_back(c);

    // position self
    c.func = [this]() {
        vec4 area = {position, size};
        vec2 true_size = vec2(0.0f, 0.0f);

        for(int i = 0; i < columns.size(); ++i) {
            true_size.x += columns[i];
            if(i != columns.size() - 1) true_size.x += column_buffers[i];
        }
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
    before.push_back(c);

    // positioning
    c.func = [this, ui_system]() {
        vec2 pos = position;
        int num_rows = ceil(float(children.size()) / num_columns);

        for(int i = 0; i < children.size(); ++i) {
            ivec2 index = {i % num_columns, floor(float(i) / num_columns)};
            int ii = index.x + (num_rows - index.y - 1) * num_columns;

            if(index.x == 0 && index.y != 0) {
                pos.x = position.x;

                pos.y += rows[index.y];
                pos.y += row_buffers[index.y - 1];
            }
            
            vec4 range = vec4(pos, columns[index.x], rows[index.y]);

            pos.x += columns[index.x];
            if(index.x != column_buffers.size()) pos.x += column_buffers[index.x];

            if(ii < children.size()) { 
                auto& c0 = ui_system->widgets[children[ii]];
            
                // positioning

                switch(c0->position_mode) {
                    case axiom::position_mode::BOTTOM_LEFT: {
                        c0->position.x = range.x;
                        c0->position.y = range.y;
                        break;
                    }
                    case axiom::position_mode::BOTTOM_CENTER: {
                        c0->position.x = position.x + (range.z - c0->size.x) * 0.5f;
                        c0->position.y = range.y;
                        break;
                    }
                    case axiom::position_mode::BOTTOM_RIGHT: {
                        c0->position.x = range.x + (range.z - c0->size.x);
                        c0->position.y = range.y;
                        break;
                    }
                    case axiom::position_mode::CENTER_LEFT: {
                        c0->position.x = range.x;
                        c0->position.y = range.y + (range.w - c0->size.y) * 0.5f;
                        break;
                    }
                    case axiom::position_mode::CENTER: {
                        c0->position.x = position.x + (range.z - c0->size.x) * 0.5f;
                        c0->position.y = range.y + (range.w - c0->size.y) * 0.5f;
                        break;
                    }
                    case axiom::position_mode::CENTER_RIGHT: {
                        c0->position.x = range.x + (range.z - c0->size.x);
                        c0->position.y = range.y + (range.w - c0->size.y) * 0.5f;
                        break;
                    }
                    case axiom::position_mode::TOP_LEFT: {
                        c0->position.x = range.x;
                        c0->position.y = range.y + (range.w - c0->size.y);
                        break;
                    }
                    case axiom::position_mode::TOP_CENTER: {
                        c0->position.x = position.x + (range.z - c0->size.x) * 0.5f;
                        c0->position.y = range.y + (range.w - c0->size.y);
                        break;
                    }
                    case axiom::position_mode::TOP_RIGHT: {
                        c0->position.x = range.x + (range.z - c0->size.x);
                        c0->position.y = range.y + (range.w - c0->size.y);
                        break;
                    }
                }
            }
        }
    };
    before.push_back(c);
}

}