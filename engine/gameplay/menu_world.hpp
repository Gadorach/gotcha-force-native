#pragma once
// Main-menu world: the real menu screen is a 3D room/desk set, not a list.
//
// The captured reference screen (private, see AGENTS.md) is a fixed-camera shot
// of a desk in a room: a large "GOTCHA FORCE" logo lying on the desk, a TV, a
// lamp, a stack of books, a wall clock and a cutting mat, with a ring of gear
// seals floating above them and one red/orange reticle around the selected seal.
// This file reproduces that *structure* with placeholder geometry: boxes for the
// room and desk, world-anchored camera-facing quads for the seals, a screen-space
// ring for the reticle, and the embedded bitmap font for the labels. There are no
// textures and no archive assets, so it runs under the headless recorder and the
// GL target identically.
//
// Separation of concerns:
//   * `MainMenu` remains the logic layer (screens, edge-triggered navigation,
//     selection, quit confirmation, revision). This class does not re-implement
//     any of it: `open()`/`update()`/`screen()`/`selection()`/`quit_requested()`
//     all delegate.
//   * The seal order and count come from `MainMenu::selection_count()` and
//     `MainMenu::item_name()`; there is deliberately no second item list here.
//     The captured screen has seven seals, so `kReferenceSealSlots` documents the
//     layout the ring was designed around, but the live ring is laid out evenly
//     for whatever count the logic layer reports. If `menu.hpp` grows to the
//     reference's seven rows, seven seals appear without touching this file.
//
// Camera: one fixed camera, chosen so a 640x480 surface frames the desk the way
// the capture does. World units are large (a desk is 840 units wide) because the
// camera's perspective scale is `near / depth`; with near = 300 and a view depth
// of roughly 1,000-1,450 a 150-unit seal lands at 45-50 px, which is the size the
// reference's gear discs occupy.
//   eye    = {0, 950, 900}    above and in front of the desk
//   target = {0, 0, 60}       desk centre (the logo plaque sits near it)
//   up     = {0, 1, 0}
//   fov_y  = 42 degrees
//   near   = 300, far = 6000
// The scene is authored for that camera: the wall is behind the desk (smaller z
// is further from the eye), the desk spans z -200..320, and every box corner
// stays between the near and far planes so no box needs clipping.
//
// Draw order and count (one `render.draw` per line):
//   1  the room/desk set, all boxes in one submission     36 vertices per box
//   N  one billboard per seal (N = MainMenu::selection_count)
//   1  the reticle ring, on the selected seal only        6 vertices per segment
//   1  the "GOTCHA FORCE" logo text on the desk plaque
//   N  one centred label per seal, anchored at its projected position
// so a four-row menu issues 11 draws. Every screen of the menu state machine
// renders the same set (the scene is the menu, the screen only changes what the
// logic layer will do), which is why "every screen draws" is a cheap check.
//
// Rendering contract, matching `MainMenu::draw` and `TextRenderer`:
//   * Geometry is appended with `render.append_vertices` into the frame arena;
//     `upload_vertices` is never called, so world geometry queued earlier in the
//     frame is preserved. The caller owns the arena lifetime and resets it per
//     frame (`RenderEngine::reset_arena`), exactly as the session/host does.
//   * Blend on (src alpha, one-minus-src alpha), depth test and depth write on,
//     culling off, texture 0, identity matrix in palette slot 0. The complete
//     `RenderState` is saved and restored, so a caller's UI pass is untouched.
//     `TextRenderer` switches depth off for its own draw and restores our state.
//   * No OS/GPU headers, no globals, no time or randomness: the same menu state
//     and the same camera produce the same vertex stream and the same draw count.
//   * Nothing is drawn (return 0) before a positive `configure()` or if the
//     camera is degenerate/behind the near plane.
#include <cmath>
#include <cstddef>
#include <vector>

#include "engine/core/types.hpp"
#include "engine/gameplay/menu.hpp"
#include "engine/graphics/camera.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"

namespace gf {

class MenuWorld {
public:
    // Seals the reference screen shows. Documentation only: the drawn count is
    // the logic layer's row count (see the header comment).
    static constexpr unsigned kReferenceSealSlots = 7;
    // Reticle resolution, exposed so a test can prove the ring replaced a quad.
    static constexpr unsigned kReticleSegments = 24;

    MenuWorld() { geometry_.reserve(kGeometryReserve); }

    // Sets the fixed camera and the target surface size. Only a positive width
    // and height configure the world; anything else makes `draw()` a no-op until
    // a valid size arrives.
    void configure(s32 width, s32 height) {
        camera_.eye[0] = 0.0f;
        camera_.eye[1] = 950.0f;
        camera_.eye[2] = 900.0f;
        camera_.target[0] = 0.0f;
        camera_.target[1] = 0.0f;
        camera_.target[2] = 60.0f;
        camera_.up[0] = 0.0f;
        camera_.up[1] = 1.0f;
        camera_.up[2] = 0.0f;
        camera_.fov_y_degrees = 42.0f;
        camera_.near_plane = 300.0f;
        camera_.far_plane = 6000.0f;
        camera_.viewport_width = width > 0 ? width : 0;
        camera_.viewport_height = height > 0 ? height : 0;
        configured_ = width > 0 && height > 0;
    }

    const Camera& camera() const { return camera_; }

    // ---- logic layer delegation --------------------------------------
    void open() { menu_.open(); }
    void update(const GamePadState& pad) { menu_.update(pad); }
    void update_ports(const std::array<GamePadState,4>& pads) { menu_.update_ports(pads); }
    void update_ports_with_sound(const std::array<GamePadState,4>& pads,UiSoundBatch* sounds){menu_.update_ports_with_sound(pads,sounds);}
    bool configure_navigation(const MenuNavigationConfig& config){return menu_.configure_navigation(config);}
    bool spatial_navigation()const{return menu_.spatial_navigation();}
    bool enter_main_menu(unsigned selection){return menu_.enter_main_menu(selection);}
    void observe_ports(const std::array<GamePadState,4>& pads){menu_.observe_ports(pads);}
    void resume_main_menu(){menu_.resume_main_menu();}
    MenuScreen screen() const { return menu_.screen(); }
    unsigned selection() const { return menu_.selection(); }
    unsigned selection_count() const { return menu_.selection_count(); }
    bool quit_requested() const { return menu_.quit_requested(); }

    // Emits the scene and returns the number of `render.draw` calls issued.
    unsigned draw(RenderEngine& render);

private:
    // One box of the room/desk set: centre and half extents in world units.
    struct Box {
        f32 centre[3];
        f32 half[3];
        Tint tint;
    };
    struct Range {
        u32 first = 0;
        u32 count = 0;
    };

    // Upper bound on seal submissions kept in fixed storage. The menu has four
    // rows today and the reference screen has seven; sixteen leaves room for a
    // future sub-menu without a heap allocation per frame.
    enum : unsigned { kMaxSealDraws = 16 };

    // Ceiling on the per-frame scratch: 12 boxes x 36, 16 seals x 6, one ring of
    // 24 segments x 6. Sized once in the constructor.
    static constexpr std::size_t kGeometryReserve = 1024;

    static constexpr f32 kTau = 6.2831853f;
    static constexpr f32 kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

    // ---- scene layout (world units) -----------------------------------
    // Room and desk furniture. The wall is behind the desk (z -450..-390); the
    // desk top is at y = 20, which every desk item rests on.
    static constexpr Box kSet[] = {
        {{0.0f, 200.0f, -300.0f}, {620.0f, 260.0f, 30.0f}, {44, 50, 66, 255}},  // wall
        {{0.0f, -260.0f, 40.0f}, {620.0f, 200.0f, 420.0f}, {30, 34, 44, 255}},  // floor
        {{0.0f, -20.0f, 40.0f}, {420.0f, 40.0f, 230.0f}, {120, 92, 60, 255}},   // desk
        {{300.0f, 140.0f, -60.0f}, {120.0f, 120.0f, 60.0f}, {58, 64, 78, 255}}, // TV body
        {{300.0f, 140.0f, 6.0f}, {95.0f, 90.0f, 4.0f}, {40, 70, 96, 255}},      // TV screen
        {{-340.0f, 55.0f, -60.0f}, {40.0f, 35.0f, 40.0f}, {70, 70, 78, 255}},   // lamp base
        {{-340.0f, 140.0f, -60.0f}, {14.0f, 60.0f, 14.0f}, {92, 92, 100, 255}}, // lamp post
        {{-340.0f, 215.0f, -60.0f}, {75.0f, 35.0f, 75.0f}, {214, 178, 96, 255}},// lamp shade
        {{-150.0f, 46.0f, 240.0f}, {100.0f, 26.0f, 70.0f}, {150, 70, 64, 255}}, // books
        {{200.0f, 240.0f, -262.0f}, {65.0f, 65.0f, 8.0f}, {188, 196, 210, 255}},// clock
        {{0.0f, 27.0f, 60.0f}, {260.0f, 7.0f, 120.0f}, {46, 88, 66, 255}},      // cutting mat
        {{0.0f, 40.0f, 60.0f}, {250.0f, 6.0f, 50.0f}, {228, 196, 92, 255}},     // logo plaque
    };
    static constexpr unsigned kBoxCount = sizeof(kSet) / sizeof(kSet[0]);
    // The wordmark sits on the desk plaque, as the captured screen has it: the
    // gold bar is the stand-in for the logo decal under the seal ring.
    static constexpr f32 kLogoCentre[3] = {0.0f, 40.0f, 60.0f};

    // Seal ring: an ellipse on a plane above the desk, centred on the logo. The
    // first seal sits at the near edge (largest z), the rest follow evenly.
    static constexpr f32 kRingCentre[3] = {0.0f, 300.0f, 40.0f};
    static constexpr f32 kRingRadiusX = 330.0f;
    static constexpr f32 kRingRadiusZ = 210.0f;
    static constexpr f32 kSealWorldSize = 120.0f;

    // Distinct per-seal tints so every gear is identifiable in a capture; the
    // reticle is deliberately red/orange and shares no colour with them.
    static constexpr unsigned kSealTintCount = 7;
    static constexpr Tint kSealTints[kSealTintCount] = {
        {96, 168, 255, 255},  // blue
        {92, 210, 150, 255},  // green
        {238, 196, 84, 255},  // gold
        {224, 110, 140, 255}, // rose
        {110, 214, 224, 255}, // cyan
        {240, 150, 80, 255},  // amber
        {180, 140, 240, 255}, // violet
    };
    static constexpr Tint kReticleTint{255, 96, 40, 255};
    static constexpr f32 kReticleRadius = 80.0f;
    static constexpr f32 kReticleThickness = 12.0f;

    // Text: one gold block on the desk plaque, white labels under each seal.
    static constexpr TextRenderer::Style kLogoStyle = {250, 240, 210, 255, 2, 1, 2};
    static constexpr TextRenderer::Style kSealLabelStyle = {235, 240, 250, 255, 2, 1, 2};
    static constexpr f32 kLabelGap = 4.0f;

    // Deterministic ring position for seal `index` of `count`. Never called with
    // a zero count.
    static void seal_anchor(unsigned index, unsigned count, f32 out[3]) {
        const f32 turn = kTau * static_cast<f32>(index) / static_cast<f32>(count);
        out[0] = kRingCentre[0] + kRingRadiusX * std::sin(turn);
        out[1] = kRingCentre[1];
        out[2] = kRingCentre[2] + kRingRadiusZ * std::cos(turn);
    }

    // Clamped pixel coordinate: keeps a wildly off-screen projection from
    // converting a NaN/inf float to an integer (undefined behaviour) and keeps
    // the label pen inside a sane range.
    static s32 pixel(f32 value) {
        if (!(value >= -1000000.0f)) return -1000000;  // NaN and very negative
        if (value > 1000000.0f) return 1000000;
        return static_cast<s32>(value);
    }

    unsigned draw_labels(RenderEngine& render, unsigned seals);

    MainMenu menu_{};
    Camera camera_{};
    TextRenderer text_{};
    std::vector<Vertex> geometry_{};
    bool configured_ = false;
};

inline unsigned MenuWorld::draw(RenderEngine& render) {
    if (!configured_) return 0;
    if (camera_.viewport_width <= 0 || camera_.viewport_height <= 0) return 0;

    // A degenerate camera (eye == target, or up parallel to the view direction)
    // must not draw mirrored or infinite geometry.
    Camera::Basis basis{};
    if (!camera_.basis(&basis)) return 0;
    if (!(basis.tan_half_fov > 0.0f) || !(basis.aspect > 0.0f)) return 0;

    const RenderState saved = render.state();
    render.state().blend = true;
    render.state().blend_src = 5; // src alpha
    render.state().blend_dst = 6; // one minus src alpha
    render.state().depth_test = true;
    render.state().depth_write = true;
    render.state().cull_backface = false;
    render.state().texture = 0;
    // Screen-space geometry (the projection already happened on the CPU): the
    // identity transform in palette slot 0, as the menu shell and UI passes use.
    render.upload_matrix(0, kIdentity);
    render.state().position_matrix = 0;

    geometry_.clear();
    Range ranges[2 + kMaxSealDraws]{};
    unsigned range_count = 0;

    // (a) The room/desk set. One submission: the boxes share a tint family and
    // depth ordering comes from the projected z, not the submission order.
    const std::size_t set_first = geometry_.size();
    for (unsigned i = 0; i < kBoxCount; ++i) {
        ScreenDraw::box(render, camera_, kSet[i].centre, kSet[i].half, kSet[i].tint, geometry_);
    }
    if (geometry_.size() > set_first) {
        ranges[range_count].first = static_cast<u32>(set_first);
        ranges[range_count].count = static_cast<u32>(geometry_.size() - set_first);
        ++range_count;
    }

    // (b) One world-anchored billboard per seal, in logic-layer order. The loop
    // leaves the last range slot for the reticle, so the fixed storage cannot be
    // overrun even if a future menu grows past kMaxSealDraws (extra rows beyond
    // the cap are dropped rather than silently shifting the ring).
    unsigned seals = menu_.selection_count();
    if (seals > kMaxSealDraws) seals = kMaxSealDraws;
    for (unsigned i = 0; i < seals && range_count < 1 + kMaxSealDraws; ++i) {
        f32 anchor[3];
        seal_anchor(i, seals, anchor);
        std::size_t first = 0;
        std::size_t count = 0;
        if (!ScreenDraw::billboard(camera_, anchor, kSealWorldSize,
                                   kSealTints[i % kSealTintCount], geometry_, &first, &count)) {
            continue;
        }
        ranges[range_count].first = static_cast<u32>(first);
        ranges[range_count].count = static_cast<u32>(count);
        ++range_count;
    }

    // (c) The reticle, on the selected seal only.
    const unsigned selected = menu_.selection();
    if (selected < seals && range_count < 2 + kMaxSealDraws) {
        f32 anchor[3];
        seal_anchor(selected, seals, anchor);
        std::size_t first = 0;
        std::size_t count = 0;
        if (ScreenDraw::ring(camera_, anchor, kReticleRadius, kReticleThickness, kReticleSegments,
                             kReticleTint, geometry_, &first, &count)) {
            ranges[range_count].first = static_cast<u32>(first);
            ranges[range_count].count = static_cast<u32>(count);
            ++range_count;
        }
    }

    unsigned issued = 0;
    if (range_count > 0 && !geometry_.empty()) {
        const u32 base = render.append_vertices(geometry_.data(), geometry_.size());
        for (unsigned i = 0; i < range_count; ++i) {
            if (ranges[i].count == 0) continue;
            if (render.draw(Primitive::Triangles, base + ranges[i].first, ranges[i].count)) {
                ++issued;
            }
        }
    }

    // (d) Labels, anchored at the projected scene positions.
    issued += draw_labels(render, seals);

    render.state() = saved;
    return issued;
}

inline unsigned MenuWorld::draw_labels(RenderEngine& render, unsigned seals) {
    unsigned issued = 0;
    f32 sx = 0.0f;
    f32 sy = 0.0f;
    f32 scale = 0.0f;

    // The logo on the desk plaque: a stand-in for the game's wordmark.
    if (camera_.project(kLogoCentre, &sx, &sy, &scale)) {
        const s32 y = pixel(sy) - text_.line_height(kLogoStyle) / 2;
        issued += text_.draw_centered(render, "GOTCHA FORCE", pixel(sx), y, kLogoStyle);
    }

    for (unsigned i = 0; i < seals; ++i) {
        f32 anchor[3];
        seal_anchor(i, seals, anchor);
        if (!camera_.project(anchor, &sx, &sy, &scale)) continue;
        const f32 half = camera_.size_scale(kSealWorldSize, scale) * 0.5f;
        const s32 y = pixel(sy + half + kLabelGap);
        issued += text_.draw_centered(render, MainMenu::item_name(i), pixel(sx), y,
                                      kSealLabelStyle);
    }
    return issued;
}

} // namespace gf
