#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_actor_local_setup.hpp"
#include "tests/unit/startup_actor_setup_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"battle local setup: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;const auto bytes=startup_actor_setup_fixture::profile();StartupActorSetupProfile setup;check(startup_actor_setup_load({bytes.data(),bytes.size()},1234,&setup,&error),"owned setup");
    CharacterPoseRig rig;rig.loaded=rig.binding.loaded=true;rig.lane_mask=3;rig.cuts={0,2,4,4,4};rig.order={2,3,0,1};rig.mirror={0,1,3,2};rig.binding.root_offset=0x100;rig.binding.preorder_slots={0,1,2,3};rig.binding.bones.resize(4);
    for(unsigned i=0;i<4;++i){rig.binding.bones[i].joint_offset=0x100+0x100*i;rig.binding.bones[i].translation[0]=f32(i)*10;rig.binding.bones[i].translation[2]=i==2?-0.f:f32(i);}
    rig.binding.bones[0].child=1;rig.binding.bones[1].parent=0;rig.binding.bones[1].child=2;rig.binding.bones[2].parent=1;rig.binding.bones[2].next=3;rig.binding.bones[3].parent=1;
    setup.class_config[2]=3;setup.class_config[12]=2;setup.class_config[13]=setup.class_config[14]=4;
    for(unsigned i=0;i<4;++i){store_be_f32(setup.class_config.data()+0x58+4*i,f32(2u<<i));}store_be_f32(setup.class_config.data()+0x68,8);
    BattleActorResourceMaps maps;maps.prepared=true;maps.bone_count1d90=4;for(const auto& b:rig.binding.bones)maps.local_translations.push_back({b.translation[0],b.translation[1],b.translation[2]});
    BattleActorLocalSetup out;check(battle_actor_local_setup(setup,rig,maps,-20,&out,&error),"owned local initialization");
    check(out.poses[0].bones.size()==4&&out.poses[0].bones.data()!=out.poses[1].bones.data(),"two independent local buffers");
    for(unsigned bank=0;bank<2;++bank)for(unsigned i=0;i<4;++i){const auto& p=out.poses[bank].bones[i];check(p.joint_offset==rig.binding.bones[i].joint_offset&&p.translation[0]==f32(i)*10&&f32_to_bits(p.translation[2])==f32_to_bits(maps.local_translations[i][2]),"logical translations and signed zero preserved");check(p.quaternion[0]==0&&p.quaternion[1]==0&&p.quaternion[2]==0&&p.quaternion[3]==1&&out.scales[bank][i]==std::array<f32,3>{1,1,1},"identity-case quaternions and unit extra scales");}
    check(out.scales[0][4]==std::array<f32,3>{}&&out.extra_angles==std::array<std::array<s16,3>,32>{},"unreached scale slots stay clear; all32 angle slots reset");
    for(unsigned bank=0;bank<2;++bank)for(unsigned lane=0;lane<4;++lane)for(unsigned at=0;at<64;++at)check(out.motion_records[bank][lane][at]==(at==0x33?bank:at==0x34?lane:at==0x37?255:0),"eight cleared/indexed records, no player request");
    std::array<unsigned,4> clears{};for(const auto& w:out.writes)if(w.kind==BattleActorLocalWriteKind::Clear&&w.offset>=0x1cd4&&w.offset<0x1d54)++clears[(w.offset-0x1cd4)/32];check(clears==std::array<unsigned,4>{2,2,2,2},"four auxiliary records are cleared twice, not eight different buffers");
    check(out.motion_delta1dc8==1&&out.timer_delta1dcc==1&&out.field6d8==1&&out.origin_y6dc==-20&&out.multipliers1dd4==std::array<f32,4>{4,2,1,.5f}&&out.selector1d70==-1,"source scalar polarity, ratios and initial selection");
    maps.local_translations[1][0]+=1;check(!battle_actor_local_setup(setup,rig,maps,0,&out,&error)&&out.writes.empty(),"foreign/mutated current map refuses");maps.local_translations[1][0]-=1;
    store_be_f32(setup.class_config.data()+0x58,0);check(!battle_actor_local_setup(setup,rig,maps,0,&out,&error),"zero ratio denominator has no default fallback");
    std::printf("battle actor local setup: %u checks PASS\n",checks);
}
