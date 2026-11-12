#include <wrapper/webgl/context/input.hpp>

#include <array>

namespace axiom {

std::unordered_map<std::string, axiom::input_code> get_key() {
    std::unordered_map<std::string, axiom::input_code> keys;

    keys["Unidentified"] = axiom::input_code::UNKNOWN;
    keys[""] = axiom::input_code::UNKNOWN;

    keys["Digit0"] = axiom::input_code::KEY_0;
    keys["Digit1"] = axiom::input_code::KEY_1;
    keys["Digit2"] = axiom::input_code::KEY_2;
    keys["Digit3"] = axiom::input_code::KEY_3;
    keys["Digit4"] = axiom::input_code::KEY_4;
    keys["Digit5"] = axiom::input_code::KEY_5;
    keys["Digit6"] = axiom::input_code::KEY_6;
    keys["Digit7"] = axiom::input_code::KEY_7;
    keys["Digit8"] = axiom::input_code::KEY_8;
    keys["Digit9"] = axiom::input_code::KEY_9;
    
    keys["KeyA"] = axiom::input_code::KEY_A;
    keys["KeyB"] = axiom::input_code::KEY_B;
    keys["KeyC"] = axiom::input_code::KEY_C;
    keys["KeyD"] = axiom::input_code::KEY_D;
    keys["KeyE"] = axiom::input_code::KEY_E;
    keys["KeyF"] = axiom::input_code::KEY_F;
    keys["KeyG"] = axiom::input_code::KEY_G;
    keys["KeyH"] = axiom::input_code::KEY_H;
    keys["KeyI"] = axiom::input_code::KEY_I;
    keys["KeyJ"] = axiom::input_code::KEY_J;
    keys["KeyK"] = axiom::input_code::KEY_K;
    keys["KeyL"] = axiom::input_code::KEY_L;
    keys["KeyM"] = axiom::input_code::KEY_M;
    keys["KeyN"] = axiom::input_code::KEY_N;
    keys["KeyO"] = axiom::input_code::KEY_O;
    keys["KeyP"] = axiom::input_code::KEY_P;
    keys["KeyQ"] = axiom::input_code::KEY_Q;
    keys["KeyR"] = axiom::input_code::KEY_R;
    keys["KeyS"] = axiom::input_code::KEY_S;
    keys["KeyT"] = axiom::input_code::KEY_T;
    keys["KeyU"] = axiom::input_code::KEY_U;
    keys["KeyV"] = axiom::input_code::KEY_V;
    keys["KeyW"] = axiom::input_code::KEY_W;
    keys["KeyX"] = axiom::input_code::KEY_X;
    keys["KeyY"] = axiom::input_code::KEY_Y;
    keys["KeyZ"] = axiom::input_code::KEY_Z;
    
    /*
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
    */

    // mouse

    return keys;
}

std::unordered_map<std::string, axiom::input_code> get_mouse_button() {
    std::unordered_map<std::string, axiom::input_code> buttons;

    // mouse

    /*
    
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
    */

    return buttons;
}

std::unordered_map<std::string, axiom::input_code> input_map_key = get_key();
std::unordered_map<std::string, axiom::input_code> input_map_mouse_button = get_mouse_button();

}