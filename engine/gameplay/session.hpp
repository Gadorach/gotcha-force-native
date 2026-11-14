#pragma once
// Game session: the integration point that turns a frame into observable work.
//
// This is the layer the testing milestone needs. It owns the world, drives a
// fixed-step update from mapped input, turns the world into draw submissions
// through the render seam, and round-trips its state through the save seam.
// It contains no platform code and no globals, so the same session runs under
// a fake, headless or OpenGL backend unchanged.
#include <cstring>
#include <string>
#include <vector>

#include "engine/assets/mesh.hpp"
#include "engine/core/context.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/input/input_map.hpp"
#include "engine/input/input_router.hpp"
#include "engine/input/touch_overlay.hpp"
#include "engine/save/save_engine.hpp"
#include "engine/scene/world.hpp"

namespace gf {

// One drawable node in the scene. Kept separate from Transform so the render
// path has a stable, serialisable description of what to submit.
struct Drawable {
    EntityId entity = kInvalidEntity;
    u32 mesh = 0;
    u32 texture = 0;
    u32 first_vertex = 0;
    u32 vertex_count = 0;
    bool visible = true;
    bool blend = false;
};

// Serialised session state. Versioned through SaveEngine; never a raw struct.
struct SessionState {
    u32 tick = 0;
    std::vector<Transform> transforms;
    std::vector<EntityId> parents;
    std::vector<u8> visible;
};

class Session {
public:
    Session(Context& context, RenderEngine& render)
        : context_(context), render_(render), saves_(*context.backends().save) {
    }

    World& world() { return world_; }
    const World& world() const { return world_; }
    std::vector<Drawable>& drawables() { return drawables_; }
    const std::vector<Drawable>& drawables() const { return drawables_; }
    u32 tick() const { return tick_; }
    // Device→game mapping for every source; the session never sees a device.
    InputRouter& router() { return router_; }
    const InputRouter& router() const { return router_; }

    // On-screen controls. Configuring the overlay also configures hit testing:
    // InputRouter's touch layout is derived from the overlay so the drawn
    // control and the responsive control cannot drift apart.
    TouchOverlay& overlay() { return overlay_; }
    const TouchOverlay& overlay() const { return overlay_; }
    void configure_overlay(const OverlayConfig& config) {
        overlay_.configure(config);
        router_.set_touch_layout(overlay_.layout());
    }
    // Call after changing buttons/stick so both sides pick up the new geometry.
    void refresh_overlay_layout() { router_.set_touch_layout(overlay_.layout()); }

    // Builds a small deterministic scene. Real content will replace this; the
    // shape (a root, a child, a drawable per entity) is the part that matters.
    void bootstrap_scene() {
        world_.clear();
        drawables_.clear();
        tick_ = 0;

        const EntityId root = world_.create();
        if (!root) return;
        Transform root_transform;
        root_transform.position[0] = 320.0f;
        root_transform.position[1] = 240.0f;
        world_.set_transform(root, root_transform);

        const EntityId child = world_.create(root);
        if (child) {
            Transform child_transform;
            child_transform.position[2] = 12.0f;
            child_transform.scale[0] = child_transform.scale[1] = child_transform.scale[2] = 0.5f;
            world_.set_transform(child, child_transform);
        }

        // One triangle per drawable, so a frame without geometry is a visible bug.
        Vertex first[3];
        first[0] = {-32.0f, -32.0f, 0.0f, 220, 40, 40, 255, 0.0f, 0.0f};
        first[1] = {32.0f, -32.0f, 0.0f, 40, 220, 40, 255, 1.0f, 0.0f};
        first[2] = {0.0f, 32.0f, 0.0f, 40, 40, 220, 255, 0.5f, 1.0f};
        Vertex second[3] = {first[0], first[1], first[2]};
        second[0].z = second[1].z = second[2].z = 6.0f;
        vertices_.assign(first, first + 3);
        vertices_.insert(vertices_.end(), second, second + 3);

        drawables_.push_back({root, 1, 0, 0, 3, true, false});
        if (child) drawables_.push_back({child, 2, 0, 3, 3, true, true});

        world_.update_matrices();
    }

    // One fixed simulation tick. `pad` is already mapped game input, so this
    // function never sees a device.
    void tick(const GamePadState& pad) {
        ++tick_;
        last_pad_ = pad;
        const EntityId root = root_entity();
        if (root == kInvalidEntity) return;
        const Transform* current = world_.transform(root);
        if (!current) return;
        Transform updated = *current;
        // Stick as pixels per tick; deterministic integer-ish movement.
        updated.position[0] += static_cast<f32>(pad.stick_x) * 0.25f;
        updated.position[1] -= static_cast<f32>(pad.stick_y) * 0.25f;
        if (updated.position[0] < 0.0f) updated.position[0] = 0.0f;
        if (updated.position[1] < 0.0f) updated.position[1] = 0.0f;
        world_.set_transform(root, updated);
        world_.update_matrices();
    }

    // Submits the world for one frame. Returns the number of draws issued.
    // Feeds the current device snapshot to the overlay (pointer cursor, etc.).
    void set_device_snapshot(const InputSnapshot& devices) { last_devices_ = devices; }

    unsigned last_overlay_draws() const { return last_overlay_draws_; }

    unsigned render_frame(u32 frame_index) {
        render_.begin_frame({frame_index, 0.05f, 0.06f, 0.08f});
        // World geometry is appended once per frame into the shared arena; the
        // overlay appends after it and cannot disturb these indices.
        render_.reset_arena();
        const u32 base = render_.append_vertices(vertices_.data(), vertices_.size());
        if (vertices_dirty_) vertices_dirty_ = false;
        unsigned submitted = 0;
        for (const Drawable& drawable : drawables_) {
            if (!drawable.visible || !world_.visible(drawable.entity)) continue;
            const f32* matrix = world_.world_matrix(drawable.entity);
            if (!matrix) continue;
            render_.state().blend = drawable.blend;
            render_.state().texture = drawable.texture;
            // World transform travels through the matrix palette, so movement is
            // observable to any backend (and to the frame digest).
            render_.state().position_matrix = drawable.entity;
            if (render_.upload_matrix(drawable.entity, matrix) &&
                render_.draw(Primitive::Triangles, base + drawable.first_vertex,
                             drawable.vertex_count)) {
                ++submitted;
            }
        }
        // UI pass after world geometry: overlay draws with depth off and blending
        // on, which `TouchOverlay::draw` sets and restores itself.
        last_overlay_draws_ = overlay_.draw(render_, last_pad_, last_devices_);
        render_.end_frame();
        render_.present();
        return submitted;
    }

    // ---- persistence -------------------------------------------------
    bool save(u32 slot) {
        SaveWriter writer;
        writer.u32_field(kSessionFormatVersion);
        writer.u32_field(tick_);
        const std::size_t count = world_.size();
        writer.u32_field(static_cast<u32>(count));
        for (u32 index = 1; index <= count; ++index) {
            const Transform* transform = world_.transform(index);
            if (!transform) continue;
            for (unsigned i = 0; i < 3; ++i) writer.f32_field(transform->position[i]);
            for (unsigned i = 0; i < 3; ++i) writer.f32_field(transform->rotation[i]);
            for (unsigned i = 0; i < 3; ++i) writer.f32_field(transform->scale[i]);
            writer.u32_field(world_.parent_of(index));
            writer.u32_field(world_.visible(index) ? 1u : 0u);
        }
        return saves_.write(slot, writer);
    }

    bool load(u32 slot) {
        SaveReader reader(empty_);
        if (!saves_.read(slot, reader)) return false;
        if (reader.u32_field() != kSessionFormatVersion) return false;
        const u32 saved_tick = reader.u32_field();
        const u32 count = reader.u32_field();
        if (!reader.ok() || count > kMaxEntities) return false;

        World restored;
        for (u32 index = 0; index < count; ++index) {
            Transform transform;
            for (unsigned i = 0; i < 3; ++i) transform.position[i] = reader.f32_field();
            for (unsigned i = 0; i < 3; ++i) transform.rotation[i] = reader.f32_field();
            for (unsigned i = 0; i < 3; ++i) transform.scale[i] = reader.f32_field();
            const EntityId parent = reader.u32_field();
            const u32 visible = reader.u32_field();
            if (!reader.ok()) return false;
            const EntityId created = restored.create(parent);
            if (!created) return false;
            if (!restored.set_transform(created, transform)) return false;
            restored.set_visible(created, visible != 0);
        }
        if (!reader.consumed()) return false;   // trailing bytes are a format error

        world_ = std::move(restored);
        tick_ = saved_tick;
        world_.update_matrices();
        return true;
    }

    void mark_vertices_dirty() { vertices_dirty_ = true; }

private:
    EntityId root_entity() const {
        for (EntityId id = 1; id <= world_.size(); ++id) {
            if (world_.parent_of(id) == kInvalidEntity && world_.alive(id)) return id;
        }
        return kInvalidEntity;
    }

    static constexpr u32 kSessionFormatVersion = 1;

    Context& context_;
    RenderEngine& render_;
    SaveEngine saves_;
    World world_{};
    std::vector<Drawable> drawables_;
    std::vector<Vertex> vertices_;
    InputRouter router_{};
    TouchOverlay overlay_{};
    GamePadState last_pad_{};
    InputSnapshot last_devices_{};
    unsigned last_overlay_draws_ = 0;
    std::vector<u8> empty_;
    bool vertices_dirty_ = true;
    u32 tick_ = 0;
};

} // namespace gf
