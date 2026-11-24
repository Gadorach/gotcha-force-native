#pragma once
// Source actor pose subset, not a motion-command controller or HSD file-SRT
// animator. 8004ADC8 resets missing channels;8004B76C mirrors and converts Euler
// to quaternion;8004A8CC blends lanes.80046BD0/8004679C/80046DD4 build world
// matrices, later copied directly to HSD joints by80056AA8.
#include "engine/assets/character_motion.hpp"
#include "engine/animation/native_transform.h"
#include "engine/graphics/transform.hpp"
namespace gf {
struct CharacterPoseRig {
    HsdModelBones binding;
    u8 lane_mask=0;
    std::array<u8,5> cuts{};
    std::vector<u8> order,mirror;
    bool loaded=false;
};
struct CharacterBonePose {
    u32 joint_offset=0;
    f32 translation[3]={};
    f32 quaternion[4]={0,0,0,1}; // x,y,z,w; no gratuitous normalization on blend
};
struct CharacterLocalPose {std::vector<CharacterBonePose> bones;};
struct CharacterPoseLane {
    const HsdBoneMotionSample* current=nullptr;
    const HsdBoneMotionSample* previous=nullptr;
    f32 previous_weight=0;
    bool mirrored=false,previous_mirrored=false;
};
struct CharacterPoseFrame {
    Mat4 actor_base; // supplied actor anchor; never inferred from a clip/camera
    f32 bone1_translation_adjust[3]={};
    std::array<s16,3> bone1_look_angles{}; // source applies Z, then X, then Y
    std::vector<std::array<s16,3>> extra_angles;
    std::vector<std::array<f32,3>> scales;
    u32 enabled_bones=0xffffffffu;
    bool inverse_kinematics=false,alternate_root=false;
};
struct CharacterWorldPose {std::vector<Mat4> matrices;std::vector<u32> joint_offsets;};
// Retarget only keyed translations to a different-sized package rig. An
// unkeyed channel is still populated by that rig's own bind translation.
inline bool character_pose_scale_translations(HsdBoneMotionSample& sample,u16 percent){
    if(percent<25||percent>200)return false;
    if(percent==100)return true;
    const f32 scale=percent*.01f;
    for(auto& bone:sample.bones)for(unsigned axis=5;axis<=7;++axis)
        if(bone.present&(1u<<axis))bone.value[axis]*=scale;
    return true;
}
// Single presentation lane; not source gameplay lane ownership or mirroring.
inline bool character_pose_whole_body_profile(ByteView model,ByteView prefix,std::vector<u8>* out,std::string* error){
    HsdModelBones bones;
    if(!out||!hsd_bones_load(prefix,model,&bones,error))return false;
    const unsigned n=unsigned(bones.bones.size()),order=32,mirror=order+n,identities=(mirror+n+3)&~3u;
    if(n<2||n>kHsdMaxCharacterBones){if(error)*error="whole-body rig size unsupported";return false;}
    out->assign(identities+n*4,0);auto* p=out->data();
    store_be32(p,0x47464350);store_be16(p+4,1);store_be16(p+6,u16(n));
    p[8]=1;p[9]=p[10]=p[11]=u8(n);
    store_be32(p+12,order);store_be32(p+16,mirror);store_be32(p+20,identities);
    store_be32(p+24,u32(out->size()));store_be32(p+28,bones.root_offset);
    for(unsigned i=0;i<n;++i){p[order+i]=p[mirror+i]=u8(i);store_be32(p+identities+i*4,bones.bones[i].joint_offset);}
    return true;
}
namespace character_pose_detail {
inline bool finite(f32 x){return std::isfinite(x)&&std::fabs(x)<=1e7f;}
inline bool matrix(const Mat4& m){for(auto x:m.m)if(!finite(x))return false;return m.m[12]==0&&m.m[13]==0&&m.m[14]==0&&m.m[15]==1;}
inline bool rig(const CharacterPoseRig& r){
    const auto& b=r.binding;const auto n=b.bones.size();
    if(!r.loaded||!b.loaded||n<2||n>32||b.preorder_slots.size()!=n||r.order.size()!=n||r.mirror.size()!=n||!r.lane_mask||(r.lane_mask&0xf0)||r.cuts[0]!=0||r.cuts[4]!=n)return false;
    if(b.bones[0].joint_offset!=b.root_offset||b.bones[0].parent!=kHsdNoBone||b.bones[0].next!=kHsdNoBone||b.bones[0].child!=1||b.bones[1].parent!=0)return false;
    std::vector<std::pair<u8,u8>> stack{{0,kHsdNoBone}};std::vector<u8> walked;std::array<bool,32> seen{};
    while(!stack.empty()){
        auto [slot,parent]=stack.back();stack.pop_back();
        if(slot>=n||seen[slot])return false;
        seen[slot]=true;walked.push_back(slot);const auto& bone=b.bones[slot];
        if(bone.parent!=parent||!b.has_identity(slot))return false;
        for(auto value:bone.translation)if(!finite(value))return false;
        for(u8 other:walked)if(other!=slot&&b.bones[other].joint_offset==bone.joint_offset)return false;
        if(bone.next!=kHsdNoBone)stack.push_back({bone.next,parent});
        if(bone.child!=kHsdNoBone)stack.push_back({bone.child,slot});
    }
    if(walked!=b.preorder_slots)return false;
    std::array<bool,32> updated{};
    for(unsigned lane=0;lane<4;++lane){
        if(r.cuts[lane]>r.cuts[lane+1]||r.cuts[lane+1]>n)return false;
        if(!(r.lane_mask&(1u<<lane)))continue;
        std::array<bool,32> lane_slots{},mapped{};
        for(unsigned i=r.cuts[lane];i<r.cuts[lane+1];++i){const auto slot=r.order[i];if(slot>=n||updated[slot])return false;updated[slot]=true;lane_slots[slot]=true;}
        for(unsigned i=r.cuts[lane];i<r.cuts[lane+1];++i){const auto target=r.mirror[r.order[i]];if(target>=n||!lane_slots[target]||mapped[target])return false;mapped[target]=true;}
    }
    for(unsigned i=0;i<n;++i)if(r.order[i]>=n||r.mirror[i]>=n)return false;
    return true;
}
inline bool quaternion(const f32 q[4]){f64 norm=0;for(unsigned i=0;i<4;++i){if(!finite(q[i]))return false;norm+=f64(q[i])*q[i];}return norm>1e-12&&std::isfinite(norm);}
inline bool euler(const f32 a[3],f32 out[4]){
    for(unsigned i=0;i<3;++i)if(!finite(a[i])||std::fabs(a[i])>1e4f)return false;
    const f32 sx=std::sin(a[0]*.5f),sy=std::sin(a[1]*.5f),sz=std::sin(a[2]*.5f);
    const f32 cx=std::cos(a[0]*.5f),cy=std::cos(a[1]*.5f),cz=std::cos(a[2]*.5f);
    // Original8025ACFC, half-angle constant8043D498=0.5; Rz*Ry*Rx.
    out[0]=sx*cy*cz-cx*sy*sz;out[1]=cx*sy*cz+sx*cy*sz;
    out[2]=cx*cy*sz-sx*sy*cz;out[3]=cx*cy*cz+sx*sy*sz;return quaternion(out);
}
inline bool blend_kernel(const f32 current[4],const f32 previous[4],f32 old_weight,f32 out[4]){
    if(!quaternion(current)||!quaternion(previous)||!std::isfinite(old_weight)||old_weight< -1||old_weight>2)return false;
    f64 old[4],diff=0,sum=0;
    for(unsigned i=0;i<4;++i){old[i]=previous[i];diff+=std::pow(f64(current[i])-old[i],2);sum+=std::pow(f64(current[i])+old[i],2);}
    if(diff>sum)for(auto& x:old)x=-x; //8004A8CC chooses the closer representation
    f64 dot=0;for(unsigned i=0;i<4;++i)dot+=f64(current[i])*old[i];
    f64 sign=1;if(dot<0){dot=-dot;sign=-1;}
    f64 a=1-old_weight,b=old_weight;
    // Original8020BF08 threshold8043CAC0=bits3F7FFF58. The close branch is
    // LINEAR without normalization; quaternion->matrix handles its norm later.
    if(dot<=bits_to_f32(0x3f7fff58u)){
        const f64 angle=std::acos(dot),denominator=std::sin(angle);
        if(!std::isfinite(angle)||!std::isfinite(denominator)||denominator==0)return false;
        a=std::sin((1-old_weight)*angle)/denominator;b=std::sin(old_weight*angle)/denominator;
    }
    for(unsigned i=0;i<4;++i)out[i]=f32(a*current[i]+sign*b*old[i]);
    return quaternion(out);
}
inline bool blend(const f32 current[4],const f32 previous[4],f32 old_weight,f32 out[4]){
    // Existing pose/transition entry keeps its interpolation-only domain.
    if(!std::isfinite(old_weight)||old_weight<0||old_weight>1)return false;
    return blend_kernel(current,previous,old_weight,out);
}
inline bool sample(const CharacterPoseRig& r,const HsdBoneMotionSample& s){
    if(s.bones.size()!=r.binding.bones.size())return false;
    for(unsigned i=0;i<s.bones.size();++i){const auto& b=s.bones[i];
        if(b.joint_offset!=r.binding.bones[i].joint_offset||(b.present&~u16(0xee)))return false;
        for(unsigned channel=1;channel<=7;++channel)if((b.present&(1u<<channel))&&!finite(b.value[channel]))return false;
    }
    return true;
}
inline bool local(const CharacterPoseRig& rig,const CharacterLocalPose& pose){
    if(pose.bones.size()!=rig.binding.bones.size())return false;
    for(unsigned i=0;i<pose.bones.size();++i){const auto& p=pose.bones[i];
        if(p.joint_offset!=rig.binding.bones[i].joint_offset||!quaternion(p.quaternion))return false;
        for(auto v:p.translation)if(!finite(v))return false;
    }
    return true;
}
// Shared lane kernels. Callers validate rig/sample/local extents before entry.
inline bool assign_lane(const CharacterPoseRig& rig,unsigned lane,const HsdBoneMotionSample& sample,bool mirrored,std::vector<CharacterBonePose>& target){
    for(unsigned i=rig.cuts[lane];i<rig.cuts[lane+1];++i){const u8 source=rig.order[i],dest=mirrored?rig.mirror[source]:source;
        const auto& b=sample.bones[source];auto& p=target[dest];f32 angles[3]={};
        for(unsigned k=0;k<3;++k){p.translation[k]=(b.present&(1u<<(5+k)))?b.value[5+k]:rig.binding.bones[source].translation[k];if(b.present&(1u<<(1+k)))angles[k]=b.value[1+k];}
        if(mirrored){p.translation[0]=-p.translation[0];angles[1]=-angles[1];angles[2]=-angles[2];}
        if(!euler(angles,p.quaternion))return false;
    }return true;
}
inline bool blend_lane(const CharacterPoseRig& rig,unsigned lane,f32 weight,const std::vector<CharacterBonePose>& old,std::vector<CharacterBonePose>& target){
    for(unsigned i=rig.cuts[lane];i<rig.cuts[lane+1];++i){const auto slot=rig.order[i];auto& p=target[slot];const auto& before=old[slot];
        for(unsigned k=0;k<3;++k)p.translation[k]=p.translation[k]*(1-weight)+before.translation[k]*weight;
        f32 q[4];if(!blend(p.quaternion,before.quaternion,weight,q))return false;
        for(unsigned k=0;k<4;++k)p.quaternion[k]=q[k];
    }return true;
}
inline Mat4 angles(const std::array<s16,3>& a,bool look=false){
    const f32 unit=bits_to_f32(0x38c90fdbu); //80437078: signed binary angle ->radians
    const auto x=mat4_rotation_x(a[0]*unit),y=mat4_rotation_y(a[1]*unit),z=mat4_rotation_z(a[2]*unit);
    return mat4_multiply(z,look?mat4_multiply(x,y):mat4_multiply(y,x));
}
}
inline bool character_pose_rig_load(ByteView profile,const HsdModelBones& binding,CharacterPoseRig* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("character pose rig: ")+why;return false;};
    if(!out||!profile.contains(0,32)||profile.size>4096||profile.be32(0)!=0x47464350u||profile.be16(4)!=1||
       profile.be16(6)!=binding.bones.size()||binding.bones.size()>32||profile.be32(24)!=profile.size||profile.be32(28)!=binding.root_offset)return fail("header/identity/budget");
    const auto n=binding.bones.size();const std::size_t order=profile.be32(12),mirror=profile.be32(16),identities=profile.be32(20);
    if(order<32||mirror<32||identities<32||(identities&3)||!profile.contains(order,n)||!profile.contains(mirror,n)||!profile.contains(identities,n*4))return fail("table extents");
    const auto overlap=[](std::size_t a,std::size_t an,std::size_t b,std::size_t bn){return a<b+bn&&b<a+an;};
    if(overlap(order,n,mirror,n)||overlap(order,n,identities,n*4)||overlap(mirror,n,identities,n*4))return fail("table overlap");
    CharacterPoseRig next;next.binding=binding;next.lane_mask=profile.data[8];next.cuts={0,profile.data[9],profile.data[10],profile.data[11],u8(n)};
    next.order.assign(profile.data+order,profile.data+order+n);next.mirror.assign(profile.data+mirror,profile.data+mirror+n);
    for(unsigned i=0;i<n;++i)if(profile.be32(identities+i*4)!=binding.bones[i].joint_offset)return fail("logical bone identity");
    next.loaded=true;if(!character_pose_detail::rig(next))return fail("topology/lane/mirror domain");
    *out=std::move(next);return true;
}
//4BB14's identity-matrix→quaternion case and logical LOCAL translations.
//This initializes pose data only; it does not sample a clip or build a world.
inline bool character_pose_initial_local(const CharacterPoseRig& rig,CharacterLocalPose* out,std::string* error){
    if(out)*out={};
    if(!out||!character_pose_detail::rig(rig)){if(error)*error="character initial local pose: current rig/output required";return false;}
    CharacterLocalPose next;next.bones.reserve(rig.binding.bones.size());
    for(const auto& bone:rig.binding.bones){CharacterBonePose p;p.joint_offset=bone.joint_offset;std::copy_n(bone.translation,3,p.translation);p.quaternion[0]=p.quaternion[1]=p.quaternion[2]=0;p.quaternion[3]=1;next.bones.push_back(p);}
    *out=std::move(next);if(error)error->clear();return true;
}
inline bool character_pose_sample(const CharacterPoseRig& rig,const std::array<CharacterPoseLane,4>& lanes,CharacterLocalPose* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("character pose sample: ")+why;return false;};
    if(!out||!character_pose_detail::rig(rig))return fail("rig/destination");
    CharacterLocalPose next;next.bones.resize(rig.binding.bones.size());
    for(unsigned i=0;i<next.bones.size();++i){auto& p=next.bones[i];p.joint_offset=rig.binding.bones[i].joint_offset;
        for(unsigned k=0;k<3;++k)p.translation[k]=rig.binding.bones[i].translation[k];}
    for(unsigned lane=0;lane<4;++lane){
        if(!(rig.lane_mask&(1u<<lane)))continue;
        const auto& input=lanes[lane];
        if(!input.current||!character_pose_detail::sample(rig,*input.current)||!std::isfinite(input.previous_weight)||input.previous_weight<0||input.previous_weight>1||
           (input.previous_weight>0&&(!input.previous||!character_pose_detail::sample(rig,*input.previous))))return fail("active lane/time-blend binding");
        std::vector<CharacterBonePose> old=next.bones;
        if(!character_pose_detail::assign_lane(rig,lane,*input.current,input.mirrored,next.bones))return fail("rotation domain");
        if(input.previous_weight>0){
            if(!character_pose_detail::assign_lane(rig,lane,*input.previous,input.previous_mirrored,old))return fail("previous rotation domain");
            if(!character_pose_detail::blend_lane(rig,lane,input.previous_weight,old,next.bones))return fail("quaternion interpolation");
        }
    }
    if(!character_pose_detail::local(rig,next))return fail("computed local pose domain");
    *out=std::move(next);return true;
}
inline bool character_pose_initial_frame(const CharacterPoseRig& rig,const Mat4& actor_base,CharacterPoseFrame* out,std::string* error){
    if(out)*out={};
    if(!out||!character_pose_detail::rig(rig)||!character_pose_detail::matrix(actor_base)){if(error)*error="character pose frame: invalid rig/anchor/destination";return false;}
    CharacterPoseFrame next;next.actor_base=actor_base;next.extra_angles.resize(rig.binding.bones.size());next.scales.assign(rig.binding.bones.size(),{1,1,1});*out=std::move(next);return true;
}
inline bool character_pose_direct_world(const CharacterPoseRig& rig,const CharacterLocalPose& local,const CharacterPoseFrame& frame,CharacterWorldPose* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("character pose world: ")+why;return false;};
    if(!out||!character_pose_detail::rig(rig)||!character_pose_detail::local(rig,local)||!character_pose_detail::matrix(frame.actor_base))return fail("rig/pose/anchor/destination");
    const auto n=local.bones.size();const u32 all=n==32?0xffffffffu:(u32(1)<<n)-1;
    if(frame.inverse_kinematics||frame.alternate_root||(frame.enabled_bones&all)!=all||frame.extra_angles.size()!=n||frame.scales.size()!=n)return fail("IK/alternate-root/retained-mask/extra binding unsupported");
    if(frame.extra_angles[0]!=std::array<s16,3>{}||frame.scales[0]!=std::array<f32,3>{1,1,1})return fail("root0 local extras belong to alternate root construction");
    for(auto v:frame.bone1_translation_adjust)if(!character_pose_detail::finite(v))return fail("bone1 adjustment domain");
    for(const auto& scale:frame.scales)for(auto v:scale)if(!character_pose_detail::finite(v)||v==0||std::fabs(v)>1e4f)return fail("bone scale domain");
    CharacterWorldPose next;next.matrices.resize(n);next.joint_offsets.resize(n);
    //80046BD0 uses the actor transform at bone0. Clip bone0 translation is
    // root-motion input for other source code, NEVER silently added here.
    next.matrices[0]=frame.actor_base;
    for(u8 slot:rig.binding.preorder_slots){
        next.joint_offsets[slot]=rig.binding.bones[slot].joint_offset;if(slot==0)continue;
        const auto parent=rig.binding.bones[slot].parent;auto base=next.matrices[parent];
        if(slot==1)base=mat4_multiply(base,character_pose_detail::angles(frame.bone1_look_angles,true));
        Mat4 q;f32 affine[12];if(!gf_transform_quaternion(local.bones[slot].quaternion,affine))return fail("quaternion matrix");
        for(unsigned i=0;i<12;++i)q.m[i]=affine[i];
        for(unsigned k=0;k<3;++k)q.at(k,3)=local.bones[slot].translation[k]+(slot==1?frame.bone1_translation_adjust[k]:0);
        auto m=mat4_multiply(base,q);m=mat4_multiply(m,character_pose_detail::angles(frame.extra_angles[slot]));
        const auto& s=frame.scales[slot];const f32 xyz[3]={s[0],s[1],s[2]};m=mat4_multiply(m,mat4_scale(xyz));
        if(!character_pose_detail::matrix(m))return fail("computed world matrix domain");
        next.matrices[slot]=m;
    }
    *out=std::move(next);return true;
}
} // namespace gf
