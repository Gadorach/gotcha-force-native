#pragma once
// Pluggable subsystem seams for the native rewrite.
//
// Design rule: the only thing shared between platforms is `engine/` plus these
// interfaces. Every system that touches an OS, a device or a GPU driver lives
// behind one of these and is compiled only in `platform/<target>/`.
//
// Isolation checklist for adding a new subsystem: if two platforms could differ
// in *how* it is done (not *what* it does), it needs a seam here. If the
// difference is only data (button tables, save layout, shader text), it belongs
// in reusable base code instead.
#include <cstddef>
#include <array>
#include <string>
#include <vector>

#include "engine/core/types.hpp"
#include "engine/input/input_snapshot.hpp"

namespace gf {

// ---------------------------------------------------------------- files
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual bool read(const std::string& name, std::vector<u8>& out) = 0;
    virtual bool file_size(const std::string&,u64*){return false;}
    virtual bool read_range(const std::string&,u64,std::size_t,std::vector<u8>& out){out.clear();return false;}
    virtual bool write(const std::string& name, const u8* data, std::size_t size) = 0;
    virtual bool exists(const std::string& name) = 0;
    virtual bool erase(const std::string& name) = 0;
};

// ---------------------------------------------------------------- user paths
// Save/config/cache locations differ per platform and per packaging model
// (portable folder, XDG, console slot). Base code only asks for a directory.
class IUserPaths {
public:
    virtual ~IUserPaths() = default;
    virtual std::string save_dir() = 0;
    virtual std::string config_dir() = 0;
    virtual std::string cache_dir() = 0;
    virtual std::string asset_dir() = 0;
};

// ---------------------------------------------------------------- logging
// Base code must never printf directly: a console, a file, a CI log and a
// headless run all want different sinks.
enum class LogLevel { Trace, Debug, Info, Warn, Error };

class ILogBackend {
public:
    virtual ~ILogBackend() = default;
    virtual void write(LogLevel level, const char* category, const std::string& message) = 0;
    virtual void flush() {}
};

// ---------------------------------------------------------------- time
class IClock {
public:
    virtual ~IClock() = default;
    virtual u64 monotonic_ns() = 0;
    virtual void sleep_for_ns(u64 ns) = 0;
};

// ---------------------------------------------------------------- haptics
class IHaptics {
public:
    virtual ~IHaptics() = default;
    virtual bool supported(unsigned) const { return false; }
    virtual void rumble(unsigned port, f32 low, f32 high) = 0;
    virtual void stop(unsigned port) { rumble(port, 0.0f, 0.0f); }
};

// ---------------------------------------------------------------- window
// Window creation, event pumping and surface lifetime. Separated from the
// renderer because a platform can offer a surface without offering the same
// rendering API, and headless runs have no window at all.
struct WindowConfig {
    unsigned width = 640;
    unsigned height = 480;
    std::string title;
    bool resizable = false;
    bool hidden = false;
};

class IWindowBackend {
public:
    virtual ~IWindowBackend() = default;
    virtual bool create(const WindowConfig& config, std::string* error) = 0;
    virtual void destroy() = 0;
    // Pump OS events; returns false when the platform wants the app to exit.
    virtual bool pump_events() = 0;
    virtual bool should_close() const = 0;
    virtual void set_title(const std::string& title) = 0;
    virtual void size(unsigned* width, unsigned* height) const = 0;
    // Pointer events use logical window coordinates, not drawable pixels.
    // Backends without display scaling may use the same dimensions for both.
    virtual void logical_size(unsigned* width,unsigned* height)const{size(width,height);}
    virtual bool focused() const = 0;
    virtual void swap_buffers() {}
    // Opaque native handle (SDL_Window*, HWND, X11 window...). Only the matching
    // renderer backend may interpret it.
    virtual void* native_handle() { return nullptr; }
};

// ---------------------------------------------------------------- audio
class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;
    virtual bool open(unsigned sample_rate, unsigned channels) = 0;
    virtual void close() = 0;
    // Interleaved s16 at the rate/channels opened above. Format conversion is
    // base-code work; backends must not reinterpret the format.
    virtual void submit(const s16* interleaved, std::size_t frames) = 0;
    virtual u64 frames_consumed() const = 0;
    virtual u64 frames_available() const { return 0; }
    // A backend output device is open. False is conservative for unknown sinks;
    // a fake/headless sink accepting PCM is not evidence of audible playback.
    virtual bool available() const { return false; }
    // Checked producer path. Unknown sinks refuse without effects; the legacy
    // submit API remains available for callers that don't need acceptance.
    virtual bool try_submit(const s16*,std::size_t){return false;}
    virtual bool discard_queued(){return false;}
    // Atomically pause/count/discard the queue and report precisely the frames
    // removed, not a racy pre-clear size. Needed for bus-selective restoration.
    // Unknown backends refuse without effects; no inference from legacy clear.
    virtual bool discard_queued_exact(u64*){return false;}
    virtual u64 frames_discarded()const{return 0;}
};

// ---------------------------------------------------------------- render
enum class Primitive { Triangles, TriangleStrip, Lines, Points };

// Portable vertex layout. Backends convert to their own format; base code owns
// the data and never sees a GPU handle.
struct Vertex {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    u8 r = 255, g = 255, b = 255, a = 255;
    f32 u = 0.0f;
    f32 v = 0.0f;
    // Reciprocal clip W. Screen-space UI keeps 1; projected 3D carries 1/depth.
    f32 inv_w = 1.0f;
    // Separate interpolated lighting channels: diffuse textures are combined
    // before illumination/specular, while EXT textures are combined afterwards.
    u8 light[4] = {255,255,255,255};
    u8 specular[4] = {};
};

// Portable texture upload. Base code owns the pixels; the backend owns the
// device object referenced by `handle` in a DrawCall.
struct TextureDesc {
    u32 width = 0;
    u32 height = 0;
    const u8* pixels = nullptr;   // tightly packed RGBA8, top-left origin
    u8 wrap_s = 1, wrap_t = 1;   // GX: clamp=0, repeat=1, mirror=2
    bool linear = true;
};

inline constexpr u32 kIdentityPositionMatrix=0xffffffffu; // explicit no-palette/identity selection
// Explicit linear eye-depth fog. Disabled is canonical zero/default; nonlinear,
// range-adjusted and orthographic source descriptors require separate admission.
struct LinearFog {bool enabled=false;f32 start=0,end=0;std::array<u8,3> colour{};bool operator==(const LinearFog&)const=default;};
struct DrawCall {
    Primitive primitive = Primitive::Triangles;
    u32 vertex_format = 0;      // base-code descriptor, not a GPU enum
    u32 vertex_first = 0;
    u32 vertex_count = 0;
    u32 position_matrix = 0;    // index into the caller's matrix palette
    u32 texture = 0;
    bool depth_test = true;
    bool depth_write = true;
    bool colour_write = true,alpha_write = true;
    bool blend = false;
    u8 blend_src = 0;
    u8 blend_dst = 0;
    bool cull_backface = true;
    // Alpha test: reject texels at or below `alpha_ref`. GX cut-outs (billboards,
    // foliage, seal discs) need this because their edges are transparent in the
    // texture, and blending alone would still write depth there.
    bool alpha_test = false;
    u8 alpha_ref = 0;
    bool cull_frontface = false;
    bool front_ccw = false;     // explicitly specified by the producer
    u8 depth_compare = 6;       // compare enum: >= for the reversed-depth seam
    u8 alpha_compare0 = 4, alpha_compare1 = 7, alpha_op = 0, alpha_ref1 = 0;
    // Default texture combiner subset, GX map-mode values. UI defaults modulate.
    u8 texture_colour_mode = 4, texture_alpha_mode = 3;
    f32 texture_weight = 1.0f;
    bool texture_colour_lerp = false;
    u32 texture_low_colour = 0, texture_high_colour = 0xFFFFFFFFu;
    bool texture_after_lighting = false;
    // Final clamped RGB addition after the material expression; alpha passes
    // through unchanged. Source fade-mobj uses this, not opacity multiplication.
    std::array<u8,3> post_rgb_add{};
    std::array<u8,3> post_rgb_multiply{255,255,255};
    LinearFog fog;
};

struct RenderConfig {
    unsigned width = 640;
    unsigned height = 480;
    bool vsync = false;
    bool depth_buffer = true;
    std::string title;
};

struct FrameDesc {
    u32 frame_index = 0;
    f32 clear_r = 0.0f;
    f32 clear_g = 0.0f;
    f32 clear_b = 0.0f;
};

struct RenderCapabilities {
    u32 max_texture_size = 0;
    bool supports_instancing = false;
    bool supports_depth_texture = false;
    // Human-readable backend identity for logs and bug reports.
    std::string name;
    bool supports_framebuffer_copy = false;
};

// Checked framebuffer -> owned RGBA8 texture. Rectangle is top-left-origin in
// reference units; backends map it to their drawable and resample to width/height.
// No GPU handle or pixel pointer crosses this boundary. Failure is NOT a capture.
struct FramebufferCopy {
    u32 texture=0,width=0,height=0;
    u32 x=0,y=0,source_width=0,source_height=0,reference_width=0,reference_height=0;
    bool linear=false;
};
inline bool framebuffer_copy_valid(const FramebufferCopy& c){
    return c.texture&&c.width&&c.height&&c.width<=4096&&c.height<=4096&&u64(c.width)*c.height<=16777216&&
        c.reference_width&&c.reference_height&&c.reference_width<=16384&&c.reference_height<=16384&&
        c.source_width&&c.source_height&&u64(c.x)+c.source_width<=c.reference_width&&u64(c.y)+c.source_height<=c.reference_height;
}

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;
    // `window` may be null for an offscreen/headless backend. Renderers that need
    // a surface must fail initialization rather than create one themselves.
    virtual bool initialize(const RenderConfig& config, IWindowBackend* window, std::string* error) = 0;
    virtual void shutdown() = 0;
    virtual RenderCapabilities capabilities() const { return {}; }
    // Replace the whole vertex stream. Index `vertex_first`/`vertex_count` in a
    // DrawCall refer to this stream.
    virtual bool upload_vertices(const Vertex* vertices, std::size_t count) = 0;
    virtual bool upload_texture(u32 handle, const TextureDesc& desc) = 0;
    // Idempotent release of a non-shared handle between frames. Backends
    // without an owned texture lifetime interface must refuse.
    virtual bool release_texture(u32) { return false; }
    // Only valid inside an active frame. Unknown/headless backends refuse; they
    // must not acknowledge a copy by accepting an unrelated texture upload.
    virtual bool copy_framebuffer(const FramebufferCopy&) { return false; }
    // Matrix palette entry, row-major affine 3x4 (12 floats). DrawCalls select a
    // slot by index; base code owns the matrices, the backend owns the upload.
    virtual bool upload_matrix(u32 slot, const f32* matrix) = 0;
    virtual void begin_frame(const FrameDesc& frame) = 0;
    virtual void submit(const DrawCall& call) = 0;
    virtual void end_frame() = 0;
    virtual void present() = 0;
};

// ---------------------------------------------------------------- save
enum class SaveReadStatus { Loaded,Missing,Error };
class ISaveBackend {
public:
    virtual ~ISaveBackend() = default;
    virtual bool load(u32 slot, std::vector<u8>& out) = 0;
    // Default fails conservatively when the backend cannot distinguish absence
    // from an I/O error. Callers must not overwrite an unreadable record merely
    // because it could not be loaded.
    virtual SaveReadStatus read_slot(u32 slot,std::vector<u8>& out){
        return load(slot,out)?SaveReadStatus::Loaded:SaveReadStatus::Error;
    }
    virtual bool store(u32 slot, const u8* data, std::size_t size) = 0;
    virtual bool erase(u32 slot) = 0;
};

// ---------------------------------------------------------------- network
struct NetPacket {
    u32 type = 0;
    std::vector<u8> payload;
};

class INetworkBackend {
public:
    virtual ~INetworkBackend() = default;
    virtual bool open(const std::string& endpoint) = 0;
    virtual void close() = 0;
    virtual bool send(const u8* data, std::size_t size) = 0;
    virtual bool receive(std::vector<u8>& out) = 0;
    virtual bool connected() const = 0;
};

// ---------------------------------------------------------------- aggregate
struct Backends {
    IWindowBackend* window = nullptr;
    IRenderBackend* render = nullptr;
    IAudioBackend* audio = nullptr;
    IInputBackend* input = nullptr;
    IHaptics* haptics = nullptr;
    ISaveBackend* save = nullptr;
    INetworkBackend* network = nullptr;
    IFileSystem* files = nullptr;
    IUserPaths* paths = nullptr;
    ILogBackend* log = nullptr;
    IClock* clock = nullptr;

    bool complete() const {
        return window && render && audio && input && haptics && save && network &&
               files && paths && log && clock;
    }
};

} // namespace gf
