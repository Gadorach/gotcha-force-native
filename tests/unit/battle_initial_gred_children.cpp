#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_gred_children.hpp"
#include "engine/assets/battle_initial_child_group_order.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,180> bytes{};
    const std::array<u32,20> header{0x47464743,1,180,0,0x80089a94,0x8008997c,0x801860c4,0x800660e8,0x80186118,0x801862d4,0x80089100,0x80007030,0x80007c54,0x80007cac,0x80047aa4,0x80186480,0x615,0x3ecccccd,0x13,3};
    const std::array<u32,25> data{0x47464154,0x00010002,100,0xe2b4439a,0x06150101,0,0x3f800000,0,0x3dcccccd,0x184b000a,0x01000000,0x07004000,0x0000c000,0xc0d9999a,0xc0666666,0,0,0x19410009,0x02000000,0x07000000,0,0xc0c7ae14,0xc091eb85,0,0};
    for(unsigned i=0;i<header.size();++i)store_be32(bytes.data()+4*i,header[i]);
    for(unsigned i=0;i<data.size();++i)store_be32(bytes.data()+80+4*i,data[i]);
    BattleInitialGRedChildrenProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready());
    StartupAttachmentParent parent;parent.model_id=0x615;StartupAttachmentUpdate step;
    for(unsigned i=0;i<2;++i){
        check(!startup_attachment_state_step(*profile.attachments(),i,3,parent,{},&step,&error));
        check(battle_initial_gred_child_state(profile,i,3,parent,&step,&error)&&step.state.phase==1&&step.state.render_group==68&&step.state.draw_scale==0&&!step.visible&&!step.alternate&&step.state.transform.bone==7);
    }
    parent.mode6e8=1;check(battle_initial_gred_child_state(profile,0,3,parent,&step,&error)&&f32_to_bits(step.state.draw_scale)==0x3dcccccd&&step.visible&&step.state.transform.flags==0x4b&&step.state.transform.angles==std::array<s16,3>{16384,0,-16384});
    parent.mode6e8=2;check(battle_initial_gred_child_state(profile,1,3,parent,&step,&error)&&step.visible&&step.state.transform.flags==0x41);
    parent.mode6e8=0x81;check(battle_initial_gred_child_state(profile,0,3,parent,&step,&error)&&!step.visible&&step.state.draw_scale==0); //G-Red compares full6E8, not low7
    for(unsigned slot:{0u,1u,2u,4u,5u,255u})check(!battle_initial_gred_child_state(profile,0,u8(slot),parent,&step,&error));
    parent.retire662=1;check(!battle_initial_gred_child_state(profile,0,3,parent,&step,&error));parent.retire662=0;
    parent.alternate_ae=1;check(!battle_initial_gred_child_state(profile,0,3,parent,&step,&error));parent.alternate_ae=0;
    parent.selector3ec=4;check(!battle_initial_gred_child_state(profile,0,3,parent,&step,&error));parent.selector3ec=0;
    for(unsigned at=0;at<bytes.size();at+=4){auto bad=bytes;bad[at]^=1;BattleInitialGRedChildrenProfile p;check(!p.load({bad.data(),bad.size()},&error)&&!p.ready()&&!p.attachments());}
    for(unsigned n=0;n<bytes.size();++n){BattleInitialGRedChildrenProfile p;check(!p.load({bytes.data(),n},&error));}
    std::array<u8,64> order{};const std::array<u32,16> ow{0x4746434f,1,64,0,0x80029434,0x80089a94,0x8008997c,0x80089898,0,16,8,24,6,56,0,0};
    for(unsigned i=0;i<ow.size();++i)store_be32(order.data()+4*i,ow[i]);
    BattleInitialChildGroupOrderProfile op;check(op.load({order.data(),order.size()},&error));
    for(unsigned at=0;at<order.size();at+=4){auto bad=order;bad[at]^=1;check(!op.load({bad.data(),bad.size()},&error));}
    for(unsigned n=0;n<order.size();++n)check(!op.load({order.data(),n},&error));
    std::printf("initial G-Red slot3/shared math and empty-group source profiles: %u checks PASS\n",checks);
}
