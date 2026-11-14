#pragma once
#include "engine/gameplay/character_program.hpp"
namespace gf {
enum class CharacterProgramHeightControlMode {DeclaredFields};
// Scope-bound numeric adapter. Fields/positions are explicitly declared here;
// this does not prove actor/world-frame production. A native actor owner must
// derive them from its same-generation accepted state before using this seam.
class CharacterProgramHeightControl final:public ICharacterProgramActorControls {
public:
    bool bind(const CharacterProgramVm& vm,const CharacterPoseHeightProfile& profile,const CharacterPoseHeightFields& fields,
              std::span<const f32> bone_y,CharacterProgramHeightControlMode mode,std::string* error){
        ready_=false;scope_.reset();bone_y_.clear();CharacterProgramClassFields observed;
        if(mode!=CharacterProgramHeightControlMode::DeclaredFields||!vm.actor_height_control_mode()||!vm.observe_class_fields(&observed)||!character_pose_height_profile_valid(profile)||bone_y.size()>32||fields.behaviour5e0!=observed.behaviour){if(error)*error="program height control: explicit VM/profile/field binding";return false;}
        profile_=profile;fields_=fields;bone_y_.assign(bone_y.begin(),bone_y.end());scope_=observed.scope;ready_=true;if(error)error->clear();return true;
    }
    std::weak_ptr<const CharacterProgramClassScope> scope()const override{return ready_?scope_:std::weak_ptr<const CharacterProgramClassScope>{};}
    bool pose_height_command(const CharacterProgramActor& actor,u8 mask,u8 operand,CharacterPoseHeightState* out)const override{
        if(!ready_||scope_.expired()){if(out)*out={};return false;}
        auto fields=fields_;fields.behaviour5e0=actor.behaviour;
        return character_pose_height_command(profile_,fields,actor.pose_height,bone_y_,mask,operand,out,nullptr);
    }
    bool pose_height_refresh(const CharacterProgramActor& actor,CharacterPoseHeightState* out)const override{
        if(!ready_||scope_.expired()){if(out)*out={};return false;}
        auto fields=fields_;fields.behaviour5e0=actor.behaviour;
        return character_pose_height_evaluate(profile_,fields,actor.pose_height,bone_y_,out,nullptr);
    }
    bool pose_height_command_at_root(const CharacterProgramActor& actor,u8 mask,u8 operand,CharacterPoseHeightState* out)const{
        if(!ready_||scope_.expired()||!character_motion_root_state_valid(actor.root)){if(out)*out={};return false;}
        auto fields=fields_;fields.behaviour5e0=actor.behaviour;fields.actor_y=actor.root.position[1];
        return character_pose_height_command(profile_,fields,actor.pose_height,bone_y_,mask,operand,out,nullptr);
    }
    bool pose_height_refresh_at_root(const CharacterProgramActor& actor,CharacterPoseHeightState* out)const{
        if(!ready_||scope_.expired()||!character_motion_root_state_valid(actor.root)){if(out)*out={};return false;}
        auto fields=fields_;fields.behaviour5e0=actor.behaviour;fields.actor_y=actor.root.position[1];
        return character_pose_height_evaluate(profile_,fields,actor.pose_height,bone_y_,out,nullptr);
    }
private:
    CharacterPoseHeightProfile profile_;CharacterPoseHeightFields fields_;std::vector<f32> bone_y_;
    std::weak_ptr<const CharacterProgramClassScope> scope_;bool ready_=false;
};
} // namespace gf
