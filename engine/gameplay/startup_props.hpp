#pragma once
//801C87DC/892C/8A84/8BD8/8C54 numeric state; not scene allocation or audio.
#include "engine/assets/startup_props.hpp"
#include "engine/gameplay/attachment_transform.hpp"
namespace gf {
struct StartupPropState {
    bool initialized=false,enabled=false;u8 motion=0,substate=0;s16 counter=0,settle_limit=0,spin=0,spin_delta=0;
    f32 y=0,velocity=0,target=0,acceleration=0;std::array<s16,3> angles{},original_angles{};
    std::array<f32,3> position{};AttachmentBoneTransform transform;u32 transforms=0;
};
struct StartupPropSound {u16 id=0;u8 part=0;}; // unfulfilled source requirement
namespace startup_prop_detail {
inline s16 wrap(s32 value){return std::bit_cast<s16>(u16(u32(value)));}
inline bool valid(const StartupPropState& s){
    if(!s.initialized||s.motion>3||s.substate>3)return false;
    for(f32 v:{s.y,s.velocity,s.target,s.acceleration})if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    for(f32 v:s.position)if(!std::isfinite(v)||std::abs(v)>1e4f)return false;
    return true;
}
}
inline bool startup_prop_initialize(const StartupPropProfile& p,unsigned part,const HsdJoint& root,u8 phase,u8 mode,StartupPropState* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup prop initialize: ")+why;return false;};
    if(!out||!startup_prop_profile_valid(p)||part>=8||root.archive_offset!=p.parts[part].root)return fail("profile/part/published root");
    StartupPropState s;s.initialized=s.enabled=true;const auto& e=p.parts[part];s.spin=e.initial_spin;
    for(unsigned k=0;k<3;++k){s.position[k]=root.translation[k];const f32 angle=f32(p.radians_to_angles*root.rotation[k]);if(!std::isfinite(angle)||f64(angle)<-2147483648.||f64(angle)>2147483647.)return fail("source fmuls/fctiwz finite domain");s.angles[k]=s.original_angles[k]=startup_prop_detail::wrap(s32(angle));}
    if(phase==12||mode==2){s.motion=3;s.y=p.zero;}
    else{s.enabled=false;s.spin_delta=e.spin_delta;s.counter=e.delay;s.y=e.height;s.velocity=e.velocity;s.acceleration=e.acceleration;s.target=e.target;}
    if(!startup_prop_detail::valid(s))return fail("constructed numeric state");
    *out=s;if(error)error->clear();return true;
}
inline bool startup_prop_update(const StartupPropProfile& p,unsigned part,u8 phase,u8 mode,const StartupPropState& input,
                                StartupPropState* out,std::vector<StartupPropSound>* cues,std::string* error){
    if(out)*out={};
    if(cues)cues->clear();
    auto fail=[&](const char* why){if(error)*error=std::string("startup prop update: ")+why;return false;};
    if(!out||!cues||!startup_prop_profile_valid(p)||part>=8||!startup_prop_detail::valid(input))return fail("profile/part/state/output");
    auto s=input;std::vector<StartupPropSound> pending;
    if(s.motion==0){if(phase>8||mode==2){s.counter=startup_prop_detail::wrap(s32(s.counter)-1);if(s.counter<1){s.motion=1;s.enabled=true;}}}
    else if(s.motion==1){s.y=f32(s.y+s.velocity);s.velocity=f32(s.velocity+s.acceleration);if(s.y<=s.target){s.motion=2;s.y=s.target;if(p.parts[part].sound>=0)pending.push_back({u16(p.parts[part].sound),u8(part)});}}
    else if(s.motion==2){
        if(part<4){
            if(s.substate==0){s.substate=1;s.spin=0;}
            else if(s.substate==1){if(phase>=10){if(part==0)pending.push_back({378,0});s.substate=2;s.counter=30;s.velocity=f32(-f32(s.y/f32(s.counter)));s.spin_delta=s16(0x20000/s32(s.counter));}}
            else{if(s.substate==2){s.y=f32(s.y+s.velocity);s.spin=startup_prop_detail::wrap(s32(s.spin)-s.spin_delta);s.counter=startup_prop_detail::wrap(s32(s.counter)-1);}
                if(s.substate==3||s.counter<=0){s.substate=3;s.y=p.zero;s.spin=s.spin_delta=s.angles[1]=0;s.motion=3;}}
        }else{if(s.substate==0){s.substate=1;s.counter=0;s.settle_limit=8;}
            if(s.substate!=1||s.counter<0||s.counter>=8||s.settle_limit!=8)return fail("settle table state/extent");
            s.y=p.settle[unsigned(s.counter)];s.counter=startup_prop_detail::wrap(s32(s.counter)+1);if(s.settle_limit<=s.counter)s.motion=3;}
    }
    if(!startup_prop_detail::valid(s))return fail("derived numeric budget");
    *out=s;*cues=std::move(pending);if(error)error->clear();return true;
}
inline bool startup_prop_transform(const StartupPropProfile& p,const HsdJoint& root,const StartupPropState& input,StartupPropState* out,std::string* error){
    if(out)*out={};
    if(!out||!startup_prop_profile_valid(p)||!startup_prop_detail::valid(input))return false;
    auto s=input;for(unsigned k=0;k<3;++k)s.position[k]=root.translation[k];s.position[1]=f32(s.position[1]+s.y);s.angles[1]=startup_prop_detail::wrap(s32(s.angles[1])+s.spin);
    AttachmentBoneTransformInput local;local.flags=15;local.local_position=s.position;local.angles=s.angles;
    if(!attachment_identity_transform(local,&s.transform,error))return false;
    ++s.transforms;*out=s;if(error)error->clear();return true;
}
} // namespace gf
