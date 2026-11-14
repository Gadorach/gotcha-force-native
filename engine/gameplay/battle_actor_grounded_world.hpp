#pragma once
#include <functional>
#include "engine/assets/battle_ground_ik.hpp"
#include "engine/graphics/source_basis.hpp"
#include "engine/gameplay/battle_ground_contacts.hpp"
#include "engine/gameplay/stage_ground_normal.hpp"
namespace gf {
struct BattleTwoBoneInput {
    bool declared=false;Mat4 parent,hip,knee,foot;std::array<f32,3> root_local{},knee_local{},target{};
    s8 primary_axis=0,hint_axis=0,bend_axis=0;
};
struct BattleTwoBoneOutput {
    Mat4 hip,knee;f32 first_length=0,second_length=0,target_distance=0,rotation_distance=0;
    std::array<f32,2> cosines{},sines{};SourceBasisTrace alignment;bool evaluated=false;
};
namespace battle_ground_ik_detail {
using Vec=std::array<f32,3>;
inline Vec position(const Mat4& m){return {m.at(0,3),m.at(1,3),m.at(2,3)};}
inline Vec point(const Mat4& m,const Vec& v){Vec p;mat4_transform_point(m,v.data(),p.data());return p;}
inline void position(Mat4* m,const Vec& p){for(unsigned k=0;k<3;++k)m->at(k,3)=p[k];}
inline bool rotation(u8 axis,f32 sine,f32 cosine,Mat4* out){
    if(!std::isfinite(sine)||!std::isfinite(cosine))return false;
    Mat4 m;switch(axis){case 120:m.at(1,1)=m.at(2,2)=cosine;m.at(1,2)=-sine;m.at(2,1)=sine;break;case 121:m.at(0,0)=m.at(2,2)=cosine;m.at(0,2)=sine;m.at(2,0)=-sine;break;case 122:m.at(0,0)=m.at(1,1)=cosine;m.at(0,1)=-sine;m.at(1,0)=sine;break;default:return false;}*out=m;return true;
}
inline bool local_matrix(const CharacterLocalPose& local,const CharacterPoseFrame& frame,unsigned bone,const Vec* override_translation,Mat4* out){
    Mat4 q;f32 raw[12];if(!gf_transform_quaternion(local.bones[bone].quaternion,raw))return false;std::copy_n(raw,12,q.m);for(unsigned k=0;k<3;++k)q.at(k,3)=override_translation?(*override_translation)[k]:local.bones[bone].translation[k];
    auto m=mat4_multiply(q,character_pose_detail::angles(frame.extra_angles[bone]));m=mat4_multiply(m,mat4_scale(frame.scales[bone].data()));if(!character_pose_detail::matrix(m))return false;*out=m;return true;
}
}
//47368: old adjacent-bone positions supply lengths; the NEW parent transforms
//local root/knee translations. Only the two chain matrices are written. Host
//bounded sqrt/f32/FMA model, not frsqrte/paired-single/FPSCR hardware parity.
inline bool battle_ground_two_bone(const BattleGroundWorldProfile& p,const BattleGroundIkProfile& ik,const BattleTwoBoneInput& input,BattleTwoBoneOutput* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle two-bone: ")+why;return false;};
    using namespace battle_ground_ik_detail;using source_intersection_detail::sub;
    if(!out||!battle_ground_world_profile_valid(p)||!battle_ground_ik_profile_valid(ik)||!input.declared)return fail("declared source profile/input required");
    for(const auto* m:{&input.parent,&input.hip,&input.knee,&input.foot})if(!character_pose_detail::matrix(*m))return fail("source matrices domain");
    for(const auto* v:{&input.root_local,&input.knee_local,&input.target})if(!source_intersection_detail::valid(*v))return fail("local/target domain");
    const int bend=input.bend_axis;if(bend==0||bend< -3||bend>3)return fail("source signed bend-axis table extent");
    BattleTwoBoneOutput next;next.first_length=source_basis_detail::length(sub(position(input.hip),position(input.knee)));next.second_length=source_basis_detail::length(sub(position(input.knee),position(input.foot)));
    auto hip=input.hip;const Vec origin=point(input.parent,input.root_local);if(!source_intersection_detail::valid(origin))return fail("new root position domain");position(&hip,origin);const Vec direction=sub(input.target,origin);
    if(!source_basis_align(hip,direction,input.primary_axis,input.hint_axis,ik.axis_pairs,bits_to_f32(ik.cross_threshold_bits),&hip,&next.alignment))return fail("source axis-basis domain");
    next.target_distance=source_basis_detail::length(direction);const f32 cap=f32(p.value(9)*f32(next.first_length+next.second_length));next.rotation_distance=std::min(next.target_distance,cap);
    const f32 a=next.first_length,b=next.second_length,d=next.rotation_distance;if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(d)||a<=0||b<=0||d<=0)return fail("source cosine division degeneracy, not a fabricated pose");
    const f32 aa=f32(a*a),bb=f32(b*b),dd=f32(d*d),den0=f32(f32(p.value(5)*d)*a),den1=f32(f32(p.value(5)*a)*b);if(!std::isfinite(den0)||!std::isfinite(den1)||den0==0||den1==0)return fail("cosine denominator domain");
    next.cosines[0]=std::clamp(f32(f32(f32(dd+aa)-bb)/den0),p.value(11),p.value(10));next.cosines[1]=std::clamp(f32(-f32(f32(aa+bb)-dd)/den1),p.value(11),p.value(10));
    for(unsigned k=0;k<2;++k){const f32 squared=std::fma(-next.cosines[k],next.cosines[k],p.value(10));next.sines[k]=squared>0?f32(std::sqrt(f64(squared))):squared;}
    if(bend<0)next.sines[1]=-next.sines[1];else next.sines[0]=-next.sines[0];
    const u8 axis=ik.angle_axes[unsigned(std::abs(bend)-1)];Mat4 first_rotation,second_rotation;if(!rotation(axis,next.sines[0],next.cosines[0],&first_rotation)||!rotation(axis,next.sines[1],next.cosines[1],&second_rotation))return fail("sine/cosine rotation domain");
    next.hip=mat4_multiply(hip,first_rotation);position(&second_rotation,input.knee_local);next.knee=mat4_multiply(next.hip,second_rotation);if(!character_pose_detail::matrix(next.hip)||!character_pose_detail::matrix(next.knee))return fail("computed chain domain");
    next.evaluated=true;*out=next;if(error)error->clear();return true;
}
enum class BattleGroundedWorldOperation:u8 {Ordinary=0,TwoBone=1,Constraint=2};
struct BattleGroundedWorldStep {BattleGroundedWorldOperation operation=BattleGroundedWorldOperation::Ordinary;u8 bone=0,parent=0,record=255;};
struct BattleGroundedConstraintTrace {u8 bone=0;StageGroundNormal surface;SourceBasisTrace alignment;};
struct BattleActorGroundedWorld {
    BattleActorFirstWorld first;BattleGroundContactPlan contacts;CharacterWorldPose world;Mat4 collider1aa4;
    std::vector<BattleGroundedWorldStep> steps;std::vector<BattleTwoBoneOutput> chains;std::vector<BattleGroundedConstraintTrace> constraints;
    bool evaluated=false,grounded_second_pass_pending=true,look_tail_pending=true,actor_frame_owner_pending=true;
};
//465FC through the second46BD0/46914 only. One pure declared-input computation
//runs the first pass, contact generation and source next-before-child traversal.
//No caller-supplied first/contact plan can masquerade as current actor authority.
//47904/47A2C, source history, resource-owner binding and live publication remain
//pending. This never publishes a model pose, renderer draw or collider receipt.
inline bool battle_actor_grounded_world(const BattleGroundWorldProfile& p,const BattleGroundIkProfile& ik,const StartupActorSetupProfile& setup,const CharacterPoseRig& rig,const CharacterLocalPose& local,const CharacterPoseFrame& modifiers,const BattleActorWorldInput& actor,const BattleGroundContactInput& ground,const ChallengeStageInputs& stage,BattleActorGroundedWorld* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle grounded world: ")+why;return false;};
    if(!out||!battle_ground_ik_profile_valid(ik))return fail("profile/destination");
    BattleActorGroundedWorld next;if(!battle_actor_first_world(p,rig,local,modifiers,actor,&next.first,error)||!battle_ground_contact_plan(p,setup,rig,next.first,ground,stage,&next.contacts,error))return false;
    using namespace battle_ground_ik_detail;next.world=next.first.world;next.world.matrices[0]=next.first.actor114;next.collider1aa4=next.first.collider1aa4;const auto n=local.bones.size();std::array<bool,32> seen{};seen[0]=true;unsigned visited=1;
    auto mark=[&](unsigned bone){if(bone>=n||seen[bone]||++visited>n)return false;seen[bone]=true;return true;};
    std::function<bool(u8,u8)> walk=[&](u8 current,u8 parent){
        while(current!=kHsdNoBone){
            if(!mark(current)||parent>=n)return fail("source traversal extent/revisit");
            unsigned record=0;for(;record<next.contacts.contacts.size();++record)if(int(next.contacts.contacts[record].source_bone_axes[0])-2==current)break;
            const auto& bone=rig.binding.bones[current];
            if(record<next.contacts.contacts.size()){
                const unsigned mid=unsigned(current)+1,foot=unsigned(current)+2;if(foot>=n||bone.child!=mid||rig.binding.bones[mid].child!=foot||rig.binding.bones[mid].parent!=current||rig.binding.bones[foot].parent!=mid)return fail("source adjacent two-bone topology unsupported");
                const auto& contact=next.contacts.contacts[record];BattleTwoBoneInput input;input.declared=true;input.parent=next.world.matrices[parent];input.hip=next.world.matrices[current];input.knee=next.world.matrices[mid];input.foot=next.world.matrices[foot];std::copy_n(local.bones[current].translation,3,input.root_local.begin());std::copy_n(local.bones[mid].translation,3,input.knee_local.begin());input.target=contact.point;input.primary_axis=std::bit_cast<s8>(contact.source_bone_axes[1]);input.hint_axis=std::bit_cast<s8>(contact.source_bone_axes[2]);input.bend_axis=std::bit_cast<s8>(contact.source_bone_axes[3]);BattleTwoBoneOutput chain;if(!battle_ground_two_bone(p,ik,input,&chain,error))return false;
                next.world.matrices[current]=chain.hip;next.world.matrices[mid]=chain.knee;next.chains.push_back(chain);next.steps.push_back({BattleGroundedWorldOperation::TwoBone,current,parent,u8(record)});
                if(bone.next!=kHsdNoBone&&!walk(bone.next,parent))return false;
                if(!mark(mid))return fail("source IK-middle revisit");
                if(rig.binding.bones[mid].next!=kHsdNoBone&&!walk(rig.binding.bones[mid].next,current))return false;
                parent=u8(mid);current=u8(foot);continue;
            }
            record=0;for(;record<next.contacts.constraints.size();++record)if(next.contacts.constraints[record].bone==current)break;
            if(record<next.contacts.constraints.size()){
                auto m=next.world.matrices[current];const Vec origin=point(next.world.matrices[parent],{local.bones[current].translation[0],local.bones[current].translation[1],local.bones[current].translation[2]});position(&m,origin);StartupActorInitialSetup dimensions;if(!startup_actor_dimensions(setup,ground.scalar_b4,&dimensions))return fail("constraint dimensions");
                BattleGroundedConstraintTrace trace;trace.bone=current;if(!stage_ground_surface_normal({stage.grid(0),stage.grid(1),stage.grid(2)},stage.parameters(),ground.ground,origin,dimensions.field670,&trace.surface,error))return false;const auto& desc=next.contacts.constraints[record];if(!source_basis_align(m,trace.surface.normal,std::bit_cast<s8>(desc.axis4),std::bit_cast<s8>(desc.axis5),ik.axis_pairs,bits_to_f32(ik.cross_threshold_bits),&m,&trace.alignment))return fail("constraint source basis domain");next.world.matrices[current]=m;next.constraints.push_back(trace);next.steps.push_back({BattleGroundedWorldOperation::Constraint,current,parent,u8(record)});
            }else{
                auto base=next.world.matrices[parent];if(current==1)base=mat4_multiply(base,character_pose_detail::angles(modifiers.bone1_look_angles,true));Mat4 local_matrix_value;const Vec* adjustment=current==1?&next.contacts.adjust1d54:nullptr;if(!local_matrix(local,modifiers,current,adjustment,&local_matrix_value))return fail("ordinary local matrix domain");const auto m=mat4_multiply(base,local_matrix_value);if(!character_pose_detail::matrix(m))return fail("ordinary second world domain");next.world.matrices[current]=m;next.steps.push_back({BattleGroundedWorldOperation::Ordinary,current,parent,255});
            }
            if(bone.next!=kHsdNoBone&&!walk(bone.next,parent))return false;
            parent=current;current=bone.child;
        }return true;
    };
    if(!walk(rig.binding.bones[0].child,0))return false;
    if(visited!=n)return fail("incomplete second traversal");
    next.evaluated=true;next.grounded_second_pass_pending=false;*out=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
