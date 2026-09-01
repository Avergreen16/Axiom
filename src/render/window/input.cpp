#include <render/window/input.hpp>

#include <array>
#include <graphicsh.hpp>

namespace axiom {

std::array<axiom::input_code, GLFW_KEY_LAST + 1> get_glfw_key() {
    std::array<axiom::input_code, GLFW_KEY_LAST + 1> keys;
    std::fill(keys.begin(), keys.end(), axiom::input_code::UNKNOWN);

    keys[GLFW_KEY_0] = axiom::input_code::KEY_0;
    keys[GLFW_KEY_1] = axiom::input_code::KEY_1;
    keys[GLFW_KEY_2] = axiom::input_code::KEY_2;
    keys[GLFW_KEY_3] = axiom::input_code::KEY_3;
    keys[GLFW_KEY_4] = axiom::input_code::KEY_4;
    keys[GLFW_KEY_5] = axiom::input_code::KEY_5;
    keys[GLFW_KEY_6] = axiom::input_code::KEY_6;
    keys[GLFW_KEY_7] = axiom::input_code::KEY_7;
    keys[GLFW_KEY_8] = axiom::input_code::KEY_8;
    keys[GLFW_KEY_9] = axiom::input_code::KEY_9;
    
    keys[GLFW_KEY_A] = axiom::input_code::KEY_A;
    keys[GLFW_KEY_B] = axiom::input_code::KEY_B;
    keys[GLFW_KEY_C] = axiom::input_code::KEY_C;
    keys[GLFW_KEY_D] = axiom::input_code::KEY_D;
    keys[GLFW_KEY_E] = axiom::input_code::KEY_E;
    keys[GLFW_KEY_F] = axiom::input_code::KEY_F;
    keys[GLFW_KEY_G] = axiom::input_code::KEY_G;
    keys[GLFW_KEY_H] = axiom::input_code::KEY_H;
    keys[GLFW_KEY_I] = axiom::input_code::KEY_I;
    keys[GLFW_KEY_J] = axiom::input_code::KEY_J;
    keys[GLFW_KEY_K] = axiom::input_code::KEY_K;
    keys[GLFW_KEY_L] = axiom::input_code::KEY_L;
    keys[GLFW_KEY_M] = axiom::input_code::KEY_M;
    keys[GLFW_KEY_N] = axiom::input_code::KEY_N;
    keys[GLFW_KEY_O] = axiom::input_code::KEY_O;
    keys[GLFW_KEY_P] = axiom::input_code::KEY_P;
    keys[GLFW_KEY_Q] = axiom::input_code::KEY_Q;
    keys[GLFW_KEY_R] = axiom::input_code::KEY_R;
    keys[GLFW_KEY_S] = axiom::input_code::KEY_S;
    keys[GLFW_KEY_T] = axiom::input_code::KEY_T;
    keys[GLFW_KEY_U] = axiom::input_code::KEY_U;
    keys[GLFW_KEY_V] = axiom::input_code::KEY_V;
    keys[GLFW_KEY_W] = axiom::input_code::KEY_W;
    keys[GLFW_KEY_X] = axiom::input_code::KEY_X;
    keys[GLFW_KEY_Y] = axiom::input_code::KEY_Y;
    keys[GLFW_KEY_Z] = axiom::input_code::KEY_Z;
    
    keys[GLFW_KEY_ESCAPE] = axiom::input_code::KEY_ESCAPE;
    keys[GLFW_KEY_F1] = axiom::input_code::KEY_F1;
    keys[GLFW_KEY_F2] = axiom::input_code::KEY_F2;
    keys[GLFW_KEY_F3] = axiom::input_code::KEY_F3;
    keys[GLFW_KEY_F4] = axiom::input_code::KEY_F4;
    keys[GLFW_KEY_F5] = axiom::input_code::KEY_F5;
    keys[GLFW_KEY_F6] = axiom::input_code::KEY_F6;
    keys[GLFW_KEY_F7] = axiom::input_code::KEY_F7;
    keys[GLFW_KEY_F8] = axiom::input_code::KEY_F8;
    keys[GLFW_KEY_F9] = axiom::input_code::KEY_F9;
    keys[GLFW_KEY_F10] = axiom::input_code::KEY_F10;
    keys[GLFW_KEY_F11] = axiom::input_code::KEY_F11;
    keys[GLFW_KEY_F12] = axiom::input_code::KEY_F12;
    keys[GLFW_KEY_F13] = axiom::input_code::KEY_F13;
    keys[GLFW_KEY_F14] = axiom::input_code::KEY_F14;
    keys[GLFW_KEY_F15] = axiom::input_code::KEY_F15;
    keys[GLFW_KEY_F16] = axiom::input_code::KEY_F16;
    keys[GLFW_KEY_F17] = axiom::input_code::KEY_F17;
    keys[GLFW_KEY_F18] = axiom::input_code::KEY_F18;
    keys[GLFW_KEY_F19] = axiom::input_code::KEY_F19;
    keys[GLFW_KEY_F20] = axiom::input_code::KEY_F20;
    keys[GLFW_KEY_F21] = axiom::input_code::KEY_F21;
    keys[GLFW_KEY_F22] = axiom::input_code::KEY_F22;
    keys[GLFW_KEY_F23] = axiom::input_code::KEY_F23;
    keys[GLFW_KEY_F24] = axiom::input_code::KEY_F24;
    keys[GLFW_KEY_F25] = axiom::input_code::KEY_F25;
    
    keys[GLFW_KEY_UP] = axiom::input_code::KEY_UP_ARROW;
    keys[GLFW_KEY_DOWN] = axiom::input_code::KEY_DOWN_ARROW;
    keys[GLFW_KEY_LEFT] = axiom::input_code::KEY_LEFT_ARROW;
    keys[GLFW_KEY_RIGHT] = axiom::input_code::KEY_RIGHT_ARROW;
    
    keys[GLFW_KEY_GRAVE_ACCENT] = axiom::input_code::KEY_GRAVE;
    keys[GLFW_KEY_TAB] = axiom::input_code::KEY_TAB;
    keys[GLFW_KEY_SPACE] = axiom::input_code::KEY_SPACE;
    keys[GLFW_KEY_MINUS] = axiom::input_code::KEY_MINUS;
    keys[GLFW_KEY_EQUAL] = axiom::input_code::KEY_EQUALS;
    keys[GLFW_KEY_COMMA] = axiom::input_code::KEY_COMMA;
    keys[GLFW_KEY_PERIOD] = axiom::input_code::KEY_PERIOD;
    keys[GLFW_KEY_SLASH] = axiom::input_code::KEY_SLASH;
    keys[GLFW_KEY_SEMICOLON] = axiom::input_code::KEY_SEMICOLON;
    keys[GLFW_KEY_APOSTROPHE] = axiom::input_code::KEY_QUOTE;
    keys[GLFW_KEY_LEFT_BRACKET] = axiom::input_code::KEY_LEFT_BRACKET;
    keys[GLFW_KEY_RIGHT_BRACKET] = axiom::input_code::KEY_RIGHT_BRACKET;
    keys[GLFW_KEY_BACKSLASH] = axiom::input_code::KEY_BACKSLASH;
    keys[GLFW_KEY_LEFT_SHIFT] = axiom::input_code::KEY_LEFT_SHIFT;
    keys[GLFW_KEY_RIGHT_SHIFT] = axiom::input_code::KEY_RIGHT_SHIFT;
    keys[GLFW_KEY_LEFT_CONTROL] = axiom::input_code::KEY_LEFT_CTRL;
    keys[GLFW_KEY_RIGHT_CONTROL] = axiom::input_code::KEY_RIGHT_CTRL;
    keys[GLFW_KEY_LEFT_SUPER] = axiom::input_code::KEY_LEFT_SUPER;
    keys[GLFW_KEY_RIGHT_SUPER] = axiom::input_code::KEY_RIGHT_SUPER;
    keys[GLFW_KEY_LEFT_ALT] = axiom::input_code::KEY_LEFT_ALT;
    keys[GLFW_KEY_RIGHT_ALT] = axiom::input_code::KEY_RIGHT_ALT;
    keys[GLFW_KEY_CAPS_LOCK] = axiom::input_code::KEY_CAPS_LOCK;
    keys[GLFW_KEY_PRINT_SCREEN] = axiom::input_code::KEY_PRINT_SCREEN;
    keys[GLFW_KEY_DELETE] = axiom::input_code::KEY_DELETE;
    keys[GLFW_KEY_BACKSPACE] = axiom::input_code::KEY_BACKSPACE;
    keys[GLFW_KEY_ENTER] = axiom::input_code::KEY_ENTER;
    
    keys[GLFW_KEY_KP_0] = axiom::input_code::KEY_PAD_0;
    keys[GLFW_KEY_KP_1] = axiom::input_code::KEY_PAD_1;
    keys[GLFW_KEY_KP_2] = axiom::input_code::KEY_PAD_2;
    keys[GLFW_KEY_KP_3] = axiom::input_code::KEY_PAD_3;
    keys[GLFW_KEY_KP_4] = axiom::input_code::KEY_PAD_4;
    keys[GLFW_KEY_KP_5] = axiom::input_code::KEY_PAD_5;
    keys[GLFW_KEY_KP_6] = axiom::input_code::KEY_PAD_6;
    keys[GLFW_KEY_KP_7] = axiom::input_code::KEY_PAD_7;
    keys[GLFW_KEY_KP_8] = axiom::input_code::KEY_PAD_8;
    keys[GLFW_KEY_KP_9] = axiom::input_code::KEY_PAD_9;

    keys[GLFW_KEY_NUM_LOCK] = axiom::input_code::KEY_NUM_LOCK;
    keys[GLFW_KEY_KP_ENTER] = axiom::input_code::KEY_PAD_ENTER;
    keys[GLFW_KEY_KP_DECIMAL] = axiom::input_code::KEY_PAD_POINT;
    keys[GLFW_KEY_KP_ADD] = axiom::input_code::KEY_PAD_ADD;
    keys[GLFW_KEY_KP_SUBTRACT] = axiom::input_code::KEY_PAD_SUBTRACT;
    keys[GLFW_KEY_KP_MULTIPLY] = axiom::input_code::KEY_PAD_MULTIPLY;
    keys[GLFW_KEY_KP_DIVIDE] = axiom::input_code::KEY_PAD_DIVIDE;
    keys[GLFW_KEY_KP_ENTER] = axiom::input_code::KEY_PAD_ENTER;

    // mouse

    return keys;
}

std::array<axiom::input_code, GLFW_MOUSE_BUTTON_LAST + 1> get_glfw_mouse_button() {
    std::array<axiom::input_code, GLFW_MOUSE_BUTTON_LAST + 1> buttons;
    std::fill(buttons.begin(), buttons.end(), axiom::input_code::UNKNOWN);

    // mouse
    
    buttons[GLFW_MOUSE_BUTTON_LEFT] = axiom::input_code::MOUSE_LEFT;
    buttons[GLFW_MOUSE_BUTTON_RIGHT] = axiom::input_code::MOUSE_RIGHT;
    buttons[GLFW_MOUSE_BUTTON_MIDDLE] = axiom::input_code::MOUSE_CENTER;

    //buttons[GLFW_MOUSE_BUTTON_1] = axiom::input_code::MOUSE_1;
    //buttons[GLFW_MOUSE_BUTTON_2] = axiom::input_code::MOUSE_2;
    //buttons[GLFW_MOUSE_BUTTON_3] = axiom::input_code::MOUSE_3;
    buttons[GLFW_MOUSE_BUTTON_4] = axiom::input_code::MOUSE_4;
    buttons[GLFW_MOUSE_BUTTON_5] = axiom::input_code::MOUSE_5;
    buttons[GLFW_MOUSE_BUTTON_6] = axiom::input_code::MOUSE_6;
    buttons[GLFW_MOUSE_BUTTON_7] = axiom::input_code::MOUSE_7;
    buttons[GLFW_MOUSE_BUTTON_8] = axiom::input_code::MOUSE_8;

    return buttons;
}

std::array<axiom::input_code, GLFW_KEY_LAST + 1> glfw_input_map_key = get_glfw_key();
std::array<axiom::input_code, GLFW_MOUSE_BUTTON_LAST + 1> glfw_input_map_mouse_button = get_glfw_mouse_button();

}