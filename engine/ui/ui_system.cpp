#include <ui/ui_system.hpp>
#include <ui/cursor.hpp>

#include <GLFW/glfw3.h>

#include <set>

namespace axiom {

ui_system::ui_system(axiom::window* window_) {
    window = window_;
}
    
void ui_system::init() {
    //fonts.emplace("default mono", font("resources/fonts/axiom_default.bdf"));
}

void ui_system::input_root(int delta) {
    while(input_state.depth > delta) {
        if(widgets[input_state.current_widget]->parent == NULL_WIDGET) break;
        input_state.current_widget = widgets[input_state.current_widget]->parent;
        --input_state.depth;
    }
}

void ui_system::input_step(int delta) {
    for(int i = 0; i < delta; ++i) {
        if(widgets[input_state.current_widget]->parent == NULL_WIDGET) break;
        input_state.current_widget = widgets[input_state.current_widget]->parent;
        --input_state.depth;
    }
}

void ui_system::input_reset() {
    input_state.current_widget = NULL_WIDGET;
    input_state.depth = 0;
}

void ui_system::input_z(float z) {
    input_state.z = z;
}

void ui_system::position(axiom::position_mode mode) {
    input_state.active_position = mode;
}
void ui_system::buffer(vec4 buffer) {
    input_state.active_buffer = buffer;
}

void ui_system::make_dirty(ulong root) {
    std::vector<ulong> path;
    std::vector<ulong> child_ids;

    //

    path = {root};
    child_ids = {0};
    while(true) {
        if (path.size() == 0) break;

        auto& widget = widgets[path.back()];

        vec2 children_size = vec2(0.0f);

        if (widget->children.size() <= child_ids.back()) {
            widget->dirty = true;

            // go up
            path.pop_back();
            child_ids.pop_back();
        } else {
            path.push_back(widget->children[child_ids.back()]);

            ++child_ids.back();
            child_ids.push_back(0);
        }
    }
}

void ui_system::solve_constraints() {
    std::vector<ulong> roots;
    for(auto& [key, widget] : widgets) {
        if(widget->parent == NULL_WIDGET) roots.push_back(key);
    }

    for(ulong root : roots) {
        std::vector<ulong> path = {root};
        std::vector<ulong> child_ids = {0};
        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];
            if(child_ids.back() == 0) {
                for(auto& f : widget->before) f.func();
            }

            if (widget->children.size() <= child_ids.back()) {
                for(auto& f : widget->after) f.func();

                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }
    }
    
    for(auto& [key, widget] : widgets) widget->dirty = true;
}

void ui_system::input_set(ulong w) {
    input_state.current_widget = w;

    uint32_t d = 0;
    while(true) {
        if(input_state.current_widget == NULL_WIDGET) break;
        input_state.current_widget = widgets[input_state.current_widget]->parent;
        
        ++d;
    }

    input_state.current_widget = w;
    input_state.depth = d;
}

void ui_system::call() {
    handle_capture();

    target = 0;

    copy = false;
    paste = false;
    copy_strings.clear();
    if(window->pressed_buttons.contains(axiom::input_code::KEY_V) && window->input_map[axiom::input_code::KEY_LEFT_CTRL]) paste = true;
    if(window->pressed_buttons.contains(axiom::input_code::KEY_C) && window->input_map[axiom::input_code::KEY_LEFT_CTRL] && !paste) copy = true;

    vertices.clear();
    cursor.cursor_mode = axiom::cursor_mode::DEFAULT;

    //do_layout();

    std::vector<vec4> sizes;
    sizes.reserve(widgets.size());
    for(auto& [key, widget] : widgets) sizes.push_back(vec4(widget->position, widget->size));

    float tolerance = 0.1f;

    std::vector<ulong> roots;
    for(auto& [key, widget] : widgets) if(widget->parent == NULL_WIDGET) roots.push_back(key);

    target_textures.clear();
    
    for(ulong root : roots) {
        std::vector<ulong> path = {root};
        std::vector<ulong> child_ids = {0};
        //

        std::vector<ulong> widget_ids;

        path = {root};
        child_ids = {0};
        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];
            if(child_ids.back() == 0) widget_ids.push_back(path.back());

            if (widget->children.size() <= child_ids.back()) {
                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }

        for(ulong i : widget_ids) {
            if(widgets.contains(i)) {
                auto& widget = widgets[i];
                widget->handle_inputs();
            }
        }
    }

    for(int i = 0; i < 8; ++i) {
        iter = i;
        
        solve_constraints();
        
        /*
        for(auto& [key, widget] : widgets) {
            for(Text& text : widget->texts) text.call();
        }*/
        
        std::vector<vec4> new_sizes;
        new_sizes.reserve(widgets.size());
        for(auto& [key, widget] : widgets) new_sizes.push_back(vec4(widget->position, widget->size));

        bool finish = true;
        for(int i = 0; i < new_sizes.size(); ++i) {
            vec4 diff = sizes[i] - new_sizes[i];
            float d = abs(diff.x) + abs(diff.y) + abs(diff.z) + abs(diff.w);
            if(d > tolerance) finish = false;
        }

        //if(finish) break;

        sizes = new_sizes;
    }
    
    if(window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
        cursor_anchor = window->cursor_pos;
    }
    cursor_pos = window->cursor_pos;

    for(auto& t : text) {
        t->call();
    }

    // mesh

    
    if(text_capture != NULL_WIDGET) {
        std::set<uint64_t> c;
        uint32_t n_focused = 0;
        
        std::vector<uint64_t> path = {text_capture};
        std::vector<uint64_t> child_ids = {0};
        //

        cursor_pos = window->cursor_pos;
        
        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];

            if(child_ids.back() == 0) {
                c.insert(path.back());
            }

            if (widget->children.size() <= child_ids.back()) {
                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }

        for(auto& [key, widget] : widgets) {
            if(c.contains(key)) {
                for(auto& text : widget->text) {
                    text->select(vec4(cursor_anchor, cursor_pos), window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT));
                    //if(text->focused) ++n_focused;
                }
            } else {
                for(auto& text : widget->text) {
                    //text->v_select.clear();
                    //text->select_range = vec2(-1);
                    //text->select(vec4(-1));
                }
            }
        }
        
        if(n_focused) isolate_selection = true;
        else isolate_selection = false;
    }

    //
    
    roots.clear();
    for(auto& [key, widget] : widgets) if(widget->parent == NULL_WIDGET) roots.push_back(key);

    for(ulong root : roots) {
        std::vector<ulong> path = {root};
        std::vector<ulong> child_ids = {0};

        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];

            vec2 children_size = vec2(0.0f);

            if(child_ids.back() == 0) {
                widget->mesh();
                vertices.insert(vertices.end(), widget->vertices_before.begin(), widget->vertices_before.end());
            }

            if (widget->children.size() <= child_ids.back()) {
                vertices.insert(vertices.end(), widget->vertices_after.begin(), widget->vertices_after.end());

                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }
    }

    if(window->cursor_hidden && !window->cursor_disabled) {
        std::vector<ui_vertex> cursor_vertices = mesh_cursor(cursor.cursor_mode, window->cursor_pos);

        vertices.insert(vertices.end(), cursor_vertices.begin(), cursor_vertices.end());
    }

    for(ulong k : delete_buffer) {
        widgets.erase(k);
    }
    delete_buffer.clear();

    // text

    std::vector<uint32_t> delete_indices;

    uint32_t i = 0;
    for(auto& t : text) {
        if(t.unique()) {
            delete_indices.push_back(i);
        }
        ++i;
    }

    for(auto iter = delete_indices.rbegin(); iter != delete_indices.rend(); ++iter) {
        uint32_t i = *iter;

        text.erase(text.begin() + i);
    }

    /*

    for(auto& [key, widget] : widgets) {
        for(Text& text : widget->texts) {
            if(text.dirty) {
                text.dirty = false;

                text.refresh();
                text.mesh();
            }
        }
    }


    
    text_selected.clear();
    for(auto& [key, widget] : widgets) {
        uint i = 0;
        for(Text& text : widget->texts) {
            if(text.select_range.x != -1 || text.select_range.y != -1) text_selected.push_back({widget->self, i});
            ++i;
            
            if(text.select_range.x != -1) {
                if(copy) {
                    Copy_String str;
                    str.str = text.retrieve();
                    str.y = text.position.y + text.size.y;

                    copy_strings.push_back(str);
                }
            }
        }
    }

    if(copy) {
        std::sort(copy_strings.begin(), copy_strings.end(), 
            [](const Copy_String& a, const Copy_String& b) {
                return a.y > b.y;
            }
        );

        std::string full_copy;
        int i = 0;
        for(auto& cs : copy_strings) {
            full_copy += cs.str;
            if(i != copy_strings.size() - 1) full_copy += "\n";
            ++i;
        }

        if(full_copy.size()) glfwSetClipboardString(window->window.window, full_copy.c_str());
    }

    if(text_selected.size() == 1) {
        auto& p = text_selected[0];

        auto& widget = widgets[p.first];
        Text& text = widget->texts[p.second];

        if(text.editable && text.focused) {
            std::string input = window->char_delta;
            if(paste) {
                const char* text = glfwGetClipboardString(window->window.window);
                if(text) {
                    input = text;
                }
            }

            auto wstring = convert_string(text.string);

            if(input.size()) {
                int minv = min(text.select_range.x, text.select_range.y);
                int maxv = max(text.select_range.x, text.select_range.y);
                
                if(text.select_range.x != text.select_range.y) {
                    text.string.erase(text.string.begin() + minv, text.string.begin() + maxv);
                    text.select_range.x = minv;
                    text.select_range.y = minv;
                }

                text.string.insert(text.string.begin() + minv, input.begin(), input.end());
                text.select_range += input.size();

                text.dirty = true;
            }

            if(window->pressed_buttons.contains(axiom::input_code::KEY_BACKSPACE) || window->repeat_buttons.contains(axiom::input_code::KEY_BACKSPACE)) {
                int minv = min(text.select_range.x, text.select_range.y);
                int maxv = max(text.select_range.x, text.select_range.y);

                if(text.select_range.x != text.select_range.y) {
                    text.string.erase(text.string.begin() + minv, text.string.begin() + maxv);
                    text.select_range.x = minv;
                    text.select_range.y = minv;

                    text.dirty = true;
                    text.start_cursor = window->current_time;
                } else {
                    if(text.select_range.x > 0) {
                        text.string.erase(text.string.begin() + (text.select_range.x - 1), text.string.begin() + text.select_range.x);
                        text.select_range -= 1;

                        text.dirty = true;
                        text.start_cursor = window->current_time;
                    }
                }
            }

            if(window->pressed_buttons.contains(axiom::input_code::KEY_LEFT) || window->repeat_buttons.contains(axiom::input_code::KEY_LEFT)) {
                if(window->input_map[axiom::input_code::KEY_LEFT_SHIFT] || window->input_map[axiom::input_code::KEY_RIGHT_SHIFT]) {
                    if(text.select_range.y > 0) {
                        --text.select_range.y;

                        text.dirty = true;
                        text.start_cursor = window->current_time;
                    }
                } else {
                    if(text.select_range.x != text.select_range.y) {
                        text.select_range.y = text.select_range.x;

                        text.dirty = true;
                        text.start_cursor = window->current_time;
                    } else if(text.select_range.x > 0) {
                        text.select_range -= 1.0f;

                        text.dirty = true;
                        text.start_cursor = window->current_time;
                    }
                }
            }
            if(window->pressed_buttons.contains(axiom::input_code::KEY_RIGHT) || window->repeat_buttons.contains(axiom::input_code::KEY_RIGHT)) {
                if(window->input_map[axiom::input_code::KEY_LEFT_SHIFT] || window->input_map[axiom::input_code::KEY_RIGHT_SHIFT]) {
                    if(text.select_range.y < wstring.size()) {
                        ++text.select_range.y;

                        text.dirty = true;
                        text.start_cursor = window->current_time;
                    }
                } else {
                    if(text.select_range.x != text.select_range.y) {
                        text.select_range.x = text.select_range.y;

                        text.dirty = true;
                        text.start_cursor = window->current_time;
                    } else if(text.select_range.x < wstring.size()) {
                        text.select_range += 1;

                        text.dirty = true;
                        text.start_cursor = window->current_time;
                    }
                }
            }
        }
    }
    
    if(window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
        cursor_anchor = window->cursor_pos;
    }
    
    if(text_capture != NULL_WIDGET) {
        std::set<ulong> c;
        uint n_focused = 0;
        
        std::vector<ulong> path = {text_capture};
        std::vector<ulong> child_ids = {0};
        //

        cursor_pos = window->cursor_pos;
        
        while(true) {
            if (path.size() == 0) break;

            auto& widget = widgets[path.back()];

            if(child_ids.back() == 0) {
                c.insert(path.back());
            }

            if (widget->children.size() <= child_ids.back()) {
                // go up
                path.pop_back();
                child_ids.pop_back();
            } else {
                path.push_back(widget->children[child_ids.back()]);

                ++child_ids.back();
                child_ids.push_back(0);
            }
        }

        for(auto& [key, widget] : widgets) {
            if(c.contains(key)) {
                for(Text& text : widget->texts) {
                    text.select(vec4(cursor_anchor, cursor_pos));
                    if(text.focused) ++n_focused;
                }
            } else {
                for(Text& text : widget->texts) {
                    text.select(vec4(-1));
                }
            }
        }
        
        if(n_focused) isolate_selection = true;
        else isolate_selection = false;
    }
    
    
    if(window->pressed_buttons.contains(axiom::input_code::KEY_ENTER) || window->pressed_buttons.contains(axiom::input_code::KEY_ESCAPE)) {
        for(auto& [key, widget] : widgets) {
            for(Text& text : widget->texts) {
                text.select(vec4(-1));
            }
        }
    }
    */
}

//

vec4 ui_system::get_range(ulong v, bool include_self) {
    ulong current = v;
    vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
    if(include_self) range = widgets[current]->view_range;

    while(true) {
        ulong parent = widgets[current]->parent;

        if(parent == NULL_WIDGET) break;

        vec4 r = widgets[parent]->view_range;
        //r.x = floor(r.x);
        //r.y = floor(r.y);
        //r.z = ceil(r.z);
        //r.w = ceil(r.w);
        
        range = intersect_range(range, r);
        current = parent;
    }

    return floor(range);
}


std::vector<ulong> ui_system::get_children(ulong root) {
    std::vector<ulong> keys;
    
    std::vector<ulong> path;
    std::vector<ulong> child_ids;

    path = {root};
    child_ids = {0};
    while(true) {
        if (path.size() == 0) break;

        auto& widget = widgets[path.back()];

        vec2 children_size = vec2(0.0f);

        if (widget->children.size() <= child_ids.back()) {
            if(widget->self != root) keys.push_back(widget->self);

            // go up
            path.pop_back();
            child_ids.pop_back();
        } else {
            path.push_back(widget->children[child_ids.back()]);

            ++child_ids.back();
            child_ids.push_back(0);
        }
    }

    return keys;
}

void ui_system::erase(std::vector<ulong> ws) {
    for(ulong key : ws) {
        widgets.erase(key);
    }
}

void ui_system::set_attrib(vec2 min_size, vec2 max_size, vec2 weights) {
    auto& widget = widgets[input_state.last_widget];

    if(min_size.x >= 0.0f) widget->min_width = min_size.x;
    if(max_size.x >= 0.0f) widget->max_width = max_size.x;

    if(min_size.y >= 0.0f) widget->min_height = min_size.y;
    if(max_size.y >= 0.0f) widget->max_height = max_size.y;

    if(weights.x >= 0.0f) widget->weight_width = weights.x;
    if(weights.y >= 0.0f) widget->weight_height = weights.y;

    if(widget->min_width == widget->max_width) widget->size.x = widget->min_width;
    if(widget->min_height == widget->max_height) widget->size.y = widget->min_height;
}

void ui_system::handle_capture() {
    capture_data capture_global;
    capture_global.z = 0.0f;
    capture_data capture_text;
    capture_text.z = 0.0f;

    ulong w = NULL_WIDGET;
    ulong wt = NULL_WIDGET;

    std::vector<ulong> roots;
    for(auto& [key, widget] : widgets) {
        capture_data data = widget->handle_capture();

        if(data.capture) {
            if(capture_global.z <= data.z) {
                w = key;
                capture_global = data;
            }

            if(data.text_capture || data.overwrite) {
                if(capture_text.z <= data.z) {
                    wt = key;
                    capture_text = data;
                }
            }
        }
    }
    
    hover_capture = w;

    if(window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT)) {
        click_capture = hover_capture;
        if(capture_text.text_capture) text_capture = wt;
    }
    if(!window->input_map[axiom::input_code::MOUSE_LEFT]) {
        click_capture = NULL_WIDGET;
        text_capture = NULL_WIDGET;
    }
}

}
