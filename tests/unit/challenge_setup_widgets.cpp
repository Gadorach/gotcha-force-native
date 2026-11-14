#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/challenge_setup_widgets.hpp"
#include "tests/unit/challenge_setup_widgets_fixture.hpp"
namespace {unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"Challenge widgets: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;const auto common=challenge_widgets_fixture::common();const auto art=challenge_widgets_fixture::art();ChallengeSetupWidgets w;
    check(common.valid()&&art.valid()&&w.open(common,art),"owned profiles/open");
    check(w.updates()==0&&w.draws().size()==9&&w.draws().back().joint_time==0,"constructor updates once; context0 retains offscreen player parts and box request");
    ChallengeSetupWidgetsInput in{2,2,0,1,0};
    for(unsigned i=0;i<32;++i)check(w.step(in),"player page updates");
    check(w.draws().size()>=10&&w.draws().size()<=11,"two player panels and eligible focus/components");
    check(w.draws()[0].alpha==.8f&&w.draws()[0].mask==36&&w.draws()[4].material_time==1,"unselected plate dimmed, selected label material1");
    const auto frozen=w.draws();const auto count=w.updates();for(unsigned i=0;i<3;++i)check(w.draws().size()==frozen.size()&&w.updates()==count,"drawing plan cannot tick");
    in.closing=2;in.interactive=0;for(unsigned i=0;i<8;++i)check(w.step(in),"selected page close");
    bool selected=false;for(const auto& pose:w.draws())selected|=pose.record>=4&&pose.record<=7;check(!selected,"selected panel fades then retires draw, no time fitting");
    in={1,1,0,1,2};for(unsigned i=0;i<32;++i)check(w.step(in),"second context box page");
    const auto& box=w.draws().back();check(box.record==21&&box.material_time==1&&box.joint_time>=0&&box.joint_time<14,"box context selects second label, joint time remains bounded floor");
    check(w.draws()[w.draws().size()-2].world.m[5]<1e-5f,"box first part fixed quarter-turn about X");
    check(!w.step({1,0,0,2,1})&&!w.ready()&&w.draws().empty()&&!w.step(in),"invalid choice latches/no stale draw replay");
    for(unsigned field=0;field<4;++field){check(w.open(common,art),"fresh guarded owner");in={1,1,0,0,1};if(field==0)in.visible=16;if(field==1)in.interactive=16;if(field==2)in.closing=16;if(field==3)in.context=3;check(!w.step(in)&&w.draws().empty(),"unknown mask/context refusal");}
    auto wrong=art;wrong.bank_hash=2;check(!w.open(common,wrong)&&w.draws().empty(),"foreign art/bank refusal");
    for(unsigned i=0;i<wrong.operands.size();++i){wrong=art;wrong.operands[i]=std::numeric_limits<f32>::quiet_NaN();check(!wrong.valid()&&!w.open(common,wrong),"nonfinite profile refused before math/cast");}
    check(w.open(common,art),"budget fixture");in={0,0,0,0,0};for(unsigned i=0;i<8192;++i)check(w.step(in),"numeric budget prefix");check(!w.step(in)&&!w.ready()&&w.draws().empty(),"budget never forces page completion");
    ChallengeSetupPagesVisual joined;check(joined.open(common,art)&&joined.step({2,2,0,0,0}),"common/widget owner join");const auto tick=joined.chrome().updates();check(joined.step({1,1,0,0,2})&&joined.chrome().updates()==tick+1&&joined.widgets().updates()==tick+1,"same clocks survive player to box");
    check(!joined.step({1,1,0,0,4})&&!joined.ready()&&joined.draws().empty(),"late invalid input clears combined publication");
    std::printf("{\"challenge_widgets_owned_checks\":%u,\"numeric_budget_callbacks\":8192,\"all_passed\":true}\n",checks);
}
