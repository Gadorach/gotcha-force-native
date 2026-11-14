#pragma once
#include "engine/gameplay/battle_actor_look_tail.hpp"
namespace battle_look_tail_fixture {
inline gf::BattleLookTailProfile profile(){return {{0x3f75c28f,0x3dcccccd,0x5000},true};}
inline gf::BattleGroundWorldProfile ground_profile(){return {{0,0x3f800000,0x38c90fdb,0x41a00000,0x3f4ccccd,0x40000000,0x3e000000,0x40400000,0x3e800000,0x3f7fbe77,0x3f800000,0xbf800000},true};}
inline gf::BattleGroundIkProfile ik_profile(){return {0x2edbe6fe,{0,0,1,2,0,3,4,5,0},{120,121,122},{0x80045fc8,0x80046090,0x80046158,0x80046220,0x800462e4,0x800463ac},true};}
inline gf::CharacterPoseRig rig(){
    using namespace gf;CharacterPoseRig r;r.loaded=r.binding.loaded=true;r.lane_mask=1;r.cuts={0,13,13,13,13};r.binding.root_offset=32;r.binding.bones.resize(13);
    for(u8 i=0;i<13;++i){auto& b=r.binding.bones[i];b.joint_offset=32+64*i;b.child=i==12?kHsdNoBone:u8(i+1);b.parent=i?u8(i-1):kHsdNoBone;r.binding.preorder_slots.push_back(i);r.order.push_back(i);r.mirror.push_back(i);}return r;
}
inline gf::CharacterWorldPose world(const gf::CharacterPoseRig& rig){gf::CharacterWorldPose w;for(const auto& b:rig.binding.bones){w.matrices.emplace_back();w.joint_offsets.push_back(b.joint_offset);}return w;}
inline gf::StartupActorSetupProfile setup(){
    using namespace gf;StartupActorSetupProfile p;p.loaded=true;p.model_id=0x615;p.one=p.size_divisor=1;p.actor_data[0xa1]=31;for(auto& r:p.ranks)r={1,1,1};store_be16(p.actor_data.data()+4,100);for(unsigned at:{0x44u,0x48u,0x4cu,0x50u,0x54u})store_be_f32(p.class_config.data()+at,1);p.class_config[0x22]=255;
    const std::array<s16,8> limits{16384,-16384,-8192,8192,8192,-4096,-4096,2048};for(unsigned i=0;i<8;++i)store_be16(p.actor_data.data()+0x15c+2*i,std::bit_cast<u16>(limits[i]));return p;
}
inline gf::BattleLookTailInput input(){gf::BattleLookTailInput i;i.gates_declared=i.yaw72_declared=i.target_flag_declared=i.position20_declared=i.destination50c_declared=i.prior12_declared=true;i.phase18=1;i.target_present=true;i.destination50c={10,0,0};return i;}
} // namespace battle_look_tail_fixture
