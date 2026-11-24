#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_initial_sasuke_children.hpp"
#include "engine/gameplay/startup_attachment_state.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,276> bytes{};
    const std::array<u32,20> header{0x47465343,1,276,0,0x80089a94,0x8008997c,0x8004d7fc,0x800660e8,0x8004d850,0x8004d978,0x80089100,0x80007030,0x80007c54,0x80007cac,0x80047aa4,0x8004dabc,10,0x3ecccccd,0x11,2};
    const std::array<u32,49> data{0x47464154,0x00010002,0xc4,0xeab3632a,0x000a0003,0,0x3f800000,0,0x3dcccccd,0x064b000b,0,0x07000000,0,0xc1000000,0xc04ccccd,0x40c66666,0,0x0d00c000,0x00004000,0xc1fc28f6,0xbfdd70a4,0xc1533333,0,0x07000000,0,0xc1000000,0xc04ccccd,0x40c66666,0,0x074b000e,0,0x0b000000,0,0x41a00000,0xc1a00000,0,0,0x0b000000,0,0x41a00000,0xc1a00000,0,0,0x0b000000,0,0x41a00000,0xc1a00000,0,0};
    for(unsigned i=0;i<header.size();++i)store_be32(bytes.data()+4*i,header[i]);
    for(unsigned i=0;i<data.size();++i)store_be32(bytes.data()+80+4*i,data[i]);
    BattleInitialSasukeChildrenProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready()&&profile.attachments());
    const auto& p=*profile.attachments();StartupAttachmentParent parent;parent.model_id=10;StartupAttachmentUpdate step;
    check(startup_attachment_state_step(p,1,1,parent,{},&step,&error)&&step.state.phase==1&&step.state.render_group==66&&step.state.mode==0&&step.state.transform.flags==0x4b&&step.state.transform.bone==11&&!step.visible&&!step.alternate);
    check(startup_attachment_state_step(p,0,1,parent,{},&step,&error)&&step.state.transform.bone==7&&step.visible&&step.state.transform.local_position==std::array<f32,3>{-8,bits_to_f32(0xc04ccccd),bits_to_f32(0x40c66666)});
    parent.mode6e8=1;check(startup_attachment_state_step(p,0,1,parent,{},&step,&error)&&step.state.mode==1&&step.state.transform.bone==13&&step.state.transform.angles==std::array<s16,3>{-16384,0,16384});
    parent.mode6e8=2;parent.reload784=1;check(startup_attachment_state_step(p,1,1,parent,{},&step,&error)&&step.visible);
    parent.mode6e8=3;check(!startup_attachment_state_step(p,0,1,parent,{},&step,&error));
    parent.mode6e8=0;check(!startup_attachment_state_step(p,0,2,parent,{},&step,&error)); //legacy isolated-index refusal unchanged
    for(unsigned at=0;at<bytes.size();at+=4){auto bad=bytes;bad[at]^=1;BattleInitialSasukeChildrenProfile rejected;check(!rejected.load({bad.data(),bad.size()},&error)&&!rejected.ready()&&!rejected.attachments());}
    for(unsigned n=0;n<bytes.size();++n){BattleInitialSasukeChildrenProfile rejected;check(!rejected.load({bytes.data(),n},&error));}
    std::printf("initial Sasuke sealed profile/shared state: %u checks PASS\n",checks);
}
