// Menu-world tests: the menu presentation is now a fixed-camera 3D set with
// billboard seals, so the interesting contracts are geometric, not textual.
//
// The recorder below implements the whole render seam, keeps every submission
// (draw parameters + the exact vertex range the engine had uploaded) and folds
// both into an FNV-1a hash. That makes "same state, same frame", "only the
// reticle moved" and "the 3D set is unchanged" checkable without a GPU: the hash
// of the first submission isolates the room/desk set, and the per-submission
// hashes are compared index by index.
//
// No SDL, GL, emulator or game data involved.
#include "engine/gameplay/menu_world.hpp"
#include "engine/graphics/render_engine.hpp"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace {

unsigned g_cases = 0;
unsigned g_draws = 0;
unsigned g_seals = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "menu-world failure: %s\n", what);
        std::abort();
    }
}

constexpr gf::u64 kFnvOffset = 1469598103934665603ull;
constexpr gf::u64 kFnvPrime = 1099511628211ull;

gf::u64 fnv_u32(gf::u64 hash, gf::u32 value) {
    hash ^= (value >> 0) & 0xFFu;
    hash *= kFnvPrime;
    hash ^= (value >> 8) & 0xFFu;
    hash *= kFnvPrime;
    hash ^= (value >> 16) & 0xFFu;
    hash *= kFnvPrime;
    hash ^= (value >> 24) & 0xFFu;
    hash *= kFnvPrime;
    return hash;
}

// ---------------------------------------------------------------- recorder
class RecordingBackend final : public gf::IRenderBackend {
public:
    struct Submission {
        gf::DrawCall call{};
        std::vector<gf::Vertex> vertices;
        gf::u64 hash = kFnvOffset;
    };

    bool initialize(const gf::RenderConfig& config, gf::IWindowBackend*,
                    std::string* error) override {
        if (config.width == 0) {
            if (error) *error = "zero width";
            return false;
        }
        return true;
    }
    void shutdown() override {}
    gf::RenderCapabilities capabilities() const override {
        return {2048, false, false, "menu-world-recording"};
    }

    bool upload_vertices(const gf::Vertex* vertices, std::size_t count) override {
        if (!vertices && count) return false;
        stream_.assign(vertices, vertices + count);
        uploaded_ += count;
        return true;
    }
    // The world binds no textures: an upload here would be a bug.
    bool upload_texture(gf::u32, const gf::TextureDesc&) override { return false; }
    bool upload_matrix(gf::u32 slot, const gf::f32* matrix) override {
        if (!matrix) return false;
        ++matrix_uploads_;
        last_matrix_slot_ = slot;
        return true;
    }

    void begin_frame(const gf::FrameDesc&) override {
        submissions_.clear();
        hash_ = kFnvOffset;
        draws_ = 0;
        vertex_count_ = 0;
        blended_draws_ = 0;
        depth_draws_ = 0;
        ++frames_;
    }
    void submit(const gf::DrawCall& call) override {
        Submission submission;
        submission.call = call;
        if (call.vertex_first <= stream_.size()) {
            const std::size_t available = stream_.size() - call.vertex_first;
            const std::size_t count =
                call.vertex_count < available ? call.vertex_count : available;
            const auto begin = stream_.begin() + static_cast<std::ptrdiff_t>(call.vertex_first);
            submission.vertices.assign(begin, begin + static_cast<std::ptrdiff_t>(count));
            vertex_count_ += count;
        }

        gf::u32 flags = 0;
        if (call.depth_test) flags |= 1u;
        if (call.depth_write) flags |= 2u;
        if (call.blend) flags |= 4u;
        if (call.cull_backface) flags |= 8u;
        // Two accumulators: `submission` hashes only this draw (so a test can
        // compare one submission between frames), `hash_` chains all of them into
        // an order-sensitive frame hash.
        gf::u64 submission_hash = kFnvOffset;
        const auto feed = [&](gf::u32 value) {
            submission_hash = fnv_u32(submission_hash, value);
            hash_ = fnv_u32(hash_, value);
        };
        feed(static_cast<gf::u32>(call.primitive));
        feed(call.vertex_first);
        feed(call.vertex_count);
        feed(call.position_matrix);
        feed(call.texture);
        feed(flags);
        feed((gf::u32(call.blend_src) << 8) | gf::u32(call.blend_dst));
        for (const gf::Vertex& v : submission.vertices) {
            feed(gf::f32_to_bits(v.x));
            feed(gf::f32_to_bits(v.y));
            feed(gf::f32_to_bits(v.z));
            feed((gf::u32(v.r) << 24) | (gf::u32(v.g) << 16) | (gf::u32(v.b) << 8) |
                 gf::u32(v.a));
            feed(gf::f32_to_bits(v.u));
            feed(gf::f32_to_bits(v.v));
        }
        submission.hash = submission_hash;

        if (call.blend) ++blended_draws_;
        if (call.depth_test) ++depth_draws_;
        ++draws_;
        submissions_.push_back(std::move(submission));
    }
    void end_frame() override {}
    void present() override {}

    unsigned draws() const { return draws_; }
    gf::u64 hash() const { return hash_; }
    std::size_t vertex_count() const { return vertex_count_; }
    std::size_t uploaded() const { return uploaded_; }
    unsigned blended_draws() const { return blended_draws_; }
    unsigned depth_draws() const { return depth_draws_; }
    unsigned frames() const { return frames_; }
    unsigned matrix_uploads() const { return matrix_uploads_; }
    gf::u32 last_matrix_slot() const { return last_matrix_slot_; }
    const std::vector<Submission>& submissions() const { return submissions_; }

private:
    std::vector<gf::Vertex> stream_;
    std::vector<Submission> submissions_;
    gf::u64 hash_ = kFnvOffset;
    unsigned draws_ = 0;
    std::size_t vertex_count_ = 0;
    std::size_t uploaded_ = 0;
    unsigned blended_draws_ = 0;
    unsigned depth_draws_ = 0;
    unsigned frames_ = 0;
    unsigned matrix_uploads_ = 0;
    gf::u32 last_matrix_slot_ = 0;
};

// ---------------------------------------------------------------- harness
struct Frame {
    unsigned draws = 0;
    gf::u64 hash = 0;
    std::vector<gf::u64> submission_hash;
    std::vector<std::size_t> submission_vertices;
    std::vector<gf::Vertex> first_vertex;
};

struct Fixture {
    RecordingBackend backend;
    gf::RenderEngine engine{backend};
    gf::MenuWorld world;
    gf::u32 frame = 0;

    Fixture() {
        world.configure(640, 480);
        world.open();
    }

    // One frame through the real RenderEngine, exactly as a host would drive it.
    Frame draw() {
        engine.reset_arena();
        engine.begin_frame({frame, 0.0f, 0.0f, 0.0f});
        ++frame;
        const unsigned issued = world.draw(engine);
        engine.end_frame();
        engine.present();
        check(issued == backend.draws(), "draw() returns the number of render.draw calls");

        Frame f;
        f.draws = backend.draws();
        f.hash = backend.hash();
        for (const RecordingBackend::Submission& s : backend.submissions()) {
            f.submission_hash.push_back(s.hash);
            f.submission_vertices.push_back(s.vertices.size());
            f.first_vertex.push_back(s.vertices.empty() ? gf::Vertex{} : s.vertices.front());
        }
        return f;
    }
};

// ---------------------------------------------------------------- inputs
gf::GamePadState neutral() { return {}; }
gf::GamePadState button_a() {
    gf::GamePadState p;
    p.a = true;
    return p;
}
gf::GamePadState button_b() {
    gf::GamePadState p;
    p.b = true;
    return p;
}
gf::GamePadState dpad_down() {
    gf::GamePadState p;
    p.dpad_down = true;
    return p;
}

// Press then release: one edge, and the menu sees a clean neutral afterwards.
void tap(gf::MenuWorld& world, gf::GamePadState down) {
    world.update(down);
    world.update(neutral());
}

bool same_colour(const gf::Vertex& a, const gf::Vertex& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// ---------------------------------------------------------------- cases
void test_unconfigured_draws_nothing() {
    ++g_cases;
    RecordingBackend backend;
    gf::RenderEngine engine{backend};
    gf::MenuWorld world;
    world.open();

    engine.begin_frame({0, 0.0f, 0.0f, 0.0f});
    check(world.draw(engine) == 0, "an unconfigured world draws nothing");
    check(backend.draws() == 0, "an unconfigured world submits no draws");
    check(backend.uploaded() == 0, "an unconfigured world uploads no vertices");

    // A zero or negative extent is not a viewport, so the world stays inert.
    world.configure(0, 480);
    check(world.draw(engine) == 0, "a zero width draws nothing");
    world.configure(640, 0);
    check(world.draw(engine) == 0, "a zero height draws nothing");
    world.configure(-4, -4);
    check(world.draw(engine) == 0, "a negative viewport draws nothing");
    check(backend.draws() == 0, "invalid viewports submit no draws");

    // A valid configure revives it, and the camera carries the surface size.
    world.configure(640, 480);
    check(world.camera().viewport_width == 640, "configure sets the camera width");
    check(world.camera().viewport_height == 480, "configure sets the camera height");
    check(world.draw(engine) > 0, "a configured world draws again");
}

void test_configured_scene() {
    ++g_cases;
    Fixture fx;
    const unsigned seals = fx.world.selection_count();
    check(seals == gf::kMenuItemCount, "the seal count follows the menu rows");

    const Frame f = fx.draw();
    g_draws = f.draws;
    g_seals = seals;

    check(f.draws > 0, "a configured world draws something");
    check(f.draws >= 1 + seals + 1 + seals, "set, seals, reticle and labels all draw");
    check(f.submission_vertices.size() == f.draws, "one recorded submission per draw");
    check(fx.backend.uploaded() > 0, "vertices reach the backend");

    // The room/desk set is one submission and holds twelve boxes.
    check(f.submission_vertices[0] == 12 * 36, "the 3D set draws twelve boxes");
    check(f.submission_vertices.size() == 3 + 2 * seals,
          "the frame is set + seals + reticle + logo + labels");

    // Blending is on for every pass; the geometry passes keep depth testing.
    check(fx.backend.blended_draws() == f.draws, "every draw blends");
    check(fx.backend.depth_draws() == 2 + seals, "set, seals and reticle test depth");
    check(fx.backend.matrix_uploads() > 0, "a matrix slot is uploaded");
    check(fx.backend.last_matrix_slot() == 0, "the identity matrix uses slot 0");
}

void test_seals_are_distinct_billboards() {
    ++g_cases;
    Fixture fx;
    const unsigned seals = fx.world.selection_count();
    check(seals >= 2, "at least two seals to compare");
    const Frame f = fx.draw();

    // Submission 0 is the set; seals follow; the reticle is the next one.
    const unsigned reticle_index = 1 + seals;
    check(f.submission_vertices.size() > reticle_index, "the reticle is submitted");

    for (unsigned i = 0; i < seals; ++i) {
        check(f.submission_vertices[1 + i] == 6, "each seal is one billboard quad");
        check(f.first_vertex[1 + i].a > 0, "the seal tint is visible");
        for (unsigned j = 0; j < i; ++j) {
            check(!same_colour(f.first_vertex[1 + i], f.first_vertex[1 + j]),
                  "every seal has its own tint");
        }
    }

    const gf::Vertex& reticle = f.first_vertex[reticle_index];
    check(f.submission_vertices[reticle_index] == 6 * gf::MenuWorld::kReticleSegments,
          "the reticle is a segmented ring");
    check(reticle.r > 200 && reticle.g < 160 && reticle.b < 120,
          "the reticle is red/orange");
    for (unsigned i = 0; i < seals; ++i) {
        check(!same_colour(reticle, f.first_vertex[1 + i]), "the reticle differs from every seal");
    }
}

void test_selection_moves_only_the_reticle() {
    ++g_cases;
    Fixture fx;
    tap(fx.world, button_a());  // Title -> MainMenu
    check(fx.world.screen() == gf::MenuScreen::MainMenu, "reached the main menu");
    check(fx.world.selection() == 0, "the main menu starts on the first row");

    const unsigned seals = fx.world.selection_count();
    const unsigned reticle_index = 1 + seals;
    const Frame before = fx.draw();

    tap(fx.world, dpad_down());
    check(fx.world.selection() == 1, "the selection moved");
    const Frame after = fx.draw();

    check(before.hash != after.hash, "a new selection is a new frame");
    check(before.submission_hash.size() == after.submission_hash.size(),
          "the submission count is unchanged");
    check(before.submission_hash[0] == after.submission_hash[0],
          "the 3D set alone is unchanged by the selection");
    for (unsigned i = 0; i < before.submission_hash.size(); ++i) {
        if (i == reticle_index) continue;
        check(before.submission_hash[i] == after.submission_hash[i],
              "only the reticle changes with the selection");
    }
    check(before.submission_hash[reticle_index] != after.submission_hash[reticle_index],
          "the reticle moved to the new seal");
}

void test_every_screen_draws() {
    ++g_cases;
    Fixture fx;

    const auto expect = [&](gf::MenuScreen want, bool quit) {
        check(fx.world.screen() == want, "the menu reached the expected screen");
        check(fx.world.quit_requested() == quit, "the quit flag matches the screen");
        const Frame f = fx.draw();
        check(f.draws > 0, "every menu screen renders");
        check(f.draws == g_draws, "the world's draw count is screen-independent");
    };

    expect(gf::MenuScreen::Title, false);

    tap(fx.world, button_a());
    expect(gf::MenuScreen::MainMenu, false);

    tap(fx.world, button_a());  // BATTLE
    expect(gf::MenuScreen::BattleSetup, false);

    tap(fx.world, button_b());
    tap(fx.world, dpad_down());  // TEAM
    tap(fx.world, button_a());
    expect(gf::MenuScreen::Team, false);

    tap(fx.world, button_b());
    check(fx.world.screen() == gf::MenuScreen::MainMenu, "b returns to the list");
    check(fx.world.selection() == 1, "the list keeps its selection");
    tap(fx.world, dpad_down());  // OPTIONS
    tap(fx.world, button_a());
    expect(gf::MenuScreen::Options, false);

    tap(fx.world, button_b());
    tap(fx.world, dpad_down());  // EXIT
    tap(fx.world, button_a());
    expect(gf::MenuScreen::Challenge, false);

    // Back out of the confirmation, then reopen from scratch.
    tap(fx.world, button_b());
    check(fx.world.screen() == gf::MenuScreen::MainMenu, "b leaves the quit screen");
    check(!fx.world.quit_requested(), "b withdraws the quit request");
    fx.world.open();
    check(fx.world.screen() == gf::MenuScreen::Title, "open() returns to the title screen");
    check(fx.world.selection() == 0, "open() clears the selection");
    check(!fx.world.quit_requested(), "open() clears the quit request");
    expect(gf::MenuScreen::Title, false);
}

void test_determinism() {
    ++g_cases;
    Fixture a;
    Fixture b;

    const gf::GamePadState script[] = {neutral(), button_a(), neutral(),   dpad_down(), neutral(),
                                       button_a(), button_b(), neutral(),  dpad_down(), neutral()};
    for (const gf::GamePadState& pad : script) {
        a.world.update(pad);
        b.world.update(pad);
        check(a.world.screen() == b.world.screen(), "same input, same screen");
        check(a.world.selection() == b.world.selection(), "same input, same selection");

        const Frame fa = a.draw();
        const Frame fb = b.draw();
        check(fa.draws == fb.draws, "identical worlds issue identical draw counts");
        check(fa.hash == fb.hash, "identical worlds produce an identical frame hash");
        check(fa.submission_hash == fb.submission_hash,
              "identical worlds submit identical geometry");
    }

    // Redrawing an unchanged world is byte-identical, which is what the frame
    // digest and any capture comparison rely on.
    const Frame first = a.draw();
    const Frame again = a.draw();
    check(first.hash == again.hash, "an unchanged world redraws identically");
}

void test_camera_projection() {
    ++g_cases;
    Fixture fx;
    const gf::Camera& camera = fx.world.camera();
    check(camera.viewport_width == 640 && camera.viewport_height == 480,
          "the camera looks at the configured surface");
    check(camera.near_plane > 0.0f && camera.far_plane > camera.near_plane,
          "the camera has a usable depth range");

    gf::f32 sx = 0.0f;
    gf::f32 sy = 0.0f;
    gf::f32 scale = 0.0f;
    check(camera.project(camera.target, &sx, &sy, &scale), "the desk centre is in front");
    check(sx >= 0.0f && sx <= 640.0f, "the desk centre projects inside the viewport (x)");
    check(sy >= 0.0f && sy <= 480.0f, "the desk centre projects inside the viewport (y)");
    check(scale > 0.0f, "a point in front has a positive perspective scale");
    check(camera.size_scale(150.0f, scale) > 0.0f, "a world size projects to a pixel size");

    // Mirroring the target through the eye puts it behind the camera.
    const gf::f32 behind[3] = {2.0f * camera.eye[0] - camera.target[0],
                               2.0f * camera.eye[1] - camera.target[1],
                               2.0f * camera.eye[2] - camera.target[2]};
    check(!camera.project(behind, &sx, &sy, &scale), "a point behind the camera is rejected");

    // A point beyond the far plane is rejected too, so the set never needs it.
    gf::f32 dir[3] = {camera.target[0] - camera.eye[0], camera.target[1] - camera.eye[1],
                      camera.target[2] - camera.eye[2]};
    const gf::f32 len = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    for (unsigned i = 0; i < 3; ++i) dir[i] /= len;
    const gf::f32 beyond[3] = {camera.eye[0] + dir[0] * (camera.far_plane + 10.0f),
                               camera.eye[1] + dir[1] * (camera.far_plane + 10.0f),
                               camera.eye[2] + dir[2] * (camera.far_plane + 10.0f)};
    check(!camera.project(beyond, &sx, &sy, &scale), "a point past the far plane is rejected");
}

void test_state_restored() {
    ++g_cases;
    Fixture fx;

    gf::RenderState before;
    before.depth_test = false;
    before.depth_write = false;
    before.blend = false;
    before.blend_src = 1;
    before.blend_dst = 2;
    before.cull_backface = true;
    before.texture = 7;
    before.position_matrix = 3;
    before.vertex_format = 9;
    fx.engine.state() = before;

    const Frame f = fx.draw();
    check(f.draws > 0, "the world draws under a caller-owned state");
    check(fx.backend.depth_draws() > 0, "geometry still used its own depth state");

    const gf::RenderState& after = fx.engine.state();
    check(after.depth_test == before.depth_test, "depth test restored");
    check(after.depth_write == before.depth_write, "depth write restored");
    check(after.blend == before.blend, "blend restored");
    check(after.blend_src == before.blend_src, "blend source restored");
    check(after.blend_dst == before.blend_dst, "blend destination restored");
    check(after.cull_backface == before.cull_backface, "culling restored");
    check(after.texture == before.texture, "texture binding restored");
    check(after.position_matrix == before.position_matrix, "matrix slot restored");
    check(after.vertex_format == before.vertex_format, "vertex format restored");
}

} // namespace

int main() {
    test_unconfigured_draws_nothing();
    test_configured_scene();
    test_seals_are_distinct_billboards();
    test_selection_moves_only_the_reticle();
    test_every_screen_draws();
    test_determinism();
    test_camera_projection();
    test_state_restored();

    std::printf("{\"menu_world_cases\":%u,\"draws\":%u,\"seals\":%u,\"seal_slots\":%u,"
                "\"all_passed\":true}\n",
                g_cases, g_draws, g_seals, gf::MenuWorld::kReferenceSealSlots);
    return 0;
}
