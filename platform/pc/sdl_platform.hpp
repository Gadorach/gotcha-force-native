#pragma once
// SDL2 platform backends: window, input, haptics, paths, logging, clock.
//
// This is the Linux/desktop target. It deliberately does not create a renderer:
// the caller chooses one (currently OpenGL) and passes it in, so a software or
// Vulkan renderer can be used with the same window and input code.
#include <memory>

#include "engine/core/backends.hpp"

namespace gf::sdl {

// Host file/log seams usable without opening a window, controllers or audio.
std::unique_ptr<IFileSystem> create_file_system();
std::unique_ptr<ILogBackend> create_log_backend();
// Existing queued SDL sink without creating a window/input/controller bundle.
// Caller explicitly opens/closes it and owns the surrounding SDL lifetime.
std::unique_ptr<IAudioBackend> create_audio_backend();

// Owns the SDL subsystems for the lifetime of the bundle.
class Platform {
public:
    Platform();
    ~Platform();

    // Creates the window and input devices. `renderer` is borrowed, not owned:
    // it must stay alive while the platform is in use.
    Backends create(const WindowConfig& config, IRenderBackend* renderer, std::string* error);

    void pump_events();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gf::sdl
