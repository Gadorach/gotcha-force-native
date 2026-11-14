#pragma once
// Render engine: portable draw submission on top of a swappable backend.
//
// The engine owns ordering and state validity. Backends own rasterisation.
// Nothing here knows about Vulkan, OpenGL, GX or any windowing system.
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <memory>

#include "engine/core/backends.hpp"
#include "engine/core/context.hpp"
#include "engine/graphics/command_packets.h"
#include "engine/graphics/lighting.hpp"
#include "engine/graphics/fog.hpp"

namespace gf {

// Portable render state, mirroring the semantics the game actually uses.
struct RenderState {
    bool depth_test = true;
    bool depth_write = true;
    bool colour_write = true,alpha_write = true;
    bool blend = false;
    u8 blend_src = 0;
    u8 blend_dst = 0;
    bool cull_backface = true;
    bool alpha_test = false;
    u8 alpha_ref = 0;
    bool cull_frontface = false;
    bool front_ccw = false;
    u8 depth_compare = 6;
    u8 alpha_compare0 = 4, alpha_compare1 = 7, alpha_op = 0, alpha_ref1 = 0;
    u8 texture_colour_mode = 4, texture_alpha_mode = 3;
    f32 texture_weight = 1.0f;
    bool texture_colour_lerp = false;
    u32 texture_low_colour = 0, texture_high_colour = 0xFFFFFFFFu;
    bool texture_after_lighting = false;
    std::array<u8,3> post_rgb_add{};
    std::array<u8,3> post_rgb_multiply{255,255,255};
    u32 texture = 0;
    u32 position_matrix = 0;
    u32 vertex_format = 0;
    LinearFog fog;
};

class RenderEngine {
public:
    explicit RenderEngine(IRenderBackend& backend) : backend_(backend) {}

    bool initialize(const RenderConfig& config, IWindowBackend* window, std::string* error) {
        return backend_.initialize(config, window, error);
    }
    RenderCapabilities capabilities() const { return backend_.capabilities(); }
    // Optional scene-global context. Legacy callers retain archive-local lights
    // and their existing draw order; invalid explicit context suppresses models.
    bool set_scene_lighting(const SceneLighting& lights){
        if(!lights.declared||!menu_light_rig_supported(lights)){lighting_valid_=false;return false;}
        scene_lighting_=lights;have_scene_lighting_=lighting_valid_=true;return true;
    }
    void clear_scene_lighting(){have_scene_lighting_=false;lighting_valid_=true;scene_lighting_={};}
    const SceneLighting* scene_lighting()const{return have_scene_lighting_&&lighting_valid_?&scene_lighting_:nullptr;}
    bool scene_context_valid()const{return lighting_valid_&&!model_pass_failed_;}
    u8 model_pass()const{return model_pass_active_?model_pass_:0;}
    bool begin_model_pass(u8 pass){
        if(!frame_open_||!lighting_valid_||model_pass_active_||(pass!=1&&pass!=2&&pass!=4))return false;
        model_pass_=pass;model_pass_start_=pending_.size();model_pass_failed_=false;model_pass_active_=true;return true;
    }
    bool end_model_pass(){
        if(!model_pass_active_)return false;
        if(model_pass_failed_){pending_.resize(model_pass_start_);depths_.resize(model_pass_start_);model_pass_active_=false;return false;}
        if(model_pass_==2||model_pass_==4){std::vector<std::size_t> order;for(auto i=model_pass_start_;i<pending_.size();++i)order.push_back(i);
            std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){return depths_[a]>depths_[b];});std::vector<DrawCall> sorted;std::vector<f32> keys;for(auto i:order){sorted.push_back(pending_[i]);keys.push_back(depths_[i]);}
            for(unsigned i=0;i<sorted.size();++i){pending_[model_pass_start_+i]=sorted[i];depths_[model_pass_start_+i]=keys[i];}
        }model_pass_active_=false;return true;
    }
    void reject_model_pass(){if(model_pass_active_)model_pass_failed_=true;}

    // Base code owns the vertex data; the backend owns how it reaches the GPU.
    // Replaces the whole arena. Prefer append_vertices when more than one
    // subsystem submits geometry in the same frame.
    bool upload_vertices(const Vertex* vertices, std::size_t count) {
        if (!vertices && count) return false;
        if (!prepare_arena_replacement()) return false;
        arena_.clear();
        if (count) arena_.assign(vertices, vertices + count);
        arena_dirty_ = true;
        return true;
    }
    // Vertex arena: world geometry and UI share one stream per frame, so an
    // overlay cannot overwrite the scene's vertices. Indices in a DrawCall refer
    // to this arena.
    u32 append_vertices(const Vertex* vertices, std::size_t count) {
        if (!vertices && count) return 0;
        const u32 base = static_cast<u32>(arena_.size());
        if (count) arena_.insert(arena_.end(), vertices, vertices + count);
        arena_dirty_ = true;
        return base;
    }
    std::size_t arena_size() const { return arena_.size(); }
    // Upload once at a submission barrier, rather than resending every growing
    // model prefix. Explicit uploads remain available for backend consumers.
    bool flush_vertices() {
        if (frame_failed_) return false;
        if (!arena_dirty_) return true;
        if (!backend_.upload_vertices(arena_.data(), arena_.size())) {
            frame_failed_ = true;
            return false;
        }
        arena_dirty_ = false;
        return true;
    }

    // Matrix palette upload; DrawCall::position_matrix selects the slot.
    bool upload_matrix(u32 slot, const f32* matrix) {
        if (!matrix) return false;
        return backend_.upload_matrix(slot, matrix);
    }
    bool upload_texture(u32 handle, const TextureDesc& desc) {
        if (!handle || !desc.width || !desc.height) return false;
        forget_shared_texture(handle);
        return backend_.upload_texture(handle, desc);
    }
    // Explicit immutable resource namespace. Equal IDs require byte-identical
    // images AND samplers; collisions refuse instead of overwriting another bank.
    bool upload_texture_shared(u32 handle,const TextureDesc& desc){
        auto failed=[&](){shared_upload_error_=true;return false;};
        if(!handle||!desc.pixels||!desc.width||!desc.height||desc.width>4096||desc.height>4096||desc.wrap_s>2||desc.wrap_t>2)return failed();
        const u64 size=u64(desc.width)*desc.height*4;if(size>(64u<<20))return failed();
        auto found=shared_textures_.find(handle);
        if(found!=shared_textures_.end()){const auto& x=found->second;const bool equal=x.width==desc.width&&x.height==desc.height&&x.wrap_s==desc.wrap_s&&x.wrap_t==desc.wrap_t&&x.linear==desc.linear&&std::equal(x.pixels->begin(),x.pixels->end(),desc.pixels);return equal?true:failed();}
        if(shared_texture_bytes_+size>(128u<<20)||!backend_.upload_texture(handle,desc))return failed();
        SharedTexture x;x.width=desc.width;x.height=desc.height;x.wrap_s=desc.wrap_s;x.wrap_t=desc.wrap_t;x.linear=desc.linear;x.hash=1469598103934665603ull;for(std::size_t i=0;i<size;++i)x.hash=(x.hash^desc.pixels[i])*1099511628211ull;
        for(const auto& [id,prior]:shared_textures_){(void)id;if(prior.hash==x.hash&&prior.pixels->size()==size&&std::equal(prior.pixels->begin(),prior.pixels->end(),desc.pixels)){x.pixels=prior.pixels;break;}}
        if(!x.pixels)x.pixels=std::make_shared<const std::vector<u8>>(desc.pixels,desc.pixels+std::size_t(size));
        shared_texture_bytes_+=std::size_t(size);shared_textures_.emplace(handle,std::move(x));return true;
    }
    std::size_t shared_texture_count()const{return shared_textures_.size();}
    std::size_t shared_texture_resident_bytes()const{std::vector<const void*> seen;std::size_t bytes=0;for(const auto& [id,t]:shared_textures_){(void)id;if(std::find(seen.begin(),seen.end(),t.pixels.get())==seen.end()){seen.push_back(t.pixels.get());bytes+=t.pixels->size();}}return bytes;}
    bool shared_texture_upload_failed()const{return shared_upload_error_;}
    void clear_shared_texture_upload_error(){shared_upload_error_=false;}
    // Mid-frame ordering barrier: preceding geometry must reach the framebuffer
    // BEFORE capture, while following draws sample the resulting owned texture.
    // A backend failure can follow emitted draws; no source atomicity/replay claim.
    bool copy_framebuffer(const FramebufferCopy& copy) {
        if(!frame_open_||model_pass_active_||!framebuffer_copy_valid(copy)||!backend_.capabilities().supports_framebuffer_copy)return false;
        forget_shared_texture(copy.texture);
        if (!flush_pending_draws()) return false;
        if (!backend_.copy_framebuffer(copy)) { frame_failed_=true; return false; }
        return true;
    }
    void shutdown() { shared_textures_.clear();shared_texture_bytes_=0;backend_.shutdown(); }

    // State changes are recorded, not applied immediately, so a backend can
    // reorder or merge work without changing observable ordering.
    RenderState& state() { return state_; }
    const RenderState& state() const { return state_; }

    // A draw is only valid if it references geometry the engine has been told
    // about. The bounds are checked here because backends differ in how they
    // report errors.
    bool draw(Primitive primitive, u32 first_vertex, u32 vertex_count) {
        if(frame_failed_)return false;
        if(model_pass_active_&&!model_enqueue_){model_pass_failed_=true;return false;}
        if (vertex_count == 0 || first_vertex + vertex_count < first_vertex) return false;
        if (static_cast<std::size_t>(first_vertex) + vertex_count > arena_.size()) return false;
        if(!linear_fog_valid(state_.fog))return false;
        if(state_.fog.enabled)for(std::size_t i=first_vertex;i<std::size_t(first_vertex)+vertex_count;++i){const auto inv=arena_[i].inv_w;if(!std::isfinite(inv)||inv<1e-7f||inv>1e6f)return false;}
        DrawCall call;
        call.primitive = primitive;
        call.vertex_format = state_.vertex_format;
        call.vertex_first = first_vertex;
        call.vertex_count = vertex_count;
        call.texture = state_.texture;
        call.position_matrix = state_.position_matrix;
        call.depth_test = state_.depth_test;
        call.depth_write = state_.depth_write;
        call.colour_write=state_.colour_write;call.alpha_write=state_.alpha_write;
        call.blend = state_.blend;
        call.blend_src = state_.blend_src;
        call.blend_dst = state_.blend_dst;
        call.cull_backface = state_.cull_backface;
        call.alpha_test = state_.alpha_test;
        call.alpha_ref = state_.alpha_ref;
        call.cull_frontface = state_.cull_frontface;
        call.front_ccw = state_.front_ccw;
        call.depth_compare = state_.depth_compare;
        call.alpha_compare0 = state_.alpha_compare0;
        call.alpha_compare1 = state_.alpha_compare1;
        call.alpha_op = state_.alpha_op;
        call.alpha_ref1 = state_.alpha_ref1;
        call.texture_colour_mode = state_.texture_colour_mode;
        call.texture_alpha_mode = state_.texture_alpha_mode;
        call.texture_weight = state_.texture_weight;
        call.texture_colour_lerp = state_.texture_colour_lerp;
        call.texture_low_colour = state_.texture_low_colour;
        call.texture_high_colour = state_.texture_high_colour;
        call.texture_after_lighting = state_.texture_after_lighting;
        call.post_rgb_add=state_.post_rgb_add;
        call.post_rgb_multiply=state_.post_rgb_multiply;
        call.fog=state_.fog;
        pending_.push_back(call);
        depths_.push_back(0);
        return true;
    }
    bool draw_model(Primitive primitive,u32 first,u32 count,f32 depth){
        if(!scene_context_valid()||!std::isfinite(depth)||std::abs(depth)>1e7f){if(model_pass_active_)model_pass_failed_=true;return false;}
        model_enqueue_=true;const bool ok=draw(primitive,first,count);model_enqueue_=false;
        if(ok)depths_.back()=depth;else if(model_pass_active_)model_pass_failed_=true;return ok;
    }

    // Matrix/light uploads are built by the portable command layer and handed to
    // the backend as opaque packets; a backend that speaks a native pipeline can
    // translate them, a backend that replays a command stream can forward them.
    void submit_packet(const gf_gfx_packet& packet) {
        for (unsigned i = 0; i < packet.count; ++i) {
            packet_writes_.push_back(packet.writes[i]);
        }
    }

    // Empties the vertex arena. Call per frame if the caller resubmits geometry;
    // upload-once assets can be appended ahead of time and left in place.
    void reset_arena() {
        if (!prepare_arena_replacement()) return;
        arena_.clear();
        arena_dirty_ = true;
    }

    void begin_frame(const FrameDesc& frame) {
        pending_.clear();
        depths_.clear();model_pass_active_=model_pass_failed_=model_enqueue_=false;
        flushed_draws_=0;frame_open_=true;frame_failed_=false;
        packet_writes_.clear();
        backend_.begin_frame(frame);
    }
    // Flushes this frame's work. Ordering across draws is preserved.
    bool end_frame() {
        if(model_pass_active_){pending_.resize(model_pass_start_);depths_.resize(model_pass_start_);model_pass_active_=false;model_pass_failed_=true;}
        flush_pending_draws();
        backend_.end_frame();
        frame_open_=false;
        return !frame_failed_;
    }
    void present() { if(!frame_failed_)backend_.present(); }
    bool frame_failed() const { return frame_failed_; }

    std::size_t pending_draws() const { return pending_.size(); }
    std::size_t pending_packet_writes() const { return packet_writes_.size(); }

private:
    struct SharedTexture {u32 width=0,height=0;u8 wrap_s=0,wrap_t=0;bool linear=false;u64 hash=0;std::shared_ptr<const std::vector<u8>> pixels;};
    void forget_shared_texture(u32 id){auto p=shared_textures_.find(id);if(p!=shared_textures_.end()){shared_texture_bytes_-=p->second.pixels->size();shared_textures_.erase(p);}}
    bool flush_pending_draws(){
        if(frame_failed_)return false;
        if(flushed_draws_==pending_.size())return true;
        if(!flush_vertices())return false;
        for(;flushed_draws_<pending_.size();++flushed_draws_)backend_.submit(pending_[flushed_draws_]);
        return true;
    }
    bool prepare_arena_replacement(){
        if(!frame_open_)return true;
        // Sorting a live pass across a replaced vertex arena is not meaningful.
        if(model_pass_active_){frame_failed_=true;model_pass_failed_=true;return false;}
        return flush_pending_draws();
    }
    IRenderBackend& backend_;
    RenderState state_{};
    std::vector<DrawCall> pending_;
    std::vector<f32> depths_;SceneLighting scene_lighting_;bool have_scene_lighting_=false,lighting_valid_=true;
    bool model_pass_active_=false,model_pass_failed_=false,model_enqueue_=false;u8 model_pass_=0;std::size_t model_pass_start_=0;
    std::size_t flushed_draws_=0;bool frame_open_=false,frame_failed_=false;
    std::vector<Vertex> arena_;
    bool arena_dirty_ = true;
    std::vector<gf_gfx_write> packet_writes_;
    std::unordered_map<u32,SharedTexture> shared_textures_;std::size_t shared_texture_bytes_=0;bool shared_upload_error_=false;
};

} // namespace gf
