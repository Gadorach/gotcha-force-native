#pragma once
#include "engine/assets/battle_initial_presentation.hpp"
#include "engine/gameplay/battle_split_projection.hpp"
#include "engine/graphics/scene_draw.hpp"
namespace gf {
enum class BattleInitialPresentationPhase {Closed,Initialized,WorldViewBegun,Failed};
struct BattleInitialPresentationState {
    BattleInitialPresentationPhase phase=BattleInitialPresentationPhase::Closed;
    u8 common31=0,common33=0,common34=0,half=0,alias=0,view=255;
    s8 common28=-1;u32 history_texture=0;bool history_restored=false,frame_composed=false,preparation_transition_complete=false;
    Camera camera;RenderFrameReceipt render_frame;
};
class BattleInitialPresentation {
public:
    const BattleInitialPresentationState* state()const{return state_.phase==BattleInitialPresentationPhase::Initialized||(state_.phase==BattleInitialPresentationPhase::WorldViewBegun&&state_.render_frame.live())?&state_:nullptr;}
private:
    friend class BattlePreparationBootstrap;
    bool initialize(const BattleInitialPresentationProfile& profile,u8 layout,u8 split,u8 source33,std::string* error){
        if(state_.phase!=BattleInitialPresentationPhase::Closed||!profile.ready()||layout!=1||split!=1||source33!=1){if(error)*error="initial presentation: fresh source two-player horizontal-split handoff required";return false;}
        try{
            //2AAAC clears the full1MiB source allocation. Its RGBA8 image uses
            //the first640*224*4 bytes; no packed original texture data is copied.
            history_.assign(0x100000,0);state_.common31=0;state_.common33=0;state_.phase=BattleInitialPresentationPhase::Initialized;
            if(error){error->clear();}return true;
        }catch(const std::exception&){state_.phase=BattleInitialPresentationPhase::Failed;if(error)*error="initial presentation: uncertain history allocation";return false;}
    }
    bool begin_first_view(const BattleInitialPresentationProfile& profile,const BattleSplitProjectionProfile& projection,RenderEngine& render,u32 history_texture,u8 actual_view,const std::array<f32,3>& eye,const std::array<f32,3>& target,const std::array<f32,3>& up,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("initial battle world view: ")+why;return false;};
        if(state_.phase!=BattleInitialPresentationPhase::Initialized||!profile.ready()||!projection.ready()||!history_texture||actual_view>=6||history_.size()!=0x100000||!render.frame_receipt().live()||render.model_pass_active()||!render.scene_context_valid()||render.canvas_size()!=std::array<u32,2>{640,448}||!render.capabilities().supports_framebuffer_copy||render.capabilities().max_texture_size<640||render.shared_texture_reserved(history_texture))return fail("fresh matching render frame/canvas/copy capability/history namespace required");
        Camera camera;for(unsigned i=0;i<3;++i){camera.eye[i]=eye[i];camera.target[i]=target[i];camera.up[i]=up[i];}
        if(!battle_split_projection(projection,0,&camera,error))return false;
        const auto frame=render.frame_receipt();
        state_.phase=BattleInitialPresentationPhase::Failed; //effects below may reach the backend; no replay
        TextureDesc texture;texture.width=640;texture.height=224;texture.pixels=history_.data();texture.wrap_s=texture.wrap_t=0;texture.linear=false;
        if(!render.upload_texture(history_texture,texture))return fail("source initial history texture upload failed");
        //2ACD0 restores the previous/zero history into the opposite half. The
        //first parity0 pass draws into viewport7 at Y0, so history goes to Y224.
        std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,0,224,640,448,0,0,0,1,1,{255,255,255,255});
        const auto saved=render.state();RenderState restore;restore.texture=history_texture;restore.position_matrix=kIdentityPositionMatrix;restore.depth_test=true;restore.depth_compare=7;restore.depth_write=false;restore.blend=false;restore.cull_backface=true;restore.colour_write=restore.alpha_write=true;
        render.state()=restore;const auto first=render.append_vertices(vertices.data(),vertices.size());const bool restored=render.draw(Primitive::Triangles,first,u32(vertices.size()));render.state()=saved;
        if(!restored)return fail("source retained-half draw failed");
        if(!frame.same_frame(render.frame_receipt()))return fail("render frame changed during history initialization");
        state_.phase=BattleInitialPresentationPhase::WorldViewBegun;state_.common34=0;state_.common28=s8(actual_view);state_.view=actual_view;state_.alias=0;state_.half=0;state_.history_texture=history_texture;state_.history_restored=true;state_.camera=camera;state_.render_frame=frame;
        if(error){error->clear();}return true;
    }
    BattleInitialPresentationState state_;std::vector<u8> history_;
};
}
