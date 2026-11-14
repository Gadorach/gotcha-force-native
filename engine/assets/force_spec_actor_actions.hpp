#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
//Source801DCFA0/57FF8 policy operands and two explicitly audited capability bits.
//Private source data binds the spec scene; this is not playback/delivery authority.
struct ForceSpecActorActionProfile {
    u32 bank_hash=0;std::array<u16,2> models{};std::array<u8,2> demonstration{};
    u8 cooldown=0,lane_mask=0,group=0,demo_choice=0,reset_choice=0;u16 pitch=0,yaw=0;s8 duration=0,mode=0;s16 start=0;bool loaded=false;
    bool valid()const{return loaded&&bank_hash&&models==std::array<u16,2>{0x615,10}&&demonstration[0]<=1&&demonstration[1]<=1&&cooldown==60&&lane_mask==15&&group==0&&demo_choice==40&&reset_choice==0&&pitch==0x800&&yaw==0&&duration==-1&&mode==-1&&start==-1;}
};
inline bool force_spec_actor_action_load(ByteView b,u32 bank_hash,ForceSpecActorActionProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="spec actor actions: profile/source/scene binding";return false;};
    if(!out||!b.data||b.size!=32||b.be32(0)!=0x47464152u||b.be16(4)!=1||b.be16(6)!=32||b.be32(8)!=bank_hash||b.data[23])return fail();
    ForceSpecActorActionProfile p;p.bank_hash=bank_hash;p.models={b.be16(12),b.be16(14)};p.demonstration={b.data[16],b.data[17]};p.cooldown=b.data[18];p.lane_mask=b.data[19];p.group=b.data[20];p.demo_choice=b.data[21];p.reset_choice=b.data[22];p.pitch=b.be16(24);p.yaw=b.be16(26);p.duration=std::bit_cast<s8>(b.data[28]);p.mode=std::bit_cast<s8>(b.data[29]);p.start=std::bit_cast<s16>(b.be16(30));p.loaded=true;
    if(!p.valid())return fail();
    *out=p;if(error)error->clear();return true;
}
} //namespace gf
