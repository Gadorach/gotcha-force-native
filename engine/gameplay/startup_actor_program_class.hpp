#pragma once
// Source57CF4 class pre -> motion/subdispatch -> class post boundary.
// This binding does NOT run the intervening VM/actor step, constructor, world,
// effects or generic update. Caller must preserve that source order explicitly.
#include "engine/gameplay/character_program.hpp"
#include "engine/gameplay/startup_actor_class.hpp"
#include "engine/assets/startup_actor_setup.hpp"
namespace gf {
class StartupActorProgramClass {
public:
    bool bind(const CharacterProgramVm& vm,const StartupActorSetupProfile& setup,const StartupActorClassProfile& profile,
              const StartupActorClassState& state,std::string* error){
        ready_=false;scope_.reset();phase_=Pre;
        CharacterProgramClassFields fields;
        if(!startup_actor_setup_detail::valid(setup)||!startup_actor_class_bound(profile,state)||!state.callbacks_initialized||state.model_id!=setup.model_id||vm.model_hash()!=setup.model_hash||!vm.observe_class_fields(&fields)||
            state.field582!=fields.default_group||state.field6e9!=fields.effect_control||state.flags5e0!=fields.behaviour||(vm.actor_directional_control_mode()&&state.field1dfc!=fields.steering1dfc))return fail(error,"model/profile/class initial-field binding");
        profile_=profile;state_=state;model_hash_=setup.model_hash;scope_=fields.scope;ready_=true;return true;
    }
    bool pre(CharacterProgramVm& vm,std::string* error){
        if(!ready_||phase_!=Pre)return refuse(error,"pre/post order");
        CharacterProgramClassFields fields;if(!observe(vm,&fields))return refuse(error,"VM scope/not ready");
        auto next=state_;synchronize(fields,next);if(vm.actor_directional_control_mode())next.field1dfc=fields.steering1dfc;if(!startup_actor_class_pre(profile_,next,&next))return refuse(error,"class pre callback");
        auto candidate=vm;if(!candidate.write_class_fields(fields,next.field582,next.field6e9,error)){ready_=false;return false;}
        if(vm.actor_directional_control_mode()){CharacterProgramClassFields current;if(!candidate.observe_class_fields(&current)||!candidate.write_class_steering(current,next.field1dfc,error)){ready_=false;return false;}}
        vm=std::move(candidate);state_=next;phase_=Post;return true;
    }
    // Scalar pre/post bookkeeping shares the same actor class fields. Refresh
    // only at the pre boundary and require represented VM aliases to agree.
    bool refresh(const CharacterProgramVm& vm,const StartupActorClassState& state,std::string* error){
        CharacterProgramClassFields fields;
        if(!ready_||phase_!=Pre||state.model_id!=state_.model_id||!state.callbacks_initialized||!observe(vm,&fields)||state.field582!=fields.default_group||state.field6e9!=fields.effect_control||state.flags5e0!=fields.behaviour)return refuse(error,"scalar/class field coherence");
        state_=state;return true;
    }
    bool post(CharacterProgramVm& vm,std::vector<StartupActorClassEffect>* effects,std::string* error){
        if(effects)effects->clear();
        if(!effects||!ready_||phase_!=Post)return refuse(error,"post/pre order/output");
        CharacterProgramClassFields fields;if(!observe(vm,&fields))return refuse(error,"VM scope/not ready");
        // Motion may have changed582/6E9/5E0 since pre: read the NEW fields,
        // never restore a saved pre-update value over an opcode's writes.
        auto next=state_;synchronize(fields,next);if(vm.actor_directional_control_mode())next.field1dfc=fields.steering1dfc;std::vector<StartupActorClassEffect> requests;
        if(!startup_actor_class_post(profile_,next,&next,&requests))return refuse(error,"class post callback");
        auto candidate=vm;if(!candidate.write_class_fields(fields,next.field582,next.field6e9,error)){ready_=false;return false;}
        vm=std::move(candidate);state_=next;*effects=std::move(requests);phase_=Pre;return true;
    }
    const StartupActorClassState& state()const{return state_;}
    bool ready()const{return ready_;}
private:
    enum Phase {Pre,Post};Phase phase_=Pre;
    static void synchronize(const CharacterProgramClassFields& f,StartupActorClassState& state){state.field582=f.default_group;state.field6e9=f.effect_control;state.flags5e0=f.behaviour;}
    bool observe(const CharacterProgramVm& vm,CharacterProgramClassFields* fields)const{return vm.model_hash()==model_hash_&&vm.observe_class_fields(fields)&&!scope_.expired()&&fields->scope.lock()==scope_.lock();}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup actor program/class: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    StartupActorClassProfile profile_;StartupActorClassState state_;u32 model_hash_=0;bool ready_=false;
    std::weak_ptr<const CharacterProgramClassScope> scope_;
};
} // namespace gf
