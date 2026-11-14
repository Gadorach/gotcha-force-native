#pragma once
// OpenGL 2.1 + GLSL 1.20 + EXT_framebuffer_object backend.
//
// This is the reference "real" backend: it proves the render seam is sufficient
// for genuine GPU work without the base code knowing anything about GL.
// Compiled only when the platform target provides a window with a GL context.
#include <memory>
#include <vector>

#include "engine/core/backends.hpp"

namespace gf::gl {

// `window` must expose a native handle created with a GL-capable surface;
// sdl_window.cpp provides one. Returns null when initialization fails.
std::unique_ptr<IRenderBackend> create_renderer(ILogBackend* log);

// Reads the back buffer BEFORE swap into RGBA8, bottom row first. write_ppm
// flips the rows. Requires a current context; errors and degenerate sizes fail.
bool capture_framebuffer(std::vector<u8>& rgba, s32* width, s32* height);

// Writes RGBA8 as a binary PPM (P6), flipping to top-down order. PPM keeps the
// capture dependency-free; convert to PNG with any image tool.
bool write_ppm(const char* path, const std::vector<u8>& rgba, s32 width, s32 height);

} // namespace gf::gl
