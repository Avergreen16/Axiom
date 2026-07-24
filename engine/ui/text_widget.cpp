#include <ui/text_widget.hpp>
#include <ui/ui_system.hpp>

namespace axiom {

void text_widget::handle_inputs() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    vec4 range = ui_system.get_range(self);

    std::string str = text[0]->string;
    str = callback(str);

    text[0]->string = str;
}

void text_widget::mesh() {
    if(dirty) {
        axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

        vec4 range = ui_system.get_range(self);

        dirty = false;

        //text[0]->width = size.x;
        std::vector<ui_vertex> text_vertices = text[0]->mesh();
        std::vector<ui_vertex> select_vertices = text[0]->mesh_select();
        text_vertices.insert(text_vertices.end(), select_vertices.begin(), select_vertices.end());

        //std::cout << select_vertices.size() << "\n";

        for(ui_vertex& v : text_vertices) {
            v.pos.x += text[0]->position.x;
            v.pos.y += text[0]->position.y;
            v.pos.z = z;

            v.range = range;
        }

        vertices_before = text_vertices;
    }
}

void text_widget::get_y() {
    //text[0]->refresh();
}

void text_widget::set_str(std::string str) {
    /*
    if(text[0]->string != str) {
        text[0]->dirty = true;
        text[0]->string = str;
        dirty = true;

        get_y();
    }
    */
}

uint64_t text_widget::insert(std::string str, axiom::text_alignment alg, bool wrap, std::function<std::string(std::string)> callback) {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    text_widget widget;

    //

    std::shared_ptr<axiom::text> text(new axiom::text);
    
    text->string = str;
    text->wrap = wrap;
    text->alignment = alg;
    text->font = ui_system.font_assets[0];

    widget.text = {text};

    //

    widget.size = vec2(0.0f);
    widget.min_width = 0.0f;
    widget.max_width = FLT_MAX;
    widget.min_height = 0.0f;
    widget.max_height = 0.0f;
    widget.weight_height = 0.0f;
    widget.callback = std::move(callback);

    widget.position = vec2(0.0f);
    
    widget.layout_mode = axiom::layout_mode::VOID;
    widget.position_mode = ui_system.input_state.active_position;
    widget.buffer = ui_system.input_state.active_buffer;

    return ui_system.insert_widget(widget);
}

void text_widget::init() {
    axiom::ui_system* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();

    widget_constraint c;
    c.func = [this, ui_system]() {
        //text[0]->click_range = ivec4(position, position + size);
        if(size.x < text[0]->wrap_limits.x || size.x > text[0]->wrap_limits.y || size.x == 0.0f) {
            text[0]->size.x = size.x;
            text[0]->width = size.x;
            //text[0]->dirty = true;
            
            text[0]->mesh();

            size.y = text[0]->size.y;
        }

        float f = text[0]->wrap_limits.y;
        if(f == FLT_MAX) {
            f = text[0]->wrap_limits.x;
        }
        size.x = glm::clamp(size.x, text[0]->wrap_limits.x, f);
        size.y = text[0]->size.y;
        
        max_width = text[0]->max_width;
    };

    before.push_back(c);
    
    c.func = [this, ui_system]() {
        position = round(position);

        text[0]->position = position;
    };
    after.push_back(c);
}

}