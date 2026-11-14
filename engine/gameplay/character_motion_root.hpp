#pragma once
#include "engine/gameplay/character_motion_driver.hpp"
namespace gf {
// Ordinary PRIMARY-root branch of source4AAE0, with actual same-library native
// operation provenance. Copies are speculative SAME owners, not independent actors.
// Placement/scalars are explicit; no collision, ground, directional shared-buffer
// root or original actor-lifetime/VM acknowledgement is implied.
class CharacterMotionRootOwner {
public:
    CharacterMotionRootOwner()=default;
    CharacterMotionRootOwner(const CharacterMotionRootOwner&)=default;
    CharacterMotionRootOwner& operator=(const CharacterMotionRootOwner&)=default;
    CharacterMotionRootOwner(CharacterMotionRootOwner&& other)noexcept:driver_(std::move(other.driver_)),current_(std::move(other.current_)),state_(other.state_),ready_(other.ready_),mode_(other.mode_){other.ready_=false;}
    CharacterMotionRootOwner& operator=(CharacterMotionRootOwner&& other)noexcept{if(this!=&other){driver_=std::move(other.driver_);current_=std::move(other.current_);state_=other.state_;ready_=other.ready_;mode_=other.mode_;other.ready_=false;}return *this;}
    bool bind(const CharacterMotionDriver& driver,const CharacterProgramMotion& initial,std::array<f32,3> position,std::string* error,CharacterMotionRootObservationMode mode=CharacterMotionRootObservationMode::AObjOnly){
        ready_=false;current_={};state_={};CharacterMotionRootObservation root;const auto* s=driver.snapshot(initial);
        if(!s||s->evaluation()==CharacterMotionEvaluation::Directional||!driver.root_observation(initial,&root,mode))return fail(error,"ordinary native root observation required");
        for(auto value:position)if(!number(value))return fail(error,"initial position domain");
        driver_=driver;current_=initial;state_.position=position;mode_=mode;ready_=true;if(error)error->clear();return true;
    }
    bool accept(const CharacterProgramMotion& next,const CharacterMotionRootFields& fields,CharacterMotionRootState* out,std::string* error){
        if(out)*out={};
        const auto* snapshot=driver_.snapshot(next);CharacterMotionRootObservation before,after;
        if(!out||!ready_||!snapshot||!driver_.successor(current_,next)||snapshot->evaluation()==CharacterMotionEvaluation::Directional||
           !driver_.root_observation(current_,&before,mode_)||!driver_.root_observation(next,&after,mode_)||state_.updates==~u64(0))return refuse(error,"successive ordinary same-library native root evaluation");
        CharacterMotionRootState candidate;
        if(!character_motion_root_step(before,after,snapshot->evaluation(),fields,state_,&candidate,error)){ready_=false;return false;}
        state_=candidate;current_=next;*out=candidate;if(error)error->clear();return true;
    }
    const CharacterMotionRootState* state()const{return ready_?&state_:nullptr;}
    bool ready()const{return ready_;}
private:
    static bool number(f32 value){return std::isfinite(value)&&std::abs(value)<=1e7f;}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("character motion root: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    CharacterMotionDriver driver_;CharacterProgramMotion current_;CharacterMotionRootState state_;bool ready_=false;
    CharacterMotionRootObservationMode mode_=CharacterMotionRootObservationMode::AObjOnly;
};
} // namespace gf
