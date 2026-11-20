#pragma once
#include "engine/gameplay/borgs/normal_gred_neutral_b.hpp"
#include "engine/gameplay/character_program_directional_control.hpp"

namespace gf {
struct GRedNeutralBLocalAssets {
    ByteView model,prefix,rig,motion_bank,program,height;
};
// Owns the checked local animation library only. Loading and binding it do
// not start a combat actor or authorize an ordinary-B action in a scene.
class GRedNeutralBLocalLibrary {
public:
    bool load(const GRedNeutralBLocalAssets& in,std::string* error){
        *this={};
        auto exact=[](ByteView bytes,std::size_t size,u32 hash){
            return bytes.data&&bytes.size==size&&character_program_model_hash(bytes)==hash;
        };
        constexpr u32 model_hash=0xe2b4439au;
        if(!exact(in.model,193095,model_hash)||!exact(in.prefix,256,0x5e71a4a7u)||
           !exact(in.rig,188,0x5b1ea8feu)||!exact(in.motion_bank,355264,0x276b4e9eu)||
           !exact(in.program,436,0xf10c9e6du)||!exact(in.height,28,0x0c0b1313u)){
            if(error)*error="G-Red neutral B: exact normal local animation inputs required";
            return false;
        }
        GRedNeutralBLocalLibrary next;
        HsdModelBones bones;
        CharacterPoseRig rig;
        CharacterLocalPose initial;
        CharacterMotionBank bank;
        if(!hsd_bones_load(in.prefix,in.model,&bones,error)||!character_pose_rig_load(in.rig,bones,&rig,error)||
           !character_pose_initial_local(rig,&initial,error)||!character_motion_bank_load(in.motion_bank,&bank,error)||
           !character_program_load_control_flow(in.program,model_hash,&next.program_,error)||
           !character_pose_height_profile_load(in.height,&next.height_,error))return false;
        constexpr std::array<std::array<u8,2>,9> keys{{
            {0,0},{0,1},{0,6},{3,0},{3,1},{3,2},{3,11},{3,12},{3,13}
        }};
        std::vector<CharacterMotionClipBinding> clips;
        for(const auto& [group,index]:keys){
            CharacterMotionSelection selected;
            if(!character_motion_select(bank,group,index,&selected,error))return false;
            if(selected.fallback){if(error)*error="G-Red neutral B: requested clip unavailable";return false;}
            CharacterMotionClipBinding clip{group,index,{}};
            if(!hsd_bone_motion_load(selected.archive,bones,&clip.clip,error))return false;
            clips.push_back(std::move(clip));
        }
        if(!next.driver_.load_directional_transitions(in.model,rig,next.program_,clips,initial,{7,11},error))return false;
        *this=std::move(next);return true;
    }
    const CharacterProgramAsset* program()const{return ready()?&program_:nullptr;}
    const CharacterPoseHeightProfile* height()const{return ready()?&height_:nullptr;}
    const CharacterMotionDriver& driver()const{return driver_;}
    bool ready()const{return driver_.directional_transitions_mode();}
private:
    CharacterMotionDriver driver_;
    CharacterProgramAsset program_;
    CharacterPoseHeightProfile height_;
};

struct GRedNeutralBVisitInput {
    u8 phase540=0,local_pose6ee=0;
    bool no_active_target_cc=false;
    s16 heading72=0;
    f32 actor_step=0,aim_clock560=0;
};
class GRedNeutralBVisitPlan {
public:
    const CharacterProgramVm& vm()const{return vm_;}
    const CharacterLocalPose& pose()const{return pose_;}
    const std::vector<CharacterProgramOperation>& operations()const{return operations_;}
    const GRedNeutralBNoTargetAim& aim()const{return aim_;}
    const GRedNeutralBReleaseGate& gate()const{return gate_;}
    f32 vm_step()const{return vm_step_;}
    bool raised_cue()const{return raised_cue_;}
    // Pure alternatives at this exact successfully composed visit. Repeated
    // queries do not consume stock or reserve a class-3 cell.
    bool propose_debit(const WeaponStockModeZeroState& channel0,GRedNeutralBReleasePlan* out)const{
        return normal_gred_neutral_b_debit(source_,gate_,channel0,out);
    }
private:
    friend bool normal_gred_neutral_b_phase_one_visit(NormalGRedNeutralBSource,const GRedNeutralBLocalLibrary&,
        const CharacterProgramVm&,const CharacterProgramDirectionalControl&,const GRedNeutralBVisitInput&,
        GRedNeutralBVisitPlan*,std::string*);
    CharacterProgramVm vm_;
    CharacterLocalPose pose_;
    std::vector<CharacterProgramOperation> operations_;
    NormalGRedNeutralBSource source_;
    GRedNeutralBNoTargetAim aim_;
    GRedNeutralBReleaseGate gate_;
    f32 vm_step_=0;
    bool raised_cue_=false;
};

// Stages the original no-target phase-1 visit on a copy of the current local
// VM. An eventual battle owner must still accept the candidate and match it
// to its live pose/stock/world update; neither VM nor stock is committed here.
inline bool normal_gred_neutral_b_phase_one_visit(NormalGRedNeutralBSource source,const GRedNeutralBLocalLibrary& library,
    const CharacterProgramVm& current,const CharacterProgramDirectionalControl& controls,const GRedNeutralBVisitInput& in,
    GRedNeutralBVisitPlan* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("G-Red neutral B visit: ")+why;return false;};
    const auto* asset=current.program_asset();const auto& driver=library.driver();
    if(!out||!source.reviewed()||!current.ready()||!library.ready()||!asset||!driver.matches_program(*asset)||
       !current.actor_directional_control_mode()||current.actor_root_control_mode()||
       in.phase540!=1||in.local_pose6ee||!in.no_active_target_cc||current.state().actor.steering1dfc)
        return fail("exact local VM and ordinary no-target phase required");
    constexpr std::array<u8,4> opening{0,0,1,0},cue{1,0,1,0};
    auto valid=[&](const CharacterProgramState& state){
        if(state.actor.control0!=opening&&state.actor.control0!=cue)return false;
        for(unsigned lane=0;lane<2;++lane){
            const auto& l=state.lanes[lane];
            const auto* pose=controls.directional_snapshot(l.motion);
            if(l.program!=1||l.done||l.descriptor_group!=2||!pose||pose->lane()!=lane||pose->side_weight()!=0)return false;
            const bool released=state.actor.control0==cue;
            if(pose->primary_target().group!=2||pose->side_target().group!=2||
               pose->primary_target().index!=(released?6:0)||pose->side_target().index!=(released?7:1))return false;
        }
        return true;
    };
    if(!valid(current.state()))return fail("bound shot lanes and current command required");
    GRedNeutralBVisitPlan next;next.source_=source;next.vm_=current;
    const auto before=current.state().actor.control0;
    if(!normal_gred_neutral_b_vm_step(source,before[0],in.actor_step,&next.vm_step_))return fail("actor step");
    if(!next.vm_.update_with_actor_controls(3,next.vm_step_,driver,controls,&next.operations_,error))return false;
    const auto& staged=next.vm_.state();
    if(!valid(staged)||(before==cue&&staged.actor.control0!=cue))return fail("unqualified staged shot pose or control byte");
    if(!controls.compose_local_pose(staged,&next.pose_,error))return false;
    next.raised_cue_=before==opening&&staged.actor.control0==cue;
    if(!normal_gred_neutral_b_no_target_aim(source,in.no_active_target_cc,in.heading72,in.actor_step,&next.aim_)||
       !normal_gred_neutral_b_release_gate(source,staged.actor.control0[0],next.aim_.aim_pass,in.aim_clock560,
                                          in.actor_step,&next.gate_))return fail("aim or release gate");
    *out=std::move(next);if(error)error->clear();return true;
}
}
