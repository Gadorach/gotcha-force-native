#pragma once
// Four owned normal/alternate model instances, bound to one isolated native
// parent. Source56AA8 selection is computed at the later registration boundary.
// Scene/retirement observations are DECLARED; no full actor/scene admission.
#include "engine/gameplay/startup_actor_frame.hpp"
#include "engine/gameplay/startup_attachment_state.hpp"
#include "engine/gameplay/startup_render_selection.hpp"
#include "engine/gameplay/attachment_alternate_scene.hpp"
namespace gf {
enum class StartupAttachmentPairMode {DeclaredSceneObservations};
struct StartupAttachmentPairInput {
    bool scene_fields_declared=false,unmasked_child_tick_declared=false;
    u8 retire662=0,selector3ec=0,scene34=0,parent_active0=1;
    // callback_called, timer720 and stored_actor_index are derived, not accepted
    // from these fields. The caller declares the other scene/viewer inputs.
    StartupRenderSelectionInput registration;
};
class StartupAttachmentPairs {
public:
    bool bind(const StartupActorFrame& parent,ByteView bank,ByteView profile,ByteView fade,ByteView alternate,
              StartupAttachmentPairMode mode,std::string* error){
        ready_=false;storage_.reset();
        if(mode!=StartupAttachmentPairMode::DeclaredSceneObservations||!parent.ready()||!parent.model()||!parent.state()||parent.state()->frames!=0||parent.state()->published||!bank.data||!bank.size||bank.size>(4u<<20))return fail(error,"fresh isolated parent/bank/mode required");
        auto next=std::make_unique<Storage>();const auto binding=parent.model()->instance_binding();
        if(!binding||!startup_attachments_load(profile,binding->model_hash,&next->profile,error)||!actor_fade_load(fade,&next->fade,error))return false;
        if(next->profile.model_id!=parent.state()->scalars.klass.model_id||!parent.state()->scalars.klass.callbacks_initialized||character_program_model_hash(bank)!=binding->model_hash||!parent.state()->material.bound(*parent.model()))return fail(error,"parent/bank/class/material scope");
        for(unsigned i=0;i<2;++i)if(!next->normal[i].load(bank,next->profile.entries[i].record,error)||!next->alternate[i].load(bank,next->profile.entries[i].record,alternate,error))return false;
        next->parent=binding;next->stored_index=parent.state()->dispatch.state().render_index;storage_=std::move(next);ready_=true;if(error)error->clear();return true;
    }
    bool step(const StartupActorFrame& parent,const StartupAttachmentPairInput& input,std::string* error){
        if(!ready_||!storage_||!bound(parent))return refuse(error,"native parent lifetime/model scope");
        auto& r=*storage_;const auto& source=*parent.state();
        if(!input.scene_fields_declared||!input.unmasked_child_tick_declared||!source.published||!parent.model()->ready()||source.frames!=r.frames+1)return refuse(error,"declared scene/tick and consecutive accepted parent required");
        auto children=r.children;std::array<StartupAttachmentUpdate,2> updates;std::array<AttachmentBoneTransform,2> transforms;
        StartupAttachmentParent observed;observed.model_id=source.scalars.klass.model_id;observed.mode6e8=source.vm.state().actor.control0[2];observed.reload784=source.scalars.reloads[2].current;observed.retire662=input.retire662;
        // Child state/transform stage precedes render registration. Do NOT use
        // AE selection here to decide visibility or mutate the native parent.
        for(unsigned i=0;i<2;++i){if(!startup_attachment_state_step(r.profile,i,r.stored_index,observed,children[i],&updates[i],error))return refuse(error,"child state refused");children[i]=updates[i].state;
            if(children[i].allocated&&children[i].phase==1&&children[i].render_group){if(!attachment_bone_transform(source.world.matrices,children[i].transform,&transforms[i],error))return refuse(error,"parent bone transform refused");
                if(r.profile.kind&&updates[i].visible){const f32 scale[3]={children[i].draw_scale,children[i].draw_scale,children[i].draw_scale};transforms[i].world=mat4_multiply(transforms[i].world,mat4_scale(scale));}}
        }
        const bool display_group=source.dispatch.state().render_group!=0&&input.scene34==0; //8008A0A8 whole actor-list gate
        auto registration=input.registration;registration.parent_callback_called=display_group&&input.parent_active0!=0;registration.timer720=source.scalars.timer720;registration.stored_actor_index=r.stored_index;
        StartupRenderSelection selection;if(!startup_render_selection(registration,r.selection.alternate_ae,&selection,error))return refuse(error,"parent selection refused");
        ActorFadeMaterial material;if(!actor_fade_material(r.fade,source.material.state(),&material))return refuse(error,"bound parent fade profile/state");
        std::array<bool,2> selected{};
        for(unsigned i=0;i<2;++i){r.normal[i].discard_pose();r.alternate[i].discard_pose();if(!updates[i].visible||!display_group)continue;
            selected[i]=selection.alternate_ae!=0||(!r.profile.kind&&input.selector3ec==4);
            const bool ok=selected[i]?r.alternate[i].set_world_fade(transforms[i],material,error):r.normal[i].set_world_fade(transforms[i],material,false,error);
            if(!ok)return refuse(error,"selected child publication refused");
        }
        // Free CPU model instances/generations only AFTER all fallible work.
        // GPU texture ownership remains the render backend's, not a GC pool claim.
        for(unsigned i=0;i<2;++i)if(updates[i].released){r.normal[i]=AttachmentModelScene{};r.alternate[i]=AttachmentAlternateModelScene{};}
        r.children=children;r.updates=updates;r.selected=selected;r.selection=selection;r.frames=source.frames;r.display_group=display_group;r.published=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){
        if(!storage_)return 0;
        unsigned n=0;for(unsigned i=0;i<2;++i){n+=storage_->normal[i].upload_textures(render,base+n);n+=storage_->alternate[i].upload_textures(render,base+n);}return n;
    }
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!storage_)return 0;unsigned n=0;for(unsigned i=0;i<2;++i){n+=storage_->normal[i].upload_textures_shared(render,base);n+=storage_->alternate[i].upload_textures_shared(render,base);}return n;}
    unsigned draw_model(unsigned i,const StartupActorFrame& parent,RenderEngine& render,const Camera& camera){if(i>1||!ready_||!storage_||!storage_->published||!storage_->display_group||!bound(parent)||!parent.state()->published||!parent.model()->ready()||!parent.state()->dispatch.state().render_group||parent.state()->frames!=storage_->frames)return 0;return storage_->updates[i].visible&&storage_->children[i].render_group?(storage_->selected[i]?storage_->alternate[i].draw(render,camera):storage_->normal[i].draw(render,camera)):0;}
    unsigned draw(const StartupActorFrame& parent,RenderEngine& render,const Camera& camera){
        if(!ready_||!storage_||!storage_->published||!storage_->display_group||!bound(parent)||!parent.state()->published||!parent.model()->ready()||!parent.state()->dispatch.state().render_group||parent.state()->frames!=storage_->frames)return 0;
        unsigned n=0;for(unsigned i=0;i<2;++i)if(storage_->updates[i].visible&&storage_->children[i].render_group)n+=storage_->selected[i]?storage_->alternate[i].draw(render,camera):storage_->normal[i].draw(render,camera);return n;
    }
    bool bound(const StartupActorFrame& parent)const{
        if(!storage_||!parent.ready()||!parent.model()||!parent.state())return false;
        const auto scope=storage_->parent.lock();return scope&&scope==parent.model()->instance_binding()&&parent.state()->scalars.klass.model_id==storage_->profile.model_id&&parent.state()->material.bound(*parent.model());
    }
    bool ready()const{return ready_&&bool(storage_);}
    u32 frames()const{return storage_?storage_->frames:0;}
    const StartupRenderSelection* selection()const{return storage_?&storage_->selection:nullptr;}
    const std::array<StartupAttachmentState,2>* states()const{return storage_?&storage_->children:nullptr;}
    const std::array<StartupAttachmentUpdate,2>* updates()const{return storage_?&storage_->updates:nullptr;}
    const AttachmentModelScene* normal(unsigned i)const{return storage_&&i<2?&storage_->normal[i]:nullptr;}
    const AttachmentAlternateModelScene* alternate(unsigned i)const{return storage_&&i<2?&storage_->alternate[i]:nullptr;}
private:
    struct Storage {StartupAttachmentProfile profile;ActorFadeProfile fade;std::weak_ptr<const CharacterModelBinding> parent;
        std::array<AttachmentModelScene,2> normal;std::array<AttachmentAlternateModelScene,2> alternate;std::array<StartupAttachmentState,2> children{};
        std::array<StartupAttachmentUpdate,2> updates{};std::array<bool,2> selected{};StartupRenderSelection selection;u32 frames=0;u8 stored_index=0;bool published=false,display_group=false;};
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup attachment pairs: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=false;if(storage_){storage_->published=false;for(unsigned i=0;i<2;++i){storage_->normal[i].discard_pose();storage_->alternate[i].discard_pose();}}fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false;
};
} // namespace gf
