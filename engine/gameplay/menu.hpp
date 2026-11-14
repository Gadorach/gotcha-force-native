#pragma once
// Main menu: the first interactive screen of the native rewrite.
//
// This is pure game code: no window, no GPU, no globals. It consumes a
// `GamePadState` (already mapped from a device by engine/input/input_map.hpp)
// and draws itself through the portable render seam, so the same code runs
// under a headless recorder, a fake backend or the OpenGL target unchanged.
//
// Two rules keep it testable:
//   * Input edges and repeat counters live here, not in the platform layer.
//     Confirm acts once; configured directions follow the content's repeat rule.
//   * `revision()` counts visible changes, so a test (or the frame digest) can
//     tell "nothing happened this frame" from "the same menu was written again".
//
// Input:
//   * Configured content supplies spatial Up/Down/Left/Right adjacency and repeat
//     counters, with independent inputs for all four ports. Navigation precedes
//     A/Start confirmation; B does nothing on the original main-menu idle path.
//   * Without navigation content, the old edge-triggered linear list remains a
//     diagnostic/placeholder policy, NOT the real game's navigation.
//   * Unported destination stubs use B to return without claiming their real
//     submenu/save/return-confirmation flows are implemented.
//   * Explicit application exit is separate from the seal destinations.
//     Platform quit events arrive through InputSnapshot, not these buttons.
//
// The captured main menu has seven world-anchored seals, so the item list has
// seven stable content IDs (0 OPTION, 1 COLLECTION, 2 TRADE, 3 CHALLENGE,
// 4 EDIT FORCE, 5 STORY, 6 VERSUS). `selection_count()` is that count, which is what
// the billboard presentation layer in engine/gameplay/menu_world.hpp iterates:
// extending the table here is what makes seven seals appear there.
//
// Drawing is rectangles only (no glyphs, no textures) so the menu works before a
// text renderer exists. Every screen draws something, in painter's order; the
// counts are for the 640x480 reference surface:
//   Title          background, title bar, logo bar, prompt bar      4 draws
//   MainMenu       background, title bar, one bar per seal          9 draws
//                  (the selected bar uses a different colour)
//   Option         background, title bar, panel, header, 1 row, hint   6 draws
//   Collection     the same panel with 2 content rows                  7 draws
//   Trade          the same panel with 3 content rows                  8 draws
//   Challenge      the same panel with 5 content rows                 10 draws
//   EditForce      the same panel with 6 content rows                 11 draws
//   Story          the same panel with 7 content rows                 12 draws
//   Versus         the same panel with 8 content rows                 13 draws
// The row counts differ per sub-screen on purpose, and none of them matches the
// main menu's nine draws, so a sub-screen is identifiable from its geometry
// alone even though no text is drawn and every screen hashes differently in a
// frame digest. Bars that would come out empty on a very small surface are
// dropped (the background always survives), so any positive size still produces
// at least one draw.
//
// `draw()` writes into a single reusable vertex buffer (the only allocation, and
// only when it first grows), appends into the render engine's frame arena so
// world geometry queued earlier in the frame is untouched, saves and restores
// the whole RenderState, and returns the number of draws issued.
#include <array>
#include <cstddef>
#include <vector>

#include "engine/core/types.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/input/input_map.hpp"
#include "engine/gameplay/menu_navigation/select_neighbor.hpp"
#include "engine/gameplay/menu_navigation/repeat.hpp"
#include "engine/gameplay/menu_navigation/input_bits.hpp"
#include "engine/gameplay/ui_sound_operations.hpp"

namespace gf {

// Screens the menu can show. `Title` is the initial screen. Rows 0..6 of the
// seal ring open the seven destination screens below, in the same row order.
//
// Exit is separate from the seven destinations; legacy screen spellings remain
// aliases for compatibility, but CHALLENGE never requests application exit.
enum class MenuScreen {
    Title,
    MainMenu,
    // The seven seal destinations, in row order.
    Option,
    Collection,
    Trade,
    Challenge,
    EditForce,
    Story,
    Versus,
    // Legacy spellings, kept for earlier callers; stable content IDs do not move.
    Quit,
    Revolution = Option,
    Force = EditForce,
    CustomVersion = Versus,
    BattleSetup = Option,
    Team = Collection,
    Options = Trade, // legacy pre-seven-item API only; canonical OPTION is Option
    OptionDoor = Option,
    VersusVersion = Versus,
};

// Main-menu rows in display order: the seven seals of the captured menu. The
// enumerator value is the row index, so the name table, the drawn bar order and
// the row-to-screen mapping share one order and cannot drift apart.
enum class MenuItem : u32 {
    Option = 0,
    Collection = 1,
    Trade = 2,
    Challenge = 3,
    EditForce = 4,
    Story = 5,
    Versus = 6,
    Count = 7,
    Revolution = Option,
    Force = EditForce,
    CustomVersion = Versus,
    OptionDoor = Option,
    VersusVersion = Versus,
};

inline constexpr unsigned kMenuItemCount = static_cast<unsigned>(MenuItem::Count);

// Display names exactly as the game shows them, kept for logs/debug output, for
// the seal labels in engine/gameplay/menu_world.hpp and for a future text
// renderer. The menu itself draws bars and never rasterises glyphs.
inline constexpr const char* kMenuItemNames[kMenuItemCount] = {
    "OPTION", "COLLECTION", "TRADE", "CHALLENGE", "EDIT FORCE", "STORY", "VERSUS"};

namespace menu_detail {
// Deflection at which the left stick counts as a d-pad direction. Roughly a
// third of full travel: above the default 0.15 deadzone, so a resting stick
// never scrolls the menu.
inline constexpr s32 kStickThreshold = 48;
constexpr bool stick_up(s32 y) { return y > kStickThreshold; }
constexpr bool stick_down(s32 y) { return y < -kStickThreshold; }
} // namespace menu_detail

class MainMenu {
public:
    MainMenu() { vertices_.reserve(kMaxRects * 6); }

    // Starts a fresh session on the Title screen: selection cleared, no pending
    // quit, previous pad state forgotten so a button held across the transition
    // counts as a fresh press. Counts as a visible change.
    void open() {
        screen_ = MenuScreen::Title;
        selection_ = 0;
        quit_requested_ = false;
        prev_ = GamePadState{};
        menu_inputs_={};
        ++revision_;
    }

    bool configure_navigation(const MenuNavigationConfig& config){
        if(!menu_navigation_valid(config))return false;
        navigation_=config;
        if(!spatial_navigation_){menu_inputs_={};menu_inputs_[0].held=menu_pad_bits(prev_,menu_detail::kStickThreshold);}
        for(auto& input:menu_inputs_)input.ticks=0;
        spatial_navigation_=true;
        return true;
    }
    bool spatial_navigation()const{return spatial_navigation_;}
    // Explicit startup/return entry, not fake input used to walk a linear list.
    bool enter_main_menu(unsigned selected){
        if(selected>=kMenuItemCount)return false;
        const bool changed=screen_!=MenuScreen::MainMenu||selection_!=selected;
        enter(MenuScreen::MainMenu);selection_=selected;prev_={};menu_inputs_={};
        if(changed)++revision_;
        return true;
    }
    // Transition ownership may consume input while this menu is not interactive.
    // Resume preserves those histories so held A/Start cannot re-activate a seal.
    void observe_ports(const std::array<GamePadState,4>& pads){
        if(spatial_navigation_)for(unsigned port=0;port<pads.size();++port)
            (void)menu_repeat_step(menu_inputs_[port],menu_pad_bits(pads[port],menu_detail::kStickThreshold),navigation_);
        prev_=pads[0];
    }
    void resume_main_menu(){if(screen_!=MenuScreen::MainMenu){enter(MenuScreen::MainMenu);++revision_;}}
    //Native mouse selection changes only the active MAIN item. It does not
    //re-enter/reset the menu, synthesize a direction, or clear held histories.
    bool point_main_item(unsigned item){if(screen_!=MenuScreen::MainMenu||item>=kMenuItemCount)return false;if(selection_!=item){selection_=item;++revision_;}return true;}

    void update_ports(const std::array<GamePadState,4>& pads){
        update_ports_impl(pads,nullptr);
    }
    void update_ports_with_sound(const std::array<GamePadState,4>& pads,UiSoundBatch* sounds){
        if(sounds)*sounds={};
        update_ports_impl(pads,sounds);
    }
private:
    void update_ports_impl(const std::array<GamePadState,4>& pads,UiSoundBatch* sounds){
        if(!spatial_navigation_){update(pads[0]);return;}
        const MenuScreen before_screen=screen_;const unsigned before_selection=selection_;
        u32 directions=0,pressed=0;
        for(unsigned port=0;port<pads.size();++port){
            const auto events=menu_repeat_step(menu_inputs_[port],menu_pad_bits(pads[port],menu_detail::kStickThreshold),navigation_);
            directions|=menu_direction_events(events.pressed|events.repeated);pressed|=events.pressed;
        }
        if(screen_==MenuScreen::MainMenu){
            // Original 800BFB54: spatial selection first, then A/Start edges.
            // B is not an idle-menu action, even when pressed simultaneously.
            selection_=menu_select_neighbor(navigation_,selection_,directions);
            if(sounds&&selection_!=before_selection)sounds->emit(UiSoundOperationKind::MainNavigate,u8(selection_));
            if(pressed&0x1100u){if(sounds)sounds->emit(selection_==unsigned(MenuItem::Option)?UiSoundOperationKind::MainOptionEnter:UiSoundOperationKind::MainConfirm,u8(selection_));confirm_current();}
        }else if(pressed&0x200u)go_back(); // native NOT PORTED/back affordance
        else if(pressed&0x1100u)confirm_current();
        if(screen_!=before_screen||selection_!=before_selection)++revision_;
        prev_=pads[0];
    }
public:

    // Single-port convenience entry. Configured direction repeat is handled by
    // update_ports; the unconfigured diagnostic list remains edge-triggered.
    void update(const GamePadState& pad) {
        if(spatial_navigation_){std::array<GamePadState,4> pads{};pads[0]=pad;update_ports(pads);return;}
        const MenuScreen before_screen = screen_;
        const unsigned before_selection = selection_;

        const bool confirm = (pad.a && !prev_.a) || (pad.start && !prev_.start);
        const bool back = pad.b && !prev_.b;
        const bool up = pressed_up(pad);
        const bool down = pressed_down(pad);

        // Back wins over confirm if both arrive in the same frame: cancelling is
        // the safe action when a frame contains contradictory input.
        if (back) {
            go_back();
        } else if (confirm) {
            confirm_current();
        } else if (up) {
            move_selection(-1);
        } else if (down) {
            move_selection(1);
        }

        if (screen_ != before_screen || selection_ != before_selection) ++revision_;
        prev_ = pad;
    }

    MenuScreen screen() const { return screen_; }
    unsigned selection() const { return selection_; }
    // Only an explicit application exit request sets this, never a seal.
    bool quit_requested() const { return quit_requested_; }
    // Exit is a separate application affordance, never the CHALLENGE row.
    void request_quit() { if(screen_!=MenuScreen::Quit){enter(MenuScreen::Quit);++revision_;} }
    unsigned selection_count() const { return kMenuItemCount; }
    // Increments once per visible change (screen or selection). Identical input
    // frames leave it untouched.
    u32 revision() const { return revision_; }

    // Display name for a main-menu row; empty string for an out-of-range row.
    static const char* item_name(unsigned row) {
        return row < kMenuItemCount ? kMenuItemNames[row] : "";
    }
    // Screen a row activates. Out-of-range rows resolve to MainMenu so a corrupt
    // index can never invent a screen.
    static MenuScreen screen_for(unsigned row) {
        switch (static_cast<MenuItem>(row)) {
        case MenuItem::Option: return MenuScreen::Option;
        case MenuItem::Collection: return MenuScreen::Collection;
        case MenuItem::Trade: return MenuScreen::Trade;
        case MenuItem::Challenge: return MenuScreen::Challenge;
        case MenuItem::EditForce: return MenuScreen::EditForce;
        case MenuItem::Story: return MenuScreen::Story;
        case MenuItem::Versus: return MenuScreen::Versus;
        case MenuItem::Count: break;
        }
        return MenuScreen::MainMenu;
    }

    // Draws the current screen as coloured rectangles. `width`/`height` are the
    // target surface in pixels (top-left origin, y down); a degenerate size
    // draws nothing and returns 0. Returns the number of draws issued.
    unsigned draw(RenderEngine& render, s32 width, s32 height) const {
        if (width <= 0 || height <= 0) return 0;

        vertices_.clear();
        Range ranges[kMaxRects]{};
        unsigned range_count = 0;
        const auto add = [&](Rect r, Colour c) {
            if (r.w <= 0 || r.h <= 0 || range_count >= kMaxRects) return;
            const std::size_t first = vertices_.size();
            emit_rect(r, c);
            ranges[range_count].first = static_cast<u32>(first);
            ranges[range_count].count = static_cast<u32>(vertices_.size() - first);
            ++range_count;
        };

        const s32 margin = imax(6, width / 24);
        const s32 title_h = imax(10, height / 9);
        const Rect title_bar{margin, margin, width - 2 * margin, title_h};
        const s32 top = title_bar.y + title_bar.h + margin;
        const s32 content_h = height - top - margin;

        add(Rect{0, 0, width, height}, kBackground);
        add(title_bar, kTitleBar);

        switch (screen_) {
        case MenuScreen::Title: {
            // Stand-in for the game logo: one wide bar.
            const s32 logo_h = imax(12, content_h / 3);
            add(Rect{width / 4, top + content_h / 5, width / 2, logo_h}, kLogo);
            // Stand-in for "press start": one shorter bar near the bottom.
            const s32 prompt_h = imax(8, content_h / 8);
            add(Rect{width / 3, height - margin - 2 * prompt_h, width / 3, prompt_h}, kPrompt);
            break;
        }
        case MenuScreen::MainMenu: {
            // Fit seven bars into the content band instead of assuming a small
            // list: with the old fixed divisor a seven-row list overflowed the
            // margin at 640x480.
            const s32 gap = imax(2, content_h / 40);
            const s32 count = static_cast<s32>(kMenuItemCount);
            const s32 bar_h = imax(8, (content_h - (count - 1) * gap) / count);
            const s32 bar_w = imax(20, width / 2);
            const s32 x = (width - bar_w) / 2;
            const s32 total = count * bar_h + (count - 1) * gap;
            s32 y = top + (content_h - total) / 2;
            for (unsigned row = 0; row < kMenuItemCount; ++row) {
                add(Rect{x, y, bar_w, bar_h}, row == selection_ ? kItemSelected : kItem);
                y += bar_h + gap;
            }
            break;
        }
        default: {
            // Sub-screens: one panel, a header strip, one bar per content row and
            // a hint bar. The row count is the screen's identity (see the header
            // comment), so no two sub-screens draw the same geometry.
            const bool quitting = screen_ == MenuScreen::Quit;
            const Rect panel{margin, top, width - 2 * margin, content_h};
            add(panel, quitting ? kQuitPanel : kPanel);
            add(Rect{panel.x + margin, panel.y + margin, panel.w - 2 * margin,
                     imax(8, panel.h / 8)},
                quitting ? kQuitHeader : kPanelHeader);

            const unsigned rows = content_rows(screen_);
            const s32 row_h = imax(6, panel.h / 14);
            const s32 row_gap = imax(2, row_h / 2);
            const s32 row_w = imax(20, (panel.w * 2) / 5);
            s32 y = panel.y + panel.h / 2 - static_cast<s32>(rows) * (row_h + row_gap) / 2;
            for (unsigned row = 0; row < rows; ++row) {
                add(Rect{panel.x + (panel.w - row_w) / 2, y, row_w, row_h}, kPanelRow);
                y += row_h + row_gap;
            }
            add(Rect{panel.x + margin, panel.y + panel.h - margin - imax(4, row_h / 2),
                     row_w, imax(4, row_h / 2)},
                kHint);
            break;
        }
        }

        if (range_count == 0) return 0;

        // Append into the frame arena: geometry queued earlier in the frame stays.
        const u32 base = render.append_vertices(vertices_.data(), vertices_.size());

        const RenderState saved = render.state();
        render.state().blend = true;
        render.state().blend_src = 5; // src alpha
        render.state().blend_dst = 6; // one minus src alpha
        render.state().depth_test = false;
        render.state().depth_write = false;
        render.state().cull_backface = false;
        render.state().texture = 0;
        // Screen-space geometry: identity transform in palette slot 0, matching
        // what the touch overlay does for UI.
        render.upload_matrix(0, kIdentity);
        render.state().position_matrix = 0;

        unsigned issued = 0;
        for (unsigned i = 0; i < range_count; ++i) {
            if (render.draw(Primitive::Triangles, base + ranges[i].first, ranges[i].count)) ++issued;
        }

        render.state() = saved;
        return issued;
    }

private:
    struct Colour {
        u8 r = 255, g = 255, b = 255, a = 255;
    };
    struct Rect {
        s32 x = 0, y = 0, w = 0, h = 0;
    };
    struct Range {
        u32 first = 0, count = 0;
    };

    // Upper bound on rectangles per screen, including the explicit Quit panel.
    static constexpr unsigned kMaxRects = 16;

    static constexpr Colour kBackground{10, 12, 18, 255};
    static constexpr Colour kTitleBar{40, 72, 122, 255};
    static constexpr Colour kLogo{120, 180, 240, 255};
    static constexpr Colour kPrompt{205, 205, 220, 255};
    static constexpr Colour kItem{52, 56, 68, 255};
    static constexpr Colour kItemSelected{214, 172, 62, 255};
    static constexpr Colour kPanel{28, 32, 42, 255};
    static constexpr Colour kPanelHeader{60, 92, 142, 255};
    static constexpr Colour kPanelRow{62, 68, 82, 255};
    static constexpr Colour kHint{120, 120, 132, 255};
    static constexpr Colour kQuitPanel{74, 26, 30, 255};
    static constexpr Colour kQuitHeader{158, 52, 56, 255};

    static constexpr f32 kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

    static constexpr s32 imax(s32 a, s32 b) { return a > b ? a : b; }

    // Content rows drawn inside a sub-screen panel. Deliberately different per
    // screen so each sub-screen has its own geometry (and frame hash).
    static constexpr unsigned content_rows(MenuScreen screen) {
        switch (screen) {
        case MenuScreen::Option: return 1;
        case MenuScreen::Collection: return 2;
        case MenuScreen::Trade: return 3;
        // Four is skipped on purpose: a four-row panel would draw nine times,
        // the same count as the main menu's seven bars plus two chrome bars.
        case MenuScreen::Challenge: return 5;
        case MenuScreen::EditForce: return 6;
        case MenuScreen::Story: return 7;
        case MenuScreen::Versus: return 8;
        case MenuScreen::Quit: return 9;
        case MenuScreen::Title:
        case MenuScreen::MainMenu: break;
        }
        return 0;
    }

    bool pressed_up(const GamePadState& pad) const {
        const bool now = pad.dpad_up || menu_detail::stick_up(pad.stick_y);
        const bool before = prev_.dpad_up || menu_detail::stick_up(prev_.stick_y);
        return now && !before;
    }
    bool pressed_down(const GamePadState& pad) const {
        const bool now = pad.dpad_down || menu_detail::stick_down(pad.stick_y);
        const bool before = prev_.dpad_down || menu_detail::stick_down(prev_.stick_y);
        return now && !before;
    }

    void move_selection(int delta) {
        if (screen_ != MenuScreen::MainMenu) return;
        const int count = static_cast<int>(kMenuItemCount);
        int next = static_cast<int>(selection_) + delta;
        if (next < 0) next += count;
        if (next >= count) next -= count;
        selection_ = static_cast<unsigned>(next);
    }

    void confirm_current() {
        switch (screen_) {
        case MenuScreen::Title:
            // Entering the main menu always starts at the first seal.
            selection_ = 0;
            enter(MenuScreen::MainMenu);
            break;
        case MenuScreen::MainMenu:
            if (selection_ < kMenuItemCount) enter(screen_for(selection_));
            break;
        case MenuScreen::Option:
        case MenuScreen::Collection:
        case MenuScreen::Trade:
        case MenuScreen::Challenge:
        case MenuScreen::EditForce:
        case MenuScreen::Story:
        case MenuScreen::Versus:
        case MenuScreen::Quit:
            // Destination screens have no confirm action yet; `b` is the only
            // way out.
            break;
        }
    }

    void go_back() {
        switch (screen_) {
        case MenuScreen::Title:
            // Nothing above the title screen.
            break;
        case MenuScreen::MainMenu:
            enter(MenuScreen::Title);
            break;
        case MenuScreen::Option:
        case MenuScreen::Collection:
        case MenuScreen::Trade:
        case MenuScreen::Challenge:
        case MenuScreen::EditForce:
        case MenuScreen::Story:
        case MenuScreen::Versus:
        case MenuScreen::Quit:
            enter(MenuScreen::MainMenu);
            break;
        }
    }

    // The only place the screen changes, so the quit request can never get out of
    // step with what is displayed.
    void enter(MenuScreen next) {
        screen_ = next;
        quit_requested_ = next == MenuScreen::Quit;
    }

    void push(f32 x, f32 y, Colour c) const {
        Vertex v;
        v.x = x;
        v.y = y;
        v.z = 0.0f;
        v.r = c.r;
        v.g = c.g;
        v.b = c.b;
        v.a = c.a;
        v.u = 0.0f;
        v.v = 0.0f;
        vertices_.push_back(v);
    }

    // Two triangles, clockwise in screen space (y down).
    void emit_rect(const Rect& r, Colour c) const {
        const f32 x0 = static_cast<f32>(r.x);
        const f32 y0 = static_cast<f32>(r.y);
        const f32 x1 = static_cast<f32>(r.x + r.w);
        const f32 y1 = static_cast<f32>(r.y + r.h);
        push(x0, y0, c);
        push(x1, y0, c);
        push(x1, y1, c);
        push(x0, y0, c);
        push(x1, y1, c);
        push(x0, y1, c);
    }

    MenuScreen screen_ = MenuScreen::Title;
    unsigned selection_ = 0;
    bool quit_requested_ = false;
    u32 revision_ = 0;
    GamePadState prev_{};
    MenuNavigationConfig navigation_{};
    std::array<MenuPadHistory,4> menu_inputs_{};
    bool spatial_navigation_=false;
    // Reused emit scratch: draw() is const, and a frame must not allocate once
    // the buffer has reached its high-water mark.
    mutable std::vector<Vertex> vertices_;
};

} // namespace gf
