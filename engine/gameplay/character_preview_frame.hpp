#pragma once
#include "engine/gameplay/character_preview_setup.hpp"
#include "engine/gameplay/startup_actor_program_class.hpp"
#include "engine/gameplay/startup_actor_material.hpp"
#include "engine/gameplay/character_motion_driver.hpp"
#include "engine/gameplay/startup_attachment_state.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/character_program_height_control.hpp"
#include "engine/gameplay/character_model_parts.hpp"
#include "engine/gameplay/character_program_sound_events.hpp"
#include "engine/gameplay/force_spec_actor_actions.hpp"
#include "engine/gameplay/force_spec_lifecycle.hpp"
namespace gf {
enum class CharacterPreviewMode {SettledIdleWithAuxiliaryRequirements,OpeningIdleWithAuxiliaryRequirements};
struct CharacterPreviewAssets {ByteView model,prefix,pose,program,setup,runtime,classes,fade,attachments;std::span<const CharacterMotionClipBinding> clips;};
struct CharacterPreviewSoundRequirement {StartupSoundCallKind kind=StartupSoundCallKind::StartDirect;s16 cue=0;f32 viewer_bias=0;u64 frame=0;std::weak_ptr<const CharacterModelBinding> binding;};
struct CharacterPreviewOutput {std::vector<StartupActorClassEffect> auxiliary_requirements;std::vector<CharacterProgramOperation> motion;u64 frame=0;u64 height_source_frame=0;bool height_world_known=false;std::vector<CharacterPreviewSoundRequirement> sound_requirements;};
struct CharacterPreviewMotionRequest {u8 group=0,choice=0,mask=3;f32 start=-1;CharacterProgramTransition transition;};
//A successful request start is not a program-end or presentation completion.
//Only this owner can mint the receipt; public output fields alone grant nothing.
class CharacterPreviewActionStart {friend class CharacterPreviewFrame;CharacterPreviewActionStart()=default;};
struct CharacterPreviewActionOutput {CharacterPreviewOutput preview;ForceSpecActorAction requested=ForceSpecActorAction::None,performed=ForceSpecActorAction::None;u32 unfulfilled_actions=0;std::weak_ptr<const CharacterModelBinding> binding;std::weak_ptr<const CharacterPreviewActionStart> start;};
struct CharacterPreviewTransitionObservation {bool blending=false;u8 mode=0;f32 time=0,duration=0;};
struct CharacterPreviewState {
    CharacterProgramVm vm;StartupActorProgramClass classes;StartupActorBookkeeping scalars;StartupActorMaterial material;CharacterPreviewSetup setup;
    CharacterLocalPose local;CharacterWorldPose world;Camera camera;ForceSpecAngles controls;u64 frames=0;bool published=false;
    std::array<StartupAttachmentState,2> children{};std::array<bool,2> child_visible{};
    Mat4 actor_frame1aa4;bool actor_frame_known=false;u64 world_frame=0;
    std::weak_ptr<const CharacterModelBinding> world_binding;
    CharacterModelPartSelection model_parts;
    s16 root_yaw5ae=0; //saved accepted actor yaw at explicit ordinary request, not current camera yaw
    CharacterMotionRootObservation root_observation;bool root_observation_known=false;
    CharacterDirectionalRootResult directional_root;bool directional_root_known=false;u64 directional_root_frame=0;
    ForceSpecActorActionState actor_actions;u8 stored_index96=0,render_group82=0;ForceSpecActorAction last_actor_action=ForceSpecActorAction::None;u64 actor_action_frame=0;
    std::array<CharacterPreviewTransitionObservation,2> action_transitions;
    u64 scene_callback=0;ForceSpecPhase scene_phase=ForceSpecPhase::Closed;bool scene_active=false;
};
// Source settled preview policy581B8/58244/58E30, using shared actor numeric,
// motion, pose, skin, material and attachment kernels. Only0615/000A normal
// models and idle are admitted by default; the explicit entry below adds ordinary
// normal controls with explicit null-root opt-in. C74EC construction remains an explicit
// unfulfilled boundary; this is not complete actor/combat or entry/exit admission.
class CharacterPreviewFrame {
public:
    bool load(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,CharacterPreviewMode mode,u8 variant,CharacterPreviewOutput* out,std::string* error){
        return load_impl(assets,preview,mode,variant,nullptr,out,error);
    }
    // Explicit GFMP3 height-owner entry. Default GFMP2/idle admission is kept.
    // This adds source idle+height control only, not arbitrary demo/root/events.
    bool load_with_actor_height(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,CharacterPreviewMode mode,u8 variant,
                                const CharacterPoseHeightProfile& height,CharacterPreviewOutput* out,std::string* error){
        return load_impl(assets,preview,mode,variant,&height,out,error);
    }
    bool load_with_actor_parts(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,CharacterPreviewMode mode,u8 variant,
                               const CharacterPoseHeightProfile& height,ByteView parts,CharacterPreviewOutput* out,std::string* error){
        if(!parts.data||!parts.size){if(out)*out={};if(error)error->clear();retire();storage_.reset();return fail(error,"explicit model-part profile required");}
        return load_impl(assets,preview,mode,variant,&height,out,error,parts);
    }
    //Explicit normal ordinary-control owner. Public-null-root support is opt-in;
    //the default keeps the earlier G-Red/AObj-only domain. Root-owning directional motion,
    //full demonstration/reset lifecycle and downstream sound delivery remain gated.
    //Optional parts retain the old default-selector guard when not supplied.
    bool load_with_actor_controls(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,CharacterPreviewMode mode,u8 variant,
                                  const CharacterPoseHeightProfile& height,ByteView events,ByteView parts,CharacterPreviewOutput* out,std::string* error,CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly){
        return load_impl(assets,preview,mode,variant,&height,out,error,parts,&events,root_mode);
    }
    //Separate opt-in actual-frame owner for ordinary/directional VM controls.
    //The source class still owns steering; UI orbit angles are not motion input.
    //Full demo/reset/auxiliary lifetime and downstream sound delivery stay gated.
    bool load_with_directional_actor_controls(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,CharacterPreviewMode mode,u8 variant,
                                              const CharacterPoseHeightProfile& height,ByteView events,ByteView parts,CharacterPreviewOutput* out,std::string* error,
                                              CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly){
        return load_impl(assets,preview,mode,variant,&height,out,error,parts,&events,root_mode,true);
    }
    bool load_with_scene_actor_controls(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,u8 variant,
                                        const CharacterPoseHeightProfile& height,ByteView events,ByteView parts,const ForceSpecActorActionProfile& actions,
                                        const ForceSpecLifecycle& scene,CharacterPreviewOutput* out,std::string* error,
                                        CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly){
        return load_impl(assets,preview,CharacterPreviewMode::OpeningIdleWithAuxiliaryRequirements,variant,&height,out,error,parts,&events,root_mode,true,&scene,&actions);
    }
    //Separate settled normal-preview opt-in. Old directional/reset-only loads
    //retain their control refusals and unfulfilled A policy. No scene/host switch.
    bool load_with_demo_actor_controls(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,u8 variant,
                                      const CharacterPoseHeightProfile& height,ByteView events,ByteView parts,
                                      CharacterPreviewOutput* out,std::string* error,
                                      CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly){
        return load_impl(assets,preview,CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements,variant,&height,out,error,parts,&events,root_mode,true,nullptr,nullptr,true);
    }
    //Same scene-owned construction/placement as reset-only, with the separately
    //qualified demo request capability. No opening/active actor reconstruction.
    bool load_with_scene_demo_controls(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,u8 variant,
                                       const CharacterPoseHeightProfile& height,ByteView events,ByteView parts,const ForceSpecActorActionProfile& actions,
                                       const ForceSpecLifecycle& scene,CharacterPreviewOutput* out,std::string* error,
                                       CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly){
        return load_impl(assets,preview,CharacterPreviewMode::OpeningIdleWithAuxiliaryRequirements,variant,&height,out,error,parts,&events,root_mode,true,&scene,&actions,true);
    }
private:
    bool load_impl(const CharacterPreviewAssets& assets,const CharacterPreviewProfile& preview,CharacterPreviewMode mode,u8 variant,const CharacterPoseHeightProfile* height,CharacterPreviewOutput* out,std::string* error,ByteView parts={},const ByteView* events=nullptr,CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly,bool directional_root=false,const ForceSpecLifecycle* scene=nullptr,const ForceSpecActorActionProfile* actions=nullptr,bool demo=false){
        if(out)*out={};
        if(error)error->clear();
        ready_=false;storage_.reset();
        if(!out||variant||(mode!=CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements&&mode!=CharacterPreviewMode::OpeningIdleWithAuxiliaryRequirements)||!character_preview_profile_valid(preview)||!assets.model.data||!assets.model.size||assets.model.size>(4u<<20))return fail(error,"explicit normal idle scope required");
        if((scene||actions)&&(!scene||!actions||!actions->valid()||!scene->scope().lock()||scene->phase()!=ForceSpecPhase::Resources||scene->bank_hash()!=preview.scene_hash||actions->bank_hash!=preview.scene_hash||!directional_root||!events))return fail(error,"explicit resource-phase scene/action binding required");
        CharacterPreviewOutput result;
        if(demo&&(!directional_root||!height||!events||(scene?(!actions||mode!=CharacterPreviewMode::OpeningIdleWithAuxiliaryRequirements):(actions||mode!=CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements))))return fail(error,"explicit demo prerequisites and placement owner required");
        auto next=std::make_unique<Storage>();auto& r=*next;r.preview=preview;r.mode=mode;r.demo_owner=demo;const u32 hash=character_program_model_hash(assets.model);
        if(events){if(!height||(mode!=CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements&&!scene)||(root_mode!=CharacterMotionRootObservationMode::AObjOnly&&root_mode!=CharacterMotionRootObservationMode::AObjOrPublicNull)||!r.events.load(*events,hash,error))return fail(error,"explicit settled ordinary-control event bank/root mode required");r.root_owner=true;r.root_mode=root_mode;r.directional_root_owner=directional_root;}
        if(height){if(!character_pose_height_profile_valid(*height))return fail(error,"source height profile required");r.height=*height;r.height_owner=true;}
        if(!r.model.load(assets.model,assets.prefix,assets.pose,error)||!(height?character_program_load_control_flow(assets.program,hash,&r.program,error):character_program_load_transitions(assets.program,hash,&r.program,error))||!startup_actor_setup_load(assets.setup,hash,&r.actor_setup,error)||!startup_actor_runtime_load(assets.runtime,hash,&r.runtime,error)||!startup_actor_class_load(assets.classes,&r.class_profile,error)||!actor_fade_load(assets.fade,&r.fade,error)||!startup_attachments_load(assets.attachments,hash,&r.attachment_profile,error))return false;
        if(r.actor_setup.model_id!=r.runtime.model_id||r.actor_setup.model_id!=r.attachment_profile.model_id||!rig_binding(r.actor_setup,r.model.rig())||!character_preview_setup(preview,r.actor_setup,&r.live.setup,error))return fail(error,"model/profile/rig normalization binding");
        if(r.root_owner&&((r.actor_setup.model_id!=0x615&&!(r.actor_setup.model_id==10&&r.root_mode==CharacterMotionRootObservationMode::AObjOrPublicNull))||r.events.model_id()!=r.actor_setup.model_id))return fail(error,"normal model/event identity and explicit public-null-root mode required");
        if(parts.size||parts.data){if(!height||!r.parts.bind(r.model,assets.model,parts,r.actor_setup.model_id,error))return fail(error,"normal model-part owner binding");r.parts_owner=true;}
        const auto& rig=r.model.rig();CharacterLocalPose initial;
        //5380C/67E18 copy logical JObj LOCAL translations;4BB14 copies these
        // into both pose buffers and creates identity quaternions from identity
        // matrices, with unit extra scales and zero extra angles.
        for(const auto& bone:rig.binding.bones){CharacterBonePose p;p.joint_offset=bone.joint_offset;std::copy_n(bone.translation,3,p.translation);initial.bones.push_back(p);}
        if(r.root_owner){const std::array<s8,2> model_bones={std::bit_cast<s8>(r.actor_setup.class_config[0x42]),std::bit_cast<s8>(r.actor_setup.class_config[0x43])};
            if((model_bones[0]>=0&&model_bones[0]==model_bones[1])||!r.driver.load_directional_transitions(assets.model,rig,r.program,assets.clips,initial,model_bones,error)||!(r.demo_owner?r.live.vm.load_fresh_actor_directional_root_follow_tail(r.program,{0,r.actor_setup.one,r.height.zero},r.live.setup.position,error):r.directional_root_owner?r.live.vm.load_fresh_actor_directional_root_controls(r.program,{0,r.actor_setup.one,r.height.zero},r.live.setup.position,error):r.live.vm.load_fresh_actor_root_controls(r.program,{0,r.actor_setup.one,r.height.zero},r.live.setup.position,error)))return fail(error,"ordinary-root library/model-bone binding");
            r.root_lane=2;for(unsigned lane=0;lane<2;++lane)for(unsigned i=rig.cuts[lane];i<rig.cuts[lane+1];++i)if(rig.order[i]==0)r.root_lane=lane;if(r.root_lane>=2)return fail(error,"active root lane required");
        }else if(!(height?r.driver.load_frozen_target_transitions(assets.model,rig,r.program,assets.clips,initial,error)&&r.live.vm.load_fresh_actor_height_controls(r.program,{0,r.actor_setup.one,r.height.zero},error):r.driver.load_transitions(assets.model,rig,r.program,assets.clips,initial,error)&&r.live.vm.load_fresh_actor_transitions(r.program,error)))return false;
        StartupActorClassState klass;klass.model_id=r.actor_setup.model_id;std::vector<StartupActorClassEffect> effects;
        if(!startup_actor_class_initialize(r.class_profile,klass,&klass,&effects)||!startup_actor_bookkeeping_initialize(r.actor_setup,r.runtime,klass,&r.live.scalars,error)||!r.live.classes.bind(r.live.vm,r.actor_setup,r.class_profile,r.live.scalars.klass,error)||!r.live.material.bind(r.model,r.actor_setup,r.fade))return false;
        r.live.scalars.phase19=2;r.live.scalars.scale_b4=r.live.setup.actor.scale_b4;r.live.controls={preview.pitch,preview.yaw};
        if(scene){r.scene_owner=r.actions_owner=true;r.scene_scope=scene->scope();r.action_profile=*actions;r.action_binding=r.model.instance_binding();r.live.actor_actions={r.live.controls,0};r.live.stored_index96=0;r.live.render_group82=65;r.live.scene_callback=scene->callbacks();r.live.scene_phase=scene->phase();}
        for(unsigned i=0;i<2;++i)if(!r.children[i].load(assets.model,r.attachment_profile.entries[i].record,error))return false;
        CharacterProgramHeightControl control;CharacterProgramDirectionalControl root_control;
        const bool selected=r.root_owner?root_context(r,r.live,&root_control,error)&&r.live.vm.request_with_actor_controls(0,0,u8(15&rig.lane_mask),-1,r.live.scalars.motion_delta,r.driver,root_control,&result.motion,error,{0,0}):height?height_context(r,r.live,&control,error)&&r.live.vm.request_with_actor_controls(0,0,u8(15&rig.lane_mask),-1,r.live.scalars.motion_delta,r.driver,control,&result.motion,error,{0,0}):r.live.vm.request_with_transition(0,0,u8(15&rig.lane_mask),-1,r.live.scalars.motion_delta,{0,0},r.driver,&result.motion,error);
        if(!selected||!(r.root_owner?motion_boundary(r,r.live,root_control):idle(r.live,r.driver,r.height_owner))||!height_boundary(r,r.live))return fail(error,"source constructor idle selection");
        for(const auto& e:effects)if(e.kind==StartupActorClassEffectKind::AuxiliaryC74ECRequired)result.auxiliary_requirements.push_back(e);
        // Other constructor requests above now own the real normal bank models;
        // no claim about original heap capacity/GC allocation acknowledgement.
        storage_=std::move(next);ready_=true;CharacterPreviewOutput first;
        if(!step_impl(storage_->live.controls,0,&first,error))return false;
        result.frame=first.frame;result.height_source_frame=first.height_source_frame;result.height_world_known=first.height_world_known;result.motion.insert(result.motion.end(),first.motion.begin(),first.motion.end());result.sound_requirements=std::move(first.sound_requirements);*out=std::move(result);return true;
    }
public:
    bool step(ForceSpecAngles controls,CharacterPreviewOutput* out,std::string* error){
        if(storage_&&storage_->actions_owner){if(out)*out={};return refuse(error,"bound actor actions require their source input entry");}
        if(storage_&&storage_->mode!=CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements){if(out)*out={};return refuse(error,"opening owner requires explicit slide placement");}
        return step_impl(controls,0,out,error);
    }
    //801DCA30 writes only actor28 after each four-weight integration. Keep X/Y
    // normalization and the settled API's placement contract independent.
    bool step_opening(ForceSpecAngles controls,f32 z,CharacterPreviewOutput* out,std::string* error){
        if(out)*out={};
        if(!storage_||storage_->scene_owner||storage_->mode!=CharacterPreviewMode::OpeningIdleWithAuxiliaryRequirements||!std::isfinite(z)||z< -201||z>1)return refuse(error,"opening slide scope/bounds");
        return step_impl(controls,z,out,error);
    }
    bool request_ordinary_motion(const CharacterPreviewMotionRequest& request,ForceSpecAngles controls,CharacterPreviewOutput* out,std::string* error){
        if(out)*out={};
        if(!ready_||!storage_||!storage_->root_owner||storage_->directional_root_owner||!storage_->live.published||request.mask!=3)return refuse(error,"explicit live ordinary-control owner/full lane request required");
        const auto group=request.group?request.group:storage_->live.vm.state().actor.default_group;
        if(!group&&request.choice==40)return refuse(error,"full demonstration lifecycle not admitted");
        return step_impl(controls,0,out,error,&request);
    }
    bool request_controlled_motion(const CharacterPreviewMotionRequest& request,ForceSpecAngles controls,CharacterPreviewOutput* out,std::string* error){
        if(out)*out={};
        if(!ready_||!storage_||!storage_->directional_root_owner||storage_->demo_owner||storage_->actions_owner||!storage_->live.published||request.mask!=3)return refuse(error,"explicit live directional-control owner/full lane request required");
        const auto group=request.group?request.group:storage_->live.vm.state().actor.default_group;
        if(!group&&request.choice==40)return refuse(error,"full demonstration lifecycle not admitted");
        return step_impl(controls,0,out,error,&request);
    }
    //Adopt the SAME settled preview/model/VM, not an idle reload. Slot0 comes
    //from source5809C(count1)->537A8->58244; no global pool/allocation ACK.
    bool bind_actor_actions(const ForceSpecActorActionProfile& profile,std::string* error){
        if(!ready_||!storage_||storage_->actions_owner||!storage_->directional_root_owner||storage_->mode!=CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements||!storage_->live.published||!profile.valid()||profile.bank_hash!=storage_->preview.scene_hash)return refuse(error,"explicit settled action profile/owner binding");
        auto& r=*storage_;if(std::find(profile.models.begin(),profile.models.end(),r.actor_setup.model_id)==profile.models.end()||r.live.world_binding.lock()!=r.model.instance_binding()||r.live.world_frame!=r.live.frames)return refuse(error,"action model/world generation binding");
        r.action_profile=profile;r.action_binding=r.model.instance_binding();r.actions_owner=true;r.live.actor_actions={r.live.controls,0};r.live.stored_index96=0;r.live.render_group82=65;return true;
    }
    bool step_actor_actions(u32 held,u32 pressed,CharacterPreviewActionOutput* out,std::string* error){
        if(storage_&&storage_->scene_owner){if(out)*out={};return refuse(error,"scene-owned actions require their lifecycle entry");}
        return step_action_impl(held,pressed,out,error);
    }
    bool step_scene_actor(const ForceSpecLifecycle& scene,u32 held,u32 pressed,CharacterPreviewActionOutput* out,std::string* error){
        if(out)*out={};
        if(!out||!ready_||!storage_||!storage_->scene_owner||!scene.scope().lock()||scene.scope().lock()!=storage_->scene_scope.lock()||scene.bank_hash()!=storage_->preview.scene_hash||storage_->live.scene_callback==~u64(0)||scene.callbacks()!=storage_->live.scene_callback+1||scene.previous_phase()!=storage_->live.scene_phase)return refuse(error,"stale/foreign/skipped scene callback");
        auto& r=*storage_;
        if(scene.phase()==ForceSpecPhase::Resources){r.live.scene_callback=scene.callbacks();r.live.scene_phase=scene.phase();r.scene_visible=false;return true;}
        if(!scene.visible()||(scene.phase()==ForceSpecPhase::Active&&(scene.slide_index()!=64||scene.actor_z()!=0)))return refuse(error,"source visible/settled scene boundary");
        const u32 effective=scene.previous_phase()==ForceSpecPhase::Active&&scene.phase()==ForceSpecPhase::Active&&!(pressed&0x200u)?pressed:0;
        return step_action_impl(held,effective,out,error,&scene);
    }
private:
    bool step_action_impl(u32 held,u32 pressed,CharacterPreviewActionOutput* out,std::string* error,const ForceSpecLifecycle* scene=nullptr){
        if(out)*out={};
        if(!out||!ready_||!storage_||!storage_->actions_owner||!storage_->live.published||storage_->action_binding.lock()!=storage_->model.instance_binding())return refuse(error,"inactive/foreign action receiver");
        auto& r=*storage_;ForceSpecActorActionInput input;input.held=held;input.pressed=pressed;input.model_id=r.actor_setup.model_id;input.actor_index96=r.live.stored_index96;input.base_position1c=r.preview.position;input.actor_yaw72=std::bit_cast<s16>(u16(r.live.controls.yaw));
        const bool entry_hold=scene&&(scene->phase()==ForceSpecPhase::Opening||scene->phase()==ForceSpecPhase::RevealWait);if(entry_hold)input.entry6b=1;
        ForceSpecActorActionPlan plan;if(!force_spec_actor_action_plan(r.action_profile,r.live.actor_actions,input,&plan,error))return refuse(error,"source actor action planning");
        CharacterPreviewActionOutput result;result.requested=plan.action;
        if(plan.action==ForceSpecActorAction::Demonstrate&&!r.demo_owner){
            //Reset-only receiver. Do not commit60 mode/cooldown or claim that
            //the still-gated0/40 request started. Continue a native idle/update
            //with a visibly unfulfilled A, matching the existing stub policy.
            result.unfulfilled_actions=0x100u;input.pressed&=~0xd00u;
            if(!force_spec_actor_action_plan(r.action_profile,r.live.actor_actions,input,&plan,error))return refuse(error,"unfulfilled action fallback");
        }
        CharacterPreviewMotionRequest request;const CharacterPreviewMotionRequest* requested=nullptr;
        if(plan.action!=ForceSpecActorAction::None){request.group=plan.request_group;request.choice=plan.request_choice;request.mask=u8(plan.request_mask&r.model.rig().lane_mask);request.start=f32(plan.start);request.transition={plan.duration,plan.mode};requested=&request;}
        const auto controls=entry_hold?ForceSpecAngles{r.preview.pitch,r.preview.yaw}:plan.state.angles;
        if(!step_impl(controls,0,&result.preview,error,requested,&plan,scene))return false;
        result.performed=plan.action;result.binding=storage_->model.instance_binding();
        if(plan.action!=ForceSpecActorAction::None){r.action_start=std::shared_ptr<const CharacterPreviewActionStart>(new CharacterPreviewActionStart);result.start=r.action_start;}
        *out=std::move(result);return true;
    }
public:
    bool reset_action_current(const CharacterPreviewActionOutput& result)const{return ready()&&storage_->actions_owner&&storage_->live.published&&result.requested==ForceSpecActorAction::Reset&&result.performed==ForceSpecActorAction::Reset&&result.preview.frame==storage_->live.frames&&result.preview.frame==storage_->live.actor_action_frame&&result.binding.lock()==storage_->model.instance_binding();}
    bool action_start_current(const CharacterPreviewActionOutput& result)const{return ready()&&storage_->actions_owner&&storage_->live.published&&result.performed!=ForceSpecActorAction::None&&result.requested==result.performed&&result.performed==storage_->live.last_actor_action&&!result.unfulfilled_actions&&result.preview.frame==storage_->live.frames&&result.preview.frame==storage_->live.actor_action_frame&&result.binding.lock()==storage_->model.instance_binding()&&storage_->action_start&&result.start.lock()==storage_->action_start;}
    bool sound_requirement_current(const CharacterPreviewSoundRequirement& requirement)const{return ready()&&storage_->live.published&&requirement.frame==storage_->live.frames&&requirement.binding.lock()==storage_->model.instance_binding();}
private:
    bool step_impl(ForceSpecAngles controls,f32 z,CharacterPreviewOutput* out,std::string* error,const CharacterPreviewMotionRequest* request=nullptr,const ForceSpecActorActionPlan* action=nullptr,const ForceSpecLifecycle* scene=nullptr){
        if(out)*out={};
        if(error)error->clear();
        if(!ready_||!storage_||!out)return refuse(error,"inactive preview/output");
        auto& r=*storage_;auto s=r.live;if(!r.root_owner)s.setup.position[2]=z;
        std::vector<CharacterProgramOperation> request_ops;CharacterProgramDirectionalControl root_control;
        bool scene_placed=false;
        if(scene){
            if(!r.scene_owner||scene->scope().lock()!=r.scene_scope.lock())return refuse(error,"scene position producer binding");
            if(scene->previous_phase()==ForceSpecPhase::Opening){auto position=s.setup.position;position[2]=scene->actor_z();if(!std::isfinite(position[2])||position[2]< -201||position[2]>1)return refuse(error,"source slide bounds");CharacterProgramRootFields expected;if(!s.vm.observe_root_fields(&expected)||!s.vm.restore_root_position(expected,position,error))return refuse(error,"native scene-Z restore");s.setup.position=position;scene_placed=true;}
            s.scene_callback=scene->callbacks();s.scene_phase=scene->phase();s.scene_active=s.scene_active||scene->phase()==ForceSpecPhase::Active;
        }
        bool position_restored=false;
        if(action){
            const bool entry_hold=scene&&(scene->phase()==ForceSpecPhase::Opening||scene->phase()==ForceSpecPhase::RevealWait);
            if(!r.actions_owner||r.action_binding.lock()!=r.model.instance_binding()||!action->actor_callback_required||!action->camera_callback_required||(!action->actor_yaw_write&&!entry_hold))return refuse(error,"source action callback/binding boundary");
            s.actor_actions=action->state;
            if(action->action!=ForceSpecActorAction::None){
                const bool demo_action=action->action==ForceSpecActorAction::Demonstrate;
                if((demo_action?!r.demo_owner:action->action!=ForceSpecActorAction::Reset)||!request||!action->position_write||!action->mode_bytes_write||action->root_yaw_write!=demo_action)return refuse(error,"unqualified actor action field consumer");
                CharacterProgramRootFields expected;if(!s.vm.observe_root_fields(&expected)||!s.vm.restore_root_position(expected,action->position20,error))return refuse(error,"same-VM actor position restore");
                s.setup.position=action->position20;s.scalars.field5da=action->field5da;s.scalars.field5db=action->field5db;s.render_group82=action->render_group82;position_restored=true;
                if(demo_action)s.root_yaw5ae=action->root_yaw5ae; //PRIOR displayed yaw, before this callback's UI rotation
                s.last_actor_action=action->action;s.actor_action_frame=s.frames+1;
            }
        }
        if(request){if(!action)s.root_yaw5ae=std::bit_cast<s16>(u16(r.live.controls.yaw));s.scalars.phase19=0;
            if(!root_context(r,s,&root_control,error)||!s.vm.request_with_actor_controls(request->group,request->choice,request->mask,request->start,s.scalars.motion_delta,r.driver,root_control,&request_ops,error,request->transition))return refuse(error,"ordinary request consumer refused");
            if(position_restored){CharacterProgramOperation restore;restore.kind=CharacterProgramOperationKind::RootPositionRestore;restore.lane=u8(r.root_lane);request_ops.insert(request_ops.begin(),restore);}
            s.setup.position=s.vm.state().actor.root.position;
            if(!motion_boundary(r,s,root_control)||!height_boundary(r,s))return refuse(error,"unconsumed ordinary request actor controls");
        }
        if(s.frames==~u64(0)||!sync(s)||!startup_actor_dimensions(r.actor_setup,s.scalars.scale_b4,&s.setup.actor))return refuse(error,"source dimension/coherence");
        if(!r.height_owner&&(s.scalars.klass.flags5e0&0x40u)&&s.scalars.field661==0)return refuse(error,"pose-dependent height consumer required");
        std::vector<CharacterProgramOperation> height_ops;CharacterProgramHeightControl height_control;
        if(r.root_owner){if(!root_context(r,s,&root_control,error)||!s.vm.refresh_height_with_actor_controls(root_control,&height_ops,error))return refuse(error,"accepted-world ordinary-control refresh");}
        else if(r.height_owner&&(!height_context(r,s,&height_control,error)||!s.vm.refresh_height_with_actor_controls(height_control,&height_ops,error)))return refuse(error,"accepted-world height refresh");
        if(!character_preview_camera(r.preview,s.setup.actor.field66c,controls,&s.camera))return refuse(error,"source camera basis");
        if(!startup_actor_pre_update(r.runtime,s.scalars,{},&s.scalars,error)||!s.classes.refresh(s.vm,s.scalars.klass,error)||!s.classes.pre(s.vm,error))return refuse(error,"source preupdate");
        s.scalars.klass=s.classes.state();
        std::vector<CharacterProgramOperation> ops;
        const bool updated=r.root_owner?root_context(r,s,&root_control,error)&&s.vm.update_with_actor_controls(u8(15&r.model.rig().lane_mask),s.scalars.motion_delta,r.driver,root_control,&ops,error):r.height_owner?height_context(r,s,&height_control,error)&&s.vm.update_with_actor_controls(u8(15&r.model.rig().lane_mask),s.scalars.motion_delta,r.driver,height_control,&ops,error):s.vm.update(u8(15&r.model.rig().lane_mask),s.scalars.motion_delta,r.driver,&ops,error);
        if(r.root_owner)s.setup.position=s.vm.state().actor.root.position;
        if(!updated||!sync(s)||!(r.root_owner?motion_boundary(r,s,root_control):idle(s,r.driver,r.height_owner))||!height_boundary(r,s))return refuse(error,"idle motion/event boundary");
        if(r.root_owner){if(!root_control.root_observation(s.vm.state().lanes[r.root_lane].motion,&s.root_observation))return refuse(error,"accepted native root callback observation");s.root_observation_known=true;}
        if(r.actions_owner)for(unsigned lane=0;lane<2;++lane){const auto* native=root_control.snapshot(s.vm.state().lanes[lane].motion);if(!native)return refuse(error,"native action transition observation");s.action_transitions[lane]={native->blending(),native->transition_mode(),native->blend_time(),native->blend_duration()};}
        ops.insert(ops.begin(),height_ops.begin(),height_ops.end());
        ops.insert(ops.begin(),request_ops.begin(),request_ops.end());
        if(scene_placed){CharacterProgramOperation placement;placement.kind=CharacterProgramOperationKind::ScenePlacement;placement.lane=u8(r.root_lane);ops.insert(ops.begin(),placement);}
        if(r.directional_root_owner){const auto* root=root_control.directional_root_result(s.vm.state().lanes[r.root_lane].motion);
            if(root){bool evaluated=false;for(const auto& op:ops)if(op.kind==CharacterProgramOperationKind::RootMotion&&op.lane==r.root_lane)evaluated=true;
                if(!evaluated&&!s.directional_root_known)return refuse(error,"directional root callback frame provenance");
                s.directional_root=*root;s.directional_root_known=true;if(evaluated)s.directional_root_frame=s.frames+1;
            }else{s.directional_root={};s.directional_root_known=false;s.directional_root_frame=0;}
        }
        std::vector<StartupActorClassEffect> effects;if(!s.classes.post(s.vm,&effects,error)||!effects.empty())return refuse(error,"class effect consumer required");s.scalars.klass=s.classes.state();
        if(!startup_actor_post_update(r.runtime,s.scalars,&s.scalars)||!s.classes.refresh(s.vm,s.scalars.klass,error))return refuse(error,"source postupdate");
        std::vector<StartupSoundPendingCall> sound_calls;
        //Slot1 is local to this model binding, NOT a global sound-manager owner.
        //Exported requirements below carry binding/frame and no fabricated actor ID.
        if(r.root_owner&&!character_program_sound_events(s.vm,root_control,r.events,s.scalars.scale_b4,1,&sound_calls,error))return refuse(error,"native frame sound-event requirements");
        std::array<CharacterProgramMotion,2> motions;for(unsigned i=0;i<2;++i)motions[i]=s.vm.state().lanes[i].motion;
        if(!(r.root_owner?root_control.compose_local_pose(s.vm.state(),&s.local,error):r.driver.compose_local_pose(motions,&s.local,error)))return refuse(error,"local lane pose");
        const auto yaw=std::bit_cast<s16>(u16(controls.yaw));const auto base=mat4_multiply(mat4_translation(s.setup.position.data()),mat4_multiply(character_pose_detail::angles({0,yaw,0}),mat4_scale(s.setup.actor.actor_scale.data())));
        CharacterPoseFrame frame;if(!character_pose_initial_frame(r.model.rig(),base,&frame,error))return refuse(error,"direct world frame");if(frame.extra_angles.size()>2)frame.extra_angles[2][1]=s.scalars.klass.angle18e2;
        if(!character_pose_direct_world(r.model.rig(),s.local,frame,&s.world,error)||!s.material.tick(r.model,s.scalars))return refuse(error,"source direct world/material");
        if(r.height_owner){if(s.world.matrices.empty())return refuse(error,"height root-frame copy");s.actor_frame1aa4=s.world.matrices[0];s.actor_frame1aa4.at(1,3)=f32(s.actor_frame1aa4.at(1,3)+s.vm.state().actor.pose_height.offset6d0);if(!character_pose_detail::matrix(s.actor_frame1aa4))return refuse(error,"derived height root-cache bounds");s.actor_frame_known=true;}
        CharacterModelRenderState state;state.dynamic_material=true;if(!s.material.publish(r.model,s.world,state,error))return refuse(error,"body publication");
        ActorFadeMaterial fade;if(!actor_fade_material(r.fade,s.material.state(),&fade))return refuse(error,"child material binding");
        if(r.parts_owner){if(!r.parts.prepare(r.model,s.world,s.frames+1,s.vm.state().actor.models,0xffffffffu,0,fade,error))return refuse(error,"normal model-part selection/world/material");s.model_parts=r.parts.selection();}
        StartupAttachmentParent parent;parent.model_id=r.actor_setup.model_id;parent.mode6e8=s.vm.state().actor.control0[2];parent.reload784=s.scalars.reloads[2].current;
        for(unsigned i=0;i<2;++i){StartupAttachmentUpdate update;if(!startup_attachment_state_step(r.attachment_profile,i,0,parent,s.children[i],&update,error))return refuse(error,"bank child state");s.children[i]=update.state;s.child_visible[i]=update.visible;r.children[i].discard_pose();if(update.visible){AttachmentBoneTransform transform;if(!attachment_bone_transform(s.world.matrices,update.state.transform,&transform,error))return refuse(error,"child transform");if(r.attachment_profile.kind){const f32 scale[3]={update.state.draw_scale,update.state.draw_scale,update.state.draw_scale};transform.world=mat4_multiply(transform.world,mat4_scale(scale));}if(!r.children[i].set_world_fade(transform,fade,false,error))return refuse(error,"child publication");}}
        if(r.height_owner){out->height_world_known=r.live.published&&r.live.world_frame==r.live.frames&&r.live.world_binding.lock()==r.model.instance_binding();out->height_source_frame=out->height_world_known?r.live.world_frame:0;s.world_binding=r.model.instance_binding();s.world_frame=s.frames+1;}
        std::vector<CharacterPreviewSoundRequirement> sounds;for(const auto& call:sound_calls)sounds.push_back({call.kind,call.cue,call.viewer_bias,s.frames+1,r.model.instance_binding()});
        s.controls=controls;++s.frames;s.published=true;r.live=std::move(s);if(scene)r.scene_visible=true;out->frame=r.live.frames;out->motion=std::move(ops);out->sound_requirements=std::move(sounds);return true;
    }
public:
    void retire(){ready_=false;if(storage_){storage_->live.published=false;storage_->model.discard_pose();storage_->parts.discard_pose();for(auto& child:storage_->children)child.discard_pose();}}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!ready_||!storage_)return 0;unsigned n=storage_->model.upload_textures_shared(render,base);if(storage_->parts_owner)n+=storage_->parts.upload_textures_shared(render,base);for(auto& child:storage_->children)n+=child.upload_textures_shared(render,base);return n;}
    unsigned draw(RenderEngine& render){if(storage_&&storage_->scene_owner)return 0;return draw_impl(render);}
    bool draw_scene(RenderEngine& render,const ForceSpecLifecycle& scene,unsigned* count,std::string* error){
        if(count)*count=0;
        if(!count||!ready_||!storage_||!storage_->scene_owner||!scene.scope().lock()||scene.scope().lock()!=storage_->scene_scope.lock()||scene.callbacks()!=storage_->live.scene_callback||scene.phase()!=storage_->live.scene_phase)return refuse(error,"stale/foreign scene draw producer");
        if(!scene.visible())return scene.phase()==ForceSpecPhase::Resources;
        if(!storage_->scene_visible)return refuse(error,"unpublished scene draw frame");
        *count=draw_impl(render);return true;
    }
    const CharacterPreviewState* state()const{return ready_&&storage_?&storage_->live:nullptr;}
    const CharacterModelScene* model()const{return ready_&&storage_?&storage_->model:nullptr;}
    bool ready()const{return ready_&&bool(storage_);}
    const CharacterModelParts* parts()const{return ready()&&storage_->parts_owner?&storage_->parts:nullptr;}
private:
    unsigned draw_impl(RenderEngine& render){if(!ready_||!storage_||!storage_->live.published||!render.scene_lighting())return 0;auto& r=*storage_;unsigned n=r.live.material.draw(r.model,render,r.live.camera);if(r.parts_owner)n+=r.parts.draw(r.model,render,r.live.camera);for(unsigned i=0;i<2;++i)if(r.live.child_visible[i])n+=r.children[i].draw(render,r.live.camera);return n;}
    struct Storage {CharacterModelScene model;CharacterMotionDriver driver;CharacterProgramAsset program;StartupActorSetupProfile actor_setup;StartupActorRuntimeProfile runtime;StartupActorClassProfile class_profile;ActorFadeProfile fade;StartupAttachmentProfile attachment_profile;CharacterPreviewProfile preview;CharacterPreviewMode mode=CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements;std::array<AttachmentModelScene,2> children;CharacterPreviewState live;CharacterPoseHeightProfile height;bool height_owner=false,parts_owner=false,root_owner=false,directional_root_owner=false,actions_owner=false,demo_owner=false;CharacterModelParts parts;ActorSoundEventBank events;CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly;unsigned root_lane=2;ForceSpecActorActionProfile action_profile;std::weak_ptr<const CharacterModelBinding> action_binding;std::shared_ptr<const CharacterPreviewActionStart> action_start;bool scene_owner=false,scene_visible=false;std::weak_ptr<const ForceSpecLifecycleScope> scene_scope;};
    static bool height_fields(const Storage& r,const CharacterPreviewState& candidate,CharacterPoseHeightFields& f,std::vector<f32>& y,std::string* error){
        f={};y.clear();f.model_group3e8=u8(r.actor_setup.model_id>>8);f.field661=candidate.scalars.field661;f.behaviour5e0=candidate.vm.state().actor.behaviour;f.scale_b4=candidate.scalars.scale_b4;f.actor_y=candidate.setup.position[1];f.height66c=candidate.setup.actor.field66c;f.radius670=candidate.setup.actor.field670;f.middle_bone=std::bit_cast<s8>(r.actor_setup.class_config[15]);for(unsigned i=0;i<4;++i)f.lower_bones[i]=std::bit_cast<s8>(r.actor_setup.class_config[0x22+i*8]);
        if(r.live.published){const auto binding=r.live.world_binding.lock();const auto& rig=r.model.rig();
            if(!binding||binding!=r.model.instance_binding()||r.live.world_frame!=r.live.frames||candidate.frames!=r.live.frames||r.live.world.matrices.size()!=rig.binding.bones.size()||r.live.world.joint_offsets.size()!=rig.binding.bones.size())return fail(error,"stale/foreign native height world frame");
            for(unsigned i=0;i<r.live.world.matrices.size();++i){if(r.live.world.joint_offsets[i]!=rig.binding.bones[i].joint_offset||!character_pose_detail::matrix(r.live.world.matrices[i]))return fail(error,"native height world/rig binding");y.push_back(r.live.world.matrices[i].at(1,3));}f.world_positions_declared=true;
        }
        return true;
    }
    static bool height_context(const Storage& r,const CharacterPreviewState& candidate,CharacterProgramHeightControl* out,std::string* error){
        CharacterPoseHeightFields f;std::vector<f32> y;return height_fields(r,candidate,f,y,error)&&out&&out->bind(candidate.vm,r.height,f,y,CharacterProgramHeightControlMode::DeclaredFields,error);
    }
    static bool root_context(const Storage& r,const CharacterPreviewState& candidate,CharacterProgramDirectionalControl* out,std::string* error){
        CharacterPoseHeightFields f;std::vector<f32> y;CharacterMotionRootFields root;root.scale_b4=candidate.scalars.scale_b4;root.delta5f4=candidate.setup.actor.field5f4;root.yaw5ae=candidate.root_yaw5ae;
        return r.root_owner&&candidate.setup.position==candidate.vm.state().actor.root.position&&height_fields(r,candidate,f,y,error)&&out&&(r.directional_root_owner?out->bind_with_directional_root(candidate.vm,r.driver,r.height,f,y,root,error,r.root_mode):out->bind_with_root(candidate.vm,r.driver,r.height,f,y,root,error,r.root_mode));
    }
    static bool motion_boundary(const Storage& r,const CharacterPreviewState& s,const CharacterProgramDirectionalControl& control){
        const auto& actor=s.vm.state().actor;
        //58EC8 runs CD24 directly, not55568/57D9C: retain1CEF/1CF0 markers
        //without inventing a consume/clear ACK.6E8 feeds the actual child owner
        //below;6E9 still passes through the class callback/effect refusal.
        const bool controls=r.demo_owner?(actor.control0[0]!=0&&actor.control0[0]!=1&&actor.control0[0]!=255)||actor.control0[1]||actor.control0[2]>1||actor.control0[3]:actor.control0!=std::array<u8,4>{};
        if(actor.auxiliary||controls||actor.control1!=std::array<u8,2>{}||(actor.behaviour&~u32(r.demo_owner?0x50:0x40))||!s.vm.actor_root_control_mode()||!character_motion_root_state_valid(actor.root))return false;
        if(r.demo_owner&&!s.vm.completed_directional_follow_tail_mode())return false;
        if(r.directional_root_owner&&(!s.vm.actor_directional_root_control_mode()||!control.directional_root_controls()))return false;
        for(unsigned lane=0;lane<2;++lane){const auto* native=control.snapshot(s.vm.state().lanes[lane].motion);if(!native||native->lane()!=lane)return false;}
        return r.root_owner;
    }
    static bool height_boundary(const Storage& r,const CharacterPreviewState& s){
        if(!r.height_owner)return true;
        if(!r.parts_owner&&s.vm.state().actor.models!=r.program.default_models)return false;
        //The branch pulse belongs to CD24's lane; preview has no57D9C branch
        //dispatch. Leave it intact until the VM's next selected callback.
        if(!r.demo_owner)for(const auto& lane:s.vm.state().lanes)if(lane.program_branch)return false;
        return true; //model-selection and actor branch-pulse consumers remain separate
    }
    static bool rig_binding(const StartupActorSetupProfile& setup,const CharacterPoseRig& rig){if(setup.class_config[2]!=rig.lane_mask)return false;for(unsigned i=0;i<3;++i)if(setup.class_config[12+i]!=rig.cuts[i+1])return false;for(unsigned i=0;i<rig.order.size();++i)if(setup.class_config[0x8c+i]!=rig.order[i]||setup.class_config[0x6c+i]!=rig.mirror[i])return false;return true;}
    static bool sync(CharacterPreviewState& s){CharacterProgramClassFields f;if(!s.vm.observe_class_fields(&f))return false;s.scalars.klass.field582=f.default_group;s.scalars.klass.field6e9=f.effect_control;s.scalars.klass.flags5e0=f.behaviour;s.scalars.field709=s.vm.state().actor.auxiliary;return true;}
    static bool idle(const CharacterPreviewState& s,const CharacterMotionDriver& d,bool height_owner=false){if(s.vm.state().actor.auxiliary||s.vm.state().actor.control0!=std::array<u8,4>{}||s.vm.state().actor.control1!=std::array<u8,2>{}||(s.vm.state().actor.behaviour&~u32(height_owner?0x40:0)))return false;for(unsigned i=0;i<2;++i){const auto& lane=s.vm.state().lanes[i];const auto* sample=d.snapshot(lane.motion);if(!sample||sample->lane()!=i||sample->group()!=0||sample->index()!=0||!sample->looping()||sample->blending()||lane.metadata[4]!=255||lane.descriptor_group||lane.descriptor_choice||lane.done)return false;}return true;}
    static bool fail(std::string* error,const char* why){if(error&&error->empty())*error=std::string("character preview: ")+why;return false;}
    bool refuse(std::string* error,const char* why){retire();return fail(error,why);}
    std::unique_ptr<Storage> storage_;bool ready_=false;
};
} // namespace gf
