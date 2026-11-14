#pragma once
// Interactive overlay for the real menu room.
//
// The room's own option joints are authored in a source pose (the game moves
// them at runtime, see local/recomp-evidence/menu-room-20261108/), so the
// interaction layer draws its own seals on a ring around the desk and says so:
// the geometry and textures are the game's, the ring placement is ours.
//
// Everything is emitted through the same seam as the rest of the scene: world
// anchors are projected on the CPU, billboards face the camera, the reticle is a
// screen-space ring, and labels sit under each seal.
#include <cmath>
#include <string_view>
#include <vector>

#include "engine/gameplay/menu.hpp"
#include "engine/graphics/camera.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"

namespace gf {

class MenuOverlay {
public:
    // World-space ring around the menu's focus point.
    void configure(const f32 centre[3], f32 radius, f32 seal_size) {
        for (unsigned i = 0; i < 3; ++i) centre_[i] = centre[i];
        radius_ = radius;
        seal_size_ = seal_size;
        configured_ = radius_ > 0.0f && seal_size_ > 0.0f;
    }

    bool configured() const { return configured_; }

    // Draws one billboard per item, a reticle on `selection` and one label per
    // seal. Returns the number of render.draw calls issued.
    unsigned draw(RenderEngine& render, const Camera& camera, unsigned selection, unsigned count) {
        if (!configured_ || count == 0) return 0;
        const RenderState saved = render.state();
        render.state().blend = true;
        render.state().blend_src = 5;
        render.state().blend_dst = 6;
        render.state().depth_test = true;
        render.state().depth_write = true;
        render.state().cull_backface = false;
        render.state().alpha_test = true;
        render.state().alpha_ref = 8;
        static constexpr f32 kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        render.upload_matrix(0, kIdentity);
        render.state().position_matrix = 0;
        render.state().texture = 0;

        unsigned issued = 0;
        render.state().texture = 0;
        for (unsigned i = 0; i < count; ++i) {
            f32 anchor[3];
            anchor_of(i, count, anchor);
            geometry_.clear();
            std::size_t first = 0, vertex_count = 0;
            // The reference draws the selected seal larger than the others; do
            // the same so the highlight is not only the reticle.
            const bool selected = (i == selection % count);
            const f32 radius = seal_size_ * (selected ? 0.62f : 0.5f);
            if (!ScreenDraw::disc(camera, anchor, radius, 20, tint_of(i, selected), geometry_,
                                  &first, &vertex_count)) {
                continue;
            }
            const u32 base = render.append_vertices(geometry_.data(), geometry_.size());
            if (render.draw(Primitive::Triangles, base,
                            static_cast<u32>(geometry_.size()))) {
                ++issued;
            }
        }

        // Reticle on the selected seal only, sized to the enlarged disc.
        f32 anchor[3];
        anchor_of(selection % count, count, anchor);
        geometry_.clear();
        std::size_t ring_first = 0, ring_count = 0;
        if (ScreenDraw::ring(camera, anchor, seal_size_ * 0.70f, seal_size_ * 0.09f, 24,
                             kReticle, geometry_, &ring_first, &ring_count)) {
            const u32 base = render.append_vertices(geometry_.data(), geometry_.size());
            if (render.draw(Primitive::Triangles, base, static_cast<u32>(geometry_.size()))) {
                ++issued;
            }
        }

        // Labels, anchored under each seal.
        for (unsigned i = 0; i < count; ++i) {
            f32 where[3];
            anchor_of(i, count, where);
            f32 sx = 0.0f, sy = 0.0f, scale = 0.0f;
            if (!camera.project(where, &sx, &sy, &scale)) continue;
            const f32 half = camera.size_scale(seal_size_, scale) * 0.5f;
            issued += text_.draw_centered(render, MainMenu::item_name(i),
                                          static_cast<s32>(sx),
                                          static_cast<s32>(sy + half + 6.0f), kLabelStyle);
        }

        render.state() = saved;
        return issued;
    }

    unsigned vertices() const { return static_cast<unsigned>(geometry_.size()); }
    void reset_geometry() { geometry_.clear(); }

private:
    void anchor_of(unsigned index, unsigned count, f32 out[3]) const {
        // Seals sit on a circle in the plane of the desk, first one nearest the
        // camera side (largest z).
        const f32 turn = 6.2831853f * static_cast<f32>(index) / static_cast<f32>(count);
        out[0] = centre_[0] + radius_ * std::sin(turn);
        out[1] = centre_[1];
        out[2] = centre_[2] + radius_ * std::cos(turn);
    }

    static Tint tint_of(unsigned index, bool selected) {
        static constexpr Tint kTints[7] = {
            {96, 168, 255, 220}, {92, 210, 150, 220}, {238, 196, 84, 220},
            {224, 110, 140, 220}, {110, 214, 224, 220}, {240, 150, 80, 220},
            {180, 140, 240, 220}};
        Tint tint = kTints[index % 7];
        if (selected) {
            // Lift the selected seal towards white so it reads as highlighted even
            // before the reticle is noticed.
            tint.r = u8(tint.r + (255 - tint.r) / 3);
            tint.g = u8(tint.g + (255 - tint.g) / 3);
            tint.b = u8(tint.b + (255 - tint.b) / 3);
            tint.a = 255;
        }
        return tint;
    }

    static constexpr Tint kReticle{255, 96, 40, 255};
    static constexpr TextRenderer::Style kLabelStyle = {235, 240, 250, 255, 2, 1, 2};

    f32 centre_[3] = {0.0f, 0.0f, 0.0f};
    f32 radius_ = 0.0f;
    f32 seal_size_ = 0.0f;
    bool configured_ = false;
    std::vector<Vertex> geometry_{};
    TextRenderer text_{};
};

} // namespace gf
