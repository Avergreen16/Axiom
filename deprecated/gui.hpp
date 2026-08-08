#pragma once;

#include "wrapper.hpp"
#include "ecs.hpp"
#include "core.hpp"
#include "utility.hpp"

#include <string>

extern vec3 color_physics;
extern vec3 color_editor;
extern vec3 color_debug;
extern vec3 color_lua;

bool includes(ivec2 point, ivec4 range);
bool includes(ivec4 range_a, ivec4 range_b);

enum cursor_mode{CURSOR_CLICK, CURSOR_DRAG_T, CURSOR_DRAG_TR, CURSOR_DRAG_R, CURSOR_DRAG_BR, CURSOR_DRAG_B, CURSOR_DRAG_BL, CURSOR_DRAG_L, CURSOR_DRAG_TL, CURSOR_TEXT};
enum panel_split{SPLIT_X, SPLIT_Y, SPLIT_LEAF};
const uint64_t NULL_WIDGET = 0xFFFFFFFFFFFFFFFF;
const uint32_t NULL_OPERATION = 0xFFFFFFFF;

struct UI_vertex {
    vec3 pos;
    vec2 tex_pos;
    vec4 color = vec4(1.0f);
    vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
    uint data = 0;
};

// font

struct Glyph_data {
    std::vector<uint8_t> bitmap;
    bool visible = true;

    ivec2 size;
    ivec2 offset;
    int advance;
    
    ivec2 pos_tex;
};

struct Font {
    int line_height;
    Glyph_data empty_data = {{}, false, {0, 0}, {0, 0}, 0, {0, 0}};
    std::map<uint32_t, Glyph_data> glyph_map;

    Glyph_data& at(uint32_t key);

    void init(std::string filepath);

    Font(std::string filepath);

    Font() = default;
    Font(const Font& f) = default;
    Font(Font&& f) = default;
};

extern vec2 text_range;
extern std::vector<uint32_t> text_start;

enum ALIGNMENT{ALIGNMENT_LEFT, ALIGNMENT_CENTER, ALIGNMENT_RIGHT};
std::vector<UI_vertex> mesh_text(Font& f, std::string text, uint32_t text_size, uint32_t width = 0xFFFFFFFF, ivec2 select_range = {-1, -1}, ALIGNMENT alignment = ALIGNMENT_LEFT, bool show_debug = false);
std::vector<float> compute_text_bounds(Font& f, std::string text, uint32_t text_size, uint32_t width, bool wrap, ALIGNMENT alignment);

// 
enum LAYOUT_MODE{LM_VOID, LM_ROW, LM_COLUMN, LM_GRID};
enum POSITION_MODE{PM_STATIC, PM_TOP_LEFT, PM_TOP_RIGHT, PM_BOTTOM_LEFT, PM_BOTTOM_RIGHT, PM_TOP_CENTER, PM_BOTTOM_CENTER, PM_CENTER_LEFT, PM_CENTER_RIGHT, PM_CENTER, PM_VOID};
enum SIZE_MODE{SM_STATIC, SM_FILL, SM_SURROUND};

struct Text_Line_Data {
    uint32_t start;
    bool bold = false;
    bool italic = false;
    vec3 color = vec3(1.0f);
};

struct Text {
    std::string string;
    ALIGNMENT alignment = ALIGNMENT_LEFT;

    bool wrap = true;
    bool selectable = true;
    bool editable = false;

    bool dirty = false;
    bool remesh = false;

    vec2 position = vec2(0.0f);
    vec2 size = vec2(0.0f);
    uint32_t width = 0xFFFFFFFF;
    float z = 0.0f;

    vec2 resize_range = vec2(0.0f); // range that the text has to go over/under to change its wrapping
    float max_width = 0.0f; // width of the text if no wrapping -> everything is on one line

    ivec2 select_range = ivec2(-1);
    int anchor = -1;

    ivec4 click_range = ivec4(-1);
    double start_cursor = 0.0;

    bool focused = false;

    std::vector<Text_Line_Data> line_data;
    std::vector<UI_vertex> vertices;
    std::vector<UI_vertex> vertices_select;

    std::vector<UI_vertex> get_vertices();
    void refresh();
    void mesh();
    void select(vec4 cursor_range);
    std::string retrieve();

    void call();
};

struct Widget_Constraint {
    std::function<void()> func;
};

struct Widget {
    uint64_t self;
    bool flag = false;

    vec2 position = vec2(0.0f);
    vec2 size = vec2(0.0f);
    vec2 next_position;
    vec2 next_size;
    vec4 buffer = vec4(0.0f);
    
    float z = 0.0f;
    
    vec4 child_region = vec4(0.0f);
    vec2 child_offset = vec2(0.0f);
    vec4 range;
    std::vector<UI_vertex> vertices_before;
    std::vector<UI_vertex> vertices_after;

    std::function<float(std::unique_ptr<Widget>&)> get_height = [](std::unique_ptr<Widget>& w) {
        return w->size.y;
    };

    //

    LAYOUT_MODE layout_mode = LM_VOID;
    POSITION_MODE position_mode = PM_STATIC;
    SIZE_MODE size_mode = SM_STATIC;
    SIZE_MODE size_mode_y = SM_STATIC;

    //

    vec2 rel_position = vec2(0.0f);
    float min_width = 0.0f;
    float max_width = 0.0f;
    float weight_width = 1.0f;
    float min_height = 0.0f;
    float max_height = 0.0f;
    float weight_height = 1.0f;

    vec4 available_space;
    bool dirty = true;

    uint64_t parent = NULL_WIDGET;
    std::vector<uint64_t> children;

    std::vector<Text> texts;

    std::vector<Widget_Constraint> before;
    std::vector<Widget_Constraint> after;

    virtual void handle_inputs() {};
    virtual void mesh() {};
    virtual void get_y() {};
    virtual void init() {};
    
    virtual void on_measure() {};
    virtual void on_transform() {};
    virtual void on_place() {};

    virtual bool handle_capture() {
        return false;
    };
};

struct Copy_String {
    float y = 0.0f;
    std::string str;
};

struct GUI_system : System {
    std::unordered_map<std::string, Font> fonts;
    cursor_mode cursor_mode = CURSOR_CLICK;
    std::vector<UI_vertex> vertices;
    bool hex_mode = true;

    uint64_t next_id = 0;
    std::map<uint64_t, std::unique_ptr<Widget>> widgets;

    uint64_t current_widget = NULL_WIDGET;
    uint64_t last_widget = NULL_WIDGET;

    POSITION_MODE active_position = PM_TOP_LEFT;
    vec4 active_buffer = vec4(0.0f);

    //

    bool copy = false;
    bool paste = false;
    std::vector<Copy_String> copy_strings;
    
    bool isolate_selection = false;

    vec2 cursor_pos;
    vec2 cursor_anchor;
    std::vector<std::pair<uint64_t, uint32_t>> text_selected;

    uint64_t hover_capture = NULL_WIDGET;
    uint64_t click_capture = NULL_WIDGET;
    uint64_t text_capture = NULL_WIDGET;

    std::vector<uint64_t> delete_buffer;

    //

    GUI_system() = default;
    void init();

    template<typename Type>
    uint64_t insert_widget(Type widget, bool step = false);
    void step();
    void position(POSITION_MODE mode);
    void buffer(vec4 buffer);
    void make_dirty(uint64_t root);

    vec4 get_range(uint64_t v);
    void call();
    void solve_constraints();
    std::vector<uint64_t> get_children(uint64_t root);
    void erase(std::vector<uint64_t> ws);

    void set_attrib(vec2 min_size, vec2 max_size, vec2 weights);
    
    void handle_capture();
};

template<typename Type>
uint64_t GUI_system::insert_widget(Type widget, bool step) {
    widget.parent = current_widget;
    widget.self = next_id;

    if(current_widget != NULL_WIDGET) widgets[current_widget]->children.push_back(next_id);

    if(step) current_widget = next_id;

    widgets.emplace(next_id, std::make_unique<Type>(widget));

    widgets[next_id]->init();

    uint64_t ret = next_id;

    last_widget = ret;

    ++next_id;

    return ret;
}

//

struct Window_Widget : Widget {
    vec3 header_color;

    uint32_t header = 24;
    uint32_t resize_border = 6;
    float shadow_width = 6;
    std::string label;

    bool hover_close = false;
    uint32_t operation = NULL_OPERATION;

    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();

    static uint64_t insert(std::string label, vec2 size, vec2 position, vec3 color);
};

struct Debug_Widget : Widget {
    vec3 color;
    
    void mesh();

    static uint64_t insert(vec2 size, float max_width, vec3 color);
};

struct Row_Widget : Widget {
    float row = 0.0f;
    std::vector<float> columns;
    std::vector<float> column_buffers;

    bool fill = false;

    static uint64_t insert(bool fill = false);
    
    void init();
};

struct Column_Widget : Widget {
    std::vector<float> rows;
    std::vector<float> row_buffers;
    float column = 0.0f;

    bool fill = false;

    static uint64_t insert(bool fill = false);
    
    void init();
};

struct Grid_Widget : Widget {
    uint32_t num_columns;
    
    std::vector<float> rows;
    std::vector<float> columns;
    std::vector<float> row_buffers;
    std::vector<float> column_buffers;

    static uint64_t insert(uint32_t num_columns);

    void init();
};

enum PANEL_MODE{PANEL_MODE_SCALE, PANEL_MODE_SIZE};

struct Panel_Constraint {
    float value = 1.0f;
    PANEL_MODE panel_mode = PANEL_MODE_SCALE;
};

struct Split_Widget : Widget {
    std::vector<Panel_Constraint> constraints;
    float prev_size = -1.0f;
    vec2 sep = vec2(0.0f);
    
    uint32_t operation = 0;

    void handle_inputs();
    void recalibrate();
    void process_size();
    bool handle_capture();

    void init();

    static uint64_t insert(LAYOUT_MODE layout, std::vector<Panel_Constraint> constraints);
};

struct Panel_Widget : Widget {
    float total_scrollable = 0.0f;
    float scroll_width;
    bool reserve = false;
    float scroll_anchor = 0.0f;

    float scroll_pos = 0.0f;

    bool capture_scroll = false;

    void handle_inputs();
    void mesh();
    void on_transform();
    void on_place();
    void init();
    bool handle_capture();

    static uint64_t insert(float scroll_width, bool reserve);
};

struct Text_Widget : Widget {
    std::function<std::string(std::string)> callback;

    void handle_inputs();
    void mesh();
    void get_y();
    void set_str(std::string str);
    void init();

    static uint64_t insert(std::string str, ALIGNMENT alg, bool wrap = true, std::function<std::string(std::string)> callback = [](std::string str) {return str;});
};

struct Text_Input_Widget : Widget {
    std::string text;
    uint32_t text_size = 1;
    float text_width = 0.0f;
    float text_x = 0.0f;
    ALIGNMENT alignment = ALIGNMENT_LEFT;
    vec2 resize_range = vec2(0.0f, 0.0f);
    bool text_dirty = true;

    uint32_t cursor = 0xFFFFFFFF;
    uint32_t selection_anchor = 0xFFFFFFFF;
    bool wraparound = false;
    vec2 cursor_pos;
    vec2 click_pos;
    bool click = false;
    std::vector<Text_Line_Data> line_indices;

    std::vector<UI_vertex> text_vertices;

    void handle_inputs();
    void mesh();
    void get_y();
    void insert_cursor();
    void set_str(std::string str);
    void init();

    static uint64_t insert(std::string str, ALIGNMENT alg);
};

struct Screen_Widget : Widget {
    uint32_t header = 24;
    std::string label;
    vec2 text_size;

    bool fullscreen = false;
    bool hover_minimize = false;
    bool hover_maximize = false;
    bool hover_close = false;

    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();

    static uint64_t insert();
};

struct Render_Target;

struct Render_Widget : Widget {
    uint32_t target;

    void mesh();
    void init();
    void handle_inputs();
    bool handle_capture();

    static uint64_t insert();
};

struct Button_Widget : Widget {
    bool text_dirty = true;
    bool hovered = false;
    bool pressed = false;
    bool held = false;

    std::string label;
    std::function<void(Button_Widget&)> callback;

    std::vector<UI_vertex> text_vertices;
    vec2 text_size;
    vec3 color;

    vec4 icon = vec4(0.0f);
    vec2 icon_size = vec2(20.0f);

    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();

    static uint64_t insert(vec2 size, vec3 color, std::string str, std::function<void(Button_Widget&)> callback = [](Button_Widget& w) {});
};

struct Slider_Widget : Widget {
    bool text_dirty = true;
    bool hovered = false;
    bool pressed = false;

    float current_value;
    vec2 range;
    float step;

    float slider_width;
    std::string label;
    std::function<void(Slider_Widget&)> callback;

    std::vector<UI_vertex> text_vertices;
    vec2 text_size;
    vec3 color;

    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();

    static uint64_t insert(vec2 size, float slider_width, vec3 color, vec2 range, float step, float start, std::string str, std::function<void(Slider_Widget&)> callback = [](Slider_Widget& w) {});
};

struct Tab {
    std::string label;
    float width;
    vec3 color;

    std::function<void()> swap = []() {};

    bool text_dirty = true;
    std::vector<UI_vertex> text_vertices;
    vec2 text_size;
};

struct Tab_Widget : Widget {
    float tab_height;
    float tab_sep;
    std::vector<Tab> tabs;
    uint32_t selected = 0;

    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();

    static uint64_t insert(float tab_height, float tab_sep, std::vector<Tab> tabs);
};

struct Drop_Option {
    std::string label;
    bool dirty = true;
    std::vector<UI_vertex> vertices;
    vec2 size;
};

struct Drop_Widget : Widget {
    uint32_t selected = 0;
    uint32_t hovered = 0xFFFFFFFF;
    
    vec3 color;
    bool drop_down = false;
    bool drop_direction = false;
    float drop_height;
    float drop_unit_height;
    float button_width;

    std::vector<Drop_Option> options;
    float scroll = 0.0f;
    float scroll_height = 0.0f;

    std::function<void(Drop_Widget&)> callback;

    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();
    
    static uint64_t insert(vec2 size, vec3 color, float w, float h, float h2, std::vector<std::string> options, uint32_t selected, std::function<void(Drop_Widget&)> callback = [](Drop_Widget& self) {});
};

struct Spacer_Widget : Widget {
    bool visual = false;

    void mesh();

    static uint64_t insert(vec2 min_size, vec2 max_size, bool visual = false);
};

template<typename Type>
struct Input_Box_Widget : Widget {
    Type value;
    std::function<void(Input_Box_Widget<Type>&)> callback;

    bool update = false;

    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();
    
    static uint64_t insert(vec2 size, Type value, std::function<void(Input_Box_Widget<Type>&)> callback = [](Input_Box_Widget<Type>& self) {});
};

struct Relative_Widget : Widget {
    std::function<void(Relative_Widget&)> callback;
    
    void init();
    
    static uint64_t insert(std::function<void(Relative_Widget&)> callback = [](Relative_Widget& self) {});
};

struct Menu_Node {
    std::string label;
    std::vector<Menu_Node> children;
    std::function<void()> callback = []() {};
};

struct Menu_Widget : Widget {
    vec3 color;
    float drop_height;
    float drop_unit_height;
    float button_width;

    uint32_t index = 0xFFFFFFFF;
    double timer = 0.0;

    std::shared_ptr<Menu_Node> root;
    std::vector<uint32_t> path;

    float scroll = 0.0f;
    float scroll_height = 0.0f;
    uint32_t hovered = 0xFFFFFFFF;
    uint32_t clicked = 0xFFFFFFFF;
    
    void handle_inputs();
    void mesh();
    void init();
    bool handle_capture();
    
    static uint64_t insert(vec2 position, float z, vec3 color, float w, float h, float h2, std::shared_ptr<Menu_Node> root, std::vector<uint32_t> path);
};

// capture data
struct Capture_Data {
    float z;
    bool text_capture = false;
};
extern Capture_Data capture_data;

#include "gui.tpp"
