#pragma once
// Native normal-style bank-child companion to the isolated actor frame.
// AE=0/3EC!=4/662=0 are DECLARED unrepresented parent fields. Other inputs
// (mode6E8, reload784, world, fade) come from the bound accepted native parent.
// Not a full constructor: alternate copies/C74EC/allocator limits remain unbound.
#include "engine/gameplay/startup_actor_frame.hpp"
#include "engine/gameplay/startup_attachment_state.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
namespace gf {
enum class StartupAttachmentOwnerMode {DeclaredNormalParent};
class StartupActorBankAttachments {
public:
    bool bind(const StartupActorFrame& parent,ByteView bank,ByteView profile,ByteView fade,
              StartupAttachmentOwnerMode mode,std::string* error){
        ready_=false;storage_.reset();
        if(mode!=StartupAttachmentOwnerMode::DeclaredNormalParent||!parent.ready()||!parent.state()||!parent.model()||parent.state()->frames!=0||parent.state()->published||!bank.data||!bank.size||bank.size>(4u<<20))return fail(error,"fresh isolated parent/normal-field contract/bank budget required");
        auto next=std::make_unique<Storage>();const auto binding=parent.model()->instance_binding();
        if(!binding||!startup_attachments_load(profile,binding->model_hash,&next->profile,error)||!actor_fade_load(fade,&next->fade,error))return false;
        const auto& p=next->profile;
        if(parent.state()->scalars.klass.model_id!=p.model_id||!parent.state()->scalars.klass.callbacks_initialized||character_program_model_hash(bank)!=p.model_hash||!parent.state()->material.bound(*parent.model()))return fail(error,"parent/model/class/material scope");
        for(unsigned i=0;i<2;++i)if(!next->models[i].load(bank,p.entries[i].record,error))return false;
        next->parent=binding;next->render_index=parent.state()->dispatch.state().render_index;storage_=std::move(next);ready_=true;if(error)error->clear();return true;
    }
    bool update(const StartupActorFrame& parent,std::string* error){
        if(!ready_||!storage_||!bound(parent))return refuse(error,"native parent lifetime/model scope");
        auto& r=*storage_;const auto& source=*parent.state();
        if(!source.published||!parent.model()->ready()||source.frames!=r.frames+1)return refuse(error,"accepted consecutive parent frame required; no duplicate/catch-up update");
        StartupAttachmentParent observation;observation.model_id=source.scalars.klass.model_id;
        observation.mode6e8=source.vm.state().actor.control0[2]; // NOT opaque1D94/1D95 model selections
        observation.reload784=source.scalars.reloads[2].current;
        ActorFadeMaterial material;if(!actor_fade_material(r.fade,source.material.state(),&material))return refuse(error,"bound parent fade profile/state");
        auto next=r.children;std::array<AttachmentBoneTransform,2> transforms;std::array<bool,2> visible{};
        for(unsigned i=0;i<2;++i){StartupAttachmentUpdate step;
            if(!startup_attachment_state_step(r.profile,i,r.render_index,observation,next[i],&step,error))return refuse(error,"source child bookkeeping refused");
            next[i]=step.state;visible[i]=step.visible;
            if(!step.state.allocated||step.state.phase!=1||!step.state.render_group)continue;
            if(!attachment_bone_transform(source.world.matrices,step.state.transform,&transforms[i],error))return refuse(error,"parent-bone transform refused");
            if(r.profile.kind&&visible[i]){const std::array<f32,3> scale={step.state.draw_scale,step.state.draw_scale,step.state.draw_scale};transforms[i].world=mat4_multiply(transforms[i].world,mat4_scale(scale.data()));}
            if(step.alternate&&visible[i])return refuse(error,"alternate material consumer required");
        }
        // Late skin/material failure invalidates BOTH child publications and
        // discards numeric child steps; the separately accepted parent stays put.
        for(unsigned i=0;i<2;++i){r.models[i].discard_pose();
            if(visible[i]&&!r.models[i].set_world_fade(transforms[i],material,false,error))return refuse(error,"child skin/material publication refused");
        }
        r.children=next;r.visible=visible;r.frames=source.frames;r.published=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){
        if(!storage_)return 0;
        unsigned total=0;for(auto& model:storage_->models)total+=model.upload_textures(render,base+total);return total;
    }
    unsigned draw(const StartupActorFrame& parent,RenderEngine& render,const Camera& camera){
        if(!ready_||!storage_||!storage_->published||!bound(parent)||!parent.state()->published||!parent.model()->ready()||parent.state()->frames!=storage_->frames)return 0;
        unsigned count=0;for(unsigned i=0;i<2;++i)if(storage_->visible[i]&&storage_->children[i].render_group)count+=storage_->models[i].draw(render,camera);return count;
    }
    bool bound(const StartupActorFrame& parent)const{
        if(!storage_||!parent.ready()||!parent.model()||!parent.state())return false;
        const auto binding=storage_->parent.lock();return binding&&binding==parent.model()->instance_binding()&&parent.state()->scalars.klass.model_id==storage_->profile.model_id&&parent.state()->material.bound(*parent.model());
    }
    bool ready()const{return ready_&&bool(storage_);}
    u32 frames()const{return storage_?storage_->frames:0;}
    const std::array<StartupAttachmentState,2>* states()const{return storage_?&storage_->children:nullptr;}
    const AttachmentModelScene* model(unsigned index)const{return storage_&&index<2?&storage_->models[index]:nullptr;}
private:
    struct Storage {StartupAttachmentProfile profile;ActorFadeProfile fade;std::weak_ptr<const CharacterModelBinding> parent;
        std::array<StartupAttachmentState,2> children{};std::array<AttachmentModelScene,2> models;std::array<bool,2> visible{};
        u32 frames=0;u8 render_index=0;bool published=false;};
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup actor attachments: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=false;if(storage_){storage_->published=false;for(auto& model:storage_->models)model.discard_pose();}fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false;
};
} // namespace gf
