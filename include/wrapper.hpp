#pragma once

#ifdef AXIOM_WEBGL
#include <wrapper/webgl/context.hpp>
#include <wrapper/webgl/wrapper.hpp>

#elifdef AXIOM_OPENGL
#include <wrapper/opengl/context.hpp>
#include <wrapper/opengl/wrapper.hpp>

#endif