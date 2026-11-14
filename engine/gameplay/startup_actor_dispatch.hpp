#pragma once
// Only post-binding startup substate dispatch (80057D8C/80057D9C).
// NOT actor construction, class pre/post callbacks or alternate-root rendering.
#include "engine/gameplay/character_program.hpp"
#include "engine/gameplay/startup_root.hpp"
namespace gf {
enum class StartupActorEffectKind {ResetWorldRequired,Trail7ERequired,Program7ERequired=Trail7ERequired,AnchorEffect2Required,AnchorEffect0Required,ScaleOutRequired,EventScriptRequired,RootScaleUpdated,FadeCurve12Required,Sound242Required};
struct StartupActorEffect {StartupActorEffectKind kind=StartupActorEffectKind::ResetWorldRequired;u8 lane=0;u8 script=255;};
struct StartupActorDispatchState {bool bound=false;u8 render_index=0,render_group=0,substate=0;enum Phase{Start,Motion,ScaleOut} phase=Start;s16 facing_yaw=0;};
class StartupActorDispatch {
public:
    // Caller must have source-qualified model/config binding. This method does
    // not perform80057B78's resource construction or callback installation.
    void bind(u8 stored_render_index){state_={};root_={};state_.bound=true;state_.render_index=stored_render_index;ready_=true;}
    // New constructor-numeric entry, NOT a source reset command. It must not
    // emit/reset curve14 or invent an actor world anchor.
    bool bind_root_state(u8 stored_render_index,const StartupRootProfile& profile,const StartupRootState& root,std::string* error){
        ready_=false;
        if(!startup_root_profile_valid(profile)||!startup_root_state_valid(root)||root.remaining!=0||root.scalar_b4<=0||root.scalar_b8<=0)return fail(error,"constructor root state/profile");
        for(auto values:{root.actor_scale,root.root_scale})for(auto v:values)if(v<=0)return fail(error,"constructor positive scales");
        state_={};state_.bound=true;state_.render_index=stored_render_index;root_profile_=profile;root_=root;ready_=true;return true;
    }
    bool set_root_profile(const StartupRootProfile& profile){
        if(!state_.bound||!startup_root_profile_valid(profile)){ready_=false;return false;}
        root_profile_=profile;root_={};return true; // explicit reset required before use
    }
    bool reset(std::vector<StartupActorEffect>* effects){
        if(effects)effects->clear();
        ready_=false;
        if(!effects||!state_.bound)return false;
        if(root_profile_.loaded&&!startup_root_reset(root_profile_,&root_))return false;
        state_.phase=StartupActorDispatchState::Start;ready_=true;effects->push_back({StartupActorEffectKind::ResetWorldRequired,0,255});return true;
    }
    //80058044 does not write actor1C: preserve an interrupted exit counter.
    // The older reset() observation seam and its journals remain unchanged.
    bool reset_source_fields(std::vector<StartupActorEffect>* effects){
        if(!root_profile_.loaded||!startup_root_state_valid(root_)){if(effects)effects->clear();ready_=false;return false;}
        const f32 remaining=root_.remaining;if(!reset(effects))return false;root_.remaining=remaining;return true;
    }
    void set_render_group(u8 group){state_.render_group=group;}
    bool request(CharacterProgramVm& vm,u8 choice,f32 start,f32 delta,const ICharacterProgramMotion& driver,
                 std::vector<CharacterProgramOperation>* operations,std::string* error){
        ready_=false;
        if(!state_.bound||(root_profile_.loaded&&!root_.initialized)){if(operations)operations->clear();return fail(error,"unbound actor/root reset required");}
        auto candidate=vm;if(!candidate.request(5,choice,3,start,delta,driver,operations,error))return false;
        vm=std::move(candidate);state_.phase=StartupActorDispatchState::Start;state_.render_group=u8(unsigned(state_.render_index)+0x41u);ready_=true;return true;
    }
    bool update(CharacterProgramVm& vm,f32 delta,const ICharacterProgramMotion& driver,std::vector<CharacterProgramOperation>* operations,
                std::vector<StartupActorEffect>* effects,std::string* error){
        if(operations)operations->clear();
        if(effects)effects->clear();
        const bool was_ready=ready_;ready_=false;
        if(!was_ready||!state_.bound||!vm.ready()||!operations||!effects||!std::isfinite(delta)||delta<0||delta>16)return fail(error,"binding/output/delta/not ready");
        if(state_.phase==StartupActorDispatchState::ScaleOut&&!root_profile_.loaded)return fail(error,"80057EA0 requires qualified root profile");
        auto next=state_;auto candidate=vm;std::vector<CharacterProgramOperation> motion;std::vector<StartupActorEffect> actor;
        auto root=root_;
        if(next.phase==StartupActorDispatchState::Start)next.phase=StartupActorDispatchState::Motion; // one update, no VM advancement
        else if(next.phase==StartupActorDispatchState::ScaleOut){
            StartupRootStep step;if(!startup_root_exit_step(root_profile_,root,next.substate,&step))return fail(error,"root-scale state/domain");
            root=step.state;next.substate=step.substate;if(step.hide)next.render_group=0;
            if(step.fade12)actor.push_back({StartupActorEffectKind::FadeCurve12Required,0,255});
            if(step.sound242)actor.push_back({StartupActorEffectKind::Sound242Required,0,255});
            if(step.updated)actor.push_back({StartupActorEffectKind::RootScaleUpdated,0,255});
        }
        else{
            if(!candidate.update(3,delta,driver,&motion,error))return false;
            u8 control=0;if(!candidate.consume_primary_control(&control))return fail(error,"control observation");
            if(control==1){next.phase=StartupActorDispatchState::ScaleOut;next.substate=0;actor.push_back({StartupActorEffectKind::ScaleOutRequired,0,255});}
            else if(control==2)actor.push_back({StartupActorEffectKind::Trail7ERequired,0,255});
            else if(control==3)actor.push_back({StartupActorEffectKind::AnchorEffect2Required,0,255});
            else if(control==4){next.facing_yaw=std::bit_cast<s16>(u16(0x8000));actor.push_back({StartupActorEffectKind::AnchorEffect0Required,0,255});}
        }
        //80057CF4 dispatches8005B880 AFTER the substate call. Preserve pending
        // callback identity; do not claim executing the actor event script.
        for(unsigned lane=0;lane<2;++lane){const auto& l=candidate.state().lanes[lane];if(l.motion.selected&&l.metadata[4]!=255)actor.push_back({StartupActorEffectKind::EventScriptRequired,u8(lane),l.metadata[4]});}
        vm=std::move(candidate);state_=next;root_=root;*operations=std::move(motion);*effects=std::move(actor);ready_=true;return true;
    }
    const StartupActorDispatchState& state()const{return state_;}
    bool ready()const{return ready_;}
    const StartupRootState& root_state()const{return root_;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup actor dispatch: ")+why;return false;}
    StartupActorDispatchState state_;StartupRootProfile root_profile_;StartupRootState root_;bool ready_=false;
};
} // namespace gf
