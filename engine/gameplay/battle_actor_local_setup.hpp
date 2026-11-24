#pragma once
#include "engine/gameplay/character_pose.hpp"
#include "engine/gameplay/startup_actor_setup.hpp"
#include "engine/gameplay/battle_actor_resource_maps.hpp"
namespace gf {
enum class BattleActorLocalWriteKind {Byte,Half,Word,FloatBits,Clear,Operation};
struct BattleActorLocalWrite {BattleActorLocalWriteKind kind;u16 offset=0,extent=0;u32 value=0;};
struct BattleActorLocalSetup {
    std::array<CharacterLocalPose,2> poses;
    std::array<std::array<std::array<f32,3>,32>,2> scales{};
    std::array<std::array<s16,3>,32> extra_angles{};
    //4BB14 initialization images. Once joined, live motion/program records
    //belong to the shared controllers, not an emulated actor-byte overlay.
    std::array<std::array<std::array<u8,64>,4>,2> motion_records{};
    std::array<std::array<u8,32>,4> auxiliary_records{};
    std::array<f32,3> retained1dbc{},adjust1d54{};s32 selector1d70=-1;
    u8 mask579=0,position1d9b=0,field1db2=0;std::array<s16,3> angles1d64{};std::array<u8,3> cuts1d91{};
    f32 motion_delta1dc8=1,timer_delta1dcc=1,weight1d9c=0,velocity1d60=0,field6d8=1,field6d4=0,offset6d0=0,origin_y6dc=0;
    std::array<f32,4> multipliers1dd4{};
    std::vector<BattleActorLocalWrite> writes;
};
inline bool battle_actor_local_setup(const StartupActorSetupProfile& setup,const CharacterPoseRig& rig,const BattleActorResourceMaps& maps,f32 actor_y,BattleActorLocalSetup* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("battle local setup: ")+why;return false;};
    if(!out||!startup_actor_setup_detail::valid(setup)||!character_pose_detail::rig(rig)||!maps.prepared||maps.bone_count1d90!=rig.binding.bones.size()||maps.local_translations.size()!=rig.binding.bones.size()||!character_pose_detail::finite(actor_y))return fail("same initialized logical resource map/rig required");
    const auto c=startup_actor_setup_detail::config(setup);if(c.data[2]!=rig.lane_mask)return fail("same class lane mask");
    for(unsigned i=0;i<3;++i)if(c.data[12+i]!=rig.cuts[i+1])return fail("same source lane cuts");
    BattleActorLocalSetup p;if(!character_pose_initial_local(rig,&p.poses[0],error))return false;p.poses[1]=p.poses[0];
    using K=BattleActorLocalWriteKind;auto put=[&](K k,unsigned at,unsigned n,u32 v){p.writes.push_back({k,u16(at),u16(n),v});};auto real=[&](unsigned at,f32 value){put(K::FloatBits,at,4,f32_to_bits(value));};
    for(unsigned i=0;i<rig.binding.bones.size();++i){
        for(unsigned k=0;k<3;++k)if(f32_to_bits(maps.local_translations[i][k])!=f32_to_bits(p.poses[0].bones[i].translation[k]))return fail("current map/local translation mismatch");
        for(unsigned k=0;k<3;++k)real(0x12d4+12*i+4*k,maps.local_translations[i][k]);
        for(unsigned k=0;k<3;++k)put(K::Word,0x1454+12*i+4*k,4,f32_to_bits(maps.local_translations[i][k]));
        for(unsigned bank=0;bank<2;++bank){p.scales[bank][i]={1,1,1};for(unsigned k:{2u,1u,0u})real((bank?0x1754:0x15d4)+12*i+4*k,1);}
        put(K::Operation,0,0,0x8020aefc);
        for(unsigned bank=0;bank<2;++bank){put(K::Operation,0,0,0x8020bcb0);const unsigned at=(bank?0x10d4:0xed4)+16*i;real(at+12,1);for(unsigned k=0;k<3;++k)real(at+4*k,0);}
    }
    for(unsigned i=0;i<32;++i)for(unsigned k:{2u,1u,0u})put(K::Half,0x18d4+6*i+2*k,2,0);
    for(unsigned at:{0x1dbcu,0x1dc0u,0x1dc4u,0x1d54u,0x1d58u,0x1d5cu})real(at,0);
    put(K::Word,0x1d70,4,0xffffffffu);p.mask579=c.data[2];put(K::Byte,0x579,1,p.mask579);
    for(unsigned at:{0x1d64u,0x1d66u,0x1d68u})put(K::Half,at,2,0);
    real(0x1dc8,1);real(0x1dcc,1);real(0x1d9c,0);put(K::Byte,0x1d9b,1,0);put(K::Byte,0x1db2,1,0);real(0x1d60,0);real(0x6d8,1);real(0x6d4,0);real(0x6d0,0);p.origin_y6dc=actor_y;real(0x6dc,actor_y);
    for(unsigned bank=0;bank<2;++bank)for(unsigned lane=0;lane<4;++lane){const unsigned at=0x1ad4+256*bank+64*lane;auto& record=p.motion_records[bank][lane];record.fill(0);record[0x33]=u8(bank);record[0x34]=u8(lane);record[0x37]=255;
        put(K::Clear,at,64,0);put(K::Byte,at+0x33,1,bank);put(K::Byte,at+0x34,1,lane);put(K::Byte,at+0x37,1,255);
        //The source resets these SAME four auxiliary records in both passes.
        p.auxiliary_records[lane].fill(0);put(K::Clear,0x1cd4+32*lane,32,0);
    }
    for(unsigned i=0;i<3;++i){p.cuts1d91[i]=c.data[12+i];put(K::Byte,0x1d91+i,1,p.cuts1d91[i]);}
    const f32 numerator=c.bef32(0x68);if(!character_pose_detail::finite(numerator))return fail("root multiplier numerator");
    for(unsigned i=0;i<4;++i){const auto denominator=c.bef32(0x58+4*i);if(!character_pose_detail::finite(denominator)||denominator==0)return fail("root multiplier denominator");p.multipliers1dd4[i]=f32(numerator/denominator);if(!character_pose_detail::finite(p.multipliers1dd4[i]))return fail("root multiplier result");real(0x1dd4+4*i,p.multipliers1dd4[i]);}
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
