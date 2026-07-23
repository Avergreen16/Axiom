namespace axiom {

template<typename Type>
void input_box_widget<Type>::handle_inputs() {
    //if(includes(core.cursor_pos, vec4(position, position + size)) && core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_LEFT)) {
        
    //}

    GUI_system& gui_system = ecs.get_system<GUI_system>();
    
    texts[0].z = z;
    
    if(gui_system.text_selected.size() == 1 && gui_system.text_selected[0].first == self) { // input
        update = true;
    } else {
        if(update) {
            std::stringstream stream;
            stream << texts[0].string;

            Type prev = value;

            if(!(stream >> value)) {
                value = prev;
            }

            callback(*this);

            update = false;
        } else {
            callback(*this);

            std::stringstream stream;
            stream << value;

            std::string new_str = stream.str();

            if(new_str != texts[0].string || new_str == "") {
                texts[0].string = new_str;
                texts[0].dirty = true;
            }
        }
        
        auto a = compute_text_bounds(gui_system.fonts["default mono"], texts[0].string, 1, 0xFFFFFFFF, false, ALIGNMENT_LEFT);
        texts[0].size = {a[0], a[2]};
    }

    if(core.pressed_buttons.contains(GLFW_MOUSE_BUTTON_RIGHT) && includes(core.cursor_pos, vec4(position, position + size))) {
        std::shared_ptr<Menu_Node> node(new Menu_Node{
            "",
            {
                Menu_Node("COPY", {},
                    [this]() {
                        if(texts[0].select_range.x == texts[0].select_range.y) {
                            glfwSetClipboardString(core.window.window, texts[0].string.c_str());
                        } else {
                            std::string str = texts[0].retrieve();
                            glfwSetClipboardString(core.window.window, str.c_str());
                        }
                    }
                ),
                Menu_Node("PASTE", {},
                    [this]() {
                        if(texts[0].select_range.x == -1) {
                            texts[0].string = glfwGetClipboardString(core.window.window);
                            update = true;
                            
                            texts[0].dirty = true;
                            texts[0].remesh = true;
                        } else {
                            paste = true;
                        }
                    }
                ),
            }
        });
        
        ecs.get_system<GUI_system>().current_widget = NULL_WIDGET;
        uint64_t f = Menu_Widget::insert(core.cursor_pos + vec2(0.0f, -32.0f), 0.6f, color_editor * 0.9f, 160, 16, 32, node, {});
    }
}

template<typename Type>
void input_box_widget<Type>::mesh() {
    if(dirty) {
        GUI_system& gui_system = ecs.get_system<GUI_system>();

        vertices_before.clear();

        dirty = false;

        UI_vertex a = {vec3(0.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f)};
        UI_vertex b = {vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f)};
        UI_vertex c = {vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f)};
        UI_vertex d = {vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f)};
        std::vector<UI_vertex> ret = {a, b, d, a, d, c};

        vec4 range = vec4(position, size);
        
        for(UI_vertex& v : ret) {
            v.pos = vec3(range.xy() + v.pos.xy() * range.zw(), z);
            v.tex_pos = vec2(1.0f, 63.0f);
            v.color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
            v.data = 0x1;
        }
        vertices_before.insert(vertices_before.end(), ret.begin(), ret.end());

        //

        //
        std::vector<UI_vertex> text_vs = texts[0].get_vertices();
        vertices_before.insert(vertices_before.end(), text_vs.begin(), text_vs.end());
    }
}

template<typename Type>
void input_box_widget<Type>::init() {
    GUI_system* gui_system = &ecs.get_system<GUI_system>();

    //
    Widget_Constraint c;
    c.func = [this, gui_system]() {
        vec2 text_pos = position + vec2(2.0f, (size.y - texts[0].size.y) * 0.5f);
        texts[0].position = round(text_pos);
        texts[0].click_range = ivec4(position, position + size);
    };
    after.push_back(c);
}

template<typename Type>
uint64_t input_box_widget<Type>::insert(vec2 size, Type value, std::function<void(input_box_widget<Type>&)> callback) {
    GUI_system& gui_system = ecs.get_system<GUI_system>();

    input_box_widget<Type> widget;

    Text text;

    text.wrap = false;
    text.editable = true;

    widget.texts = {text};

    widget.value = value;
    widget.callback = callback;

    widget.size = size;
    widget.min_width = size.x;
    widget.max_width = size.x;
    widget.min_height = size.y;
    widget.max_height = size.y;

    widget.size_mode = SM_STATIC;
    widget.layout_mode = LM_VOID;
    widget.position_mode = gui_system.active_position;
    widget.buffer = gui_system.active_buffer;

    return gui_system.insert_widget(widget);
}

template<typename Type>
bool input_box_widget<Type>::handle_capture() {
    capture_data.z = z;

    std::vector<vec4> ranges = {
        vec4(position, position + size)
    };

    for(vec4 range : ranges) {
        if(includes(core.cursor_pos, range)) return true;
    }

    return false;
}

}