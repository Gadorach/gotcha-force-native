#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include "engine/gameplay/startup_sound_manager.hpp"
#include "tests/unit/startup_sound_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup sound manager: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;std::string error;auto raw=startup_sound_fixture::profile();StartupSoundProfile p;
    check(startup_sound_load({raw.data(),raw.size()},&p,&error),"owned explicit sparse profile");
    for(unsigned i=0;i<raw.size();++i){auto bad=raw;bad[i]^=0x80;StartupSoundProfile result;check(!startup_sound_load({bad.data(),bad.size()},&result,&error)&&!result.loaded,"any changed source/header/reserved byte refused");}
    StartupSoundProfile rejected;check(!startup_sound_load({raw.data(),raw.size()-1},&rejected,&error),"truncated");raw.push_back(0);check(!startup_sound_load({raw.data(),raw.size()},&rejected,&error),"trailing");
    StartupSoundState empty;StartupSoundOperation op;op.owner=101;StartupSoundPlan plan;
    check(!startup_sound_plan(p,empty,op,&plan,&error)&&plan.pending_count==0,"new start requires source tick");
    op.tick_declared=true;op.tick=77;
    check(startup_sound_plan(p,empty,op,&plan,&error)&&plan.pending_count==1&&plan.tick_samples==1,"F0104 initial new call");
    check(plan.pending[0]==StartupSoundPendingCall{StartupSoundCallKind::StartBiased,40,101,p.viewer_bias},"descriptor0 is sound40 biased, not auxiliary effect40");
    check(plan.candidate.rows[5]==StartupSoundRow{77,2,0,101}&&empty==StartupSoundState{},"candidate only; original state unchanged");auto state=plan.candidate;
    op.owner=202;op.tick_declared=false;op.tick=999;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&plan.tick_samples==0&&plan.candidate.rows[5]==StartupSoundRow{77,2,0,202},"same start refresh replaces owner but samples no tick/restarts nothing");
    op.descriptor=1;op.tick_declared=true;op.tick=88;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==2&&plan.pending[0]==StartupSoundPendingCall{StartupSoundCallKind::StartDirect,41,202,0}&&plan.pending[1].cue==126,"different start stops old using incoming owner before new start");
    op.kind=StartupSoundOperationKind::Restart;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==1&&plan.candidate.rows[5].timestamp==88,"F0214 explicit restart overwrites without old stop");
    state.rows[5]={123,17,4,101};op.kind=StartupSoundOperationKind::Refresh;op.descriptor=10;op.tick_declared=false;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&plan.candidate.rows[5]==StartupSoundRow{123,2,4,202},"compare start cue ONLY;alias retains descriptor4 NOTrequested10");
    op.kind=StartupSoundOperationKind::Clear;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==1&&plan.candidate.rows[5]==StartupSoundRow{0,17,startup_sound_no_descriptor,0},"F02D0 clears timestamp/descriptor/owner but retains lease");
    state=empty;state.rows[5].timestamp=17;state.rows[5].lease=-5;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&plan.candidate.rows[5]==StartupSoundRow{17,-5,startup_sound_no_descriptor,202},"F02D0 empty row retains timestamp/lease and incoming owner");
    for(int actor=-1;actor<=6;++actor)for(unsigned slot=0;slot<9;++slot){op.kind=StartupSoundOperationKind::Refresh;op.actor96=actor;op.slot=slot;op.descriptor=0;op.tick_declared=true;
        check(startup_sound_plan(p,empty,op,&plan,&error)==(actor>=0&&actor<6&&slot<8),"signed actor96 and eight physical slots guarded");
        if(actor>=0&&actor<6&&slot<8)check(plan.candidate.rows[unsigned(actor)*8+slot].owner==202,"row indexing six times eight");
    }
    op.actor96=0;op.slot=261;op.descriptor=256;
    check(startup_sound_plan(p,empty,op,&plan,&error)&&plan.candidate.rows[5].descriptor==0,"source lowbyte masks on slot/descriptor, no clamp");op.descriptor=3;
    check(!startup_sound_plan(p,empty,op,&plan,&error)&&plan.pending_count==0,"unaudited sparse cell refuses even when inside neighboring table");
    op={};op.kind=StartupSoundOperationKind::Update;
    check(!startup_sound_plan(p,empty,op,&plan,&error),"EFF30 samples tick even with no rows");op.tick_declared=true;
    check(startup_sound_plan(p,empty,op,&plan,&error)&&plan.tick_samples==1,"no active division on empty manager");
    state=empty;state.rows[5]={123,1,0,101};op.tick=999;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==1&&plan.pending[0].kind==StartupSoundCallKind::StartDirect&&plan.pending[0].cue==41&&plan.candidate.rows[5]==StartupSoundRow{123,0,startup_sound_no_descriptor,0},"expiry is direct stop-CUE start, timestamp retained and no division");
    state.rows[5].lease=2;
    check(!startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&state.rows[5].lease==2,"surviving row requires bus clock; failed candidate not published");
    op.bus_clock=500000;op.tick=1373;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==1&&plan.pending[0].kind==StartupSoundCallKind::UpdateBiased&&plan.candidate.rows[5].timestamp==1373&&plan.candidate.rows[5].lease==1,"interval reached: next0 SELF row update NOT terminal and not start");
    op.tick=1372;check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&plan.candidate.rows[5].timestamp==123,"strict interval boundary");
    state.rows[5]={0xfffffff0u,2,2,101};op.tick=0x4d2;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==1&&plan.pending[0].kind==StartupSoundCallKind::UpdateDirect,"tick subtraction wraps u32 before times8/division");
    state.rows[5].timestamp=0;op.tick=0x20000000u;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0,"times8 wraps BEFORE division, not u64 elapsed");
    state.rows[5].lease=std::numeric_limits<s32>::min();op.tick=0;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.candidate.rows[5].lease==std::numeric_limits<s32>::max(),"source signed lease addic wraps without C signed overflow");
    state.mode=1;op.bus_clock=0;
    check(!startup_sound_plan(p,state,op,&plan,&error),"paused still computes divisor before mode gate");op.bus_clock=500000;op.tick=10000;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.candidate==state&&plan.pending_count==0,"mode1 no expiry or sequence mutation");
    state.mode=2;state.rows[5].lease=-1;op.tick=0;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.candidate.mode==0&&plan.candidate.rows[5].lease==-1&&plan.pending_count==1,"forced resume updates once without decrement then mode0");
    state=empty;state.rows[5]={333,4,0,101};op.kind=StartupSoundOperationKind::Pause;op.tick_declared=false;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.tick_samples==0&&plan.candidate.mode==1&&plan.candidate.pause_depth==1&&plan.pending[0].kind==StartupSoundCallKind::PauseCentered,"pause emits centered zero-volume update, not spatial/start/lease refresh");state=plan.candidate;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==1&&plan.candidate.pause_depth==2,"nested pause repeats updates");state=plan.candidate;op.kind=StartupSoundOperationKind::Resume;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&plan.candidate.pause_depth==1&&plan.candidate.mode==1,"inner resume retains mode");state=plan.candidate;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.candidate.pause_depth==0&&plan.candidate.mode==2,"outer resume forces next manager update");
    for(unsigned depth=0;depth<256;++depth){state.pause_depth=u8(depth);op.kind=StartupSoundOperationKind::Resume;
        const u8 decremented=u8(depth-1u);const bool clamp=std::bit_cast<s8>(decremented)<1;
        check(startup_sound_plan(p,state,op,&plan,&error)&&plan.candidate.pause_depth==(clamp?0:decremented)&&plan.candidate.mode==(clamp?2:state.mode),"resume uses signed wrapped BYTE, not saturating unsigned depth");
    }
    state.rows[47]={3,4,0,0};op.kind=StartupSoundOperationKind::Pause;
    check(!startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&state.rows[5].owner==101,"late null-owner pause failure discards earlier staged calls/state");
    state=empty;state.pause_depth=7;state.mode=2;
    for(unsigned i=0;i<48;++i)state.rows[i]={i+1,s32(i)-24,u16(i%2?0:2),100+i};
    op.kind=StartupSoundOperationKind::StopAll;
    check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==48&&plan.candidate.mode==0&&plan.candidate.pause_depth==7,"bounded stopall preserves depth");
    for(unsigned i=0;i<48;++i)check(plan.pending[i].owner==100+i&&plan.pending[i].kind==StartupSoundCallKind::StartDirect&&plan.candidate.rows[i]==StartupSoundRow{i+1,s32(i)-24,startup_sound_no_descriptor,0},"row-order48 stop requests and partial reset semantics");
    op.kind=StartupSoundOperationKind::Reset;check(startup_sound_plan(p,state,op,&plan,&error)&&plan.pending_count==0&&plan.tick_samples==0,"reset emits no implicit stops/tick");
    auto invalid=state;invalid.rows[47].descriptor=3;
    check(!startup_sound_plan(p,invalid,op,&plan,&error),"unknown retained descriptor refused, no partial earlier reset");op.kind=static_cast<StartupSoundOperationKind>(255);
    check(!startup_sound_plan(p,state,op,&plan,&error),"unknown operation");
    const unsigned owned=checks;std::ifstream f("local/content/startup-sound-manager/manager.gfsm",std::ios::binary);std::vector<u8> source{std::istreambuf_iterator<char>(f),{}};
    if(!source.empty()){check(source==startup_sound_fixture::profile(),"offline original sparse source records equal represented profile");check(startup_sound_load({source.data(),source.size()},&p,&error),"original loose profile");}
    std::printf("{\"startup_sound_manager_checks\":%u,\"owned_checks\":%u,\"pending_calls_fulfilled\":false,\"actor708_or_visible_startup_admitted\":false,\"all_passed\":true}\n",checks,owned);
}
