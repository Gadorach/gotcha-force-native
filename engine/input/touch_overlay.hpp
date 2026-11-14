#pragma once
// On-screen control overlay: the shared source of truth for touch controls.
//
// A touch control has two consumers that must never disagree: hit testing (does
// this touch press the button?) and drawing (does the player see that button
// there?). Duplicating the geometry is how overlays end up highlighting one
// place and responding in another, so both are derived from the same shapes
// here: `layout()` is what InputRouter hit-tests, `draw()` is what the renderer
// draws.
//
// Colours and visibility are configuration, not constants, so a phone can show
// a translucent pad while a desktop shows nothing.
#include <cmath>
#include <vector>

#include "engine/core/types.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/input/input_router.hpp"

namespace gf {

struct OverlayColour {
    u8 r = 255, g = 255, b = 255, a = 255;
};

// A rectangular on-screen button in reference-resolution space (640x480 unless
// the overlay is scaled).
struct OverlayButton {
    s32 x = 0, y = 0, width = 0, height = 0;
    u32 raw_button = 0;          // device bit this control stands in for
    OverlayColour fill{40, 40, 48, 160};
    OverlayColour pressed{120, 200, 255, 220};
    OverlayColour outline{200, 200, 210, 200};
    bool visible = true;
};

// Analogue pad: a circular base plus a knob that follows the current deflection.
struct OverlayStick {
    s32 center_x = 0, center_y = 0, radius = 0;
    OverlayColour base{32, 32, 40, 140};
    OverlayColour knob{200, 210, 230, 200};
    bool visible = true;
};

struct OverlayConfig {
    bool enabled = false;        // mobile-like targets enable this; desktop leaves it off
    f32 scale = 1.0f;            // reference-space scale
    s32 offset_x = 0, offset_y = 0;
    bool debug_outline = false;  // draw the hit rectangle exactly, for verification
};

class TouchOverlay {
public:
    static constexpr unsigned kMaxButtons = 12;
    static constexpr unsigned kStickSegments = 24;

    // ---- configuration ------------------------------------------------
    void configure(const OverlayConfig& config) { config_ = config; }
    const OverlayConfig& config() const { return config_; }
    bool enabled() const { return config_.enabled; }

    bool add_button(const OverlayButton& button) {
        if (buttons_.size() >= kMaxButtons) return false;
        buttons_.push_back(button);
        return true;
    }
    void set_stick(const OverlayStick& stick) {
        stick_ = stick;
        has_stick_ = true;
    }
    void clear() {
        buttons_.clear();
        has_stick_ = false;
    }
    const std::vector<OverlayButton>& buttons() const { return buttons_; }

    // ---- shared geometry ----------------------------------------------
    // Reference-space rectangle after scale/offset, used for BOTH hit testing
    // and drawing so they cannot drift apart.
    struct Rect { s32 x, y, width, height; };
    Rect rect_of(const OverlayButton& button) const {
        return {scale_x(button.x), scale_y(button.y), scale_span(button.width),
                scale_span(button.height)};
    }
    struct Circle { s32 x, y, radius; };
    Circle circle_of(const OverlayStick& stick) const {
        return {scale_x(stick.center_x), scale_y(stick.center_y), scale_span(stick.radius)};
    }

    // Hit-test layout consumed by InputRouter.
    TouchLayout layout() const {
        TouchLayout layout;
        layout.enabled = config_.enabled;
        for (const OverlayButton& button : buttons_) {
            if (!button.visible) continue;
            const Rect r = rect_of(button);
            layout.add_button({r.x, r.y, r.width, r.height, button.raw_button});
        }
        if (config_.enabled && has_stick_ && stick_.visible) {
            const Circle c = circle_of(stick_);
            layout.has_stick = true;
            layout.stick = {c.x, c.y, c.radius, 0, 1};
        }
        return layout;
    }

    // ---- drawing ------------------------------------------------------
    // Emits the overlay through the render seam. `pad` is current game input so
    // pressed controls can highlight; `pointer` optionally draws a cursor.
    // Returns the number of draws issued.
    unsigned draw(RenderEngine& render, const GamePadState& pad,
                  const InputSnapshot& snapshot) const {
        if (!config_.enabled) return 0;

        vertices_.clear();
        draws_.clear();

        for (unsigned index = 0; index < buttons_.size(); ++index) {
            const OverlayButton& button = buttons_[index];
            if (!button.visible) continue;
            const Rect r = rect_of(button);
            const bool pressed = is_pressed(button, pad);
            const OverlayColour& colour = pressed ? button.pressed : button.fill;
            const std::size_t first = vertices_.size();
            emit_rect(r, colour);
            if (config_.debug_outline) emit_outline(r, button.outline);
            draws_.push_back({static_cast<u32>(first),
                              static_cast<u32>(vertices_.size() - first), button.raw_button});
        }

        if (has_stick_ && stick_.visible) {
            const Circle c = circle_of(stick_);
            const std::size_t first = vertices_.size();
            emit_disc(c, stick_.base);
            const s32 knob_x = c.x + (pad.stick_x * c.radius) / 128;
            const s32 knob_y = c.y - (pad.stick_y * c.radius) / 128;
            emit_disc({knob_x, knob_y, c.radius / 3}, stick_.knob);
            draws_.push_back({static_cast<u32>(first),
                              static_cast<u32>(vertices_.size() - first), 0});
        }

        if (snapshot.pointer.within_window && pointer_cursor_) {
            const std::size_t first = vertices_.size();
            emit_rect({snapshot.pointer.x - 4, snapshot.pointer.y - 4, 8, 8},
                      snapshot.pointer.left ? OverlayColour{255, 200, 120, 220}
                                            : OverlayColour{220, 220, 230, 160});
            draws_.push_back({static_cast<u32>(first), 6, 0});
        }

        if (vertices_.empty()) return 0;
        // Append into the frame arena so world geometry already queued this frame
        // is not replaced.
        const u32 base = render.append_vertices(vertices_.data(), vertices_.size());

        // Overlay is UI: blend on, depth off, single identity matrix slot.
        const bool was_blend = render.state().blend;
        const bool was_colour_write=render.state().colour_write,was_alpha_write=render.state().alpha_write;
        render.state().blend = true;
        render.state().depth_test = false;
        render.state().depth_write = false;
        render.state().colour_write=true;render.state().alpha_write=true;
        render.state().cull_backface = false;
        render.upload_matrix(0, identity_);
        render.state().position_matrix = 0;

        unsigned issued = 0;
        for (const Draw& draw : draws_) {
            if (render.draw(Primitive::Triangles, base + draw.first, draw.count)) ++issued;
        }

        render.state().blend = was_blend;
        render.state().colour_write=was_colour_write;render.state().alpha_write=was_alpha_write;
        render.state().depth_test = true;
        render.state().depth_write = true;
        render.state().cull_backface = true;
        return issued;
    }

    void set_pointer_cursor(bool enabled) { pointer_cursor_ = enabled; }

private:
    struct Draw { u32 first, count, tag; };

    s32 scale_x(s32 value) const { return config_.offset_x + static_cast<s32>(value * config_.scale); }
    s32 scale_y(s32 value) const { return config_.offset_y + static_cast<s32>(value * config_.scale); }
    s32 scale_span(s32 value) const {
        const s32 span = static_cast<s32>(value * config_.scale);
        return span > 0 ? span : 0;
    }

    static bool is_pressed(const OverlayButton& button, const GamePadState& pad) {
        // The overlay mirrors game state, not raw device bits, so a remap cannot
        // desynchronise highlight from behaviour.
        switch (button.raw_button) {
        case 0x001: return pad.a;
        case 0x002: return pad.b;
        case 0x004: return pad.x;
        case 0x008: return pad.y;
        case 0x010: return pad.start;
        case 0x020: return pad.dpad_up;
        case 0x040: return pad.dpad_down;
        case 0x080: return pad.dpad_left;
        case 0x100: return pad.dpad_right;
        case 0x200: return pad.l;
        case 0x400: return pad.r;
        case 0x800: return pad.z;
        default: return false;
        }
    }

    void push(f32 x, f32 y, OverlayColour c) const {
        Vertex v;
        v.x = x;
        v.y = y;
        v.z = 0.0f;
        v.r = c.r; v.g = c.g; v.b = c.b; v.a = c.a;
        vertices_.push_back(v);
    }

    void emit_rect(const Rect& r, OverlayColour c) const {
        const f32 x0 = static_cast<f32>(r.x), y0 = static_cast<f32>(r.y);
        const f32 x1 = static_cast<f32>(r.x + r.width), y1 = static_cast<f32>(r.y + r.height);
        push(x0, y0, c); push(x1, y0, c); push(x1, y1, c);
        push(x0, y0, c); push(x1, y1, c); push(x0, y1, c);
    }

    void emit_outline(const Rect& r, OverlayColour c) const {
        OverlayColour t = c;
        emit_rect({r.x, r.y, r.width, 1}, t);
        emit_rect({r.x, r.y + r.height - 1, r.width, 1}, t);
        emit_rect({r.x, r.y, 1, r.height}, t);
        emit_rect({r.x + r.width - 1, r.y, 1, r.height}, t);
    }

    void emit_disc(const Circle& circle, OverlayColour c) const {
        const f32 cx = static_cast<f32>(circle.x), cy = static_cast<f32>(circle.y);
        const f32 radius = static_cast<f32>(circle.radius);
        for (unsigned i = 0; i < kStickSegments; ++i) {
            const f32 a0 = 6.2831853f * static_cast<f32>(i) / kStickSegments;
            const f32 a1 = 6.2831853f * static_cast<f32>(i + 1) / kStickSegments;
            push(cx, cy, c);
            push(cx + radius * cosf(a0), cy + radius * sinf(a0), c);
            push(cx + radius * cosf(a1), cy + radius * sinf(a1), c);
        }
    }

    OverlayConfig config_{};
    std::vector<OverlayButton> buttons_;
    OverlayStick stick_{};
    bool has_stick_ = false;
    bool pointer_cursor_ = false;
    // Out-of-class definition follows the class because a constexpr array member
    // needs storage when its address is taken.
    static constexpr f32 identity_[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    // Mutable emit scratch: draw() is const, so the buffers are mutable state.
    mutable std::vector<Vertex> vertices_;
    mutable std::vector<Draw> draws_;
};

} // namespace gf
