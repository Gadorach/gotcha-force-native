#pragma once
// Staged SOURCE-SUBSET actor frame, opt-in isolated requirements mode ONLY.
// No host admission, actual auxiliary/effect consumers or recovered placement.
#include "engine/gameplay/startup_actor_program_class.hpp"
#include "engine/gameplay/startup_actor_material.hpp"
#include "engine/gameplay/startup_actor_dispatch.hpp"
#include "engine/gameplay/character_motion_driver.hpp"
#include "engine/gameplay/startup_world.hpp"
namespace gf {
enum class StartupActorFrameMode {IsolatedRequirements};
enum class StartupActorFrameStage {Dimensions,ScalarPre,ClassPre,Substate,ClassPost,ScalarPost,Events,World,Material,Publish};
enum class StartupActorRequirementOrigin {ClassInitializer,Substate,ClassPost,EventScript,Models};
struct StartupActorFrameRequirement {
    StartupActorRequirementOrigin origin=StartupActorRequirementOrigin::Substate;u8 code=0,lane=0,id=255,model1=0;
    bool anchor_known=false;std::array<f32,3> anchor{};f32 factor=0;
};
struct StartupActorFrameOutput {
    std::vector<StartupActorFrameRequirement> requirements;std::vector<CharacterProgramOperation> motion;
    std::vector<StartupActorFrameStage> stages;u32 frame=0;bool reset_fields=false,root_updated=false;
};
struct StartupActorFrameAssets {
    ByteView model,prefix,pose,program,setup,runtime,classes,root,fade;
    std::span<const CharacterMotionClipBinding> clips;
};
struct StartupActorFrameInput {
    bool isolated_external_mutations=false,placement_declared=false;
    std::array<f32,3> position{};std::array<s16,3> angles{};
    std::span<const StartupActorPeerObservation> peers;
};
struct StartupActorFrameState {
    CharacterProgramVm vm;StartupActorDispatch dispatch;StartupActorProgramClass classes;
    StartupActorBookkeeping scalars;StartupActorMaterial material;StartupActorInitialSetup dimensions;
    CharacterLocalPose local;CharacterWorldPose world;u32 frames=0;bool have_world=false,published=false;
    // Owned accepted placement input, NOT recovered original actor20/history.
    // Consumers must never reconstruct this value from animated clip/world roots.
    std::array<f32,3> declared_position{};bool have_declared_position=false;
};
class StartupActorFrame {
public:
    bool load(const StartupActorFrameAssets& assets,StartupActorFrameMode mode,u8 stored_index,StartupActorFrameOutput* out,std::string* error){
        if(out)*out={};
        if(error)error->clear();
        ready_=false;storage_.reset();
        if(!out||mode!=StartupActorFrameMode::IsolatedRequirements)return fail(error,"explicit isolated requirements mode/output required");
        if(!assets.model.data||!assets.model.size||assets.model.size>(4u<<20))return fail(error,"model input budget");
        auto next=std::make_unique<Storage>();auto& r=*next;const auto hash=character_program_model_hash(assets.model);
        if(!r.model.load(assets.model,assets.prefix,assets.pose,error)||!character_program_load(assets.program,hash,&r.program,error)||!startup_actor_setup_load(assets.setup,hash,&r.setup,error)||!startup_actor_runtime_load(assets.runtime,hash,&r.runtime,error)||!startup_actor_class_load(assets.classes,&r.classes,error)||!startup_root_load(assets.root,&r.root,error)||!actor_fade_load(assets.fade,&r.fade,error))return false;
        if(r.setup.model_id!=r.runtime.model_id||stored_index!=(r.setup.model_id==0x615?0:1)||!rig_config(r.setup,r.model.rig()))return fail(error,"startup slot/rig/config scope");
        if(!r.driver.load(assets.model,r.model.rig(),r.program,assets.clips,error)||!r.live.vm.load_fresh_actor(r.program,error))return false;
        StartupActorClassState input,klass;input.model_id=r.setup.model_id;std::vector<StartupActorClassEffect> effects;
        if(!startup_actor_class_initialize(r.classes,input,&klass,&effects)||!startup_actor_bookkeeping_initialize(r.setup,r.runtime,klass,&r.live.scalars,error)||!startup_actor_initial_setup(r.setup,{},&r.live.dimensions,error))return false;
        if(!r.live.classes.bind(r.live.vm,r.setup,r.classes,r.live.scalars.klass,error))return false;
        StartupRootState root;root.initialized=true;root.actor_scale=r.live.dimensions.actor_scale;root.root_scale.fill(r.setup.one);root.scalar_b4=r.live.dimensions.scale_b4;root.scalar_b8=r.live.dimensions.rank.scale_b8;
        if(!r.live.dispatch.bind_root_state(stored_index,r.root,root,error)||!r.live.material.bind(r.model,r.setup,r.fade))return fail(error,"constructor numeric root/material binding");
        r.construction.bones.resize(r.model.rig().binding.bones.size());for(unsigned i=0;i<r.construction.bones.size();++i)r.construction.bones[i].joint_offset=r.model.rig().binding.bones[i].joint_offset;
        for(const auto& e:effects)out->requirements.push_back({StartupActorRequirementOrigin::ClassInitializer,u8(e.kind),0,e.id});
        storage_=std::move(next);ready_=true;return true;
    }
    bool request(u8 choice,StartupActorFrameOutput* out,std::string* error){
        if(out)*out={};
        if(error)error->clear();
        if(!storage_||!out)return refuse(error,"request on unloaded actor/output");
        auto next=storage_->live;std::vector<CharacterProgramOperation> ops;
        if(!next.dispatch.request(next.vm,choice,-1,next.scalars.motion_delta,storage_->driver,&ops,error))return refuse(error,"motion request refused");
        if(!sync(next,error))return refuse(error,"request field coherence");
        next.scalars.phase19=u8(next.dispatch.state().phase);
        next.published=false;model_requirement(next,*out);out->motion=std::move(ops);storage_->model.discard_pose();storage_->live=std::move(next);ready_=true;return true;
    }
    bool reset(StartupActorFrameOutput* out,std::string* error){
        if(out)*out={};
        if(error)error->clear();
        if(!storage_||!out)return refuse(error,"reset on unloaded actor/output");
        auto next=storage_->live;std::vector<StartupActorEffect> effects;
        if(!next.dispatch.reset_source_fields(&effects)||!next.material.request(storage_->model,14))return refuse(error,"source reset fields/curve14");
        sync_root(next);next.scalars.phase19=0;next.published=false;
        storage_->model.discard_pose();storage_->live=std::move(next);out->reset_fields=true;ready_=true;return true;
    }
    bool set_render_enabled(bool enabled){
        if(!storage_)return false;
        auto& s=storage_->live;s.dispatch.set_render_group(enabled?u8(unsigned(s.dispatch.state().render_index)+0x41u):0);return true;
    }
    bool step(const StartupActorFrameInput& input,StartupActorFrameOutput* out,std::string* error){
        if(out)*out={};
        if(error)error->clear();
        if(!ready_||!storage_||!out||!input.isolated_external_mutations||!input.placement_declared)return refuse(error,"isolated effects/declared placement/ready/output contract");
        auto& r=*storage_;auto next=r.live;StartupActorFrameOutput result;if(next.frames>=8192)return refuse(error,"frame budget");
        if(!sync(next,error))return refuse(error,"pre field coherence");
        next.scalars.phase19=u8(next.dispatch.state().phase);sync_root(next);
        result.stages.push_back(StartupActorFrameStage::Dimensions);
        //55DCC->4D08C: the source pose-dependent height branch is not silently
        // evaluated from an old matrix. Fresh supported actors take zero path.
        if((next.scalars.klass.flags5e0&0x40u)&&next.scalars.field661==0)return refuse(error,"8004D08C pose-dependent branch required");
        if(!dimensions(r.setup,next))return refuse(error,"derived dimension bounds");
        result.stages.push_back(StartupActorFrameStage::ScalarPre);if(!startup_actor_pre_update(r.runtime,next.scalars,input.peers,&next.scalars,error))return refuse(error,"scalar preupdate refused");
        result.stages.push_back(StartupActorFrameStage::ClassPre);if(!next.classes.refresh(next.vm,next.scalars.klass,error)||!next.classes.pre(next.vm,error))return refuse(error,"class preupdate refused");next.scalars.klass=next.classes.state();
        result.stages.push_back(StartupActorFrameStage::Substate);std::vector<StartupActorEffect> actor;
        if(!next.dispatch.update(next.vm,next.scalars.motion_delta,r.driver,&result.motion,&actor,error))return refuse(error,"subdispatch refused");
        next.scalars.phase19=u8(next.dispatch.state().phase);
        std::vector<StartupActorEffect> scripts;
        for(const auto& e:actor){if(e.kind==StartupActorEffectKind::EventScriptRequired){scripts.push_back(e);continue;}
            if(e.kind==StartupActorEffectKind::FadeCurve12Required){if(!next.material.request(r.model,12))return refuse(error,"curve12 request refused");continue;}
            if(e.kind==StartupActorEffectKind::RootScaleUpdated){result.root_updated=true;continue;}
            if(e.kind==StartupActorEffectKind::ScaleOutRequired)continue;
            StartupActorFrameRequirement req;req.origin=StartupActorRequirementOrigin::Substate;req.code=u8(e.kind);req.lane=e.lane;
            req.id=e.kind==StartupActorEffectKind::Sound242Required?242:e.kind==StartupActorEffectKind::Trail7ERequired?126:e.kind==StartupActorEffectKind::AnchorEffect2Required?2:e.kind==StartupActorEffectKind::AnchorEffect0Required?0:e.script;
            if(e.kind==StartupActorEffectKind::AnchorEffect2Required||e.kind==StartupActorEffectKind::AnchorEffect0Required){req.factor=f32(next.scalars.scale_b4*next.scalars.size7fc);if(r.live.have_world){req.anchor_known=true;for(unsigned k=0;k<3;++k)req.anchor[k]=r.live.world.matrices[0].at(k,3);}}
            result.requirements.push_back(req);
        }
        bool models=false;for(const auto& op:result.motion)models|=(op.kind==CharacterProgramOperationKind::Select&&op.lane==0)||op.kind==CharacterProgramOperationKind::Models;
        if(models)model_requirement(next,result); // final opaque selection observation, not per-op consumer calls
        result.stages.push_back(StartupActorFrameStage::ClassPost);std::vector<StartupActorClassEffect> class_effects;if(!next.classes.post(next.vm,&class_effects,error))return refuse(error,"class postupdate refused");next.scalars.klass=next.classes.state();next.scalars.field709=next.vm.state().actor.auxiliary;
        for(const auto& e:class_effects)result.requirements.push_back({StartupActorRequirementOrigin::ClassPost,u8(e.kind),0,e.id});
        result.stages.push_back(StartupActorFrameStage::ScalarPost);if(!startup_actor_post_update(r.runtime,next.scalars,&next.scalars))return refuse(error,"scalar postupdate refused");
        if(!next.classes.refresh(next.vm,next.scalars.klass,error))return refuse(error,"post scalar/class coherence");
        result.stages.push_back(StartupActorFrameStage::Events);for(const auto& e:scripts)result.requirements.push_back({StartupActorRequirementOrigin::EventScript,u8(e.kind),e.lane,e.script});
        result.stages.push_back(StartupActorFrameStage::World);
        std::array<CharacterPoseLane,4> lanes;for(unsigned i=0;i<2;++i){const auto& m=next.vm.state().lanes[i].motion;if(m.selected){const auto* sample=r.driver.snapshot(m);if(!sample)return refuse(error,"motion snapshot binding");lanes[i].current=&sample->sample();lanes[i].mirrored=sample->mirrored();}else lanes[i].current=&r.construction;}
        CharacterPoseFrame modifiers;StartupActorTransform placement;
        if(!character_pose_sample(r.model.rig(),lanes,&next.local,error)||!character_pose_initial_frame(r.model.rig(),Mat4{},&modifiers,error))return refuse(error,"local pose/constructor modifiers");
        modifiers.alternate_root=true;modifiers.scales[0]=next.dispatch.root_state().root_scale;
        // Constructor1DBC/1D64 are clear. 46754 derivesbone1 translation by
        // subtracting retained1DBC; -0 preserves source subtraction's sign.
        for(unsigned k=0;k<3;++k){modifiers.bone1_translation_adjust[k]=-0.f;placement.position[k]=input.position[k];placement.scale[k]=next.dispatch.root_state().actor_scale[k];}placement.angles=input.angles;
        if(modifiers.extra_angles.size()>2)modifiers.extra_angles[2][1]=next.scalars.klass.angle18e2;else if(next.scalars.klass.angle18e2)return refuse(error,"class bone2 modifier outside rig");
        if(!character_pose_startup_world(r.model.rig(),next.local,placement,modifiers,&next.world,error))return refuse(error,"alternate world refused");
        result.stages.push_back(StartupActorFrameStage::Material);if(!next.material.tick(r.model,next.scalars))return refuse(error,"material actor-clock update refused");
        result.stages.push_back(StartupActorFrameStage::Publish);CharacterModelRenderState render_state;render_state.dynamic_material=true;
        if(!next.material.publish(r.model,next.world,render_state,error))return refuse(error,"model/material publication refused");
        next.declared_position=input.position;next.have_declared_position=true;
        ++next.frames;next.have_world=next.published=true;result.frame=next.frames;r.live=std::move(next);*out=std::move(result);return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){return storage_?storage_->model.upload_textures(render,base):0;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){return storage_?storage_->model.upload_textures_shared(render,base):0;}
    unsigned draw(RenderEngine& render,const Camera& camera){if(!ready_||!storage_||!storage_->live.published||!storage_->live.dispatch.state().render_group)return 0;return storage_->live.material.draw(storage_->model,render,camera);}
    const StartupActorFrameState* state()const{return storage_?&storage_->live:nullptr;}
    const CharacterModelScene* model()const{return storage_?&storage_->model:nullptr;}
    bool ready()const{return ready_&&bool(storage_);}
private:
    struct Storage {
        CharacterModelScene model;CharacterMotionDriver driver;CharacterProgramAsset program;StartupActorSetupProfile setup;
        StartupActorRuntimeProfile runtime;StartupActorClassProfile classes;StartupRootProfile root;ActorFadeProfile fade;
        HsdBoneMotionSample construction;StartupActorFrameState live;
    };
    static bool rig_config(const StartupActorSetupProfile& setup,const CharacterPoseRig& rig){
        const auto& c=setup.class_config;if(c[2]!=rig.lane_mask)return false;for(unsigned i=0;i<3;++i)if(c[12+i]!=rig.cuts[i+1])return false;
        for(unsigned i=0;i<rig.order.size();++i)if(c[0x8c+i]!=rig.order[i]||c[0x6c+i]!=rig.mirror[i])return false;
        return true;
    }
    static bool sync(StartupActorFrameState& state,std::string* error){
        CharacterProgramClassFields fields;if(!state.vm.observe_class_fields(&fields)){if(error)*error="startup frame VM aliases unavailable";return false;}
        state.scalars.klass.flags5e0=fields.behaviour;state.scalars.klass.field582=fields.default_group;state.scalars.klass.field6e9=fields.effect_control;state.scalars.field709=state.vm.state().actor.auxiliary;return true;
    }
    static void sync_root(StartupActorFrameState& state){const auto& root=state.dispatch.root_state();state.scalars.scale_b4=root.scalar_b4;state.dimensions.scale_b4=root.scalar_b4;state.dimensions.rank.scale_b8=root.scalar_b8;state.dimensions.actor_scale=root.actor_scale;}
    static bool dimensions(const StartupActorSetupProfile& setup,StartupActorFrameState& state){
        return startup_actor_dimensions(setup,state.scalars.scale_b4,&state.dimensions);
    }
    static void model_requirement(const StartupActorFrameState& state,StartupActorFrameOutput& out){out.requirements.push_back({StartupActorRequirementOrigin::Models,0,0,state.vm.state().actor.models[0],state.vm.state().actor.models[1]});}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup actor frame: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const std::string detail=error?*error:std::string{};ready_=false;if(storage_){storage_->live.published=false;storage_->model.discard_pose();}fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false;
};
} // namespace gf
