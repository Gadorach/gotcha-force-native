#include <cstdio>
#include <cstdlib>
#include "tests/unit/briefing_cards_fixture.hpp"
#include "tests/unit/briefing_type2_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;const auto b5=briefing_cards_fixture::five();const auto b6=briefing_cards_fixture::six();BattleInitialBriefingType5Profile five,bad5;BattleInitialBriefingType6Profile six,bad6;
    check(five.load({b5.data(),b5.size()},&error)&&six.load({b6.data(),b6.size()},&error));const auto facts=briefing_type2_fixture::facts();const std::array<u8,6> counts{1,2,6,30,0,0};unsigned active=0,inactive=0,cards=0;
    for(unsigned arg=0;arg<36;++arg){BattleInitialBriefingType5State s;check(battle_initial_briefing_type5(five,u8(arg),facts,counts,&s,&error));check(s.slot96==arg/6&&s.initialized&&s.generation==1);if(arg>=24){++inactive;check(!s.active&&s.phase18==2&&s.page12==arg&&!s.team88&&!s.count1b&&!s.timer1c);continue;}++active;const unsigned page=arg%6,total=counts[arg/6],n=unsigned(std::min(5,std::max(0,int(total)-int(page*5))));cards+=n;check(s.active&&s.phase18==1&&s.page12==page&&s.count1b==n&&s.member_begin==page*5&&s.flags84==0x11&&s.render82==0x11&&s.timer1c==15);check(s.field38==-200&&s.rate4c==f32(-200.f/15.f)&&s.values58_60[0]==bits_to_f32(0x3ecccccd));for(auto record:s.records)check(record==-1);}
    check(active==24&&inactive==12&&cards==39);auto empty=counts;empty[0]=0;BattleInitialBriefingType5State zero;check(battle_initial_briefing_type5(five,0,facts,empty,&zero,&error)&&!zero.count1b&&zero.field44==0);ForceEditorArt art;ForceCatalog catalog;check(!five.mapping_valid(art,catalog));
    for(unsigned at=0;at<b5.size();++at){auto edited=b5;edited[at]^=1;check(!bad5.load({edited.data(),edited.size()},&error));}
    BattleInitialBriefingType6State last;check(battle_initial_briefing_type6(six,0,&last,&error)&&last.phase18==1&&last.flags84==0x19&&last.render82==0x12&&last.timer1e==8&&!last.timer1c&&last.rate80==0x444&&last.record==21&&last.target64_6c[2]==-400&&last.initialized);
    for(unsigned arg=1;arg<256;++arg)check(!battle_initial_briefing_type6(six,u8(arg),&last,&error));
    for(unsigned at=0;at<b6.size();++at){auto edited=b6;edited[at]^=1;check(!bad6.load({edited.data(),edited.size()},&error));}
    std::printf("source36 card pages/inactive slots/final argument0 arithmetic: %u checks PASS\n",checks);
}
