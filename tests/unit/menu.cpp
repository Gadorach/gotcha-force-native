// Main-menu tests: the menu is the first screen a player touches, so its input
// edges, transitions and drawing have to be provable without a GPU.
//
// The recorder below implements the whole render seam, counts what was submitted
// and folds every submitted vertex into an FNV-1a hash, so "same state, same
// frame" and "different screen, different frame" are both checkable here.
#include "engine/gameplay/menu.hpp"
#include "engine/graphics/render_engine.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "menu failure: %s\n", what);
        std::abort();
    }
}

// ---------------------------------------------------------------- recorder
class RecordingBackend final : public gf::IRenderBackend {
public:
    bool initialize(const gf::RenderConfig& config, gf::IWindowBackend*, std::string* error) override {
        if (config.width == 0) {
            if (error) *error = "zero width";
            return false;
        }
        return true;
    }
    void shutdown() override {}
    gf::RenderCapabilities capabilities() const override { return {2048, false, false, "menu-recording"}; }

    bool upload_vertices(const gf::Vertex* vertices, std::size_t count) override {
        stream_.assign(vertices, vertices + count);
        uploaded_ += count;
        return true;
    }
    // The menu never binds a texture: any upload here would be a bug worth
    // failing on, so the recorder refuses them.
    bool upload_texture(gf::u32, const gf::TextureDesc&) override { return false; }
    bool upload_matrix(gf::u32 slot, const gf::f32* matrix) override {
        ++matrix_uploads_;
        last_matrix_slot_ = slot;
        return matrix != nullptr;
    }

    void begin_frame(const gf::FrameDesc&) override {
        hash_ = kOffset;
        draws_ = 0;
        vertex_count_ = 0;
        blended_draws_ = 0;
        depth_draws_ = 0;
        ++frames_;
    }
    void submit(const gf::DrawCall& call) override {
        ++draws_;
        if (call.blend) ++blended_draws_;
        if (call.depth_test) ++depth_draws_;
        add_u32(static_cast<gf::u32>(call.primitive));
        add_u32(call.vertex_first);
        add_u32(call.vertex_count);
        add_u32(call.position_matrix);
        add_u32(call.texture);
        if (call.vertex_first > stream_.size()) return;
        const std::size_t available = stream_.size() - call.vertex_first;
        const std::size_t count = call.vertex_count < available ? call.vertex_count : available;
        for (std::size_t i = 0; i < count; ++i) add_vertex(stream_[call.vertex_first + i]);
        vertex_count_ += count;
    }
    void end_frame() override {}
    void present() override {}

    gf::u64 hash() const { return hash_; }
    unsigned draws() const { return draws_; }
    std::size_t vertex_count() const { return vertex_count_; }
    std::size_t uploaded() const { return uploaded_; }
    unsigned matrix_uploads() const { return matrix_uploads_; }
    gf::u32 last_matrix_slot() const { return last_matrix_slot_; }
    unsigned blended_draws() const { return blended_draws_; }
    unsigned depth_draws() const { return depth_draws_; }
    unsigned frames() const { return frames_; }

private:
    static constexpr gf::u64 kOffset = 1469598103934665603ull;
    static constexpr gf::u64 kPrime = 1099511628211ull;

    void add_u32(gf::u32 value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            hash_ ^= (value >> shift) & 0xFFu;
            hash_ *= kPrime;
        }
    }
    void add_vertex(const gf::Vertex& v) {
        add_u32(gf::f32_to_bits(v.x));
        add_u32(gf::f32_to_bits(v.y));
        add_u32(gf::f32_to_bits(v.z));
        add_u32((gf::u32(v.r) << 24) | (gf::u32(v.g) << 16) | (gf::u32(v.b) << 8) | gf::u32(v.a));
        add_u32(gf::f32_to_bits(v.u));
        add_u32(gf::f32_to_bits(v.v));
    }

    std::vector<gf::Vertex> stream_;
    gf::u64 hash_ = kOffset;
    unsigned draws_ = 0;
    std::size_t vertex_count_ = 0;
    std::size_t uploaded_ = 0;
    unsigned matrix_uploads_ = 0;
    gf::u32 last_matrix_slot_ = 0;
    unsigned blended_draws_ = 0;
    unsigned depth_draws_ = 0;
    unsigned frames_ = 0;
};

// ---------------------------------------------------------------- harness
struct Snapshot {
    gf::u64 hash = 0;
    unsigned draws = 0;
    std::size_t vertices = 0;
};

struct Fixture {
    RecordingBackend backend;
    gf::RenderEngine engine{backend};
    gf::u32 frame = 0;

    // One frame through the real RenderEngine, exactly as a host would drive it.
    Snapshot draw(const gf::MainMenu& menu, gf::s32 width = 640, gf::s32 height = 480) {
        engine.reset_arena();
        engine.begin_frame({frame, 0.0f, 0.0f, 0.0f});
        ++frame;
        const unsigned issued = menu.draw(engine, width, height);
        engine.end_frame();
        engine.present();
        check(issued == backend.draws(), "draw() returns the number of draws it issued");
        return Snapshot{backend.hash(), backend.draws(), backend.vertex_count()};
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
gf::GamePadState button_start() {
    gf::GamePadState p;
    p.start = true;
    return p;
}
gf::GamePadState dpad_up() {
    gf::GamePadState p;
    p.dpad_up = true;
    return p;
}
gf::GamePadState dpad_down() {
    gf::GamePadState p;
    p.dpad_down = true;
    return p;
}
gf::GamePadState stick(gf::s32 y) {
    gf::GamePadState p;
    p.stick_y = y;
    return p;
}

// Press then release: one action, and the next press is a fresh edge.
void tap(gf::MainMenu& menu, gf::GamePadState down) {
    menu.update(down);
    menu.update(neutral());
}

// Drives a fresh menu to a screen through real input, so the tests never poke at
// private state.
gf::MainMenu menu_in_screen(gf::MenuScreen target) {
    gf::MainMenu menu;
    menu.open();
    if (target == gf::MenuScreen::Title) return menu;
    tap(menu, button_a());
    if (target == gf::MenuScreen::MainMenu) return menu;
    for (unsigned row = 0; row < gf::kMenuItemCount; ++row) {
        if (gf::MainMenu::screen_for(row) != target) continue;
        for (unsigned i = 0; i < row; ++i) tap(menu, dpad_down());
        tap(menu, button_a());
        return menu;
    }
    check(false, "requested screen is reachable from the main menu");
    return menu;
}

} // namespace

int main() {
    unsigned cases = 0;

    // 1. Fresh menu opens on the title screen with a known item table.
    ++cases;
    {
        gf::MainMenu menu;
        menu.open();
        check(menu.screen() == gf::MenuScreen::Title, "open() starts on the title screen");
        check(menu.selection() == 0, "open() clears the selection");
        check(!menu.quit_requested(), "open() clears the quit request");
        check(menu.selection_count() == 7, "seven main-menu seals");
        check(gf::kMenuItemCount == 7, "the item table has seven rows");
        check(menu.revision() == 1, "open() is the first visible change");
        check(std::string(gf::MainMenu::item_name(0)) == "OPTION", "row 0 is OPTION");
        check(std::string(gf::MainMenu::item_name(1)) == "COLLECTION", "row 1 is COLLECTION");
        check(std::string(gf::MainMenu::item_name(2)) == "TRADE", "row 2 is TRADE");
        check(std::string(gf::MainMenu::item_name(3)) == "CHALLENGE", "row 3 is CHALLENGE");
        check(std::string(gf::MainMenu::item_name(4)) == "EDIT FORCE", "row 4 is EDIT FORCE");
        check(std::string(gf::MainMenu::item_name(5)) == "STORY", "row 5 is STORY");
        check(std::string(gf::MainMenu::item_name(6)) == "VERSUS", "row 6 is VERSUS");
        check(std::string(gf::MainMenu::item_name(7)).empty(), "out-of-range row has no name");
        check(gf::MainMenu::screen_for(7) == gf::MenuScreen::MainMenu,
              "out-of-range row cannot invent a screen");
        menu.request_quit();
        check(menu.quit_requested() && menu.screen()==gf::MenuScreen::Quit,"explicit exit is separate");
        gf::GamePadState back;back.b=true;menu.update(back);
        check(!menu.quit_requested(),"back cancels explicit exit");
    }

    // 2. A press enters the main menu; `start` is equivalent to `a`.
    ++cases;
    {
        gf::MainMenu menu;
        menu.open();
        menu.update(neutral());
        check(menu.screen() == gf::MenuScreen::Title, "an idle frame stays on the title screen");
        menu.update(button_a());
        check(menu.screen() == gf::MenuScreen::MainMenu, "a confirms from the title screen");
        check(menu.revision() == 2, "one screen change is one revision");

        gf::MainMenu via_start;
        via_start.open();
        tap(via_start, button_start());
        check(via_start.screen() == gf::MenuScreen::MainMenu,
              "start confirms from the title screen");
    }

    // 3. Edge detection: a held button performs one action, however many frames.
    ++cases;
    {
        gf::MainMenu held;
        held.open();
        held.update(neutral());
        held.update(button_a());
        check(held.screen() == gf::MenuScreen::MainMenu, "first press enters the main menu");
        const gf::u32 after_enter = held.revision();
        // Keep the same button down: it was already used to leave the title
        // screen, so it must not also activate the first row.
        for (int i = 0; i < 8; ++i) held.update(button_a());
        check(held.screen() == gf::MenuScreen::MainMenu, "held a does not activate a row");
        check(held.revision() == after_enter, "held a does not bump the revision");

        // A held direction must not scroll either.
        gf::MainMenu scrolling;
        scrolling.open();
        tap(scrolling, button_a());
        scrolling.update(dpad_down());
        check(scrolling.selection() == 1, "first down press moves one row");
        for (int i = 0; i < 8; ++i) scrolling.update(dpad_down());
        check(scrolling.selection() == 1, "held down does not keep scrolling");

        // A held `b` must not walk back through screens either.
        gf::MainMenu back_held;
        back_held.open();
        tap(back_held, button_a());
        back_held.update(button_b());
        check(back_held.screen() == gf::MenuScreen::Title, "b returns to the title screen");
        const gf::u32 after_back = back_held.revision();
        for (int i = 0; i < 8; ++i) back_held.update(button_b());
        check(back_held.screen() == gf::MenuScreen::Title, "held b stays on the title screen");
        check(back_held.revision() == after_back, "held b does not bump the revision");
    }

    // 4. Selection wraps in both directions on d-pad and stick.
    ++cases;
    {
        gf::MainMenu menu;
        menu.open();
        tap(menu, button_a());
        check(menu.selection() == 0, "entering the main menu selects the first row");

        menu.update(dpad_up());
        check(menu.selection() == 6, "up from row 0 wraps to the last row");
        menu.update(neutral());
        menu.update(dpad_down());
        check(menu.selection() == 0, "down from the last row wraps to row 0");
        menu.update(neutral());

        for (unsigned i = 0; i < gf::kMenuItemCount; ++i) tap(menu, dpad_down());
        check(menu.selection() == 0, "seven down presses return to row 0");
        for (unsigned i = 0; i < gf::kMenuItemCount; ++i) tap(menu, dpad_up());
        check(menu.selection() == 0, "seven up presses return to row 0");

        menu.update(stick(120));
        check(menu.selection() == 6, "stick up wraps like the d-pad");
        menu.update(stick(0));
        menu.update(stick(-120));
        check(menu.selection() == 0, "stick down wraps like the d-pad");
        menu.update(stick(0));
        menu.update(stick(20));
        check(menu.selection() == 0, "a stick inside the threshold does not move");
    }

    // 5. Every row reaches its own screen, in table order.
    ++cases;
    {
        const gf::MenuScreen expected[7] = {
            gf::MenuScreen::Revolution, gf::MenuScreen::Collection, gf::MenuScreen::Trade,
            gf::MenuScreen::Challenge,  gf::MenuScreen::Force,      gf::MenuScreen::Story,
            gf::MenuScreen::CustomVersion};

        for (unsigned row = 0; row < gf::kMenuItemCount; ++row) {
            gf::MainMenu menu = menu_in_screen(gf::MenuScreen::MainMenu);
            for (unsigned i = 0; i < row; ++i) tap(menu, dpad_down());
            check(menu.selection() == row, "selection landed on the requested row");
            check(gf::MainMenu::screen_for(row) == expected[row],
                  "each row names its own destination");
            tap(menu, button_a());
            check(menu.screen() == expected[row], "each row opens its own screen");
        }
        for (unsigned i = 0; i < 7; ++i) {
            for (unsigned j = i + 1; j < 7; ++j) {
                check(expected[i] != expected[j], "every seal has its own screen");
            }
        }
        // The captured menu has no EXIT seal: the fourth seal is the ring's quit
        // slot, and the legacy `Quit` spelling names that same screen.
        check(gf::MenuScreen::Quit != gf::MenuScreen::Challenge,
              "exit is independent of CHALLENGE");
        // The pre-seven-row spellings still work for the presentation layer.
        check(gf::MenuScreen::BattleSetup == gf::MenuScreen::Revolution,
              "BattleSetup is a legacy spelling of Revolution");
        check(gf::MenuScreen::Team == gf::MenuScreen::Collection,
              "Team is a legacy spelling of Collection");
        check(gf::MenuScreen::Options == gf::MenuScreen::Trade,
              "Options is a legacy spelling of Trade");
    }

    // 6. `b` goes back, and the quit slot is the only row that requests a quit.
    ++cases;
    {
        const gf::MenuScreen subs[7] = {
            gf::MenuScreen::Revolution, gf::MenuScreen::Collection, gf::MenuScreen::Trade,
            gf::MenuScreen::Challenge,  gf::MenuScreen::Force,      gf::MenuScreen::Story,
            gf::MenuScreen::CustomVersion};
        for (const gf::MenuScreen screen : subs) {
            gf::MainMenu menu = menu_in_screen(screen);
            check(menu.screen() == screen, "reached the destination screen");
            check(!menu.quit_requested(), "no menu destination requests application exit");
            tap(menu, button_b());
            check(menu.screen() == gf::MenuScreen::MainMenu, "b returns to the main menu");
            check(!menu.quit_requested(), "leaving the quit screen withdraws the request");
        }

        // The selection survives a trip into a destination and back.
        gf::MainMenu keep = menu_in_screen(gf::MenuScreen::MainMenu);
        tap(keep, dpad_down());
        tap(keep, dpad_down());
        check(keep.selection() == 2, "moved to TRADE");
        tap(keep, button_a());
        check(keep.screen() == gf::MenuScreen::Trade, "opened trade");
        tap(keep, button_b());
        check(keep.selection() == 2, "selection is preserved on return");
    }

    // 7. Revision counts visible changes only.
    ++cases;
    {
        gf::MainMenu menu;
        menu.open();
        const gf::u32 opened = menu.revision();
        menu.update(neutral());
        menu.update(neutral());
        check(menu.revision() == opened, "identical idle frames do not bump the revision");

        menu.update(button_a());
        check(menu.revision() == opened + 1, "a screen change bumps the revision once");
        const gf::u32 entered = menu.revision();

        menu.update(neutral());
        menu.update(dpad_down());
        check(menu.revision() == entered + 1, "a selection change bumps the revision once");
        const gf::u32 moved = menu.revision();

        menu.update(dpad_down());
        check(menu.revision() == moved, "a held direction does not bump the revision");
        menu.update(neutral());
        menu.update(dpad_down());
        check(menu.revision() == moved + 1, "a fresh press moves and bumps the revision again");
    }

    // 8. Every screen draws, with the documented bar counts, and differs.
    ++cases;
    {
        Fixture fixture;
        const gf::MenuScreen screens[9] = {
            gf::MenuScreen::Title,      gf::MenuScreen::MainMenu, gf::MenuScreen::Revolution,
            gf::MenuScreen::Collection, gf::MenuScreen::Trade,    gf::MenuScreen::Challenge,
            gf::MenuScreen::Force,      gf::MenuScreen::Story,    gf::MenuScreen::CustomVersion};
        const unsigned expected[9] = {4, 9, 6, 7, 8, 10, 11, 12, 13};
        gf::u64 hashes[9] = {};
        for (unsigned i = 0; i < 9; ++i) {
            const gf::MainMenu menu = menu_in_screen(screens[i]);
            const Snapshot snapshot = fixture.draw(menu);
            check(snapshot.draws == expected[i], "screen draws its documented number of bars");
            check(snapshot.vertices == static_cast<std::size_t>(snapshot.draws) * 6,
                  "each bar is two triangles");
            check(fixture.backend.blended_draws() == snapshot.draws, "every bar is blended");
            check(fixture.backend.depth_draws() == 0, "no bar tests depth");
            hashes[i] = snapshot.hash;
        }
        for (unsigned i = 0; i < 9; ++i) {
            for (unsigned j = i + 1; j < 9; ++j) {
                check(expected[i] != expected[j], "every screen has its own bar count");
                check(hashes[i] != hashes[j], "different screens produce different frames");
            }
        }
    }

    // 9. Draw restores the render state it found and leaves the menu untouched.
    ++cases;
    {
        Fixture fixture;
        const gf::MainMenu menu = menu_in_screen(gf::MenuScreen::MainMenu);
        const gf::u32 revision = menu.revision();

        fixture.engine.state().blend = false;
        fixture.engine.state().blend_src = 9;
        fixture.engine.state().blend_dst = 9;
        fixture.engine.state().depth_test = true;
        fixture.engine.state().depth_write = true;
        fixture.engine.state().cull_backface = true;
        fixture.engine.state().texture = 3;
        fixture.engine.state().position_matrix = 2;
        fixture.engine.state().vertex_format = 7;

        fixture.draw(menu);

        const gf::RenderState& state = fixture.engine.state();
        check(!state.blend && state.blend_src == 9 && state.blend_dst == 9,
              "blend state is restored");
        check(state.depth_test && state.depth_write, "depth state is restored");
        check(state.cull_backface, "cull state is restored");
        check(state.texture == 3 && state.position_matrix == 2, "texture and matrix slot restored");
        check(state.vertex_format == 7, "vertex format is restored");
        check(fixture.backend.matrix_uploads() >= 1 && fixture.backend.last_matrix_slot() == 0,
              "identity matrix uploaded to slot 0");
        check(menu.revision() == revision && menu.screen() == gf::MenuScreen::MainMenu,
              "drawing never mutates the menu");
        check(fixture.backend.uploaded() >= 6, "vertices reached the backend");
    }

    // 10. The menu appends to the frame arena instead of replacing it.
    ++cases;
    {
        Fixture fixture;
        const gf::MainMenu menu = menu_in_screen(gf::MenuScreen::Title);
        const gf::Vertex marker{1.0f, 2.0f, 0.0f, 9, 9, 9, 255, 0.0f, 0.0f};

        fixture.engine.reset_arena();
        check(fixture.engine.append_vertices(&marker, 1) == 0, "world geometry sits at index 0");
        fixture.engine.begin_frame({0, 0.0f, 0.0f, 0.0f});
        const unsigned issued = menu.draw(fixture.engine, 640, 480);
        fixture.engine.end_frame();
        fixture.engine.present();

        check(issued == 4, "title screen still draws four bars");
        check(fixture.engine.arena_size() == 1 + static_cast<std::size_t>(issued) * 6,
              "menu appended to the arena and kept the earlier vertex");
        check(fixture.backend.draws() == issued, "only the menu's own draws were submitted");
        check(fixture.backend.vertex_count() == static_cast<std::size_t>(issued) * 6,
              "the recorder only hashed the menu's vertex ranges");
    }

    // 11. A degenerate surface draws nothing and changes nothing.
    ++cases;
    {
        Fixture fixture;
        gf::MainMenu menu;
        menu.open();
        const gf::u32 revision = menu.revision();
        fixture.engine.begin_frame({0, 0.0f, 0.0f, 0.0f});
        check(menu.draw(fixture.engine, 0, 480) == 0, "zero width draws nothing");
        check(menu.draw(fixture.engine, 640, -1) == 0, "negative height draws nothing");
        fixture.engine.end_frame();
        fixture.engine.present();
        check(fixture.backend.draws() == 0, "no draws submitted for a degenerate surface");
        check(menu.revision() == revision && menu.screen() == gf::MenuScreen::Title,
              "drawing a degenerate surface does not mutate the menu");
    }

    // 12. Two identical input sequences produce identical frames.
    ++cases;
    {
        const std::vector<gf::GamePadState> sequence{
            neutral(),      button_a(),   button_a(),     neutral(),      dpad_down(),
            neutral(),      dpad_down(),  neutral(),      stick(120),     neutral(),
            button_a(),     neutral(),    button_b(),     neutral(),      button_start(),
            neutral(),      button_b(),   neutral(),      dpad_down(),    neutral(),
            button_a(),     neutral()};

        const auto run = [&](std::vector<Snapshot>& frames, std::vector<gf::u32>& revisions) {
            Fixture fixture;
            gf::MainMenu menu;
            menu.open();
            frames.push_back(fixture.draw(menu));
            revisions.push_back(menu.revision());
            for (const gf::GamePadState& pad : sequence) {
                menu.update(pad);
                frames.push_back(fixture.draw(menu));
                revisions.push_back(menu.revision());
            }
        };

        std::vector<Snapshot> first_frames;
        std::vector<Snapshot> second_frames;
        std::vector<gf::u32> first_revisions;
        std::vector<gf::u32> second_revisions;
        run(first_frames, first_revisions);
        run(second_frames, second_revisions);

        check(first_frames.size() == second_frames.size(), "both runs captured the same frames");
        check(first_revisions == second_revisions, "both runs produced the same revisions");
        for (std::size_t i = 0; i < first_frames.size(); ++i) {
            check(first_frames[i].hash == second_frames[i].hash, "identical frames hash identically");
            check(first_frames[i].draws == second_frames[i].draws, "identical frames draw identically");
            check(first_frames[i].vertices == second_frames[i].vertices,
                  "identical frames submit the same vertex count");
        }
        check(first_revisions.back() > first_revisions.front(),
              "the scripted sequence actually changed visible state");
    }

    std::printf("{\"menu_cases\":%u,\"screens\":9,\"all_passed\":true}\n", cases);
    return 0;
}
