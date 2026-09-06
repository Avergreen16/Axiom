#include <set>

#include <ui/widgets/split_widget.hpp>
#include <ui/system.hpp>

#include <GLFW/glfw3.h>

namespace axiom {


uint64_t split_widget::insert(axiom::layout_mode layout, std::vector<panel_constraint> constraints_) {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    split_widget widget;
    widget.layout_mode = layout;
    widget.position_mode = axiom::position_mode::BOTTOM_LEFT;
    widget.sep = vec2(1.0f);
    widget.constraints = constraints_;
    
    return ui_system.insert_widget(widget, true);
}

void split_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
    
    process_size();

    if(ui_system.hover_capture == self || ui_system.click_capture == self) {
        if(layout_mode == axiom::layout_mode::ROW) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_L;
        } else if(layout_mode == axiom::layout_mode::COLUMN) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_B;
        }
    }
    
    float buffer = 4.0f;

    vec4 buffer_range;
    if(layout_mode == axiom::layout_mode::ROW) buffer_range = ivec4(-buffer, 0.0f, buffer, 0.0f);
    else if(layout_mode == axiom::layout_mode::COLUMN) buffer_range = ivec4(0.0f, -buffer, 0.0f, buffer);

    if(ui_system.click_capture != self) operation = 0;

    if(ui_system.click_capture == self) {
        if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
            for(int i = 0; i < children.size() - 1; ++i) {
                int i0 = i;
                int i1 = i + 1;


                if(layout_mode == axiom::layout_mode::ROW) {
                    auto& w0 = ui_system.widgets[children[i0]];
                    auto& w1 = ui_system.widgets[children[i1]];
                    
                    vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x, w1->position.y + w1->size.y);

                    range += buffer_range;

                    if(includes(ui_system.window->cursor_pos, range)) {
                        ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_L;

                        operation = i;
                    }
                } else if(layout_mode == axiom::layout_mode::COLUMN) {
                    auto& w0 = ui_system.widgets[children[i1]];
                    auto& w1 = ui_system.widgets[children[i0]];
                    
                    vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x + w1->size.x, w1->position.y);

                    range += buffer_range;

                    if(includes(ui_system.window->cursor_pos, range)) {
                        ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_B;
                        
                        operation = i;
                    }
                }
            }
            float mw = 20.0f;
        }

        //
        
        if(layout_mode == axiom::layout_mode::ROW) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_L;

            int i = operation;

            panel_constraint& c0 = constraints[i];
            panel_constraint& c1 = constraints[i + 1];

            auto& w0 = ui_system.widgets[children[i]];
            auto& w1 = ui_system.widgets[children[i + 1]];

            float s = size.x;
            float borders = (children.size() - 1) * sep.x;
            s -= borders;
            
            float p = position.x;
            float c = ui_system.window->cursor_pos.x;
            
            c = glm::clamp(c, w0->position.x + w0->min_width, w1->position.x + w1->size.x - w1->min_width - sep.x);

            float r = (c - w0->position.x);
            float new_ratio = r / s;

            float difference = c0.value - new_ratio;
            c0.value -= difference;
            c1.value += difference;
        } else if(layout_mode == axiom::layout_mode::COLUMN) {
            ui_system.cursor.cursor_mode = axiom::cursor_mode::DRAG_B;

            int i = operation;

            panel_constraint& c0 = constraints[i + 1];
            panel_constraint& c1 = constraints[i];

            auto& w0 = ui_system.widgets[children[i + 1]];
            auto& w1 = ui_system.widgets[children[i]];

            float s = size.y;
            float borders = (children.size() - 1) * sep.y;
            s -= borders;
            
            float p = position.y;
            float c = ui_system.window->cursor_pos.y;
            
            c = glm::clamp(c, w0->position.y + w0->min_height, w1->position.y + w1->size.y - w1->min_height - sep.y);

            float r = (c - w0->position.y);
            float new_ratio = r / s;

            float difference = c0.value - new_ratio;
            c0.value -= difference;
            c1.value += difference;
        }
    }

    // handle inputs
    recalibrate();
}

void split_widget::process_size() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    min_width = 0;
    min_height = 0;

    if(layout_mode == axiom::layout_mode::ROW) {
        for(int i = 0; i < children.size(); ++i) {
            auto& child = ui_system.widgets[children[i]];
            
            if(child->flag) {
                child->min_width = 20;
                child->min_height = 20;
            }

            if(layout_mode == axiom::layout_mode::COLUMN) {
                min_width = child->min_width;
                min_height += child->min_height + ((i == 0) ? 0 : sep.y);
            } else if(layout_mode == axiom::layout_mode::ROW) {
                min_width += child->min_width + ((i == 0) ? 0 : sep.x);
                min_height = child->min_height;
            }
        }
        
        if(prev_size != size.x && prev_size >= 0.0f) {
            float prev_asize = prev_size - (children.size() - 1) * sep.x;
            float new_asize = size.x - (children.size() - 1) * sep.x;
             
            float change = 0.0f;
            std::set<uint> locked;

            for(int i = 0; i < children.size(); ++i) {
                panel_constraint& constraint = constraints[i];

                if(constraint.panel_mode == axiom::panel_mode::SIZE) {
                    float new_frac = constraint.value * prev_asize / new_asize;
                    
                    float delta = new_frac - constraint.value;
                    
                    constraint.value += delta;

                    change += delta;

                    locked.insert(i);
                }
            }

            int num = children.size() - locked.size();

            if(num != 0) {
                for(int i = 0; i < children.size(); ++i) {
                    if(locked.find(i) == locked.end()) {
                        panel_constraint& constraint = constraints[i];
                        constraint.value -= change / num;
                    }
                }
            }
        }

        prev_size = size.x;
    } else if(layout_mode == axiom::layout_mode::COLUMN) {
        for(int i = 0; i < children.size(); ++i) {
            auto& child = ui_system.widgets[children[children.size() - i - 1]];
            
            if(child->flag) {
                child->min_width = 20;
                child->min_height = 20;
            }

            if(layout_mode == axiom::layout_mode::COLUMN) {
                min_width = child->min_width;
                min_height += child->min_height + ((i == 0) ? 0 : sep.y);
            } else if(layout_mode == axiom::layout_mode::ROW) {
                min_width += child->min_width + ((i == 0) ? 0 : sep.x);
                min_height = child->min_height;
            }
        }

        if(prev_size != size.y && prev_size >= 0.0f) {
            float prev_asize = prev_size - (children.size() - 1) * sep.y;
            float new_asize = size.y - (children.size() - 1) * sep.y;
             
            float change = 0.0f;
            std::set<uint> locked;

            for(int i = 0; i < children.size(); ++i) {
                panel_constraint& constraint = constraints[children.size() - i - 1];

                if(constraint.panel_mode == axiom::panel_mode::SIZE) {
                    float new_frac = constraint.value * prev_asize / new_asize;
                    
                    float delta = new_frac - constraint.value;

                    constraint.value += delta;

                    change += delta;

                    locked.insert(i);
                }
            }

            int num = children.size() - locked.size();

            if(num != 0) {
                for(int i = 0; i < children.size(); ++i) {
                    if(locked.find(i) == locked.end()) {
                        panel_constraint& constraint = constraints[children.size() - i - 1];
                        constraint.value -= change / num;
                    }
                }
            }
        }
        
        prev_size = size.y;
    }
}

void split_widget::recalibrate() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    float mw = 20.0f;

    if(layout_mode == axiom::layout_mode::ROW) {
        float total_area = size.x;

        float borders = (children.size() - 1) * sep.x;
        total_area -= borders;
        
        float change = 1.0f;
        std::set<uint> locked;

        while(change != 0.0f) {    
            change = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& child = ui_system.widgets[children[i]];
                panel_constraint& constraint = constraints[i];
                
                float new_size = glm::max(constraint.value, child->min_width / total_area);
                if(constraint.value <= child->min_width / total_area) {
                    float diff = new_size - constraint.value;
                    constraint.value += diff;
                    locked.insert(i);

                    change += diff;
                }
            }

            int num = children.size() - locked.size();

            if(num == 0) break;
            
            for(int i = 0; i < children.size(); ++i) {
                if(locked.find(i) == locked.end()) {
                    panel_constraint& constraint = constraints[i];
                    constraint.value -= change / num;
                }
            }
        }

        //

        float total = 0.0f;

        for(panel_constraint& constraint : constraints) {
            total += constraint.value;
        }
        for(panel_constraint& constraint : constraints) constraint.value /= total;
    } else if(layout_mode == axiom::layout_mode::COLUMN) {
        float total_area = size.y;

        float borders = (children.size() - 1) * sep.y;
        total_area -= borders;

        float change = 1.0f;
        std::set<uint> locked;

        while(change != 0.0f) {    
            change = 0.0f;

            for(int i = 0; i < children.size(); ++i) {
                auto& child = ui_system.widgets[children[children.size() - i - 1]];
                panel_constraint& constraint = constraints[children.size() - i - 1];
                
                float new_size = glm::max(constraint.value, child->min_height / total_area);
                if(constraint.value <= child->min_height / total_area) {
                    float diff = new_size - constraint.value;
                    constraint.value += diff;
                    locked.insert(i);

                    change += diff;
                }
            }

            int num = children.size() - locked.size();

            if(num == 0) break;
            
            for(int i = 0; i < children.size(); ++i) {
                if(locked.find(i) == locked.end()) {
                    panel_constraint& constraint = constraints[children.size() - i - 1];
                    constraint.value -= change / num;
                }
            }
        }
        
        //

        float total = 0.0f;

        for(panel_constraint& constraint : constraints) {
            total += constraint.value;
        }
        for(panel_constraint& constraint : constraints) constraint.value /= total;
    }
}

void split_widget::init() {
    axiom::ui_system* ui_system = &axiom::ecs.get_system<axiom::ui_system>();

    widget_constraint c;

    c.func = [this, ui_system]() {
        process_size();

        float s = 0.0f;
        if(layout_mode == axiom::layout_mode::COLUMN) s = size.y - sep.y * (constraints.size() - 1);
        else if(layout_mode == axiom::layout_mode::ROW) s = size.x - sep.x * (constraints.size() - 1);

        for(int i = 0; i < 16; ++i) {
            float change = 0.0f;
            uint num = constraints.size();
            float total = 0.0f;

            for(int i = 0; i < constraints.size(); ++i) {
                panel_constraint& panel_constraint = constraints[i];
                auto& p0 = ui_system->widgets[children[i]];

                float minimum;
                if(layout_mode == axiom::layout_mode::COLUMN) minimum = p0->min_height;
                else if(layout_mode == axiom::layout_mode::ROW) minimum = p0->min_width;
                    
                float v = panel_constraint.value * s;
                if(v <= minimum) {
                    --num;
                }
                float c = (v - glm::max(v, minimum)) / s;
                change += c;
                panel_constraint.value -= c;
                total += panel_constraint.value;
            }

            
            for(int i = 0; i < constraints.size(); ++i) {
                panel_constraint& panel_constraint = constraints[i];
                panel_constraint.value /= total;
            }
        }
    };
    before.push_back(c);

    if(layout_mode == axiom::layout_mode::COLUMN) {
        c.func = [this, ui_system]() {
            for(int i = 0; i < 16; ++i) {
                for(int j = children.size() - 1; j >= 0; --j) {
                    int jj = j;

                    auto& p0 = ui_system->widgets[children[jj]];
                    panel_constraint& panel_constraint = constraints[jj];

                    if(jj == children.size() - 1) {
                        float C = p0->position.y - position.y;
                        p0->position.y -= C;
                    } else {
                        auto& pp = ui_system->widgets[children[jj + 1]];

                        float C = p0->position.y - (pp->position.y + pp->size.y + sep.y);
                        p0->position.y -= C;
                    }

                    float C0 = p0->size.y - (size.y - sep.y * (children.size() - 1)) * panel_constraint.value;
                    p0->size.y -= C0;
                    
                    float C1 = p0->size.x - size.x;
                    p0->size.x -= C1;

                    float C = p0->position.x - position.x;
                    p0->position.x -= C;
                }
            }
        };
        before.push_back(c);
    } else if(layout_mode == axiom::layout_mode::ROW) {
        c.func = [this, ui_system]() {
            for(int i = 0; i < 16; ++i) {
                for(int j = 0; j < children.size(); ++j) {
                    auto& p0 = ui_system->widgets[children[j]];
                    panel_constraint& panel_constraint = constraints[j];

                    if(j == 0) {
                        float C = p0->position.x - position.x;
                        p0->position.x -= C;
                    } else {
                        auto& pp = ui_system->widgets[children[j - 1]];

                        float C = p0->position.x - (pp->position.x + pp->size.x + sep.x);
                        p0->position.x -= C;
                    }
                    
                    float C = p0->position.y - position.y;
                    p0->position.y -= C;
                    
                    float C0 = p0->size.x - (size.x - sep.x * (children.size() - 1)) * panel_constraint.value;
                    p0->size.x -= C0;
                    
                    float C1 = p0->size.y - size.y;
                    p0->size.y -= C1;
                }
            }
        };
        before.push_back(c);
    }
}
    
capture_data split_widget::handle_capture() {
    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    float buffer = 4.0f;
    
    vec4 buffer_range;
    if(layout_mode == axiom::layout_mode::ROW) buffer_range = ivec4(-buffer, 0.0f, buffer, 0.0f);
    else if(layout_mode == axiom::layout_mode::COLUMN) buffer_range = ivec4(0.0f, -buffer, 0.0f, buffer);

    for(int i = 0; i < children.size() - 1; ++i) {
        int i0 = i;
        int i1 = i + 1;


        if(layout_mode == axiom::layout_mode::ROW) {
            auto& w0 = ui_system.widgets[children[i0]];
            auto& w1 = ui_system.widgets[children[i1]];
            
            vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x, w1->position.y + w1->size.y);

            range += buffer_range;

            if(includes(ui_system.window->cursor_pos, range)) {
                return {self, z + 0.001f, true, false};
            }
        } else if(layout_mode == axiom::layout_mode::COLUMN) {
            auto& w0 = ui_system.widgets[children[i1]];
            auto& w1 = ui_system.widgets[children[i0]];
            
            vec4 range = vec4(w1->position.x, w1->position.y, w1->position.x + w1->size.x, w1->position.y);

            range += buffer_range;

            if(includes(ui_system.window->cursor_pos, range)) {
                return {self, z + 0.001f, true};
            }
        }
    }

    return {self, z, false};
}

}