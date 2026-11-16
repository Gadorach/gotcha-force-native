#pragma once
// Source8004BEB8/C020/C050/C298/CD24 prefix/wait subset. Motion lifecycle is
// EXPLICITLY supplied through a pure driver, not fabricated from a clip end.
// No actor-event execution, root integration, render or startup admission.
#include <cmath>
#include <memory>
#include "engine/assets/character_program.hpp"
#include "engine/gameplay/character_pose_height.hpp"
#include "engine/gameplay/character_motion_root_state.hpp"
namespace gf {
enum class CharacterProgramGroundLossFlagPhase {Correction,FallingEntry,LandingEntry,Idle};
struct CharacterProgramMotionContext {virtual ~CharacterProgramMotionContext()=default;};
struct CharacterProgramMotion {
    bool selected=false;f32 elapsed=0,frame=0;s16 end_frame=0;s8 finished=0,rewound=0;
    std::shared_ptr<const CharacterProgramMotionContext> context;
};
class ICharacterProgramMotion {
public:
    virtual ~ICharacterProgramMotion()=default;
    // READ-ONLY evaluators: no external effects, saves, render or mutable shared
    // actor state. Failed VM operations discard all outputs. Real AObj/actor
    // branches require a separately qualified driver; this seam grants none.
    virtual bool select(unsigned lane,const CharacterProgramDescriptor& descriptor,bool mirrored,
                        const CharacterProgramMotion& previous,f32 start,f32 delta,CharacterProgramMotion* out)const=0;
    virtual bool advance(unsigned lane,const CharacterProgramMotion& previous,f32 delta,CharacterProgramMotion* out)const=0;
    virtual bool select_transition(unsigned lane,const CharacterProgramDescriptor& descriptor,bool mirrored,
                                   const CharacterProgramMotion& previous,f32 start,f32 delta,u8 mode,u8 duration,CharacterProgramMotion* out)const{
        return mode==0&&duration==0&&select(lane,descriptor,mirrored,previous,start,delta,out);
    }
};
struct CharacterProgramActor {
    u8 default_group=0,auxiliary=0;
    std::array<u8,4> control0{}; // actor1CEF/1CF0/6E8/6E9
    std::array<u8,2> control1{}; // actor1D0F/1D10
    std::array<u8,2> models{}; // actor1D94/1D95 (opaque attachment selections)
    std::array<u8,3> lane1_descriptor{}; // actor1D98/1D99/1D9B
    u32 behaviour=0;
    CharacterPoseHeightState pose_height; //explicit opcode8 owner, not implicit constructor state
    s16 steering1dfc=0;
    std::array<u8,2> primary_models{},alternate_models{}; //source1D74/75 and1D76/77, not displayed1D94/95
    CharacterMotionRootState root; //explicit root-control mode only; old entries do not consume it
};
struct CharacterProgramLane {
    u16 program=0xffff;u32 wait=0,cursor=0;f32 threshold=0,clock=0,previous_clock=0,offset=0;
    bool done=true,saw_clip=false,mirrored=false,program_branch=false; //source1E pulse, NOT done1A
    u8 descriptor_group=0,descriptor_choice=0;std::array<u8,8> metadata{};
    CharacterProgramMotion motion;
    bool following=false;std::array<u8,6> direction_controls{};
};
struct CharacterProgramState {CharacterProgramActor actor;std::array<CharacterProgramLane,2> lanes;};
struct CharacterProgramClassScope {};
struct CharacterProgramClassFields {
    u8 default_group=0,effect_control=0;u32 behaviour=0;
    std::weak_ptr<const CharacterProgramClassScope> scope;
    s16 steering1dfc=0;
};
struct CharacterProgramRootFields {CharacterMotionRootState root;std::weak_ptr<const CharacterProgramClassScope> scope;};
struct CharacterProgramMotionResult {
    CharacterProgramMotion motion;u8 descriptor_group=0,descriptor_choice=0;bool mirrored=false;
    std::array<u8,8> metadata{};std::array<u8,2> models{},primary_models{},alternate_models{};std::array<u8,3> lane1_descriptor{};
    CharacterMotionRootState root;bool root_checked=false;
};
class ICharacterProgramActorControls {
public:
    virtual ~ICharacterProgramActorControls()=default;
    virtual std::weak_ptr<const CharacterProgramClassScope> scope()const=0;
    // PURE evaluation only. Candidate VM fields, not delayed global actor reads.
    virtual bool pose_height_command(const CharacterProgramActor& actor,u8 mask,u8 operand,CharacterPoseHeightState* out)const=0;
    // Source55DCC->4D08C refresh is distinct from opcode8's mode/multiplier write.
    virtual bool pose_height_refresh(const CharacterProgramActor&,CharacterPoseHeightState*)const{return false;}
    virtual bool motion_driver_matches(const ICharacterProgramMotion&)const{return false;}
    virtual bool ordinary_root_controls()const{return false;}
    virtual bool directional_root_controls()const{return false;}
    virtual bool motion_select(const CharacterProgramActor&,unsigned,const CharacterProgramDescriptor&,bool,const CharacterProgramMotion&,f32,f32,u8,u8,CharacterProgramMotionResult*)const{return false;}
    virtual bool directional_select(const CharacterProgramActor&,unsigned,const std::array<u8,6>&,const CharacterProgramMotion&,f32,f32,u8,CharacterProgramMotionResult*)const{return false;}
    virtual bool motion_advance(const CharacterProgramActor&,unsigned,const CharacterProgramMotion&,f32,CharacterProgramMotionResult*)const{return false;}
};
struct CharacterProgramTransition {s16 duration=-1; s8 mode=-1;}; // negative: authored command field
enum class CharacterProgramSelectionHeightMode {HandlerOnly,OrdinaryLane1};
enum class CharacterProgramOperationKind {Select,Advance,Control0,Control1,Models,Auxiliary,ResetLane1,ProgramBranch,PoseHeight,PoseHeightRefresh,DirectionalSelect,PoseHeightSelection,RootMotion,RootPositionRestore,ScenePlacement};
struct CharacterProgramOperation {
    CharacterProgramOperationKind kind=CharacterProgramOperationKind::Select;
    u8 lane=0;f32 time=0;std::array<u8,8> command{};
};
namespace character_program_vm_detail {
inline bool time(f32 x){return std::isfinite(x)&&x>=0&&x<=8192;}
// Extended playback separates accumulated clocks from bounded clip sampling.
// A loop does not become invalid after8192 total updates. All clip/frame/track
// work budgets stay in time(); legacy observation admission stays unchanged.
inline bool accumulated_time(f32 x,bool transitions){return transitions?std::isfinite(x)&&x>=0:time(x);}
inline bool motion(const CharacterProgramMotion& m,bool transitions=false){return accumulated_time(m.elapsed,transitions)&&time(m.frame)&&m.end_frame>=0&&m.end_frame<=8192&&m.finished>=0&&m.finished<=1&&m.rewound>=0&&m.rewound<=1;}
inline bool actor(const CharacterProgramActor& a){return a.default_group>0&&a.default_group<128&&a.models[0]<128&&a.models[1]<128;}
inline bool pose_height(const CharacterPoseHeightState& state){return std::isfinite(state.multiplier6d8)&&std::abs(state.multiplier6d8)<=1e7f&&std::isfinite(state.offset6d0)&&std::abs(state.offset6d0)<=1e7f;}
} // namespace character_program_vm_detail
class CharacterProgramVm {
public:
    bool load(const CharacterProgramAsset& asset,const CharacterProgramActor& actor,
              const std::array<CharacterProgramMotion,2>& initial,std::string* error){
        loaded_=ready_=fresh_actor_=transitions_=control_flow_=height_controls_=directional_controls_=selection_height_controls_=root_controls_=directional_root_controls_=completed_follow_tail_=false;class_scope_.reset();asset_={};state_={};
        if(!character_program_detail::valid(asset)||!character_program_vm_detail::actor(actor))return fail(error,"asset/initial actor domain");
        for(const auto& m:initial)if(!character_program_vm_detail::motion(m))return fail(error,"initial motion domain");
        asset_=asset;state_.actor=actor;for(unsigned i=0;i<2;++i)state_.lanes[i].motion=initial[i];loaded_=ready_=true;return true;
    }
    // Opt-in8004BB14 fresh two-lane state. Unlike the explicit-input load(),
    // group/models/controls remain pool-cleared, not profile fallback defaults.
    bool load_fresh_actor(const CharacterProgramAsset& asset,std::string* error){
        return load_fresh_impl(asset,error,false);
    }
    bool load_fresh_actor_transitions(const CharacterProgramAsset& asset,std::string* error){
        return load_fresh_impl(asset,error,true);
    }
    bool load_fresh_actor_control_flow(const CharacterProgramAsset& asset,std::string* error){return load_fresh_impl(asset,error,true,true);}
    bool load_fresh_actor_height_controls(const CharacterProgramAsset& asset,const CharacterPoseHeightState& initial,std::string* error){
        if(!load_fresh_impl(asset,error,true,true))return false;
        if(!character_program_vm_detail::pose_height(initial)){loaded_=ready_=false;class_scope_.reset();return fail(error,"explicit initial pose-height state required");}
        state_.actor.pose_height=initial;height_controls_=true;return true;
    }
    bool load_fresh_actor_directional_controls(const CharacterProgramAsset& asset,const CharacterPoseHeightState& initial,std::string* error,
                                               CharacterProgramSelectionHeightMode selection_height=CharacterProgramSelectionHeightMode::HandlerOnly){
        if(!load_fresh_actor_height_controls(asset,initial,error))return false;
        if(selection_height!=CharacterProgramSelectionHeightMode::HandlerOnly&&selection_height!=CharacterProgramSelectionHeightMode::OrdinaryLane1){loaded_=ready_=false;class_scope_.reset();return fail(error,"explicit ordinary selection height mode");}
        //48648 ordinary dispatch only: lane1 sets6D8=1 and evaluates4D08C
        //with the OLD71D, BEFORE outer C298 clears71D. Existing entries retain
        //their handler-only seam. Opcode13 dispatches48F9C, not48648.
        selection_height_controls_=selection_height==CharacterProgramSelectionHeightMode::OrdinaryLane1;
        directional_controls_=true;return true;
    }
    //Same scoped VM/driver path, with candidate ordinary-root state publication.
    //Directional motion on the root-owning lane still needs a shared actor buffer
    //consumer and REFUSES; this entry is not full actor/demo admission.
    bool load_fresh_actor_root_controls(const CharacterProgramAsset& asset,const CharacterPoseHeightState& height,
                                         std::array<f32,3> position,std::string* error){
        if(!load_fresh_actor_directional_controls(asset,height,error,CharacterProgramSelectionHeightMode::OrdinaryLane1))return false;
        for(auto value:position)if(!character_motion_root_number(value)){loaded_=ready_=false;class_scope_.reset();return fail(error,"explicit initial root position domain");}
        state_.actor.root.position=position;root_controls_=true;return true;
    }
    //Separate opt-in: existing root entries keep their directional-root refusal.
    bool load_fresh_actor_directional_root_controls(const CharacterProgramAsset& asset,const CharacterPoseHeightState& height,
                                                      std::array<f32,3> position,std::string* error){
        if(!load_fresh_actor_root_controls(asset,height,position,error))return false;
        directional_root_controls_=true;return true;
    }
    //8004CD24's done branch still invokes49E58 when motion buffer+36==1.
    //This explicit tail retains program clocks/done status; it never satisfies a
    //wait or changes original program bytes. Existing load domains keep freezing.
    bool load_fresh_actor_directional_root_follow_tail(const CharacterProgramAsset& asset,const CharacterPoseHeightState& height,
                                                       std::array<f32,3> position,std::string* error){
        if(!load_fresh_actor_directional_root_controls(asset,height,position,error))return false;
        completed_follow_tail_=true;return true;
    }
private:
    bool load_fresh_impl(const CharacterProgramAsset& asset,std::string* error,bool transitions,bool control_flow=false){
        loaded_=ready_=fresh_actor_=transitions_=control_flow_=height_controls_=directional_controls_=selection_height_controls_=root_controls_=directional_root_controls_=completed_follow_tail_=false;class_scope_.reset();asset_={};state_={};
        if(!character_program_detail::valid_impl(asset,transitions,control_flow)||asset.lane_mask!=3)return fail(error,"fresh actor requires reviewed two-lane asset");
        asset_=asset;for(auto& lane:state_.lanes){lane.done=false;lane.metadata[4]=255;}
        class_scope_=std::make_shared<const CharacterProgramClassScope>();loaded_=ready_=fresh_actor_=true;transitions_=transitions;control_flow_=control_flow;return true;
    }
public:
    bool request(u8 group,u8 choice,u8 requested_mask,f32 start,f32 delta,const ICharacterProgramMotion& driver,
                  std::vector<CharacterProgramOperation>* operations,std::string* error){
        return request_impl(group,choice,requested_mask,start,delta,driver,operations,error,nullptr);
    }
    //8004BEB8 forwards independent duration/mode overrides to the initial prefix
    // only. Preview constructor58244 supplies0,0 (instant), despite authored2/10.
    bool request_with_transition(u8 group,u8 choice,u8 requested_mask,f32 start,f32 delta,CharacterProgramTransition transition,const ICharacterProgramMotion& driver,
                                 std::vector<CharacterProgramOperation>* operations,std::string* error){
        if(!transitions_||transition.duration<-1||transition.duration>255||(transition.mode!=-1&&transition.mode!=0&&transition.mode!=2&&!(control_flow_&&transition.mode==1))){
            if(operations)operations->clear();
            return refuse(error,"explicit transition mode/duration domain");
        }
        return request_impl(group,choice,requested_mask,start,delta,driver,operations,error,&transition);
    }
    bool request_with_actor_controls(u8 group,u8 choice,u8 requested_mask,f32 start,f32 delta,const ICharacterProgramMotion& driver,
                                     const ICharacterProgramActorControls& controls,std::vector<CharacterProgramOperation>* operations,std::string* error,CharacterProgramTransition transition={}){
        if(operations)operations->clear();
        if(!height_controls_||controls.scope().lock()!=class_scope_||!class_scope_||transition.duration<-1||transition.duration>255||transition.mode< -1||transition.mode>2)return refuse(error,"foreign/stale actor control scope or transition");
        if(directional_controls_&&!controls.motion_driver_matches(driver))return refuse(error,"directional driver library binding");
        if(root_controls_&&!controls.ordinary_root_controls())return refuse(error,"explicit ordinary-root consumer required");
        return request_impl(group,choice,requested_mask,start,delta,driver,operations,error,&transition,&controls);
    }
private:
    bool request_impl(u8 group,u8 choice,u8 requested_mask,f32 start,f32 delta,const ICharacterProgramMotion& driver,
                        std::vector<CharacterProgramOperation>* operations,std::string* error,const CharacterProgramTransition* transition,const ICharacterProgramActorControls* controls=nullptr){
        if(operations)operations->clear();
        ready_=false;
        if(directional_controls_&&(!controls||!controls->motion_driver_matches(driver)))return fail(error,"explicit directional motion owner required");
        if(root_controls_&&(!controls||!controls->ordinary_root_controls()))return fail(error,"explicit ordinary-root consumer required");
        if(directional_root_controls_&&(!controls||!controls->directional_root_controls()))return fail(error,"explicit ordered directional-root consumer required");
        if(!loaded_||!operations||!std::isfinite(start)||start>8192||!std::isfinite(delta)||delta<0||delta>16||choice>=128||group>=128||(requested_mask&~3u))return fail(error,"request domain/common-bank/unsupported lanes");
        const u8 mask=requested_mask&asset_.lane_mask;if(!mask)return fail(error,"empty selected lanes");
        const u8 selected_group=group?group:state_.actor.default_group;u16 index=0xffff;
        for(unsigned i=0;i<asset_.programs.size();++i)if(asset_.programs[i].group==selected_group&&asset_.programs[i].choice==choice)index=u16(i);
        if(index==0xffff)return fail(error,"missing explicit program (no invented fallback)");
        auto next=state_;std::vector<CharacterProgramOperation> journal;
        if(mask&2)next.actor.auxiliary=0; //8004BEB8 fresh lane1 request
        for(unsigned i=0;i<2;++i)if(mask&(1u<<i)){
            //8004C020 clears program pointers/clocks/flags, NOT motion or mirror.
            auto& lane=next.lanes[i];lane.program=index;lane.wait=lane.cursor=0;
            lane.threshold=lane.clock=lane.previous_clock=lane.offset=0;lane.done=lane.saw_clip=lane.program_branch=false;
        }
        if(!prefix(next,mask,index,0,start,delta,driver,journal,transition,controls))return fail(error,"prefix/motion driver refused");
        state_=std::move(next);*operations=std::move(journal);ready_=true;return true;
    }
public:
    bool update(u8 requested_mask,f32 delta,const ICharacterProgramMotion& driver,
                 std::vector<CharacterProgramOperation>* operations,std::string* error){
        return update_impl(requested_mask,delta,driver,operations,error,nullptr);
    }
    bool update_with_actor_controls(u8 requested_mask,f32 delta,const ICharacterProgramMotion& driver,const ICharacterProgramActorControls& controls,
                                    std::vector<CharacterProgramOperation>* operations,std::string* error){
        if(operations)operations->clear();
        if(!height_controls_||!class_scope_||controls.scope().lock()!=class_scope_)return refuse(error,"foreign/stale actor control scope");
        if(directional_controls_&&!controls.motion_driver_matches(driver))return refuse(error,"directional driver library binding");
        if(root_controls_&&!controls.ordinary_root_controls())return refuse(error,"explicit ordinary-root consumer required");
        return update_impl(requested_mask,delta,driver,operations,error,&controls);
    }
    bool refresh_height_with_actor_controls(const ICharacterProgramActorControls& controls,std::vector<CharacterProgramOperation>* operations,std::string* error){
        if(operations)operations->clear();
        if(!ready_||!operations||!height_controls_||!class_scope_||controls.scope().lock()!=class_scope_||(root_controls_&&!controls.ordinary_root_controls()))return refuse(error,"height refresh scope/output/readiness/root consumer");
        if(directional_root_controls_&&!controls.directional_root_controls())return refuse(error,"ordered directional-root height consumer required");
        auto next=state_;CharacterPoseHeightState height;std::vector<CharacterProgramOperation> journal;
        if(!controls.pose_height_refresh(next.actor,&height)||!character_program_vm_detail::pose_height(height)||!publish(journal,CharacterProgramOperationKind::PoseHeightRefresh,1,0))return refuse(error,"height refresh consumer refused");
        next.actor.pose_height=height;state_=std::move(next);*operations=std::move(journal);if(error)error->clear();return true;
    }
private:
    bool update_impl(u8 requested_mask,f32 delta,const ICharacterProgramMotion& driver,std::vector<CharacterProgramOperation>* operations,std::string* error,const ICharacterProgramActorControls* controls){
        if(operations)operations->clear();
        if(directional_controls_&&(!controls||!controls->motion_driver_matches(driver)))return refuse(error,"explicit directional update owner required");
        if(directional_root_controls_&&(!controls||!controls->directional_root_controls()))return refuse(error,"explicit ordered directional-root consumer required");
        if(!ready_||!operations||(requested_mask&~3u)||!std::isfinite(delta)||delta<0||delta>16){ready_=false;return fail(error,"update domain/not ready");}
        auto next=state_;std::vector<CharacterProgramOperation> journal;const u8 mask=requested_mask&asset_.lane_mask;
        //8004CD24 checks PRIMARY program before the mask/lane loop. A null
        // primary marks every completion byte; no motion selection/advance.
        // Only this explicit fresh-actor mode admits that global null path.
        if(fresh_actor_&&next.lanes[0].program==0xffff){for(auto& lane:next.lanes)lane.done=true;state_=std::move(next);return true;}
        for(unsigned i=0;i<2;++i){if(!(mask&(1u<<i)))continue;auto& lane=next.lanes[i];if(control_flow_)lane.program_branch=false;
            if(lane.done){if(completed_follow_tail_&&lane.following&&!advance(next,i,delta,driver,journal,transitions_,controls,directional_controls_,root_controls_,false))return refuse(error,"completed directional follow consumer refused");continue;}
            if(lane.program>=asset_.programs.size())return refuse(error,"program identity");
            const auto& bytes=asset_.programs[lane.program].bytes;if(lane.wait+4>bytes.size()||bytes[lane.wait]!=0)return refuse(error,"wait identity");
            const u8 flags=bytes[lane.wait+1];bool satisfied=false;
            //8004CD24 tests the OLD motion before advancing the selected lane.
            const bool timer=control_flow_&&(flags&8);
            if(timer){lane.threshold=f32(lane.threshold-delta);if(lane.threshold<=0){lane.threshold=0;satisfied=true;}}
            else if(flags&2){const f32 threshold=lane.threshold<0?std::max(0.f,f32(lane.motion.end_frame)+lane.threshold):lane.threshold;satisfied=lane.motion.frame>=threshold;}
            else if(lane.threshold<0)satisfied=lane.motion.finished>0||((flags&1)&&lane.motion.rewound>0);
            else satisfied=lane.motion.elapsed>=lane.threshold;
            if(satisfied){
                if(flags&1)lane.done=true;
                else if(!prefix(next,u8(1u<<i),lane.program,lane.cursor,-1,delta,driver,journal,nullptr,controls))return refuse(error,"continuation/motion driver refused");
            }else if(!timer&&!advance(next,i,delta,driver,journal,transitions_,controls,directional_controls_,root_controls_))return refuse(error,"motion advance refused");
        }
        state_=std::move(next);*operations=std::move(journal);return true;
    }
public:
    const CharacterProgramState& state()const{return state_;}
    bool ready()const{return ready_;}
    bool fresh_actor_mode()const{return loaded_&&fresh_actor_;}
    bool actor_height_control_mode()const{return loaded_&&height_controls_;}
    bool actor_directional_control_mode()const{return loaded_&&directional_controls_;}
    bool actor_root_control_mode()const{return loaded_&&root_controls_;}
    bool actor_directional_root_control_mode()const{return loaded_&&directional_root_controls_;}
    bool completed_directional_follow_tail_mode()const{return loaded_&&completed_follow_tail_;}
    const CharacterProgramAsset* program_asset()const{return loaded_?&asset_:nullptr;}
    u32 model_hash()const{return loaded_?asset_.model_hash:0;}
    bool observe_class_fields(CharacterProgramClassFields* out)const{
        if(!out||!ready_||!fresh_actor_||!class_scope_)return false;
        *out={state_.actor.default_group,state_.actor.control0[3],state_.actor.behaviour,class_scope_,state_.actor.steering1dfc};return true;
    }
    //Source UI restores actor20 before57FF8. Only an explicit ordered-root
    //candidate owner can write this alias; no motion/clock/scope is recreated.
    bool observe_root_fields(CharacterProgramRootFields* out)const{
        if(out)*out={};
        if(!out||!ready_||!directional_root_controls_||!class_scope_)return false;
        *out={state_.actor.root,class_scope_};return true;
    }
    bool restore_root_position(const CharacterProgramRootFields& expected,std::array<f32,3> position,std::string* error){
        const auto& current=state_.actor.root;const auto& prior=expected.root;
        if(!ready_||!directional_root_controls_||!class_scope_||expected.scope.lock()!=class_scope_||current.position!=prior.position||current.delta1da0!=prior.delta1da0||current.speed1dac!=prior.speed1dac||current.heading1db0!=prior.heading1db0||current.updates!=prior.updates)return refuse(error,"stale/foreign root position restore");
        for(auto value:position)if(!character_motion_root_number(value))return refuse(error,"root position restore domain");
        state_.actor.root.position=position;return true;
    }
    bool write_class_steering(const CharacterProgramClassFields& expected,s16 steering,std::string* error){
        if(!ready_||!directional_controls_||!class_scope_||expected.scope.lock()!=class_scope_||expected.default_group!=state_.actor.default_group||expected.effect_control!=state_.actor.control0[3]||expected.behaviour!=state_.actor.behaviour||expected.steering1dfc!=state_.actor.steering1dfc)return refuse(error,"stale/foreign steering writeback");
        state_.actor.steering1dfc=steering;return true;
    }
    // Source class callbacks write actor582/6E9, not other controls/lanes.
    // Compare the exact scope and represented input fields before publication.
    bool write_class_fields(const CharacterProgramClassFields& expected,u8 group,u8 selector,std::string* error){
        if(!ready_||!fresh_actor_||!class_scope_||expected.scope.lock()!=class_scope_||group>=128||expected.default_group!=state_.actor.default_group||expected.effect_control!=state_.actor.control0[3]||expected.behaviour!=state_.actor.behaviour)return refuse(error,"stale/foreign/unqualified class writeback");
        state_.actor.default_group=group;state_.actor.control0[3]=selector;return true;
    }
    //Ordinary ground walk/stop callbacks own only behaviour bit10 here.
    //This numeric write grants no actor/contact or callback execution receipt.
    bool write_ground_walk_flag(const CharacterProgramClassFields& expected,bool walking,std::string* error){
        if(!ready_||!fresh_actor_||!height_controls_||!class_scope_||expected.scope.lock()!=class_scope_||
           expected.default_group!=state_.actor.default_group||expected.effect_control!=state_.actor.control0[3]||
           expected.behaviour!=state_.actor.behaviour)return refuse(error,"stale/foreign ground walk flag writeback");
        state_.actor.behaviour=(state_.actor.behaviour&~0x10u)|(walking?0x10u:0u);return true;
    }
    //Scoped numeric flag portions of6A5A4/5FECC/60228/6A474 only.
    //No callback, selector, collision, actor or frame acknowledgement.
    bool write_ground_loss_flags(const CharacterProgramClassFields& expected,CharacterProgramGroundLossFlagPhase phase,std::string* error){
        if(!ready_||!fresh_actor_||!height_controls_||!class_scope_||expected.scope.lock()!=class_scope_||
           expected.default_group!=state_.actor.default_group||expected.effect_control!=state_.actor.control0[3]||
           expected.behaviour!=state_.actor.behaviour||expected.steering1dfc!=state_.actor.steering1dfc||
           (expected.behaviour&0x81000003u)||unsigned(phase)>unsigned(CharacterProgramGroundLossFlagPhase::Idle))
            return refuse(error,"stale/foreign/unqualified ground-loss flags");
        auto& flags=state_.actor.behaviour;
        if(phase==CharacterProgramGroundLossFlagPhase::Correction){
            if(flags&0x2040u)return refuse(error,"ground-loss correction requires reviewed ground flags");
            flags&=0x00ff0041u;
        }else if(phase==CharacterProgramGroundLossFlagPhase::FallingEntry){
            if(flags&0x2000u)return refuse(error,"ground-loss falling requires unblocked motion");
            flags|=0x40u;
        }else if(phase==CharacterProgramGroundLossFlagPhase::LandingEntry)flags=(flags&~0x2040u)|0x200u;
        else flags&=0x00ff0003u;
        return true;
    }
    //61160's one-time control3 acknowledgement only; no VM advance/root update.
    bool clear_ground_dash_recovery_control(const CharacterProgramClassFields& expected,u8 control,std::string* error){
        if(!ready_||!fresh_actor_||!height_controls_||!class_scope_||expected.scope.lock()!=class_scope_||
           expected.default_group!=state_.actor.default_group||expected.effect_control!=state_.actor.control0[3]||
           expected.behaviour!=state_.actor.behaviour||expected.steering1dfc!=state_.actor.steering1dfc||
           (expected.behaviour&0x81002043u)||control!=3||state_.actor.control1[0]!=control)
            return refuse(error,"stale/foreign/unqualified ground-dash recovery control");
        state_.actor.control1[0]=0;return true;
    }
    //80057D9C consumes actor1CEF after the program update, not inside the VM.
    bool consume_primary_control(u8* out){
        if(!ready_||!out)return false;
        *out=state_.actor.control0[0];state_.actor.control0[0]=0;return true;
    }
private:
    bool fail(std::string* error,const char* why){if(error)*error=std::string("character program VM: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    static bool publish(std::vector<CharacterProgramOperation>& journal,CharacterProgramOperationKind kind,unsigned lane,f32 time,const u8* bytes=nullptr,unsigned size=0){
        if(journal.size()>=512)return false;
        CharacterProgramOperation op;op.kind=kind;op.lane=u8(lane);op.time=time;if(bytes)std::copy_n(bytes,size,op.command.begin());journal.push_back(op);return true;
    }
    static bool clocks(const CharacterProgramLane& lane,bool transitions){return character_program_vm_detail::accumulated_time(lane.clock,transitions)&&character_program_vm_detail::accumulated_time(lane.previous_clock,transitions)&&character_program_vm_detail::accumulated_time(lane.offset,transitions);}
    static bool apply_motion(CharacterProgramState& state,unsigned i,const CharacterProgramMotionResult& r,bool root=false){
        if(i>=2||!r.motion.selected||!r.motion.context||!character_program_vm_detail::motion(r.motion,true)||r.descriptor_group>=128||r.descriptor_choice>=128)return false;
        for(auto values:{r.models,r.primary_models,r.alternate_models})for(auto value:values)if(value>=128)return false;
        if(root){if(!r.root_checked||!character_motion_root_state_valid(r.root))return false;state.actor.root=r.root;}
        auto& lane=state.lanes[i];lane.motion=r.motion;lane.descriptor_group=r.descriptor_group;lane.descriptor_choice=r.descriptor_choice;lane.metadata=r.metadata;lane.mirrored=r.mirrored;
        if(!i){state.actor.models=r.models;state.actor.primary_models=r.primary_models;state.actor.alternate_models=r.alternate_models;}else state.actor.lane1_descriptor=r.lane1_descriptor;
        return true;
    }
    static bool advance(CharacterProgramState& state,unsigned i,f32 delta,const ICharacterProgramMotion& driver,std::vector<CharacterProgramOperation>& journal,bool transitions,const ICharacterProgramActorControls* controls=nullptr,bool directional=false,bool root=false,bool program_clocks=true){
        auto& lane=state.lanes[i];CharacterProgramMotion sampled;const auto prior_root_updates=state.actor.root.updates;
        if(directional){CharacterProgramMotionResult result;if(!controls||!controls->motion_advance(state.actor,i,lane.motion,delta,&result)||!apply_motion(state,i,result,root))return false;sampled=lane.motion;}
        else if(!lane.motion.selected||!driver.advance(i,lane.motion,delta,&sampled)||!sampled.selected||!character_program_vm_detail::motion(sampled,transitions))return false;
        lane.motion=sampled;
        if(program_clocks){lane.previous_clock=lane.clock;
            if(sampled.rewound)lane.offset+=f32(sampled.end_frame); // NOT program.saw_clip
            lane.clock=lane.offset+sampled.frame;
        }
        return (!program_clocks||clocks(lane,transitions))&&publish(journal,CharacterProgramOperationKind::Advance,i,delta)&&
               (!root||state.actor.root.updates==prior_root_updates||publish(journal,CharacterProgramOperationKind::RootMotion,i,0));
    }
    bool prefix(CharacterProgramState& state,u8 mask,u16 program,u32 pc,f32 start,f32 delta,
                  const ICharacterProgramMotion& driver,std::vector<CharacterProgramOperation>& journal,const CharacterProgramTransition* transition=nullptr,const ICharacterProgramActorControls* controls=nullptr)const{
        if(program>=asset_.programs.size())return false;
        const auto& bytes=asset_.programs[program].bytes;bool saw_clip=false;
        for(unsigned count=0;count<128;++count){
            if(pc>=bytes.size())return false;
            const auto* c=bytes.data()+pc;const auto size=character_program_detail::command_size(c[0],control_flow_);if(!size||size>bytes.size()-pc)return false;
            if(control_flow_&&c[0]==13){
                if(!directional_controls_||!controls||controls->scope().lock()!=class_scope_||!controls->motion_driver_matches(driver))return false;
                f32 requested=start<0?f32(character_program_detail::signed_word(c+6)):start;const u8 duration=transition&&transition->duration>=0?u8(transition->duration):c[5];const std::array<u8,6> values={c[1],c[1],c[1],c[2],c[3],c[4]};
                for(unsigned i=0;i<2;++i)if(mask&(1u<<i)){auto& lane=state.lanes[i];const auto prior_root_updates=state.actor.root.updates;lane.previous_clock=lane.clock;if(lane.saw_clip)lane.offset+=f32(lane.motion.end_frame);if(requested<0)requested=f32(lane.motion.frame+delta);CharacterProgramMotionResult result;
                    if(!character_program_vm_detail::time(requested)||!controls->directional_select(state.actor,i,values,lane.motion,requested,delta,duration,&result)||!apply_motion(state,i,result,root_controls_))return false;
                    lane.following=true;lane.direction_controls=values;lane.saw_clip=true;lane.clock=lane.offset+lane.motion.frame;
                    if(!clocks(lane,transitions_)||!publish(journal,CharacterProgramOperationKind::DirectionalSelect,i,requested,c,size))return false;
                    if(root_controls_&&state.actor.root.updates!=prior_root_updates&&!publish(journal,CharacterProgramOperationKind::RootMotion,i,0))return false;
                }
                if(mask&2){state.actor.pose_height.mode71d=0;if(!publish(journal,CharacterProgramOperationKind::ResetLane1,1,0))return false;}
                //C050 only treats opcode1 as its local "selected" marker.
                //A prefix containing13 but no1 advances once again at its wait.
                pc+=size;continue;
            }
            if(control_flow_&&c[0]==8){
                if(!height_controls_||!controls||!class_scope_||controls->scope().lock()!=class_scope_)return false;
                if(mask&2){CharacterPoseHeightState result;if(!controls->pose_height_command(state.actor,mask,c[3],&result)||!character_program_vm_detail::pose_height(result))return false;state.actor.pose_height=result;if(!publish(journal,CharacterProgramOperationKind::PoseHeight,1,0,c,size))return false;}
                pc+=size;continue;
            }
            if(control_flow_&&c[0]==6){const auto next=s32(pc)+2-character_program_detail::signed_word(c+2);if(next<0||std::size_t(next)>=bytes.size())return false;for(unsigned lane=0;lane<2;++lane)if(mask&(1u<<lane)){state.lanes[lane].program_branch=true;if(!publish(journal,CharacterProgramOperationKind::ProgramBranch,lane,0,c,size))return false;}pc=u32(next);continue;}
            if(c[0]==0){
                //8004C050 immediately advances a continuation prefix with NO
                // opcode1. Delta is supplied by update, not a guessed FPS.
                if(!saw_clip)for(unsigned i=0;i<2;++i)if(mask&(1u<<i))if(!advance(state,i,delta,driver,journal,transitions_,controls,directional_controls_,root_controls_))return false;
                for(unsigned i=0;i<2;++i)if(mask&(1u<<i)){auto& lane=state.lanes[i];lane.program=program;lane.wait=pc;lane.cursor=pc+4;lane.threshold=f32(character_program_detail::signed_word(c+2));}
                return true;
            }
            if(c[0]==1){
                const u8 descriptor_group=directional_controls_&&!c[1]?state.actor.default_group:c[1];
                const auto* d=character_program_detail::descriptor(asset_,descriptor_group,c[2]);if(!d)return false;
                const f32 requested=start<0?f32(character_program_detail::signed_word(c+6)):start;saw_clip=true;
                for(unsigned i=0;i<2;++i)if(mask&(1u<<i)){auto& lane=state.lanes[i];const auto prior_root_updates=state.actor.root.updates;lane.previous_clock=lane.clock;
                    if(lane.saw_clip)lane.offset+=f32(lane.motion.end_frame);
                    const unsigned at=4+i*8;std::copy_n(d->bytes.data()+at,8,lane.metadata.begin());
                    const u8 flags=d->bytes[at+2]&3;if(flags==3)lane.mirrored=!lane.mirrored;else if(flags==1)lane.mirrored=false;else if(flags==2)lane.mirrored=true;
                    CharacterProgramMotion sampled;
                    // The source can overwrite lane frame while visiting bones
                    // (notably missing public entries). Preserve the requested
                    // input; do not demand a fabricated equal returned frame.
                    const u8 mode=transition&&transition->mode>=0?u8(transition->mode):c[3];const u8 duration=transition&&transition->duration>=0?u8(transition->duration):c[4];
                    bool selected=false;
                    if(directional_controls_){CharacterProgramMotionResult result;selected=controls&&controls->motion_select(state.actor,i,*d,lane.mirrored,lane.motion,requested,delta,mode,mode?duration:0,&result)&&apply_motion(state,i,result,root_controls_);if(selected)sampled=lane.motion;}
                    else selected=transitions_?driver.select_transition(i,*d,lane.mirrored,lane.motion,requested,delta,mode,mode?duration:0,&sampled):driver.select(i,*d,lane.mirrored,lane.motion,requested,delta,&sampled);
                    if(!selected||!sampled.selected||!character_program_vm_detail::motion(sampled,transitions_))return false;
                    lane.motion=sampled;lane.descriptor_group=d->group;lane.descriptor_choice=d->choice;lane.saw_clip=true;lane.clock=lane.offset+sampled.frame;
                    if(directional_controls_){lane.following=false;lane.direction_controls={};}
                    if(!clocks(lane,transitions_)||!publish(journal,CharacterProgramOperationKind::Select,i,requested,c,size))return false;
                    if(root_controls_&&state.actor.root.updates!=prior_root_updates&&!publish(journal,CharacterProgramOperationKind::RootMotion,i,0))return false;
                    if(i==1&&selection_height_controls_){
                        state.actor.pose_height.multiplier6d8=1;CharacterPoseHeightState height;
                        if(!controls||!controls->pose_height_refresh(state.actor,&height)||!character_program_vm_detail::pose_height(height)||height.mode71d!=state.actor.pose_height.mode71d||height.multiplier6d8!=1||!publish(journal,CharacterProgramOperationKind::PoseHeightSelection,1,0))return false;
                        state.actor.pose_height=height;
                    }
                    if(i==0&&!directional_controls_)for(unsigned model=0;model<2;++model){const auto raw=d->bytes[9+model];state.actor.models[model]=character_program_detail::signed_byte(raw)<0?asset_.default_models[model]:raw;}
                    else if(i==1)std::copy_n(d->bytes.data()+17,3,state.actor.lane1_descriptor.begin());
                }
                if(mask&2){if(height_controls_)state.actor.pose_height.mode71d=0;if(!publish(journal,CharacterProgramOperationKind::ResetLane1,1,0))return false;}
            }else if(c[0]==2&&(mask&1)){
                std::copy_n(c+2,4,state.actor.control0.begin());if(c[1]==0xfe)state.actor.default_group^=1;else if(c[1]!=0xff)state.actor.default_group=c[1];
                if(!publish(journal,CharacterProgramOperationKind::Control0,0,0,c,size))return false;
            }else if(c[0]==3&&(mask&2)){
                state.actor.control1={c[1],c[2]};if(character_program_detail::signed_word(c+6)!=-1)state.actor.behaviour=(state.actor.behaviour&~0xff0u)|(u32(load_be16(c+6))&0xff0u);
                if(!publish(journal,CharacterProgramOperationKind::Control1,1,0,c,size))return false;
            }else if(c[0]==7&&(mask&1)){
                for(unsigned model=0;model<2;++model)state.actor.models[model]=character_program_detail::signed_byte(c[2+model])<0?asset_.default_models[model]:c[2+model];
                if(directional_controls_)state.actor.primary_models=state.actor.models;
                if(!publish(journal,CharacterProgramOperationKind::Models,0,0,c,size))return false;
            }else if(c[0]==11&&(mask&2)){
                if(c[1])state.actor.auxiliary|=c[2];else state.actor.auxiliary&=u8(~c[2]);
                if(!publish(journal,CharacterProgramOperationKind::Auxiliary,1,0,c,size))return false;
            }
            pc+=size;
        }
        return false;
    }
    CharacterProgramAsset asset_;CharacterProgramState state_;bool loaded_=false,ready_=false,fresh_actor_=false,transitions_=false,control_flow_=false,height_controls_=false,directional_controls_=false,selection_height_controls_=false,root_controls_=false,directional_root_controls_=false,completed_follow_tail_=false;
    std::shared_ptr<const CharacterProgramClassScope> class_scope_;
};
} // namespace gf
