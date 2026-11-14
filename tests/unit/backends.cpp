// Backend-seam tests: the rewrite only works if every platform system can be
// replaced without touching game code. These tests swap in fakes for all eleven
// seams and assert the contracts base code relies on.
// No SDL, Vulkan, GL, emulator or game data involved.
#include "engine/core/backends.hpp"
#include "engine/core/context.hpp"
#include "engine/core/headless_backends.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/net/net_engine.hpp"
#include "engine/save/save_engine.hpp"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "backend seam failure: %s\n", what);
        std::abort();
    }
}

// ---- fakes -----------------------------------------------------------
class FakeWindow final : public gf::IWindowBackend {
public:
    unsigned w = 640, h = 480;
    bool closed = false;
    std::string title;
    int swaps = 0;
    bool create(const gf::WindowConfig& c, std::string*) override { w = c.width; h = c.height; return true; }
    void destroy() override {}
    bool pump_events() override { return !closed; }
    bool should_close() const override { return closed; }
    void set_title(const std::string& t) override { title = t; }
    void size(unsigned* width, unsigned* height) const override { if (width) *width = w; if (height) *height = h; }
    bool focused() const override { return true; }
    void swap_buffers() override { ++swaps; }
    void* native_handle() override { return this; }
};

class FakeRender final : public gf::IRenderBackend {
public:
    struct Event { int kind; gf::DrawCall call; };
    std::vector<Event> events;
    std::vector<gf::Vertex> vertices;
    std::map<gf::u32, gf::u32> textures;
    std::map<gf::u32, gf::f32> matrices;
    int frames = 0;
    bool saw_window = false;

    bool initialize(const gf::RenderConfig& config, gf::IWindowBackend* window, std::string* error) override {
        if (config.width == 0) { if (error) *error = "zero width"; return false; }
        saw_window = window != nullptr;
        return true;
    }
    void shutdown() override {}
    gf::RenderCapabilities capabilities() const override { return {4096, false, false, "fake"}; }
    bool upload_vertices(const gf::Vertex* v, std::size_t count) override {
        vertices.assign(v, v + count);
        return true;
    }
    bool upload_texture(gf::u32 handle, const gf::TextureDesc& desc) override {
        textures[handle] = desc.width * desc.height;
        return handle && desc.pixels;
    }
    bool upload_matrix(gf::u32 slot, const gf::f32* matrix) override {
        matrices[slot] = matrix ? matrix[12] : 0xFFFFFFFFu;
        return matrix != nullptr;
    }
    void begin_frame(const gf::FrameDesc&) override { ++frames; }
    void submit(const gf::DrawCall& call) override { events.push_back({1, call}); }
    void end_frame() override { events.push_back({2, {}}); }
    void present() override { events.push_back({3, {}}); }
};

class FakeAudio final : public gf::IAudioBackend {
public:
    gf::u64 frames = 0;
    bool is_open = false;
    bool open(unsigned, unsigned) override { is_open = true; return true; }
    void close() override { is_open = false; }
    void submit(const gf::s16*, std::size_t n) override { frames += n; }
    gf::u64 frames_consumed() const override { return frames; }
};

class FakeInput final : public gf::IInputBackend {
public:
    gf::InputSnapshot staged{};
    void pump() override { published = staged; }
    const gf::InputSnapshot& snapshot() const override { return published; }
private:
    gf::InputSnapshot published{};
};

class FakeHaptics final : public gf::IHaptics {
public:
    gf::f32 low = -1, high = -1;
    bool supported(unsigned) const override { return true; }
    void rumble(unsigned, gf::f32 l, gf::f32 h) override { low = l; high = h; }
};

class FakeSave final : public gf::ISaveBackend {
public:
    std::map<gf::u32, std::vector<gf::u8>> slots;
    bool load(gf::u32 slot, std::vector<gf::u8>& out) override {
        auto it = slots.find(slot);
        if (it == slots.end()) { out.clear(); return false; }
        out = it->second;
        return true;
    }
    bool store(gf::u32 slot, const gf::u8* data, std::size_t size) override {
        slots[slot] = std::vector<gf::u8>(data, data + size);
        return true;
    }
    bool erase(gf::u32 slot) override { return slots.erase(slot) != 0; }
};

class LoopbackNet final : public gf::INetworkBackend {
public:
    std::vector<std::vector<gf::u8>> queued;
    std::vector<std::vector<gf::u8>> sent;
    bool is_open = false;
    bool open(const std::string& endpoint) override { is_open = endpoint == "loopback"; return is_open; }
    void close() override { is_open = false; }
    bool connected() const override { return is_open; }
    bool send(const gf::u8* data, std::size_t size) override {
        if (!is_open) return false;
        sent.emplace_back(data, data + size);
        return true;
    }
    bool receive(std::vector<gf::u8>& out) override {
        if (queued.empty()) return false;
        out = queued.front();
        queued.erase(queued.begin());
        return true;
    }
};

class FakeFiles final : public gf::IFileSystem {
public:
    std::map<std::string, std::vector<gf::u8>> files;
    bool read(const std::string& n, std::vector<gf::u8>& out) override {
        auto it = files.find(n);
        if (it == files.end()) { out.clear(); return false; }
        out = it->second;
        return true;
    }
    bool write(const std::string& n, const gf::u8* d, std::size_t s) override {
        files[n] = std::vector<gf::u8>(d, d + s);
        return true;
    }
    bool exists(const std::string& n) override { return files.count(n) != 0; }
    bool erase(const std::string& n) override { return files.erase(n) != 0; }
};

class FakePaths final : public gf::IUserPaths {
public:
    std::string save_dir() override { return "/tmp/gf-save"; }
    std::string config_dir() override { return "/tmp/gf-config"; }
    std::string cache_dir() override { return "/tmp/gf-cache"; }
    std::string asset_dir() override { return "/tmp/gf-assets"; }
};

class CountingLog final : public gf::ILogBackend {
public:
    std::vector<std::string> messages;
    void write(gf::LogLevel, const char*, const std::string& m) override { messages.push_back(m); }
};

class FakeClock final : public gf::IClock {
public:
    gf::u64 now = 0;
    gf::u64 monotonic_ns() override { return now; }
    void sleep_for_ns(gf::u64 ns) override { now += ns; }
};

struct Swapped {
    FakeWindow window;
    FakeRender render;
    FakeAudio audio;
    FakeInput input;
    FakeHaptics haptics;
    FakeSave save;
    LoopbackNet network;
    FakeFiles files;
    FakePaths paths;
    CountingLog log;
    FakeClock clock;

    gf::Backends backends() {
        return gf::Backends{&window, &render, &audio, &input, &haptics, &save, &network,
                            &files, &paths, &log, &clock};
    }
};

} // namespace

int main() {
    // 1. The headless set is complete: base code has no hidden platform need.
    gf::headless::Bundle headless;
    gf::Context ctx(headless.get(), 60);
    check(ctx.backends().complete(), "headless backends must be complete");
    check(ctx.tick_ns() == 1000000000ull / 60, "tick derived from tick rate");
    ctx.add_elapsed(ctx.tick_ns() * 3);
    int ticks = 0;
    while (ctx.tick_ready()) { ctx.consume_tick(); ++ticks; }
    check(ticks == 3, "fixed-step accumulator yields whole ticks only");
    check(!ctx.tick_ready(), "no partial tick consumed");
    gf::headless::Log& log = headless.log();
    ctx.backends().log->write(gf::LogLevel::Info, "test", "hi");
    check(log.total() == 1, "headless log counts writes");

    // 2. Every seam swapped at once: base code cannot tell the difference.
    Swapped fake;
    gf::Backends backends = fake.backends();
    check(backends.complete(), "swapped set complete");
    gf::Context custom(backends, 60);

    // window
    check(custom.backends().window->create({320, 240, "title"}, nullptr), "window creates");
    unsigned w = 0, h = 0;
    custom.backends().window->size(&w, &h);
    check(w == 320 && h == 240, "window reports its size");

    // render: initialization receives the window, draws keep order and state.
    gf::RenderEngine engine(*backends.render);
    std::string error;
    check(engine.initialize({640, 480, false, true, "test"}, backends.window, &error), "render init");
    check(fake.render.saw_window, "renderer receives the window seam, not a native handle");
    check(engine.capabilities().name == "fake", "capabilities pass through");
    // Five vertices: a triangle followed by a two-vertex line list, so the
    // ordering test uses geometry that is actually in the arena.
    gf::Vertex triangle[5]{};
    triangle[0] = {0.0f, 0.0f, 0.0f, 255, 0, 0, 255, 0.0f, 0.0f};
    triangle[1] = {10.0f, 0.0f, 0.0f, 0, 255, 0, 255, 1.0f, 0.0f};
    triangle[2] = {0.0f, 10.0f, 0.0f, 0, 0, 255, 255, 0.0f, 1.0f};
    triangle[3] = {20.0f, 0.0f, 0.0f, 255, 255, 0, 255, 0.0f, 0.0f};
    triangle[4] = {30.0f, 0.0f, 0.0f, 255, 255, 0, 255, 0.0f, 0.0f};
    check(engine.upload_vertices(triangle, 5), "vertex upload");
    check(engine.flush_vertices(), "explicit vertex flush");
    check(fake.render.vertices.size() == 5, "backend received the vertex stream");
    check(!engine.upload_vertices(nullptr, 1), "null upload refused");
    const gf::u8 texel[4] = {1, 2, 3, 4};
    check(engine.upload_texture(7, {1, 1, texel}), "texture upload");
    check(fake.render.textures.count(7) == 1, "backend received the texture");
    check(!engine.upload_texture(0, {1, 1, texel}), "zero handle refused");
    check(!engine.upload_texture(8, {0, 0, texel}), "empty texture refused");

    engine.state().blend = true;
    engine.state().blend_src = 5;
    engine.state().blend_dst = 6;
    engine.begin_frame({1, 0.1f, 0.2f, 0.3f});
    check(engine.draw(gf::Primitive::Triangles, 0, 3), "draw accepted");
    check(engine.draw(gf::Primitive::Lines, 3, 2), "second draw accepted");
    check(!engine.draw(gf::Primitive::Triangles, 0, 0), "empty draw refused");
    check(!engine.draw(gf::Primitive::Triangles, 0xFFFFFFFFu, 4), "wrapping range refused");
    engine.end_frame();
    engine.present();
    check(fake.render.frames == 1, "one frame begun");
    std::vector<const gf::DrawCall*> calls;
    for (const auto& e : fake.render.events) if (e.kind == 1) calls.push_back(&e.call);
    check(calls.size() == 2, "both draws submitted");
    check(calls[1]->vertex_first == 3, "submission order preserved");
    check(calls[0]->blend && calls[0]->blend_src == 5, "state captured per draw");

    // 3. Save: round-trip plus fail-closed corruption handling.
    gf::SaveEngine saves(*backends.save);
    gf::SaveWriter writer;
    writer.u32_field(0xC0FFEEu);
    writer.f64_field(1234.5);
    writer.blob(reinterpret_cast<const gf::u8*>("profile"), 7);
    check(saves.write(3, writer), "save write");
    gf::SaveReader reader(fake.save.slots[3]);
    check(saves.read(3, reader), "save read");
    check(reader.u32_field() == 0xC0FFEEu, "u32 round-trip");
    check(reader.f64_field() == 1234.5, "u64 round-trip");
    auto blob = reader.blob();
    check(blob.size() == 7 && std::string(blob.begin(), blob.end()) == "profile", "blob round-trip");
    check(reader.consumed(), "payload fully consumed");

    int rejects = 0;
    auto corrupt = [&](auto mutate, const char* what) {
        FakeSave broken;
        broken.slots[1] = fake.save.slots[3];
        mutate(broken.slots[1]);
        gf::SaveEngine engine2(broken);
        gf::SaveReader r(broken.slots[1]);
        check(!engine2.read(1, r), what);
        ++rejects;
    };
    corrupt([](std::vector<gf::u8>& r) { r[0] ^= 0xFF; }, "bad magic rejected");
    corrupt([](std::vector<gf::u8>& r) { r[4] = 0x7F; }, "unknown version rejected");
    corrupt([](std::vector<gf::u8>& r) { r[8] = 0xFF; }, "oversized length rejected");
    corrupt([](std::vector<gf::u8>& r) { r.back() ^= 0x01; }, "checksum mismatch rejected");
    corrupt([](std::vector<gf::u8>& r) { r.resize(r.size() - 1); }, "truncation rejected");
    FakeSave missing;
    gf::SaveEngine engine3(missing);
    gf::SaveReader missing_reader(fake.save.slots[3]);
    check(!engine3.read(9, missing_reader), "missing slot rejected");
    ++rejects;

    // 4. Network framing.
    gf::NetEngine net(*backends.network);
    check(net.open("loopback"), "transport opens");
    const gf::u8 payload[] = {1, 2, 3, 4};
    check(net.send(0x1234u, payload, sizeof(payload)), "frame sent");
    fake.network.queued = fake.network.sent;
    gf::NetPacket packet;
    check(net.poll(packet), "frame received");
    check(packet.type == 0x1234u && packet.payload.size() == 4, "type and payload decoded");
    fake.network.queued.push_back({0, 0, 0});
    check(!net.poll(packet), "short frame refused");
    check(net.malformed() == 1, "malformed frame counted");
    check(!net.send(1, payload, gf::kMaxNetPayload + 1) && net.rejected() == 1, "oversized send refused");

    // 5. Remaining seams: audio, input, haptics, files, paths, log, clock.
    check(custom.audio().open(48000, 2), "audio opens");
    const gf::s16 samples[8]{};
    custom.audio().submit(samples, 4);
    check(custom.audio().frames_consumed() == 4, "audio frames counted");
    fake.input.staged.pad(0).buttons = 0x0100;
    fake.input.staged.pad(0).axis[0] = -1234;
    fake.input.staged.pointer.x = 77;
    fake.input.staged.push_touch(9, 12, 34);
    custom.input().pump();
    check(custom.input().snapshot().pad(0).buttons == 0x0100, "raw input passes through");
    check(custom.input().snapshot().pad(0).axis[0] == -1234, "analogue axis passes through");
    check(custom.input().snapshot().pointer.x == 77, "pointer passes through");
    check(custom.input().snapshot().touch_count == 1, "touch passes through");
    custom.haptics().rumble(0, 0.25f, 0.75f);
    check(fake.haptics.low == 0.25f && fake.haptics.high == 0.75f, "haptics pass through");
    const gf::u8 data[2] = {7, 9};
    check(custom.files().write("a/b.bin", data, 2), "file write");
    std::vector<gf::u8> readback;
    check(custom.files().read("a/b.bin", readback) && readback.size() == 2 && readback[1] == 9, "file round-trip");
    check(custom.backends().paths->save_dir() == "/tmp/gf-save", "paths pass through");
    custom.backends().log->write(gf::LogLevel::Warn, "cat", "message");
    check(fake.log.messages.size() == 1, "log passes through");
    fake.clock.now = 42;
    check(custom.clock().monotonic_ns() == 42, "clock passes through");
    custom.clock().sleep_for_ns(1000);
    check(fake.clock.now == 1042, "sleep passes through the clock seam");

    std::printf("{\"seams_complete\":11,\"headless_complete\":true,\"render_order\":true,"
                "\"save_rejects\":%d,\"net_framing\":true,\"raw_input\":true,\"all_passed\":true}\n",
                rejects);
    return 0;
}
