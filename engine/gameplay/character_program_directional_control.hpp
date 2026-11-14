#pragma once
#include "engine/gameplay/character_motion_driver.hpp"
#include "engine/gameplay/character_directional_motion_root.hpp"
#include "engine/gameplay/character_program_height_control.hpp"
namespace gf {
enum class CharacterProgramDirectionalControlMode {DeclaredHeightFields};
// Pure candidate-only VM/driver join. Scope/library identity are qualified here;
// height fields and root/event/whole-actor producers remain explicit boundaries.
class CharacterProgramDirectionalControl final:public ICharacterProgramActorControls {
    struct Context final:CharacterProgramMotionContext {
        CharacterProgramMotion primary,alternate,observed;CharacterProgramDescriptor descriptor;
        std::shared_ptr<const CharacterDirectionalMotionSnapshot> directional;
        std::weak_ptr<const CharacterProgramClassScope> scope;unsigned lane=0;
        std::array<u8,6> controls{};
        CharacterDirectionalRootResult directional_root;bool directional_root_checked=false;
    };
public:
    bool bind(const CharacterProgramVm& vm,const CharacterMotionDriver& driver,const CharacterPoseHeightProfile& height,
              const CharacterPoseHeightFields& fields,std::span<const f32> bone_y,CharacterProgramDirectionalControlMode mode,std::string* error){
        ready_=root_enabled_=directional_root_enabled_=false;root_mode_=CharacterMotionRootObservationMode::AObjOnly;scope_.reset();multipliers_.clear();CharacterProgramClassFields observed;const auto* asset=vm.program_asset();const auto* rig=driver.rig();
        if(mode!=CharacterProgramDirectionalControlMode::DeclaredHeightFields||!vm.actor_directional_control_mode()||!asset||!vm.observe_class_fields(&observed)||!driver.directional_transitions_mode()||!driver.matches_program(*asset)||!rig||!height_.bind(vm,height,fields,bone_y,CharacterProgramHeightControlMode::DeclaredFields,error))return fail(error,"VM/profile/library/height scope");
        // Shared secondary-lane model buffers are deliberately not fabricated.
        for(auto bone:driver.model_bones())if(bone>=0){bool primary=false;for(unsigned i=rig->cuts[0];i<rig->cuts[1];++i)primary|=rig->order[i]==u8(bone);if(!primary)return fail(error,"secondary model-buffer owner required");}
        driver_=driver;defaults_=asset->default_models;scope_=observed.scope;ready_=true;if(error)error->clear();return true;
    }
    bool bind_with_root(const CharacterProgramVm& vm,const CharacterMotionDriver& driver,const CharacterPoseHeightProfile& height,
                        const CharacterPoseHeightFields& fields,std::span<const f32> bone_y,const CharacterMotionRootFields& root,std::string* error,CharacterMotionRootObservationMode mode=CharacterMotionRootObservationMode::AObjOnly){
        if(!bind(vm,driver,height,fields,bone_y,CharacterProgramDirectionalControlMode::DeclaredHeightFields,error))return false;
        const auto* rig=driver.rig();root_lane_=2;
        for(unsigned lane=0;lane<2;++lane)for(unsigned i=rig->cuts[lane];i<rig->cuts[lane+1];++i)if(rig->order[i]==0)root_lane_=lane;
        if(!vm.actor_root_control_mode()||root_lane_>=2||rig->mirror[0]!=0||root.scale_b4!=fields.scale_b4||fields.actor_y!=vm.state().actor.root.position[1]||root.multipliers1dd4.size()>128||(mode!=CharacterMotionRootObservationMode::AObjOnly&&mode!=CharacterMotionRootObservationMode::AObjOrPublicNull)){ready_=false;scope_.reset();return fail(error,"explicit VM root/height/scalar binding");}
        root_fields_=root;multipliers_.assign(root.multipliers1dd4.begin(),root.multipliers1dd4.end());root_fields_.multipliers1dd4={};root_mode_=mode;root_enabled_=true;return true;
    }
    bool bind_with_directional_root(const CharacterProgramVm& vm,const CharacterMotionDriver& driver,const CharacterPoseHeightProfile& height,
                                     const CharacterPoseHeightFields& fields,std::span<const f32> bone_y,const CharacterMotionRootFields& root,std::string* error,
                                     CharacterMotionRootObservationMode mode=CharacterMotionRootObservationMode::AObjOnly){
        if(!bind_with_root(vm,driver,height,fields,bone_y,root,error,mode))return false;
        if(!vm.actor_directional_root_control_mode()){ready_=root_enabled_=false;scope_.reset();return fail(error,"explicit ordered directional-root VM mode required");}
        directional_root_enabled_=true;return true;
    }
    std::weak_ptr<const CharacterProgramClassScope> scope()const override{return ready_?scope_:std::weak_ptr<const CharacterProgramClassScope>{};}
    bool motion_driver_matches(const ICharacterProgramMotion& driver)const override{const auto* native=dynamic_cast<const CharacterMotionDriver*>(&driver);return active()&&native&&driver_.shares_library(*native);}
    bool ordinary_root_controls()const override{return active()&&root_enabled_;}
    bool directional_root_controls()const override{return active()&&root_enabled_&&directional_root_enabled_;}
    bool pose_height_command(const CharacterProgramActor& actor,u8 mask,u8 operand,CharacterPoseHeightState* out)const override{return active()&&(root_enabled_?height_.pose_height_command_at_root(actor,mask,operand,out):height_.pose_height_command(actor,mask,operand,out));}
    bool pose_height_refresh(const CharacterProgramActor& actor,CharacterPoseHeightState* out)const override{return active()&&(root_enabled_?height_.pose_height_refresh_at_root(actor,out):height_.pose_height_refresh(actor,out));}
    bool motion_select(const CharacterProgramActor& actor,unsigned lane,const CharacterProgramDescriptor& d,bool mirrored,
                       const CharacterProgramMotion& previous,f32 start,f32 delta,u8 mode,u8 duration,CharacterProgramMotionResult* out)const override{
        if(out)*out={};
        if(!out||!active()||lane>=2)return false;
        const auto* old=previous.selected?context(previous):nullptr;
        if(previous.selected&&(!old||old->lane!=lane))return false;
        if(root_enabled_&&lane==root_lane_&&old&&old->directional&&!directional_root_enabled_)return false;
        CharacterProgramMotion raw;
        const bool selected=old&&old->directional?driver_.select_from_directional(lane,d,mirrored,old->directional,start,delta,mode,duration,&raw):driver_.select_transition(lane,d,mirrored,old?old->primary:previous,start,delta,mode,duration,&raw);
        if(!selected)return false;
        auto result=initial(actor);if(!lane){if(mode&&duration)result.alternate_models=actor.primary_models;result.primary_models=descriptor_models(d);result.models=result.primary_models;}
        const auto* snap=driver_.snapshot(raw);if(!snap)return false;if(snap->blending())blend_models(lane,snap->previous_weight(),result);
        if(lane==1)std::copy_n(d.bytes.data()+17,3,result.lane1_descriptor.begin());
        //select_from_directional requests from a temporary displayed-pose seed;
        //that private weak-parent is not old->primary. Operation provenance is
        //the successful native request above, not a fabricated successor link.
        const bool from_directional=directional_root_enabled_&&old&&old->directional;
        if(!root_result(actor,lane,old?old->primary:previous,raw,lane==1?d.bytes[19]:actor.lane1_descriptor[2],result,from_directional))return false;
        auto next=std::make_shared<Context>();next->primary=raw;next->alternate=mode&&duration?(old?old->primary:previous):(old?old->alternate:CharacterProgramMotion{});next->descriptor=d;next->lane=lane;
        return finish(std::move(next),result,out);
    }
    bool directional_select(const CharacterProgramActor& actor,unsigned lane,const std::array<u8,6>& values,const CharacterProgramMotion& previous,
                            f32 start,f32 delta,u8 duration,CharacterProgramMotionResult* out)const override{
        if(out)*out={};
        if(!out||!active()||lane>=2)return false;
        if(root_enabled_&&lane==root_lane_&&!directional_root_enabled_)return false;
        const auto* old=previous.selected?context(previous):nullptr;
        if(previous.selected&&(!old||old->lane!=lane))return false;
        std::shared_ptr<const CharacterDirectionalMotionSnapshot> directional;bool selected=false;
        if(old&&old->directional){std::shared_ptr<const CharacterDirectionalMotionSnapshot> rebased;if(!driver_.rebase_directional_models(old->directional,actor.primary_models,actor.alternate_models,actor.models,&rebased))return false;selected=driver_.interrupt_directional_transition(rebased,values,actor.steering1dfc,actor.default_group,start,delta,duration,actor.models,&directional);}
        else if(!duration)selected=driver_.request_directional_follow(lane,values,actor.steering1dfc,actor.default_group,start,delta,{old?old->primary:previous,old?old->alternate:CharacterProgramMotion{}},actor.models,&directional);
        else selected=driver_.request_directional_transition(lane,values,actor.steering1dfc,actor.default_group,start,delta,duration,old?old->primary:previous,actor.primary_models,actor.models,&directional);
        if(!selected)return false;
        auto result=initial(actor);if(!lane){result.models=directional->models();result.primary_models=directional->primary_models();result.alternate_models=directional->alternate_models();}
        if(lane==1){const auto& last=directional->blending()?directional->primary_descriptor():directional->side_descriptor();std::copy_n(last.bytes.data()+17,3,result.lane1_descriptor.begin());}
        auto next=std::make_shared<Context>();next->primary=directional->primary();next->alternate=directional->side();next->descriptor=directional->primary_descriptor();next->directional=std::move(directional);next->lane=lane;next->controls=values;
        if(!directional_root_result(actor,lane,old,*next,true,result))return false;
        return finish(std::move(next),result,out);
    }
    bool motion_advance(const CharacterProgramActor& actor,unsigned lane,const CharacterProgramMotion& previous,f32 delta,CharacterProgramMotionResult* out)const override{
        if(out)*out={};
        const auto* old=context(previous);if(!out||!old||old->lane!=lane)return false;if(root_enabled_&&lane==root_lane_&&old->directional&&!directional_root_enabled_)return false;auto result=initial(actor);auto next=std::make_shared<Context>(*old);next->directional_root_checked=false;
        if(old->directional){std::shared_ptr<const CharacterDirectionalMotionSnapshot> rebased;if(!driver_.rebase_directional_models(old->directional,actor.primary_models,actor.alternate_models,actor.models,&rebased))return false;
            const auto& values=old->controls;
            if(!driver_.advance_directional_transition(rebased,values,actor.steering1dfc,actor.default_group,delta,actor.models,&next->directional))return false;
            next->primary=next->directional->primary();next->alternate=next->directional->side();next->descriptor=next->directional->primary_descriptor();
            if(!lane){result.models=next->directional->models();result.primary_models=next->directional->primary_models();result.alternate_models=next->directional->alternate_models();}
            //Initial mode6 writes SIDE then CENTRE; active mode6 writes SIDE
            //only on rebind. Completion49B98 writes CENTRE then SIDE. Unchanged
            //follow targets do not call D244 and retain candidate global bytes.
            if(lane==1&&((old->directional->blending()&&!next->directional->blending())||next->directional->side_rebound()))std::copy_n(next->directional->side_descriptor().bytes.data()+17,3,result.lane1_descriptor.begin());
        }else{if(!driver_.advance(lane,old->primary,delta,&next->primary))return false;const auto* snap=driver_.snapshot(next->primary);if(!snap)return false;if(snap->blending())blend_models(lane,snap->previous_weight(),result);}
        if(old->directional){if(!directional_root_result(actor,lane,old,*next,false,result))return false;}
        else if(!root_result(actor,lane,old->primary,next->primary,actor.lane1_descriptor[2],result))return false;
        return finish(std::move(next),result,out);
    }
    const CharacterMotionSnapshot* snapshot(const CharacterProgramMotion& motion)const{const auto* c=context(motion);return c?driver_.snapshot(c->primary):nullptr;}
    const CharacterDirectionalMotionSnapshot* directional_snapshot(const CharacterProgramMotion& motion)const{const auto* c=context(motion);return c?c->directional.get():nullptr;}
    const CharacterDirectionalRootResult* directional_root_result(const CharacterProgramMotion& motion)const{const auto* c=context(motion);return c&&directional_root_enabled_&&c->directional_root_checked?&c->directional_root:nullptr;}
    bool root_observation(const CharacterProgramMotion& motion,CharacterMotionRootObservation* out)const{if(out)*out={};const auto* c=context(motion);return c&&root_enabled_&&driver_.root_observation(c->primary,out,root_mode_);}
    bool compose_local_pose(const CharacterProgramState& state,CharacterLocalPose* out,std::string* error)const{
        if(out)*out={};
        if(!out||!active())return false;
        std::array<CharacterProgramMotion,2> raw;std::array<const Context*,2> contexts{};
        for(unsigned lane=0;lane<2;++lane){contexts[lane]=context(state.lanes[lane].motion);if(!contexts[lane]||contexts[lane]->lane!=lane)return fail(error,"lane snapshot scope");raw[lane]=contexts[lane]->primary;}
        CharacterLocalPose pose;if(!driver_.compose_local_pose(raw,&pose,error))return false;const auto& rig=*driver_.rig();
        for(unsigned lane=0;lane<2;++lane)if(contexts[lane]->directional)for(unsigned i=rig.cuts[lane];i<rig.cuts[lane+1];++i){const auto slot=rig.order[i];pose.bones[slot]=contexts[lane]->directional->local_pose().bones[slot];}
        if(!character_pose_detail::local(rig,pose))return fail(error,"composed pose domain");
        *out=std::move(pose);if(error)error->clear();return true;
    }
private:
    bool root_result(const CharacterProgramActor& actor,unsigned lane,const CharacterProgramMotion& before,const CharacterProgramMotion& after,u8 apply,CharacterProgramMotionResult& result,bool from_directional=false)const{
        if(!root_enabled_)return true;
        result.root_checked=true;result.root=actor.root;if(lane!=root_lane_)return true;
        const auto* snapshot=driver_.snapshot(after);CharacterMotionRootObservation prior,current;
        if(!snapshot||snapshot->evaluation()==CharacterMotionEvaluation::Directional||!driver_.root_observation(after,&current,root_mode_))return false;
        if(from_directional){if(!directional_root_enabled_||snapshot->evaluation()!=CharacterMotionEvaluation::Request)return false;}
        else if(before.selected){if(!driver_.successor(before,after)||!driver_.root_observation(before,&prior,root_mode_))return false;}
        else if(snapshot->evaluation()!=CharacterMotionEvaluation::Request)return false;
        auto fields=root_fields_;fields.apply_position1d9b=apply;fields.multipliers1dd4=multipliers_;
        return character_motion_root_step(prior,current,snapshot->evaluation(),fields,actor.root,&result.root,nullptr);
    }
    bool directional_root_result(const CharacterProgramActor& actor,unsigned lane,const Context* old,Context& next,bool requested,CharacterProgramMotionResult& result)const{
        if(!root_enabled_)return true;
        result.root_checked=true;result.root=actor.root;if(lane!=root_lane_)return true;
        if(!directional_root_enabled_||!next.directional)return false;
        const CharacterLocalPose* prior=driver_.initial_local_pose();
        if(old){const auto* raw=driver_.snapshot(old->primary);if(!raw)return false;prior=old->directional?&old->directional->local_pose():raw->local_pose();}
        if(!prior||prior->bones.empty())return false;
        CharacterDirectionalRootInput input;input.root=actor.root;input.lane1_descriptor=actor.lane1_descriptor;input.requested=requested;input.was_blending=old&&old->directional&&old->directional->blending();std::copy_n(prior->bones[0].translation,3,input.primary_translation.begin());
        auto fields=root_fields_;fields.apply_position1d9b=actor.lane1_descriptor[2];fields.multipliers1dd4=multipliers_;
        if(!character_directional_motion_root_evaluate(driver_,*next.directional,input,fields,root_mode_,&next.directional_root,nullptr))return false;
        //Lane1 descriptors/root fields publish as one candidate, including
        //earlier writes by the other lane in this same VM prefix/update.
        if(lane==1&&next.directional_root.lane1_descriptor!=result.lane1_descriptor)return false;
        next.directional_root_checked=true;result.root=next.directional_root.root;return true;
    }
    bool active()const{return ready_&&!scope_.expired();}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("program directional control: ")+why;return false;}
    const Context* context(const CharacterProgramMotion& motion)const{const auto* c=dynamic_cast<const Context*>(motion.context.get());return active()&&c&&c->scope.lock()==scope_.lock()&&character_motion_driver_detail::same(motion,c->observed)&&driver_.snapshot(c->primary)?c:nullptr;}
    static CharacterProgramMotionResult initial(const CharacterProgramActor& actor){CharacterProgramMotionResult r;r.models=actor.models;r.primary_models=actor.primary_models;r.alternate_models=actor.alternate_models;r.lane1_descriptor=actor.lane1_descriptor;return r;}
    std::array<u8,2> descriptor_models(const CharacterProgramDescriptor& d)const{std::array<u8,2> result{};for(unsigned i=0;i<2;++i)result[i]=character_program_detail::signed_byte(d.bytes[9+i])<0?defaults_[i]:d.bytes[9+i];return result;}
    void blend_models(unsigned lane,f32 weight,CharacterProgramMotionResult& r)const{const auto& rig=*driver_.rig();const auto bones=driver_.model_bones();for(unsigned i=rig.cuts[lane];i<rig.cuts[lane+1];++i)for(unsigned model=0;model<2;++model)if(bones[model]==s8(rig.order[i])){r.models[model]=weight>.5f?r.alternate_models[model]:r.primary_models[model];break;}}
    bool finish(std::shared_ptr<Context> next,CharacterProgramMotionResult result,CharacterProgramMotionResult* out)const{
        const auto* raw=driver_.snapshot(next->primary);if(!raw)return false;next->scope=scope_;next->observed=next->primary;next->observed.context.reset();result.motion=next->primary;result.motion.context=std::move(next);const auto* c=dynamic_cast<const Context*>(result.motion.context.get());result.descriptor_group=c->directional?c->directional->primary_target().group:c->descriptor.group;result.descriptor_choice=c->directional?c->directional->primary_target().index:c->descriptor.choice;result.mirrored=raw->mirrored();const unsigned at=4+c->lane*8;std::copy_n(c->descriptor.bytes.data()+at,8,result.metadata.begin());*out=std::move(result);return true;
    }
    CharacterMotionDriver driver_;CharacterProgramHeightControl height_;std::weak_ptr<const CharacterProgramClassScope> scope_;std::array<u8,2> defaults_{};bool ready_=false;
    CharacterMotionRootFields root_fields_;std::vector<f32> multipliers_;unsigned root_lane_=2;bool root_enabled_=false,directional_root_enabled_=false;
    CharacterMotionRootObservationMode root_mode_=CharacterMotionRootObservationMode::AObjOnly;
};
} //namespace gf
