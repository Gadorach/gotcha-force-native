#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_spec_widgets.hpp"
#include "engine/assets/source_messages.hpp"
#include "tests/unit/force_name_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"spec widgets: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;ForceSpecArt art;art.angle_unit=bits_to_f32(0x38c90fdb);art.number_step=-60;art.value_step=-50;art.part_shift=-29;art.part_variant_x=275;
    ForceSpecWidgetInput input;input.borg.level=10;input.borg.hp=1234;input.borg.cost=350;input.borg.number=205;input.experience=54321;ForceSpecWidgetPose pose;
    auto sample=[&](u8 type,s8 index,u16 record=11){art.widgets={{record,type,index,{660,90,-2999}}};return force_spec_widget_pose(art,input,0,&pose);};
    check(sample(4,0)&&pose.visible&&pose.frame==6&&pose.world.m[3]==660,"catalog number+1 units");check(sample(4,2)&&pose.frame==2&&pose.world.m[3]==540,"catalog hundreds placement");
    input.borg.number=0;check(sample(4,1)&&pose.visible&&pose.frame==0,"source catalogue leading zeros");check(sample(4,3)&&!pose.visible,"normal variant badge hidden");input.borg.variant=5;check(sample(4,3)&&pose.frame==4&&pose.visible,"variant filmstrip");
    check(sample(5,3)&&!pose.visible,"cost leading digit suppression");check(sample(5,2)&&pose.frame==3&&pose.world.m[3]==560,"cost hundreds");check(sample(6,1)&&pose.frame==1&&pose.world.m[3]==610,"level10 tens");
    check(sample(12,0)&&pose.frame==4&&pose.world.m[3]==735,"HP centered units");check(sample(12,3)&&pose.frame==1&&pose.world.m[3]==585,"HP centered thousands");check(sample(13,4)&&pose.frame==5&&pose.world.m[3]==560,"EXP centered fifth digit");
    input.borg.feature=-1;check(sample(10,2)&&!pose.visible,"absent feature panel");check(sample(11,1)&&!pose.visible,"zero jump-count bar hidden");input.borg.jump_count=6;check(sample(11,1)&&pose.frame==5,"override jump selector");
    input.borg.ratings={1,4,8,10};check(sample(14,3)&&pose.frame==10,"stat rating frame");input.page=ForceSpecPage::Moves;check(sample(14,3)&&!pose.visible,"other page hides stats");
    input.borg.move_count=2;input.borg.moves[1]={4,1,0,26,1029};check(sample(17,1)&&pose.visible&&pose.world.m[7]==-150,"move row240 spacing");check(sample(18,1,26)&&pose.record==27,"move button record");check(sample(19,1)&&!pose.visible,"no activation icon");check(sample(20,1)&&pose.frame==26,"move icon frame");check(sample(20,2)&&!pose.visible,"unused move row");
    input.page=ForceSpecPage::Description;input.callbacks=1;check(sample(16,1)&&pose.visible&&pose.world.m[0]!=1,"decoration phase");
    input.blink=49;check(sample(7,0)&&pose.visible,"arrow below50");input.blink=50;check(sample(7,0)&&!pose.visible,"arrow off at50");input.blink=0;
    input.callbacks=29;check(sample(8,2,34)&&pose.record==34,"footer first phase");input.callbacks=30;check(sample(8,2,34)&&pose.record==35,"footer second phase");input.callbacks=60;check(sample(8,2,34)&&pose.record==35,"footer60 retained");input.callbacks=61;check(sample(8,2,34)&&pose.record==34,"footer61 wraps");
    art.widgets={{1,1,0,{2000,-650,-3900}}};input.callbacks=800;check(force_spec_widget_pose(art,input,0,&pose)&&pose.world.m[3]==-6000,"floor retains exact lower endpoint");input.callbacks=801;check(force_spec_widget_pose(art,input,0,&pose)&&pose.world.m[3]==2000,"floor wraps only below lower endpoint");input.callbacks=802;check(force_spec_widget_pose(art,input,0,&pose)&&pose.world.m[3]==1990,"floor next cycle");
    art.widgets[0].position[0]=NAN;check(!force_spec_widget_pose(art,input,0,&pose)&&!pose.visible,"NaN refuses before integer conversion");input.callbacks=0;check(!sample(2,0),"explicit first callback required");input.callbacks=1;input.experience=0x80000000u;check(!sample(13,0),"unsupported signed EXP refuses");
    SourceMessageBank messages;auto font=force_name_fixture::font();std::vector<u8> text={0x47,0x46,0x54,0x58,0,0,0,1,0,1,0,0,0,2,0,2,0,2,0x82,0x60,0,4,0x81,0x40,0x82,0x61,0,1,0,0};std::string error;
    check(messages.load({text.data(),text.size()},font,&error)&&messages.groups()==1,"owned message bank");check(messages.message(0,0)->size()==2&&messages.message(0,0)->at(1)==std::string("\x81\x40\x82\x61",4)&&messages.message(0,1)->at(0).empty(),"lines, blank advance and empty message preserved");check(!messages.message(1,0)&&!messages.message(0,2),"message index bounds");
    text[19]=0x62;check(!messages.load({text.data(),text.size()},font,&error)&&!messages.message(0,0)&&messages.hash()==0,"late unknown glyph clears old bank");
    std::printf("specification widgets/messages: %u owned checks PASS\n",checks);
}
