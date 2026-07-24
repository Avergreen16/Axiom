#pragma once

#include <ui/widget_base.hpp>
#include <window/window.hpp>
#include <ui/text.hpp>
#include <ui/cursor.hpp>

namespace axiom {

struct copy_string {
    float y = 0.0f;
    std::string str;
};

struct widget_input_state {
    ulong current_widget = NULL_WIDGET;
    ulong last_widget = NULL_WIDGET;

    axiom::position_mode active_position = axiom::position_mode::TOP_LEFT;
    vec4 active_buffer = vec4(0.0f);

    float z = 0.0f;
    
    ulong next_id = 0;

    int depth = 0;
};

struct ui_system : system {
    uint32_t iter;
    axiom::window* window;
    std::vector<axiom::font_asset*> font_assets;

    axiom::cursor cursor;

    std::vector<ui_vertex> vertices;
    bool hex_mode = true;

    std::map<ulong, std::unique_ptr<widget>> widgets;

    widget_input_state input_state;

    //

    bool copy = false;
    bool paste = false;
    std::vector<copy_string> copy_strings;
    
    bool isolate_selection = false;

    vec2 cursor_pos;
    vec2 cursor_anchor;
    std::vector<std::pair<ulong, uint>> text_selected;

    ulong hover_capture = NULL_WIDGET;
    ulong click_capture = NULL_WIDGET;
    ulong text_capture = NULL_WIDGET;

    std::vector<ulong> delete_buffer;

    //

    std::vector<std::shared_ptr<axiom::text>> text;

    //

    ui_system(axiom::window* window_);

    void init();
    
    void input_root(int delta);
    void input_step(int delta = 1);
    void input_reset();
    void input_set(ulong w);

    void input_z(float z);

    template<typename Type>
    ulong insert_widget(Type widget, bool step = false);
    void position(axiom::position_mode mode);
    void buffer(vec4 buffer);
    void make_dirty(ulong root);

    vec4 get_range(ulong v, bool include_self = false);
    void call();
    void solve_constraints();
    std::vector<ulong> get_children(ulong root);
    void erase(std::vector<ulong> ws);

    void set_attrib(vec2 min_size, vec2 max_size, vec2 weights);
    
    void handle_capture();
};

template<typename Type>
uint64_t ui_system::insert_widget(Type widget, bool step) {
    widget.parent = input_state.current_widget;
    widget.self = input_state.next_id;

    widget.z = input_state.z;

    if(input_state.current_widget != NULL_WIDGET) widgets[input_state.current_widget]->children.push_back(input_state.next_id);

    if(step) {
        ++input_state.depth;
        input_state.current_widget = input_state.next_id;
    }

    widgets.emplace(input_state.next_id, std::make_unique<Type>(widget));

    widgets[input_state.next_id]->init();

    uint64_t ret = input_state.next_id;

    input_state.last_widget = ret;

    ++input_state.next_id;

    for(std::shared_ptr<axiom::text>& t : widget.text) {
        text.push_back(t);
    }

    return ret;
}

}