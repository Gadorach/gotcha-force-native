#pragma once
// Headless implementations of every seam.
//
// These are the reference behaviour for "no platform": they must be safe to call
// in any order and must never allocate unbounded resources. Tests use them as a
// complete, valid backend set, which is what proves the base code has no hidden
// platform dependency.
#include <string>
#include <vector>

#include "engine/core/backends.hpp"

namespace gf::headless {

class FileSystem final : public IFileSystem {
public:
    bool read(const std::string&, std::vector<u8>& out) override { out.clear(); return false; }
    bool write(const std::string&, const u8*, std::size_t) override { return false; }
    bool exists(const std::string&) override { return false; }
    bool erase(const std::string&) override { return false; }
};

class Paths final : public IUserPaths {
public:
    std::string save_dir() override { return "save"; }
    std::string config_dir() override { return "config"; }
    std::string cache_dir() override { return "cache"; }
    std::string asset_dir() override { return "assets"; }
};

class Log final : public ILogBackend {
public:
    // Keeps only counters so a headless run cannot grow memory without bound.
    u64 counts[5]{};
    void write(LogLevel level, const char*, const std::string&) override {
        const auto index = static_cast<unsigned>(level);
        if (index < 5) ++counts[index];
    }
    u64 total() const { return counts[0] + counts[1] + counts[2] + counts[3] + counts[4]; }
};

class Clock final : public IClock {
    u64 now_ = 0;
public:
    // Deterministic 60 Hz step: no wall clock, so tests are reproducible.
    u64 monotonic_ns() override { return now_ += 16666667ull; }
    void sleep_for_ns(u64 ns) override { now_ += ns; }
};

// Scriptable input: tests inject a snapshot, nothing is read from a device.
class Input final : public IInputBackend {
public:
    void pump() override { snapshot_ = staged_; }
    const InputSnapshot& snapshot() const override { return snapshot_; }
    const char* describe() const override { return "headless scripted"; }

    // Test surface: set the snapshot the next pump() will publish.
    InputSnapshot& stage() { return staged_; }
    void set_staged(const InputSnapshot& value) { staged_ = value; }
    void clear() { staged_.clear(); }
private:
    InputSnapshot staged_{};
    InputSnapshot snapshot_{};
};

class Haptics final : public IHaptics {
public:
    void rumble(unsigned, f32, f32) override {}
};

class Window final : public IWindowBackend {
    unsigned width_ = 640, height_ = 480;
public:
    bool create(const WindowConfig& config, std::string*) override {
        width_ = config.width;
        height_ = config.height;
        return config.width != 0 && config.height != 0;
    }
    void destroy() override {}
    bool pump_events() override { return true; }
    bool should_close() const override { return false; }
    void set_title(const std::string&) override {}
    void size(unsigned* width, unsigned* height) const override {
        if (width) *width = width_;
        if (height) *height = height_;
    }
    bool focused() const override { return true; }
};

class Audio final : public IAudioBackend {
    u64 frames_ = 0;
public:
    bool open(unsigned, unsigned) override { return true; }
    void close() override {}
    void submit(const s16*, std::size_t frames) override { frames_ += frames; }
    u64 frames_consumed() const override { return frames_; }
};

class Render final : public IRenderBackend {
public:
    bool initialize(const RenderConfig&, IWindowBackend*, std::string*) override { return true; }
    void shutdown() override {}
    RenderCapabilities capabilities() const override { return {0, false, false, "headless"}; }
    bool upload_vertices(const Vertex*, std::size_t) override { return true; }
    bool upload_texture(u32, const TextureDesc&) override { return true; }
    bool release_texture(u32 handle) override { return handle!=0; }
    bool upload_matrix(u32, const f32*) override { return true; }
    void begin_frame(const FrameDesc&) override {}
    void submit(const DrawCall&) override {}
    void end_frame() override {}
    void present() override {}
};

class Save final : public ISaveBackend {
public:
    bool load(u32, std::vector<u8>& out) override { out.clear(); return false; }
    bool store(u32, const u8*, std::size_t) override { return false; }
    bool erase(u32) override { return false; }
};

class Network final : public INetworkBackend {
public:
    bool open(const std::string&) override { return false; }
    void close() override {}
    bool send(const u8*, std::size_t) override { return false; }
    bool receive(std::vector<u8>&) override { return false; }
    bool connected() const override { return false; }
};

// Owns one instance of each headless backend.
class Bundle {
public:
    Bundle() : backends_{&window_, &render_, &audio_, &input_, &haptics_, &save_,
                         &network_, &files_, &paths_, &log_, &clock_} {}
    Backends get() const { return backends_; }
    Backends* operator->() { return &backends_; }
    Log& log() { return log_; }
    Clock& clock() { return clock_; }
    Input& input() { return input_; }
private:
    Window window_;
    Render render_;
    Audio audio_;
    Input input_;
    Haptics haptics_;
    Save save_;
    Network network_;
    FileSystem files_;
    Paths paths_;
    Log log_;
    Clock clock_;
    Backends backends_{};
};

} // namespace gf::headless
