#pragma once
// Whole graphics-only join of existing source-qualified components. Explicit
// isolated constructor/placement/readiness scope; NOT original full actor/audio
// admission. No platform, filesystem, original DOL/AFS or renderer fork.
#include "engine/gameplay/startup_static_graphics_scene.hpp"
#include "engine/gameplay/startup_reveal_scene.hpp"
#include "engine/gameplay/startup_props_scene.hpp"
#include "engine/gameplay/startup_intro_props_scene.hpp"
#include "engine/gameplay/startup_thruster_scene.hpp"
#include "engine/gameplay/startup_landing_scene.hpp"
#include "engine/gameplay/startup_attachment_pairs.hpp"
#include "engine/gameplay/startup_scene_lighting.hpp"
#include "engine/gameplay/startup_environment_scene.hpp"
#include "engine/gameplay/model_pass_queue.hpp"
#include "engine/gameplay/startup_feedback.hpp"
#include "engine/gameplay/startup_registration.hpp"
#include "engine/gameplay/startup_menu_scene.hpp"
#include "engine/gameplay/startup_menu_handoff.hpp"
#include "engine/gameplay/startup_trail_scene.hpp"
#include "engine/gameplay/actor_sound_events.hpp"
#include "engine/gameplay/startup_sound_capture.hpp"
#include "engine/gameplay/startup_sound_no_viewers.hpp"
namespace gf {
enum class StartupGraphicsMode {DeclaredIsolatedConstructorsPlacementAndReadiness};
enum class StartupSoundManagerProducerMode {NativeNoViewersHeldConstructionPlans};
struct StartupGraphicsAssets {
    std::array<StartupActorFrameAssets,2> actors;std::array<ByteView,2> attachments;
    ByteView title,stage,effects,jet_model,director,reveal,props,intro,statics,auxiliary,turn,landing,passes,feedback,handoff,alternate;
    std::array<ByteView,3> ground;std::span<const StartupCameraInput> cameras;ByteView menu_layout,menu_motion,menu_navigation;
    ByteView trail,trail_atlas;std::array<StartupTrailParentFields,2> trail_fields;
};
struct StartupGraphicsFrame {
    u32 frame=0;bool menu_created=false,navigation_enabled=false;unsigned landing_requests=0,trail_requests=0,pending_sounds=0;std::array<unsigned,2> actor_requirements{};
};
class StartupGraphicsScene {
public:
    bool load_with_manager_sound_intents(const StartupGraphicsAssets& a,StartupGraphicsMode mode,u32 width,u32 height,const std::array<ByteView,2>& events,ByteView manager,StartupSoundManagerProducerMode sound_mode,std::string* error){
        if(!load_with_sound_intents(a,mode,width,height,events,error))return false;
        auto& s=*storage_;if(sound_mode!=StartupSoundManagerProducerMode::NativeNoViewersHeldConstructionPlans||!startup_sound_load(manager,&s.manager_profile,error)||!s.no_viewers.initialize(error))return refuse(error,"explicit native no-viewer/held constructor manager scope required");
        auto scope=std::make_shared<StartupSoundCaptureScope>();for(unsigned i=0;i<2;++i)scope->actors[i]=s.actors[i].model()->instance_binding();scope->manager_producers=true;s.sound_scope=std::move(scope);s.manager_producers=true;if(error)error->clear();return true;
    }
    bool load_with_sound_intents(const StartupGraphicsAssets& a,StartupGraphicsMode mode,u32 width,u32 height,const std::array<ByteView,2>& events,std::string* error){
        if(!load(a,mode,width,height,error))return false;
        auto& s=*storage_;
        for(unsigned i=0;i<2;++i)if(!s.sound_events[i].load(events[i],s.actors[i].model()->instance_binding()->model_hash,error)||s.sound_events[i].model_id()!=s.actors[i].state()->scalars.klass.model_id)return refuse(error,"explicit model-bound actor event profile");
        auto scope=std::make_shared<StartupSoundCaptureScope>();for(unsigned i=0;i<2;++i)scope->actors[i]=s.actors[i].model()->instance_binding();
        s.sound_scope=std::move(scope);if(error)error->clear();return true;
    }
    bool load(const StartupGraphicsAssets& a,StartupGraphicsMode mode,u32 width,u32 height,std::string* error){
        storage_.reset();ready_=false;
        if(mode!=StartupGraphicsMode::DeclaredIsolatedConstructorsPlacementAndReadiness||!width||!height||width>4096||height>4096)return fail(error,"explicit graphics-only scope/canvas required");
        auto next=std::make_unique<Storage>();auto& s=*next;s.width=width;s.height=height;
        if(!startup_director_load(a.director,&s.director_profile,error)||!startup_reveal_load(a.reveal,&s.reveal_profile,error)||!startup_prop_load(a.props,&s.prop_profile,error)||!startup_intro_prop_load(a.intro,&s.intro_profile,error)||!startup_static_graphics_load(a.statics,&s.static_profile,error)||!startup_auxiliary_load(a.auxiliary,&s.aux_profile,error)||!startup_auxiliary_turn_load(a.turn,&s.turn_profile,error)||!startup_landing_load(a.landing,&s.landing_profile,error)||!startup_scene_pass_load(a.passes,&s.pass_profile,error)||!startup_feedback_load(a.feedback,&s.feedback_profile,error)||!startup_menu_handoff_load(a.handoff,&s.handoff_profile,error))return false;
        if(!s.title.load(a.title,error)||!s.stage.load(a.stage,error)||!s.effects.load(a.effects,error))return false;
        for(unsigned i=0;i<2;++i){StartupActorFrameOutput out;if(!s.actors[i].load(a.actors[i],StartupActorFrameMode::IsolatedRequirements,u8(i),&out,error)||!constructor_world(s.actors[i],&s.construction[i],error)||!s.parts[i].bind(s.actors[i],a.actors[i].model,a.attachments[i],a.actors[i].fade,a.alternate,StartupAttachmentPairMode::DeclaredSceneObservations,error))return false;s.constructor_requirements[i]=unsigned(out.requirements.size());}
        if(!a.trail_fields[0].declared||!a.trail_fields[1].declared||!startup_trail_load(a.trail,&s.trail_profile,error)||!s.trails.load(s.trail_profile,a.trail_atlas,parents(s),error))return fail(error,"explicit trail parent fields/profile/atlas required");
        s.trail_fields=a.trail_fields;
        std::array<StihGrid,3> ground;for(unsigned i=0;i<3;++i)if(!ground[i].load(a.ground[i],error))return false;
        if(!s.statics.load(s.title,s.static_profile,StartupStaticGraphicsMode::DeclaredSceneObservations,error)||!s.reveal.load(s.title,s.reveal_profile,{s.actors[0].model(),s.actors[1].model()},StartupRevealMode::DeclaredNativeActorSnapshots,error)||!s.props.load(s.stage,s.prop_profile,StartupPropMode::DeclaredSceneObservations,error)||!s.intro.load(a.stage,s.stage,s.intro_profile,a.alternate,StartupIntroPropMode::DeclaredSceneObservations,error)||!s.jets.bind(s.actors[0],a.jet_model,s.aux_profile,s.turn_profile,StartupThrusterMode::DeclaredIsolatedConstructionAndPlacement,0,{true,true},error)||!s.landings.load(s.effects,s.landing_profile,ground,StartupLandingMode::DeclaredSourceGroundAndAnchorSnapshots,error))return false;
        HsdRecordAnimationBinding environment;if(!s.stage.select(51,0,0,0,&environment,error)||!s.lighting.load(environment.scene()->lighting,s.pass_profile,s.stage.hash(),error)||!s.environment.load(a.stage,s.stage,s.pass_profile,error)||!s.queue.load(s.pass_profile)||!s.feedback.load(s.feedback_profile,0x70000000,error)||!s.menu.load(a.stage,a.menu_layout,a.menu_motion,a.menu_navigation,error))return false;
        const std::array<u8,6> indices={0,1,2,3,4,5};if(!s.director.load(s.director_profile,indices,error)||!s.cameras.load(a.cameras,error)||!s.cameras.initialize(&s.camera)||!s.cameras.observations(&s.observations))return false;
        s.current=s.camera;s.current.camera.viewport_width=s32(width);s.current.camera.viewport_height=s32(height);storage_=std::move(next);ready_=true;if(error)error->clear();return true;
    }
    bool step(const std::array<GamePadState,4>& pads,StartupGraphicsFrame* out,std::string* error){
        return step_impl(pads,out,nullptr,error);
    }
    bool step_with_sound_intents(const std::array<GamePadState,4>& pads,StartupGraphicsFrame* out,StartupSoundCaptureFrame* sound,std::string* error){
        if(sound)*sound={};
        if(!sound||!ready()||!storage_->sound_scope){if(out)*out={};return fail(error,"explicit loaded sound-intent scope/output required");}
        return step_impl(pads,out,sound,error);
    }
    bool step_with_manager_sound_intents(const std::array<GamePadState,4>& pads,StartupSoundTickSource& clock,StartupGraphicsFrame* out,StartupSoundCaptureFrame* sound,std::string* error){
        if(sound)*sound={};
        if(!sound||!ready()||!storage_->sound_scope||!storage_->manager_producers){if(out)*out={};return fail(error,"explicit loaded manager producer scope/output required");}
        return step_impl(pads,out,sound,error,&clock);
    }
    bool sound_no_viewer_frame(u32 frame,StartupSoundNoViewerFrame* out,std::string* error)const{
        if(out)*out={};
        if(!out||!ready()||!storage_->manager_producers||!frame||frame!=storage_->frames)return fail(error,"accepted current manager/no-viewer frame required");
        return storage_->no_viewers.frame(frame,out,error);
    }
private:
    bool step_impl(const std::array<GamePadState,4>& pads,StartupGraphicsFrame* out,StartupSoundCaptureFrame* sound,std::string* error,StartupSoundTickSource* clock=nullptr){
        if(out)*out={};
        if(!ready_||!storage_||!out)return refuse(error,"loaded scene/output required");
        auto& s=*storage_;if(s.frames>=8192)return refuse(error,"native update budget");const u32 frame=s.frames+1;const bool menu_mode=s.terminal;std::vector<StartupEffect> events;
        if(s.manager_producers!=bool(clock)||(bool(clock)&&!sound))return refuse(error,"manager and legacy capture modes cannot be mixed");
        StartupSoundCaptureFrame captured;captured.scope=s.sound_scope;captured.frame=frame;std::size_t prop_prefix=0,reveal_prefix=0;
        auto manager_candidate=s.manager_candidate;captured.manager_producers=bool(clock);
        auto manager_action=[&](StartupSoundManagerCapture action){
            StartupSoundClockedPlan plan;if(!clock||!startup_sound_plan_sampled(s.manager_profile,manager_candidate,action.operation,*clock,&plan,error))return false;
            action.tick_sampled=plan.clock_sampled;action.clock=plan.sampled;
            if(plan.clock_sampled){action.operation.tick_declared=true;action.operation.tick=plan.sampled.tick;action.operation.bus_clock=plan.sampled.bus_clock;}
            captured.manager_actions.push_back(action);manager_candidate=std::move(plan.manager.candidate);
            for(unsigned i=0;i<plan.manager.pending_count;++i){const auto& call=plan.manager.pending[i];if(!call.owner||call.owner>2)return fail(error,"scoped native manager owner required");StartupSoundIntent intent;intent.origin=action.pre_actor?StartupSoundIntentOrigin::ActorManagerPre:StartupSoundIntentOrigin::ManagerTail;intent.actor=u8(call.owner-1);intent.call=call;if(!startup_sound_intent_append(&captured,intent))return fail(error,"manager sound-intent frame budget");}
            return true;
        };
        if(!menu_mode&&!s.director.step(s.observations,&events,error))return refuse(error,"director");
        auto director=s.director.state();director.updates=frame;
        if(sound)for(const auto& e:events)if(e.kind==StartupEffectKind::Command){StartupSoundIntent intent;intent.origin=StartupSoundIntentOrigin::Director;intent.opcode=e.opcode;intent.command_offset=e.command_offset;
            if(e.opcode==21){intent.operand=u16(u16(e.operands[0])<<8|e.operands[1]);if(intent.operand>=384)return refuse(error,"qualified director sound cue domain");intent.call={StartupSoundCallKind::StartDirect,s16(intent.operand),0,0};if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");}
            else if(e.opcode==19||e.opcode==20){intent.kind=StartupSoundIntentKind::MusicCommand;intent.operand=e.opcode==19?u16(u16(e.operands[0])<<8|e.operands[1]):e.operands[0];if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");}
        }
        bool actor_tick=false,light_request=false;for(const auto& e:events){actor_tick|=e.kind==StartupEffectKind::ActorFrame;light_request|=e.kind==StartupEffectKind::Command&&e.opcode==18;
            if(e.kind==StartupEffectKind::Command&&e.opcode==13){StartupActorFrameOutput result;auto& actor=s.actors.at(e.actor);bool ok=false;if(e.operands[0]==1)ok=actor.reset(&result,error);else if(e.operands[0]==2)ok=actor.set_render_enabled(e.render_group!=0);else{ok=actor.request(e.operands[2],&result,error);++s.motion_requests;}if(!ok)return refuse(error,"actor request prefix");s.actor_requirements[e.actor]+=unsigned(result.requirements.size());}}
        const u8 mode=menu_mode?2:1;
        if(!s.statics.before_actors(director,mode,true,error)||!s.props.before_actors(director,mode,error)||!s.intro.before_actors(director,mode,0,error))return refuse(error,"group28");
        if(!menu_mode&&(!s.reveal.before_actors(director,1,true,error)||!s.jets.before_actors(s.actors[0],s.entry,frame,actor_tick,error)))return refuse(error,"source pre-actor groups");
        if(sound){for(const auto& cue:s.props.pending_source_sounds()){StartupSoundIntent intent;intent.origin=StartupSoundIntentOrigin::PropPre;intent.part=cue.part;intent.call={StartupSoundCallKind::StartDirect,s16(cue.id),0,0};intent.bypass_disabled=true;if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");++prop_prefix;}
            if(!menu_mode)for(const auto& cue:s.reveal.pending_source_sounds()){StartupSoundIntent intent;intent.origin=StartupSoundIntentOrigin::RevealPre;intent.part=cue.record;intent.call={StartupSoundCallKind::StartDirect,s16(cue.id),0,0};intent.bypass_disabled=true;if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");++reveal_prefix;}}
        if(!menu_mode&&!s.trails.before_actors(frame,parents(s),s.trail_fields,error))return refuse(error,"source trail group18 before actors");
        std::vector<StartupLandingRequest> requests;std::vector<u8> request_parents;
        std::vector<u8> trail_requests;
        if(actor_tick)for(unsigned i=0;i<2;++i){
            if(clock&&i==0){StartupThrusterSoundRequirement requirement;if(!s.jets.sound_preupdate_requirement(s.actors[i],frame,&requirement,error))return refuse(error,"native pre-VM sound requirement producer");if(requirement.refresh_required){StartupSoundManagerCapture action;action.actor=u8(i);action.pre_actor=true;action.operation.actor96=s.actors[i].state()->dispatch.state().render_index;action.operation.owner=u64(i)+1;action.operation.slot=5;action.operation.descriptor=0;action.parent_frame_before=requirement.parent_frame_before;action.native_parent708=requirement.native_parent708;action.construction708=requirement.construction708;action.pre709=requirement.pre709;action.pre707=requirement.pre707;if(!manager_action(action))return refuse(error,"sampled manager refresh producer");}}
            const auto previous=world(s,i).matrices[0];StartupActorFrameOutput result;if(!s.actors[i].step(s.entry,&result,error))return refuse(error,"staged actor");s.actor_requirements[i]+=unsigned(result.requirements.size());
            StartupAttachmentPairInput child;child.scene_fields_declared=child.unmasked_child_tick_declared=true;child.registration.viewer_slot=-1;child.registration.scene_counter=frame;if(!s.parts[i].step(s.actors[i],child,error))return refuse(error,"bound attachment pair");
            if(sound)for(const auto& requirement:result.requirements){StartupSoundIntent intent;intent.actor=u8(i);intent.lane=requirement.lane;
                if(requirement.origin==StartupActorRequirementOrigin::Substate&&requirement.code==u8(StartupActorEffectKind::Sound242Required)){if(requirement.id!=242)return refuse(error,"source actor exit sound identity");intent.origin=StartupSoundIntentOrigin::ActorExit;intent.call={StartupSoundCallKind::StartDirect,242,u64(i)+1,0};if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");}
                else if(requirement.origin==StartupActorRequirementOrigin::EventScript){if(requirement.lane>=2)return refuse(error,"qualified actor event lane domain");const auto* state=s.actors[i].state();const auto& motion=state->vm.state().lanes[requirement.lane].motion;const auto* snapshot=dynamic_cast<const CharacterMotionSnapshot*>(motion.context.get());if(!motion.selected||!snapshot)return refuse(error,"accepted source event motion snapshot");ActorSoundClock clocks{motion.elapsed,snapshot->previous_elapsed(),motion.frame,snapshot->previous_frame(),motion.end_frame,state->scalars.scale_b4};std::vector<StartupSoundPendingCall> calls;if(!actor_sound_events(s.sound_events[i],requirement.id,clocks,u64(i)+1,&calls,error))return refuse(error,"source actor sound-event rule");intent.origin=StartupSoundIntentOrigin::ActorScript;intent.script=requirement.id;for(const auto& call:calls){intent.call=call;if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");}}
            }
            for(const auto& r:result.requirements)if(r.origin==StartupActorRequirementOrigin::Substate&&r.code==u8(StartupActorEffectKind::Trail7ERequired)){if(r.id!=126)return refuse(error,"qualified source trail7E only");trail_requests.push_back(u8(i));}
            for(const auto& r:result.requirements)if(r.origin==StartupActorRequirementOrigin::Substate&&(r.code==u8(StartupActorEffectKind::AnchorEffect2Required)||r.code==u8(StartupActorEffectKind::AnchorEffect0Required))){if(!r.anchor_known)return refuse(error,"landing requires accepted previous root");for(unsigned k=0;k<3;++k)if(r.anchor[k]!=previous.at(k,3))return refuse(error,"landing previous-root provenance");StartupLandingRequest request;request.kind=r.id;request.anchor=r.anchor;request.factor=r.factor;request.height668=s.actors[i].state()->dimensions.field668;request.ground_actor.fields_declared=true;request.ground_actor.current24=s.actors[i].state()->declared_position[1];request.ground_actor.previous30=s.entry.position[1];requests.push_back(request);request_parents.push_back(u8(i));}}
        auto actors=snapshots(s,frame);std::vector<StartupPropSound> prop_sounds;std::vector<StartupRevealSound> reveal_sounds;
        if(!s.props.after_actors(frame,0,&prop_sounds,error))return refuse(error,"prop transforms");
        if(!menu_mode){if(!s.reveal.after_actors(actors,0,&reveal_sounds,error)||!s.jets.after_actors(s.actors[0],frame,0,error)||!s.landings.step(frame,requests,1,0,error))return refuse(error,"after-actor groups");for(unsigned n=0;n<requests.size();++n){const unsigned index=unsigned(s.landings.instances()-requests.size()+n);s.landing_owners.push_back({s.landings.key(index),request_parents[n]});}}
        if(sound){if(prop_prefix>prop_sounds.size()||reveal_prefix>reveal_sounds.size())return refuse(error,"source prefix/after-actor sound publication mismatch");
            unsigned prop_seen=0,reveal_seen=0;for(const auto& intent:captured.intents){if(intent.origin==StartupSoundIntentOrigin::PropPre){const auto& cue=prop_sounds[prop_seen++];if(intent.call.cue!=cue.id||intent.part!=cue.part)return refuse(error,"source prop prefix changed during transform publication");}else if(intent.origin==StartupSoundIntentOrigin::RevealPre){const auto& cue=reveal_sounds[reveal_seen++];if(intent.call.cue!=cue.id||intent.part!=cue.record)return refuse(error,"source reveal prefix changed during transform publication");}}
            for(unsigned n=unsigned(prop_prefix);n<prop_sounds.size();++n){StartupSoundIntent intent;intent.origin=StartupSoundIntentOrigin::PropPost;intent.part=prop_sounds[n].part;intent.call={StartupSoundCallKind::StartDirect,s16(prop_sounds[n].id),0,0};intent.bypass_disabled=true;if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");}
            for(unsigned n=unsigned(reveal_prefix);n<reveal_sounds.size();++n){StartupSoundIntent intent;intent.origin=StartupSoundIntentOrigin::RevealPost;intent.part=reveal_sounds[n].record;intent.call={StartupSoundCallKind::StartDirect,s16(reveal_sounds[n].id),0,0};intent.bypass_disabled=true;if(!startup_sound_intent_append(&captured,intent))return refuse(error,"source sound-intent frame budget");}}
        if(!menu_mode&&!s.trails.after_actors(frame,parents(s),s.trail_fields,trail_requests,error))return refuse(error,"source trail history/constructor requirements");
        if(!s.lighting.step(frame,light_request,error)||!s.environment.colour(s.lighting,error))return refuse(error,"global light/colour-mobj state");
        bool create=false,enable=false;
        if(!menu_mode){for(const auto& e:events)if((e.kind==StartupEffectKind::Command&&e.opcode==7)||e.kind==StartupEffectKind::CameraSample){if(!s.cameras.sample(director.camera_source,u32(e.value),s.camera,&s.camera,error))return refuse(error,"source camera");s.current=s.camera;}}
        else{if(!s.handoff.ready()&&!s.handoff.begin(s.handoff_profile,s.cameras,s.current,true,error))return refuse(error,"handoff begin");StartupMenuHandoffStep effects;if(!s.handoff.step(s.cameras,&effects,error))return refuse(error,"fractional camera handoff");create=effects.create_menu_objects;enable=effects.navigation_enabled;if(effects.camera_sample)s.current=effects.camera;}
        if(clock&&!menu_mode){StartupSoundManagerCapture action;action.operation.kind=StartupSoundOperationKind::Update;if(!manager_action(action))return refuse(error,"sampled manager source tail producer");}
        s.current.camera.viewport_width=s32(s.width);s.current.camera.viewport_height=s32(s.height);
        if(!s.statics.register_camera(frame,s.current,0,error)||!(sound?s.menu.step_with_sound(create,enable,pads,&captured.ui,error):s.menu.step(create,enable,pads,error)))return refuse(error,"camera registration/menu handoff");
        if(!menu_mode){if(!s.feedback.step(director,events,error))return refuse(error,"feedback state");}else{StartupDirectorState idle;idle.updates=frame;if(!s.feedback.step(idle,{},error))return refuse(error,"idle feedback state");}
        if(!build_registration(s,menu_mode,error))return refuse(error,"source callback registration");
        if(sound){for(unsigned i=0;i<2;++i){const auto* state=s.actors[i].state();captured.actors[i]=s.actors[i].model()->instance_binding();captured.owners[i]={u64(i)+1,frame,s.actors[i].ready(),state->have_declared_position,s8(state->dispatch.state().render_index),state->declared_position};}if(clock){s.manager_candidate=manager_candidate;captured.manager_candidate=manager_candidate;}*sound=std::move(captured);}
        s.frames=frame;s.terminal=menu_mode||s.director.state().terminal;s.rendered=false;out->frame=frame;out->menu_created=create;out->navigation_enabled=enable;out->landing_requests=unsigned(requests.size());out->trail_requests=unsigned(trail_requests.size());out->pending_sounds=unsigned(prop_sounds.size()+reveal_sounds.size());out->actor_requirements=s.actor_requirements;if(error)error->clear();return true;
    }
public:
    unsigned upload_textures(RenderEngine& r){if(!ready_||!storage_)return 0;auto& s=*storage_;unsigned n=0;n+=s.statics.upload_textures_shared(r,1);n+=s.reveal.upload_textures_shared(r,1);n+=s.environment.upload_textures_shared(r,1000);n+=s.props.upload_textures_shared(r,1000);n+=s.intro.upload_textures_shared(r,1000);n+=s.menu.upload_textures_shared(r,1000);for(unsigned i=0;i<2;++i){n+=s.actors[i].upload_textures_shared(r,2000+i*1000);n+=s.parts[i].upload_textures_shared(r,2000+i*1000);}n+=s.jets.upload_textures_shared(r,5000);n+=s.landings.upload_textures_shared(r,10000);if(!s.trails.upload_texture(r,12000)||r.shared_texture_upload_failed()){ready_=false;return 0;}return n+1;}
    bool draw(RenderEngine& render,std::string* error){
        if(!ready_||!storage_||!storage_->frames||storage_->rendered)return refuse(error,"one render per accepted scene frame");
        auto& s=*storage_;if(!s.lighting.camera_without_fog_contribution(s.current.camera)||!render.set_scene_lighting(*s.lighting.rig()))return refuse(error,"source fog/light domain");
        if(s.manager_producers&&!s.no_viewers.select_render_slot(0,error))return refuse(error,"source no-viewer render selection");
        // New landing instances share the same immutable bank texture namespace.
        s.landings.upload_textures_shared(render,10000);if(render.shared_texture_upload_failed())return refuse(error,"immutable texture namespace/upload failed");unsigned draws=0;
        for(unsigned q=0;q<6;++q){if(!render.begin_model_pass(s.pass_profile.passes[q]))return refuse(error,"begin world pass");for(const auto& v:s.plan)if(v.queue==q)draws+=draw_model(s,v.model,render);if(!render.end_model_pass())return refuse(error,"world pass publication");if(q==4&&!s.menu.created()){unsigned n=0;if(!s.trails.draw(render,s.current.camera,parents(s),&n,error))return refuse(error,"source50800 queue2 trail boundary");draws+=n;}}
        StartupFeedbackRender feedback;if(!s.feedback.render(render,s.frames,s.width,s.height,&feedback,error))return refuse(error,"ordered feedback capture");
        if(s.menu.created()){draws+=s.menu.draw_selector(render,s.current.camera);draws+=s.menu.draw_caption(render,s.current.camera);}s.draws=draws;s.rendered=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_&&bool(storage_);}u32 frame()const{return storage_?storage_->frames:0;}const Camera* camera()const{return ready()?&storage_->current.camera:nullptr;}
    const StartupActorFrame* actor(unsigned i)const{return ready()&&i<2?&storage_->actors[i]:nullptr;}const StartupStaticGraphicsScene* statics()const{return ready()?&storage_->statics:nullptr;}const StartupLandingScene* landings()const{return ready()?&storage_->landings:nullptr;}
    const StartupMenuScene* menu()const{return ready()?&storage_->menu:nullptr;}const StartupFeedbackState* feedback()const{return ready()?&storage_->feedback.state():nullptr;}unsigned draws()const{return ready()?storage_->draws:0;}unsigned motion_requests()const{return ready()?storage_->motion_requests:0;}
    const StartupTrailScene* trails()const{return ready()?&storage_->trails:nullptr;}
private:
    struct LandingOwner {u32 key;u8 parent;};
    struct Storage {
        Storage(){entry.isolated_external_mutations=entry.placement_declared=true;}
        std::array<StartupActorFrame,2> actors;std::array<CharacterWorldPose,2> construction;std::array<StartupAttachmentPairs,2> parts;
        HsdRecordAnimationBank title,stage,effects;StartupDirectorProfile director_profile;StartupRevealProfile reveal_profile;StartupPropProfile prop_profile;StartupIntroPropProfile intro_profile;StartupStaticGraphicsProfile static_profile;StartupAuxiliaryProfile aux_profile;StartupAuxiliaryTurnProfile turn_profile;StartupLandingProfile landing_profile;StartupScenePassProfile pass_profile;StartupFeedbackProfile feedback_profile;StartupMenuHandoffProfile handoff_profile;
        StartupStaticGraphicsScene statics;StartupRevealScene reveal;StartupPropScene props;StartupIntroPropScene intro;StartupThrusterScene jets;StartupLandingScene landings;StartupSceneLighting lighting;StartupEnvironmentScene environment;ModelPassQueue queue;StartupFeedback feedback;StartupMenuScene menu;StartupMenuHandoff handoff;StartupDirector director;StartupCameraSet cameras;StartupCameraState camera;StartupCameraPoseState current;StartupDirectorObservations observations;
        StartupActorFrameInput entry;std::vector<LandingOwner> landing_owners;std::vector<ModelPassVisit> plan;std::array<unsigned,2> actor_requirements{},constructor_requirements{};u32 frames=0,width=0,height=0;unsigned draws=0,motion_requests=0;bool terminal=false,rendered=false;
        StartupTrailScene trails;StartupTrailProfile trail_profile;std::array<StartupTrailParentFields,2> trail_fields;
        std::array<ActorSoundEventBank,2> sound_events;std::shared_ptr<const StartupSoundCaptureScope> sound_scope;
        StartupSoundProfile manager_profile;StartupSoundState manager_candidate;StartupSoundNoViewerOwner no_viewers;bool manager_producers=false;
    };
    static bool constructor_world(const StartupActorFrame& actor,CharacterWorldPose* out,std::string* error){
        const auto& rig=actor.model()->rig();HsdBoneMotionSample rest;rest.bones.resize(rig.binding.bones.size());for(unsigned i=0;i<rest.bones.size();++i)rest.bones[i].joint_offset=rig.binding.bones[i].joint_offset;std::array<CharacterPoseLane,4> lanes;for(auto& l:lanes)l.current=&rest;CharacterLocalPose local;CharacterPoseFrame modifiers;if(!character_pose_sample(rig,lanes,&local,error)||!character_pose_initial_frame(rig,Mat4{},&modifiers,error))return false;modifiers.alternate_root=true;modifiers.scales[0]=actor.state()->dispatch.root_state().root_scale;for(auto& v:modifiers.bone1_translation_adjust)v=-0.f;StartupActorTransform placement;for(unsigned k=0;k<3;++k)placement.scale[k]=actor.state()->dispatch.root_state().actor_scale[k];return character_pose_startup_world(rig,local,placement,modifiers,out,error);
    }
    static const CharacterWorldPose& world(const Storage& s,unsigned i){return s.actors[i].state()->have_world?s.actors[i].state()->world:s.construction[i];}
    static std::array<const StartupActorFrame*,2> parents(const Storage& s){return {&s.actors[0],&s.actors[1]};}
    static std::array<StartupRevealActor,2> snapshots(const Storage& s,u32 frame){std::array<StartupRevealActor,2> out;for(unsigned i=0;i<2;++i)out[i]={s.actors[i].model(),&world(s,i),frame,s.actors[i].state()->dispatch.state().render_group};return out;}
    static bool build_registration(Storage& s,bool menu_mode,std::string* error){
        std::vector<StartupRegistrationItem> items;auto add=[&](u32 id,u32 birth,u8 group,u8 render,u8 part,u8 mask,bool visible,bool tail=false){items.push_back({id,birth,group,render,part,mask,visible,tail});};
        for(unsigned i=0;i<32;++i){const auto& p=s.static_profile.entries[i];add(100+p.record,i,0x28,1,0,p.group,s.statics.states()->at(i).visible);}
        for(unsigned r=0;r<5;++r)add(100+r,r<3?34-r:39-r,r<3?0x28:0x10,1,0,0x44,s.reveal.states()->at(r).allocated&&s.reveal.states()->at(r).visible);
        const auto* intro=s.intro.state();add(200,37,0x28,1,0,0x47,intro->path_visible);add(201,38,0x28,1,0,0x44,intro->fade_visible);for(unsigned i=0;i<8;++i)add(202+i,39+i,0x28,1,0,1,s.props.states()->at(i).enabled);
        if(!menu_mode){for(unsigned e=0;e<2;++e)for(unsigned n=0;n<2;++n){const auto& c=s.jets.states()->at(e);add(400+e*2+n,47+e,0x19,1,u8(n),0x44,c.allocated&&c.render_group&&s.jets.leases()->at(e*2));}
            for(unsigned i=0;i<s.landings.instances();++i){const auto key=s.landings.key(i);const auto* state=s.landings.state(i);auto owner=std::find_if(s.landing_owners.begin(),s.landing_owners.end(),[&](const auto& o){return o.key==key;});if(owner==s.landing_owners.end())return false;add(0x10000+key,1000+key,u8(0x19+owner->parent),1,0,0x24,state->visible);}
            for(unsigned parent=0;parent<2;++parent)if(s.parts[parent].frames())for(unsigned i=0;i<2;++i){const auto& c=s.parts[parent].states()->at(i);const auto& u=s.parts[parent].updates()->at(i);add(300+parent*2+i,i,u8(parent+1),u8(0x41+parent),0,0x47,c.allocated&&u.visible&&c.render_group);}}
        if(s.menu.created())for(unsigned item=0;item<7;++item)for(unsigned part=0;part<2;++part)add(600+item*2+part,2000+item,0x28,1,u8(part),0x47,true,true);
        for(unsigned i=0;i<s.environment.prop_count();++i)add(700+i,100+i,0x28,1,0,s.environment.prop_record(i)<69?0x17:0x14,s.frames>0);
        const std::array<bool,2> visible={s.actors[0].state()->dispatch.state().render_group!=0,s.actors[1].state()->dispatch.state().render_group!=0};std::vector<StartupRegisteredModel> registrations;if(!startup_registration_plan(items,visible,menu_mode,&registrations)||!s.queue.begin(s.frames+1))return false;registrations.push_back({50,0xf});registrations.push_back({51,1});registrations.push_back({52,0xf});for(const auto& r:registrations)if(!s.queue.register_model(r.model,r.mask,true))return false;if(!s.queue.plan(&s.plan))return false;if(error)error->clear();return true;
    }
    static unsigned draw_model(Storage& s,u32 id,RenderEngine& r){const auto& c=s.current.camera;if(id>=50&&id<=52)return s.environment.draw_model(id-50,r,c);if(id>=700&&id<746)return s.environment.draw_prop(id-700,r,c);if(id>=0x10000){for(unsigned i=0;i<s.landings.instances();++i)if(s.landings.key(i)==id-0x10000)return s.landings.draw_model(i,r,c);return 0;}if(id<3)return s.actors[id-1].draw(r,c);if(id<200){unsigned record=id-100;if(record<5)return s.reveal.draw_model(record,r,c,snapshots(s,s.frames));return s.statics.draw_model(36-record,r,c);}if(id<300){unsigned part=id-200;return part<2?s.intro.draw_model(part,r,c):s.props.draw_model(part-2,r,c);}if(id<400){unsigned parent=(id-300)/2;return s.parts[parent].draw_model((id-300)%2,s.actors[parent],r,c);}if(id<500)return s.jets.draw_model((id-400)/2,(id-400)%2,s.actors[0],r,c);return s.menu.draw_world((id-600)/2,(id-600)%2,r,c);}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup graphics join: ")+why;return false;}
    bool refuse(std::string* error,const char* why){const auto detail=error?*error:std::string{};ready_=false;if(storage_){storage_->sound_scope.reset();storage_->no_viewers.reset();}fail(error,why);if(error&&!detail.empty())*error+="; "+detail;return false;}
    std::unique_ptr<Storage> storage_;bool ready_=false;
};
} // namespace gf
