#ifndef AF2_GRAPHICS_OPENGL_HPP
#define AF2_GRAPHICS_OPENGL_HPP

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef near
#undef far

#endif

#include <GL/glew.h>

#endif
