#pragma once
// Deterministic frame digest for headless testing.
//
// A frame is only testable if "did this frame come out the same?" has an answer.
// Hashing the observable output (draw calls and their state, plus the vertex
// stream actually uploaded) gives regression detection without needing a GPU,
// a golden image, or a display.
//
// This is a *test* facility: it hashes what was submitted, not what a driver
// rasterised. Two different backends can hash equal while rendering differently,
// so a mismatch is a real change in submitted work and a match is not a proof of
// identical pixels.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/core/backends.hpp"

#include "engine/core/types.hpp"

namespace gf {

class FrameDigest {
public:
    void reset() { hash_ = kOffset; draws_ = 0; vertices_ = 0; }

    void add_scalar(u32 value) {
        hash_ ^= value & 0xFFu;
        hash_ *= kPrime;
        hash_ ^= (value >> 8) & 0xFFu;
        hash_ *= kPrime;
        hash_ ^= (value >> 16) & 0xFFu;
        hash_ *= kPrime;
        hash_ ^= (value >> 24) & 0xFFu;
        hash_ *= kPrime;
    }

    void add_float(f32 value) {
        add_scalar(f32_to_bits(value));
    }

    void add_draw(const DrawCall& call) {
        add_scalar(static_cast<u32>(call.primitive));
        add_scalar(call.vertex_format);
        add_scalar(call.vertex_first);
        add_scalar(call.vertex_count);
        add_scalar(call.texture);
        add_scalar(call.position_matrix);
        add_scalar(pack_flags(call));
        add_scalar(call.cull_frontface | (u32(call.front_ccw) << 1) | (u32(call.depth_compare) << 8));
        add_scalar(call.alpha_compare0 | (u32(call.alpha_compare1) << 8) |
                   (u32(call.alpha_op) << 16) | (u32(call.alpha_ref1) << 24));
        add_scalar(call.texture_colour_mode | (u32(call.texture_alpha_mode) << 8));
        add_float(call.texture_weight);
        add_scalar(call.texture_colour_lerp);add_scalar(call.texture_low_colour);add_scalar(call.texture_high_colour);
        add_scalar(call.texture_after_lighting);
        for(auto v:call.post_rgb_add)add_scalar(v);
        if(call.post_rgb_multiply!=std::array<u8,3>{255,255,255}){add_scalar(0x4d554c54u);for(auto v:call.post_rgb_multiply)add_scalar(v);}
        add_scalar(u32(call.colour_write)|(u32(call.alpha_write)<<1));
        if(call.fog.enabled){add_scalar(0x464f474cu);add_float(call.fog.start);add_float(call.fog.end);for(auto v:call.fog.colour)add_scalar(v);}
        ++draws_;
    }

    void add_vertices(const Vertex* vertices, std::size_t count) {
        if (!vertices) return;
        for (std::size_t i = 0; i < count; ++i) {
            add_float(vertices[i].x);
            add_float(vertices[i].y);
            add_float(vertices[i].z);
            add_scalar((u32(vertices[i].r) << 24) | (u32(vertices[i].g) << 16) |
                       (u32(vertices[i].b) << 8) | u32(vertices[i].a));
            add_float(vertices[i].u);
            add_float(vertices[i].v);
            add_float(vertices[i].inv_w);
            for(unsigned k=0;k<4;++k){add_scalar(vertices[i].light[k]);add_scalar(vertices[i].specular[k]);}
        }
        vertices_ += count;
    }

    // Hash of the world matrices a scene produced, so scene regressions surface
    // in the same digest as render regressions.
    void add_matrix(const f32* matrix) {
        if (!matrix) { add_scalar(0xDEADBEEFu); return; }
        for (unsigned i = 0; i < 12; ++i) add_float(matrix[i]);
    }

    u64 value() const { return hash_; }
    unsigned draws() const { return draws_; }
    std::size_t vertices() const { return vertices_; }

private:
    static u32 pack_flags(const DrawCall& call) {
        u32 flags = 0;
        if (call.depth_test) flags |= 1u << 0;
        if (call.depth_write) flags |= 1u << 1;
        if (call.blend) flags |= 1u << 2;
        if (call.cull_backface) flags |= 1u << 3;
        flags |= u32(call.blend_src & 0xFu) << 4;
        flags |= u32(call.blend_dst & 0xFu) << 8;
        if (call.alpha_test) flags |= 1u << 12;
        flags |= u32(call.alpha_ref) << 13;
        return flags;
    }

    static constexpr u64 kOffset = 1469598103934665603ull;
    static constexpr u64 kPrime = 1099511628211ull;
    u64 hash_ = kOffset;
    unsigned draws_ = 0;
    std::size_t vertices_ = 0;
};

// A render backend that records submitted work into a FrameDigest instead of
// drawing. Swapped in for tests, so the same game code path is exercised with no
// GPU present.
class RecordingRenderBackend final : public IRenderBackend {
public:
    bool initialize(const RenderConfig&, IWindowBackend*, std::string*) override { return true; }
    void shutdown() override {}
    RenderCapabilities capabilities() const override { return {0, false, false, "recording"}; }
    bool upload_vertices(const Vertex* vertices, std::size_t count) override {
        digest_.add_vertices(vertices, count);
        return true;
    }
    bool upload_matrix(u32 slot, const f32* matrix) override {
        digest_.add_scalar(slot);
        digest_.add_matrix(matrix);
        return matrix != nullptr;
    }
    bool upload_texture(u32 handle, const TextureDesc& desc) override {
        digest_.add_scalar(handle);
        digest_.add_scalar(desc.width);
        digest_.add_scalar(desc.height);
        return handle != 0 && desc.width != 0 && desc.height != 0;
    }
    void begin_frame(const FrameDesc& frame) override {
        frames_.push_back(digest_.value());
        digest_.reset();
        digest_.add_scalar(frame.frame_index);
        digest_.add_float(frame.clear_r);
        digest_.add_float(frame.clear_g);
        digest_.add_float(frame.clear_b);
    }
    void submit(const DrawCall& call) override { digest_.add_draw(call); }
    void end_frame() override { digest_.add_scalar(0xE0F0E0F0u); }
    void present() override { last_ = digest_.value(); }

    u64 last_frame_digest() const { return last_; }
    unsigned draws() const { return digest_.draws(); }
    const std::vector<u64>& frame_digests() const { return frames_; }

private:
    FrameDigest digest_{};
    std::vector<u64> frames_;
    u64 last_ = 0;
};

} // namespace gf
