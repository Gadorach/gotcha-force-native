#pragma once
// Constructor numeric subset80056180/6826C/55DCC/68004, notfull57B78.
#include "engine/assets/startup_actor_setup.hpp"
namespace gf {
struct StartupActorRankState {u8 requested=0,selected=0;f32 scale_b8=0,field1dd0=0,field5f8=0;};
struct StartupActorInitialSetup {
    StartupActorRankState rank;
    f32 field5f4=0,field768=0,scale_b4=0,fieldc0=0,fieldc4=0;
    std::array<f32,3> actor_scale{};s16 health736=0,health738=0;
    f32 field668=0,field66c=0,field670=0,field674=0,field770=0,field7fc=0;
    std::array<f32,3> fields678{};
    bool numeric_setup_only=false;
};
// Explicit fresh-construction branch. Other scale/lifecycle branches must not
// silently use these results; no position or host-admission token is produced.
struct StartupActorInitialSetupInputs {u8 field489=0,flags48a=0,field500=0;f32 field400=0;};
inline bool startup_actor_rank_select(const StartupActorSetupProfile& profile,s8 requested,StartupActorRankState* out){
    using namespace startup_actor_setup_detail;
    if(!out||!valid(profile))return false;
    const auto upper=signed_byte(profile.actor_data[0xa1]),lower=signed_byte(profile.actor_data[0xa2]);
    const int chosen=upper<requested?upper:requested<lower?lower:requested;const auto& rank=profile.ranks[unsigned(chosen)];
    *out={std::bit_cast<u8>(requested),u8(chosen),rank[0],rank[1],rank[2]};return true;
}
// Shared55DCC dimension products; normalization/actor scale policy stays with
// the caller. The preview and startup owners use the same numeric kernel.
inline bool startup_actor_dimensions(const StartupActorSetupProfile& profile,f32 scale,StartupActorInitialSetup* out){
    if(!out||!startup_actor_setup_detail::valid(profile)||!std::isfinite(scale))return false;
    auto next=*out;const auto c=startup_actor_setup_detail::config(profile);next.scale_b4=scale;
    next.field668=f32(c.bef32(0x4c)*scale);next.field66c=f32(c.bef32(0x44)*scale);next.field670=f32(c.bef32(0x48)*scale);next.field674=f32(c.bef32(0x54)*scale);next.field770=f32(c.bef32(0x50)*scale);
    for(auto value:{next.field668,next.field66c,next.field670,next.field674,next.field770})if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
    *out=next;return true;
}
inline bool startup_actor_initial_setup(const StartupActorSetupProfile& profile,const StartupActorInitialSetupInputs& input,
                                       StartupActorInitialSetup* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("startup actor initial setup: ")+why;return false;};
    if(!out||!startup_actor_setup_detail::valid(profile)||input.field489||input.flags48a||input.field500||!std::isfinite(input.field400)||input.field400!=0)return fail("fresh numeric branch/config/output");
    StartupActorInitialSetup next;if(!startup_actor_rank_select(profile,16,&next.rank))return fail("source rank selection");
    next.field5f4=next.field768=profile.one;next.scale_b4=next.rank.scale_b8;next.fieldc0=next.fieldc4=next.scale_b4;next.actor_scale.fill(next.scale_b4);
    const auto data=startup_actor_setup_detail::data(profile),config=startup_actor_setup_detail::config(profile);
    next.health736=next.health738=std::bit_cast<s16>(data.be16(4));
    if(!startup_actor_dimensions(profile,next.scale_b4,&next))return fail("derived numeric bounds");
    for(unsigned k=0;k<3;++k)next.fields678[k]=data.bef32(0x74+k*4);
    //68004 uses unscaled class dimensions; do not applyB4 twice here.
    next.field7fc=f32(std::max(config.bef32(0x4c),config.bef32(0x50))/profile.size_divisor);
    for(f32 v:{next.field668,next.field66c,next.field670,next.field674,next.field770,next.field7fc})if(!std::isfinite(v)||std::abs(v)>1e7f)return fail("derived numeric bounds");
    next.numeric_setup_only=true;*out=next;return true;
}
} // namespace gf
