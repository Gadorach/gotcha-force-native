#pragma once
#include "engine/gameplay/character_motion_driver.hpp"
namespace gf {
struct CharacterDirectionalRootCallback {
    u8 motion_buffer=0,pose_buffer=0,flags=0;
    CharacterMotionRootObservation before,after;
    CharacterMotionRootState state;
    u8 apply_position1d9b=0;
};
struct CharacterDirectionalRootResult {
    CharacterMotionRootState root;
    std::array<CharacterDirectionalRootCallback,2> callbacks;
    std::array<f32,3> displayed_translation{};
    std::array<u8,3> lane1_descriptor{};
};
struct CharacterDirectionalRootInput {
    CharacterMotionRootState root;
    std::array<f32,3> primary_translation{};
    std::array<u8,3> lane1_descriptor{};
    bool requested=false,was_blending=false;
};
//Shared candidate evaluator. Native snapshots are verified by driver library;
//the calling owner must supply its actual prior displayed pose/operation/fields.
//This numeric seam alone is not an actor/VM operation-provenance acknowledgement.
inline bool character_directional_motion_root_evaluate(const CharacterMotionDriver& driver,const CharacterDirectionalMotionSnapshot& next,
                                                       CharacterDirectionalRootInput input,const CharacterMotionRootFields& fields,
                                                       CharacterMotionRootObservationMode mode,CharacterDirectionalRootResult* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("directional motion root: ")+why;return false;};
    if(!out)return fail("output required");
    const bool scratch=next.blending();const bool completed=!input.requested&&input.was_blending&&!scratch;
    CharacterDirectionalRootResult result;result.root=input.root;result.lane1_descriptor=input.lane1_descriptor;auto primary=input.primary_translation;
    for(unsigned i=0;i<2;++i){
        const unsigned motion=scratch?1-i:i;const unsigned pose=scratch?0:motion;
        const auto& raw=motion?next.side():next.primary();const auto* s=driver.snapshot(raw);CharacterMotionRootObservation observed;
        if(!s||s->lane()!=next.lane()||!driver.root_observation(raw,&observed,mode))return fail("native callback root/public/mirror domain");
        const bool selection=input.requested||completed||(motion==1&&next.side_rebound());
        const auto& descriptor=motion?next.side_descriptor():next.primary_descriptor();
        if(next.lane()==1&&selection)std::copy_n(descriptor.bytes.data()+17,3,result.lane1_descriptor.begin());
        auto current_fields=fields;if(next.lane()==1)current_fields.apply_position1d9b=result.lane1_descriptor[2];
        auto& callback=result.callbacks[i];callback.motion_buffer=u8(motion);callback.pose_buffer=u8(pose);callback.flags=selection?1:scratch?0:2;
        callback.before=observed;callback.before.translation=primary;
        //AAE0 rereads12D4 after ACDC. Alternate1 leaves that value unchanged.
        if(!pose)primary=observed.translation;
        callback.after=observed;callback.after.translation=primary;callback.apply_position1d9b=current_fields.apply_position1d9b;
        const auto evaluation=callback.flags==2&&!s->blending()?CharacterMotionEvaluation::Advance:CharacterMotionEvaluation::Request;
        if(!character_motion_root_step(callback.before,callback.after,evaluation,current_fields,result.root,&callback.state,error))return false;
        result.root=callback.state;
    }
    //Composition feeds the NEXT primary input, never a second displacement.
    std::copy_n(next.local_pose().bones[0].translation,3,result.displayed_translation.begin());
    for(auto x:result.displayed_translation)if(!character_motion_root_number(x))return fail("composed displayed root domain");
    *out=result;if(error)error->clear();return true;
}
// Explicit primary-output directional component, NOT a VM/actor admission.
// Source AAE0 always reads primary T0 at12D4, even when ACDC writes pose buffer1.
// Perform both root callbacks in source order; final displayed-pose differences
// are not actor displacement. Copies represent speculative SAME logical owners.
class CharacterDirectionalMotionRootOwner {
public:
    CharacterDirectionalMotionRootOwner()=default;
    CharacterDirectionalMotionRootOwner(const CharacterDirectionalMotionRootOwner&)=default;
    CharacterDirectionalMotionRootOwner& operator=(const CharacterDirectionalMotionRootOwner&)=default;
    CharacterDirectionalMotionRootOwner(CharacterDirectionalMotionRootOwner&& other)noexcept{*this=std::move(other);}
    CharacterDirectionalMotionRootOwner& operator=(CharacterDirectionalMotionRootOwner&& other)noexcept{
        if(this!=&other){driver_=std::move(other.driver_);ordinary_=std::move(other.ordinary_);directional_=std::move(other.directional_);root_=other.root_;primary_=other.primary_;globals_=other.globals_;mode_=other.mode_;lane_=other.lane_;ready_=other.ready_;other.ready_=false;}return *this;
    }
    bool bind(const CharacterMotionDriver& driver,const CharacterProgramMotion& initial,
              std::array<f32,3> position,std::array<u8,3> lane1_descriptor,std::string* error,
              CharacterMotionRootObservationMode mode=CharacterMotionRootObservationMode::AObjOnly){
        ready_=false;directional_.reset();ordinary_={};root_={};CharacterMotionRootObservation observed;const auto* s=driver.snapshot(initial);
        if(!driver.directional_transitions_mode()||!s||s->evaluation()==CharacterMotionEvaluation::Directional||!s->local_pose()||!driver.root_observation(initial,&observed,mode))return fail(error,"ordinary same-library root/displayed pose required");
        CharacterMotionRootState state;state.position=position;if(!character_motion_root_state_valid(state))return fail(error,"initial position domain");
        std::array<f32,3> displayed;std::copy_n(s->local_pose()->bones[0].translation,3,displayed.begin());
        for(auto x:displayed)if(!character_motion_root_number(x))return fail(error,"displayed root domain");
        driver_=driver;ordinary_=initial;root_=state;primary_=displayed;globals_=lane1_descriptor;mode_=mode;lane_=s->lane();ready_=true;if(error)error->clear();return true;
    }
    bool request(const std::array<u8,6>& controls,s16 steering,u8 default_group,f32 start,f32 delta,u8 duration,
                 std::array<u8,2> primary_models,std::array<u8,2> displayed_models,const CharacterMotionRootFields& fields,
                 CharacterDirectionalRootResult* out,std::string* error){
        if(out)*out={};
        if(!out||!ready_)return refuse(error,"bound root owner required");
        std::shared_ptr<const CharacterDirectionalMotionSnapshot> next;bool ok=false;
        if(directional_){std::shared_ptr<const CharacterDirectionalMotionSnapshot> rebased;
            //This component owns the alternate model bytes; a VM rebase of those
            //bytes needs its separate actor candidate join, not an invented value.
            if(!driver_.rebase_directional_models(directional_,primary_models,directional_->alternate_models(),displayed_models,&rebased))return refuse(error,"model rebase");
            ok=driver_.interrupt_directional_transition(rebased,controls,steering,default_group,start,delta,duration,displayed_models,&next);
        }else ok=driver_.request_directional_transition(lane_,controls,steering,default_group,start,delta,duration,ordinary_,primary_models,displayed_models,&next);
        if(!ok)return refuse(error,"native directional request");
        return accept(std::move(next),true,fields,out,error);
    }
    bool advance(const std::array<u8,6>& controls,s16 steering,u8 default_group,f32 delta,
                 const CharacterMotionRootFields& fields,CharacterDirectionalRootResult* out,std::string* error){
        if(out)*out={};
        if(!out||!ready_||!directional_)return refuse(error,"directional request required");
        std::shared_ptr<const CharacterDirectionalMotionSnapshot> next;
        if(!driver_.advance_directional_transition(directional_,controls,steering,default_group,delta,directional_->models(),&next))return refuse(error,"native directional advance");
        return accept(std::move(next),false,fields,out,error);
    }
    bool ready()const{return ready_;}
    const CharacterMotionRootState* state()const{return ready_?&root_:nullptr;}
    const CharacterDirectionalMotionSnapshot* snapshot()const{return ready_?directional_.get():nullptr;}
private:
    bool accept(std::shared_ptr<const CharacterDirectionalMotionSnapshot> next,bool requested,const CharacterMotionRootFields& input,
                 CharacterDirectionalRootResult* out,std::string* error){
        if(!next||next->lane()!=lane_)return refuse(error,"native lane provenance");
        CharacterDirectionalRootResult result;const CharacterDirectionalRootInput before{root_,primary_,globals_,requested,directional_&&directional_->blending()};
        if(!character_directional_motion_root_evaluate(driver_,*next,before,input,mode_,&result,error))return refuse(error,"ordered root callback domain");
        root_=result.root;primary_=result.displayed_translation;globals_=result.lane1_descriptor;directional_=std::move(next);ordinary_={};*out=result;if(error)error->clear();return true;
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("directional motion root: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    CharacterMotionDriver driver_;CharacterProgramMotion ordinary_;std::shared_ptr<const CharacterDirectionalMotionSnapshot> directional_;
    CharacterMotionRootState root_;std::array<f32,3> primary_{};std::array<u8,3> globals_{};
    CharacterMotionRootObservationMode mode_=CharacterMotionRootObservationMode::AObjOnly;unsigned lane_=2;bool ready_=false;
};
} //namespace gf
