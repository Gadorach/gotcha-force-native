// End-to-end session test: the testing milestone in one file.
//
// Exercises the whole path the milestone defines: mapped input drives a fixed
// step, the step moves a scene hierarchy, world matrices flow through the
// matrix palette into render submission, state round-trips through the save
// seam, and the whole frame is reproducible by digest.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "engine/core/context.hpp"
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "engine/gameplay/session.hpp"
#include "engine/graphics/render_engine.hpp"

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "session failure: %s\n", what);
        std::abort();
    }
}

// In-memory save store so the test needs no filesystem.
class MemorySave final : public gf::ISaveBackend {
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

struct Rig {
    gf::headless::Bundle headless;
    MemorySave saves;
    gf::RecordingRenderBackend recorder;
    std::unique_ptr<gf::Context> context;
    std::unique_ptr<gf::RenderEngine> render;
    std::unique_ptr<gf::Session> session;

    Rig() {
        gf::Backends backends = headless.get();
        backends.save = &saves;
        backends.render = &recorder;
        check(backends.complete(), "backend set complete");
        context = std::make_unique<gf::Context>(backends, 60);
        render = std::make_unique<gf::RenderEngine>(recorder);
        std::string error;
        check(render->initialize({640, 480, false, true, "session"}, context->backends().window, &error),
              "render initialize");
        session = std::make_unique<gf::Session>(*context, *render);
        session->bootstrap_scene();
    }
};

// Runs `ticks` steps with the given pad and returns the frame digest.
gf::u64 run(Rig& rig, const gf::GamePadState& pad, unsigned ticks, gf::u32 frame) {
    for (unsigned i = 0; i < ticks; ++i) {
        rig.context->add_elapsed(rig.context->tick_ns());
        while (rig.context->tick_ready()) {
            rig.context->consume_tick();
            rig.session->tick(pad);
        }
    }
    const unsigned draws = rig.session->render_frame(frame);
    check(draws == 2, "both drawables submitted");
    return rig.recorder.last_frame_digest();
}

} // namespace

int main() {
    // 1. A bootstrapped scene renders geometry, not an empty frame.
    Rig rig;
    check(rig.session->world().size() == 2, "scene has a root and a child");
    const gf::u64 idle = run(rig, {}, 1, 0);
    check(rig.recorder.draws() == 2, "two draws issued in the first frame");
    check(idle != 0, "digest produced");

    // 2. Input moves the world, and the change is visible in the frame digest.
    gf::GamePadState push;
    push.stick_x = 64;
    const gf::u64 moved = run(rig, push, 4, 1);
    check(moved != idle, "moving the root changes the submitted frame");

    // 3. Determinism: a fresh rig with the same input stream matches exactly.
    {
        Rig other;
        const gf::u64 a = run(other, {}, 1, 0);
        const gf::u64 b = run(other, push, 4, 1);
        check(a == idle, "idle frame reproducible across sessions");
        check(b == moved, "moved frame reproducible across sessions");
    }

    // 4. Save / load round-trips the scene and keeps rendering identical.
    {
        const gf::u64 before = run(rig, push, 3, 2);
        check(rig.session->save(1), "save written");
        gf::u64 after = 0;
        {
            // Same rig, but reload from the stored state and render again.
            check(rig.session->load(1), "save loaded");
            after = run(rig, {}, 0, 2);
        }
        check(after == before, "reloaded state submits the same frame");
        // Trailing-byte corruption must be rejected rather than half-applied.
        rig.saves.slots[2] = rig.saves.slots[1];
        rig.saves.slots[2].push_back(0x7F);
        check(!rig.session->load(2), "trailing bytes rejected");
        rig.saves.slots[3] = {};
        check(!rig.session->load(3), "empty record rejected");
        check(!rig.session->load(99), "missing slot rejected");
    }

    // 5. Nothing leaks between loads: a load fully replaces world state.
    {
        const std::size_t size_before = rig.session->world().size();
        check(rig.session->load(1), "reload");
        check(rig.session->world().size() == size_before, "world size stable across reload");
        check(rig.session->world().world_matrix(1) != nullptr, "matrices rebuilt after load");
    }

    // 6. Input mapping is the only place a device becomes game input, and it is
    //    deterministic for identical raw state.
    {
        gf::InputSnapshot devices;
        devices.port_count = 1;
        devices.pad(0).buttons = 0x1;      // A
        devices.pad(0).axis[0] = 30000;
        const gf::GamePadState one = rig.session->router().route(devices, 0);
        const gf::GamePadState two = rig.session->router().route(devices, 0);
        check(one == two, "input routing is deterministic");
        check(one.a, "A bit maps to the A button");
        check(one.stick_x > 0, "positive stick maps positive");
        rig.session->tick(one);
        check(rig.session->tick() >= 1, "tick counter advances");
    }

    std::printf("{\"session_ticks\":%u,\"draws_per_frame\":2,\"deterministic\":true,"
                "\"save_roundtrip\":true,\"corrupt_rejects\":3,\"input_deterministic\":true,"
                "\"all_passed\":true}\n",
                rig.session->tick());
    return 0;
}
