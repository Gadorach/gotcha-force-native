#pragma once
// Frame context and the injection point for subsystem backends.
//
// Game code takes a `Context&`. Nothing in the engine reaches for a global,
// which is what makes swapping a renderer, a save layer or a network transport
// a matter of constructing a different `Backends` value.
#include <memory>

#include "engine/core/backends.hpp"
#include "engine/core/types.hpp"

namespace gf {

class Context {
public:
    Context(Backends backends, u32 tick_rate)
        : backends_(backends), tick_rate_(tick_rate ? tick_rate : 60) {}

    Backends& backends() { return backends_; }
    const Backends& backends() const { return backends_; }

    IRenderBackend& render() const { return *backends_.render; }
    ISaveBackend& save() const { return *backends_.save; }
    INetworkBackend& network() const { return *backends_.network; }
    IHaptics& haptics() const { return *backends_.haptics; }
    IWindowBackend& window() const { return *backends_.window; }
    IUserPaths& paths() const { return *backends_.paths; }
    ILogBackend& log() const { return *backends_.log; }
    IAudioBackend& audio() const { return *backends_.audio; }
    IInputBackend& input() const { return *backends_.input; }
    IClock& clock() const { return *backends_.clock; }
    IFileSystem& files() const { return *backends_.files; }

    u32 tick_rate() const { return tick_rate_; }
    // Fixed simulation step in nanoseconds, derived from the tick rate.
    u64 tick_ns() const { return 1000000000ull / tick_rate_; }

    u32 frame_index() const { return frame_index_; }
    void advance_frame() { ++frame_index_; }

    // Deterministic accumulator for fixed-step logic; never sleeps.
    void add_elapsed(u64 ns) { accumulated_ns_ += ns; }
    bool tick_ready() const { return accumulated_ns_ >= tick_ns(); }
    void consume_tick() { accumulated_ns_ -= tick_ns(); }
    u64 accumulated_ns() const { return accumulated_ns_; }

private:
    Backends backends_{};
    u32 tick_rate_;
    u32 frame_index_ = 0;
    u64 accumulated_ns_ = 0;
};

} // namespace gf
