#pragma once

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <emscripten/key_codes.h>

#include <wrapper/context.hpp>

#include <array>
#include <string>
#include <unordered_map>

using uint = uint32_t;

namespace axiom {

extern std::unordered_map<std::string, axiom::input_code> input_map_key;
extern std::unordered_map<std::string, axiom::input_code> input_map_mouse_button;

}