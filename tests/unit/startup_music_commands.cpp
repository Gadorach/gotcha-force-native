#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/startup_music_commands.hpp"
#include "tests/unit/startup_music_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup music: %s\n",why);std::abort();}}
gf::StartupSoundIntent intent(gf::u8 opcode,gf::u16 operand){gf::StartupSoundIntent i;i.kind=gf::StartupSoundIntentKind::MusicCommand;i.opcode=opcode;i.operand=operand;return i;}}
int main(){
 using namespace gf;auto bytes=startup_music_fixture::profile(),parent=startup_music_fixture::menu();std::string error;StartupMusicProfile profile;
 auto load=[&](const auto& data){return startup_music_profile_load({data.data(),data.size()},{parent.data(),parent.size()},123,&profile,&error);};
 check(load(bytes)&&profile.ready()&&profile.resolve(27)->entry==0&&profile.resolve(29)->entry==1&&!profile.resolve(28),"two explicit source bindings/no whole table or cue alias");
 for(unsigned at:{0u,4u,8u,12u,24u,28u,192u,196u,32u,40u,44u,112u,120u}){auto bad=bytes;bad[at]^=0x80;check(!load(bad)&&!profile.ready(),"malformed binding refuses and clears output");}
 auto bad=bytes;startup_music_fixture::word(bad,16,32768);check(!load(bad),"negative source index not fabricated");
 bad=bytes;startup_music_fixture::word(bad,16,29);check(!load(bad),"duplicate script index refuses");
 bad=bytes;startup_music_fixture::word(bad,32+28,1);check(!load(bad),"same stream cannot masquerade as two source entries");
 bad=bytes;startup_music_fixture::word(bad,32+24,457);check(!load(bad),"foreign archive directory refuses");
 check(!startup_music_profile_load({nullptr,200},{parent.data(),parent.size()},123,&profile,&error)&&
       !startup_music_profile_load({bytes.data(),199},{parent.data(),parent.size()},123,&profile,&error)&&
       !startup_music_profile_load({bytes.data(),200},{nullptr,80},123,&profile,&error)&&
       !startup_music_profile_load({bytes.data(),200},{parent.data(),parent.size()},123,nullptr,&error),"null/short/profile output gates");
 check(load(bytes),"reload immutable music bindings");
 auto scope=std::make_shared<StartupSoundCaptureScope>();StartupSoundCaptureFrame capture;capture.scope=scope;capture.frame=1;
 capture.intents={intent(19,27),intent(20,1)};StartupMusicCommandState state;StartupMusicCommandPlan plan;
 check(startup_music_command_plan(profile,state,capture,&plan,&error)&&plan.commands.size()==2&&plan.candidate.member==0&&plan.candidate.transport_paused&&plan.candidate.pause_lanes[1]==1&&plan.candidate.starts==1,"start then pause preserved in same frame/no playback generation inferred");
 check(plan.commands[0].archive==0&&plan.commands[0].player==2&&plan.commands[1].pause_lane==1&&plan.requires_native_pcm_consumer,"exact source routes retained, PCM consumer still required");
 state=plan.candidate;capture.frame=2;capture.intents.clear();check(startup_music_command_plan(profile,state,capture,&plan,&error)&&plan.commands.empty()&&plan.candidate.transport_paused&&plan.candidate.commands==2,"empty frame neither resumes nor advances source transport");
 state=plan.candidate;capture.frame=3;capture.intents={intent(20,0)};check(startup_music_command_plan(profile,state,capture,&plan,&error)&&!plan.candidate.transport_paused&&plan.candidate.starts==1,"resume releases lane without restarting stream");
 state=plan.candidate;capture.frame=4;capture.intents={intent(19,27)};check(startup_music_command_plan(profile,state,capture,&plan,&error)&&plan.candidate.starts==2,"explicit same-member source start is not wrongly deduplicated");
 state=plan.candidate;capture.frame=5;capture.intents={intent(19,29)};check(startup_music_command_plan(profile,state,capture,&plan,&error)&&plan.candidate.member==1&&plan.candidate.starts==3,"later member switch follows actual command, not menu frame0");
 state=plan.candidate;capture.frame=6;capture.intents={intent(20,1),intent(19,27)};check(startup_music_command_plan(profile,state,capture,&plan,&error)&&!plan.candidate.transport_paused&&plan.candidate.pause_lanes[1]==1,"source start unpauses transport but preserves stored lane bytes");
 state=plan.candidate;capture.frame=7;capture.intents={intent(20,0)};state.pause_lanes[0]=1;state.transport_paused=true;
 check(startup_music_command_plan(profile,state,capture,&plan,&error)&&plan.candidate.transport_paused&&plan.candidate.pause_lanes[1]==0,"lane1 resume cannot cancel independent lane0 pause");
 const auto before=state;capture.intents={intent(19,29),intent(20,2)};
 check(!startup_music_command_plan(profile,state,capture,&plan,&error)&&plan.commands.empty()&&plan.candidate.frame==0&&state.starts==before.starts,"late unsupported pause refuses whole plan/no prefix commit");
 for(auto command:{intent(19,28),intent(19,65535),intent(21,0)}){capture.intents={command};check(!startup_music_command_plan(profile,state,capture,&plan,&error),"unknown table index/signed negative/opcode refuses");}
 capture.intents={intent(20,0)};capture.intents[0].origin=StartupSoundIntentOrigin::ActorScript;check(!startup_music_command_plan(profile,state,capture,&plan,&error),"actor origin cannot forge director music");
 capture.intents[0]=intent(20,0);capture.intents[0].actor=0;check(!startup_music_command_plan(profile,state,capture,&plan,&error),"director music must not claim an actor identity");
 capture.intents[0]=intent(20,0);capture.frame=state.frame;check(!startup_music_command_plan(profile,state,capture,&plan,&error),"duplicate frame cannot replay music commands");
 capture.frame=state.frame+2;check(!startup_music_command_plan(profile,state,capture,&plan,&error),"missing frames cannot be silently skipped");capture.frame=state.frame+1;
 StartupMusicProfile neighbor;check(startup_music_profile_load({bytes.data(),bytes.size()},{parent.data(),parent.size()},123,&neighbor,&error)&&!startup_music_command_plan(neighbor,state,capture,&plan,&error),"same-value separately loaded binding cannot replace live music context");
 auto other=std::make_shared<StartupSoundCaptureScope>();capture.scope=other;check(!startup_music_command_plan(profile,state,capture,&plan,&error),"foreign capture scope cannot extend music plan");capture.scope=scope;
 capture.intents.assign(49,intent(20,0));check(!startup_music_command_plan(profile,state,capture,&plan,&error),"native command budget retained");capture.intents.clear();scope.reset();check(!startup_music_command_plan(profile,state,capture,&plan,&error),"dead capture refuses without new scope fabrication");
 std::printf("{\"startup_music_profile_command_checks\":%u,\"source_calls_NOT_PCM_or_speaker_ACK\":true,\"all_passed\":true}\n",checks);
}
