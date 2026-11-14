#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/startup_sound_capture_commands.hpp"
#include "tests/unit/startup_sound_commands_fixture.hpp"
#include "tests/unit/startup_sound_spatial_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup capture commands: %s\n",why);std::abort();}}
}
int main(){
 using namespace gf;std::string error;auto tones=startup_sound_commands_fixture::tones();const auto original=tones;
 for(unsigned cue=0;cue<128;++cue)std::copy_n(original.data()+16+40*16,16,tones.data()+16+cue*16);
 auto pb=startup_sound_commands_fixture::profile(tones);StartupSoundCommandProfile p;check(startup_sound_command_load({pb.data(),pb.size()},&p,&error),"owned command profile");
 SoundCommandBanks banks;const std::array<SoundCommandBankInput,3> inputs={SoundCommandBankInput{0,{tones.data(),tones.size()}},SoundCommandBankInput{1,{tones.data(),tones.size()}},SoundCommandBankInput{2,{tones.data(),tones.size()}}};check(banks.load(p,inputs,&error),"explicit owned three banks");
 auto bytes=startup_sound_spatial_fixture::profile();StartupSoundSpatialProfile spatial;check(startup_sound_spatial_load({bytes.data(),bytes.size()},&spatial,&error),"owned source spatial profile");
 auto scope=std::make_shared<StartupSoundCaptureScope>();auto binding=std::make_shared<const CharacterModelBinding>(CharacterModelBinding{123});scope->actors[0]=binding;
 StartupSoundCaptureFrame frame;frame.scope=scope;frame.frame=77;frame.actors[0]=binding;frame.owners[0]={1,77,true,true,0,{0,0,20}};
 StartupSoundCaptureContext context;context.scope=scope;context.frame=77;context.commands={true,true,0,4};context.spatial=startup_sound_spatial_fixture::snapshot();
 StartupSoundIntent music;music.kind=StartupSoundIntentKind::MusicCommand;music.opcode=19;music.operand=27;music.command_offset=13;frame.intents.push_back(music);
 StartupSoundIntent prop;prop.origin=StartupSoundIntentOrigin::PropPre;prop.part=0;prop.call={StartupSoundCallKind::StartDirect,378,0,0};prop.bypass_disabled=true;frame.intents.push_back(prop);
 StartupSoundIntent actor;actor.origin=StartupSoundIntentOrigin::ActorScript;actor.actor=0;actor.lane=0;actor.script=26;actor.call={StartupSoundCallKind::StartDirect,18,1,0};frame.intents.push_back(actor);
 StartupSoundIntent exit=actor;exit.origin=StartupSoundIntentOrigin::ActorExit;exit.call.cue=242;frame.intents.push_back(exit);
 StartupSoundIntent reveal=prop;reveal.origin=StartupSoundIntentOrigin::RevealPre;reveal.part=2;reveal.call.cue=377;frame.intents.push_back(reveal);
 frame.ui.emit(UiSoundOperationKind::MainNavigate,1);StartupSoundCommandQueue input;StartupSoundCapturePlan plan;
 auto lower=[&](const StartupSoundCaptureFrame& f){return startup_sound_capture_commands(banks,spatial,f,context,input,&plan,&error);};
 check(lower(frame)&&plan.scalar_calls.size()==4&&plan.ui_operations==1&&plan.candidate.count==5,"ordered capture through common command core plus UI");
 check(plan.music.size()==1&&plan.music[0].order==0&&plan.music[0].opcode==19&&plan.music[0].operand==27&&plan.music[0].command_offset==13,"music stays ordered pending index metadata, no SFX or playback ACK");
 check(plan.scalar_calls[0].volume==127&&plan.scalar_calls[0].pan==64&&plan.scalar_calls[0].pitch==8192&&!plan.scalar_calls[0].owner,"null source doesn't depend on spatial/controller");
 check(plan.candidate.packets[0].bytes[1]==2&&plan.candidate.packets[0].bytes[2]==122&&plan.candidate.packets[1].bytes[1]==0&&plan.candidate.packets[1].bytes[2]==18&&plan.candidate.packets[2].bytes[1]==1&&plan.candidate.packets[2].bytes[2]==114&&plan.candidate.packets[3].bytes[2]==121&&plan.candidate.packets[4].bytes[2]==119,"source outer bank bits/local tone/order retained");
 for(unsigned setting=0;setting<5;++setting){context.commands.sfx_index=s8(setting);check(lower(frame)&&plan.candidate.count==5,"all five live setting indices accepted");}
 context.commands={true,true,0,4};auto biased=frame;biased.intents[2].call.kind=StartupSoundCallKind::StartBiased;biased.intents[2].call.viewer_bias=bits_to_f32(0x3e4ccccd);check(lower(biased)&&plan.scalar_calls[1].bias_used,"biased actor uses source viewer-beyond-actor rule");
 auto null_only=frame;null_only.intents={prop,reveal};null_only.ui={};context.commands={false,true,1,4};context.spatial={};check(lower(null_only)&&plan.candidate.count==2,"ungated prop/reveal bypass disabled byte without spatial reads");
 context.commands.settings_declared=false;check(!lower(null_only)&&plan.scalar_calls.empty()&&plan.candidate.count==0,"ungated still requires settings, failed plan clears all");
 context.commands={true,true,0,4};context.spatial=startup_sound_spatial_fixture::snapshot();
 auto bad=frame;bad.frame=76;check(!lower(bad),"stale frame refused");auto foreign=std::make_shared<StartupSoundCaptureScope>();bad=frame;bad.scope=foreign;check(!lower(bad),"foreign weak capture scope refused");
 auto neighbor=std::make_shared<const CharacterModelBinding>(CharacterModelBinding{123});bad=frame;bad.actors[0]=neighbor;check(!lower(bad),"same-resource neighboring actor not source capture actor");bad=frame;bad.owners[0].epoch=76;check(!lower(bad),"stale owner epoch refused");
 bad=frame;bad.owners[0].position_declared=false;check(!lower(bad),"no animated-root/origin substitute for source actor20");bad=frame;bad.owners[0].live=false;check(!lower(bad),"dead owner refused");
 for(unsigned kind=unsigned(StartupSoundCallKind::UpdateDirect);kind<=unsigned(StartupSoundCallKind::PauseCentered);++kind){bad=frame;bad.intents[2].call.kind=StartupSoundCallKind(kind);check(!lower(bad),"actor script cannot forge update/pause");}
 bad=frame;bad.intents[3].call.kind=StartupSoundCallKind::StartBiased;bad.intents[3].call.viewer_bias=bits_to_f32(0x3e4ccccd);check(!lower(bad),"actor exit direct-start only");
 bad=frame;bad.intents[1].bypass_disabled=false;check(!lower(bad),"prop must retain ungated source path");bad=frame;bad.intents[4].part=3;check(!lower(bad),"early orb cannot forge late-reveal sound377");bad=frame;bad.intents[2].script=28;check(!lower(bad),"unrecovered sparse script refused");
 bad=frame;bad.intents[0].opcode=21;check(!lower(bad),"music cannot be SFX opcode");bad=frame;bad.intents[0].call.owner=1;check(!lower(bad),"music cannot be actor call");
 bad=frame;bad.intents.resize(49,prop);check(!lower(bad),"frame budget bounded");bad=frame;bad.ui.overflow=true;check(!lower(bad)&&plan.candidate.count==0&&plan.music.empty()&&plan.scalar_calls.empty(),"late UI failure rolls back music/scalar/queue candidate");
 input.count=32;check(lower(frame)&&plan.candidate.count==32&&plan.candidate.start_calls==5,"full queue keeps source start-call counters and metadata, no fake admission");input={};input.count=33;check(!lower(frame),"invalid input queue refused");input={};
 plan.candidate=input;check(!startup_sound_capture_commands(banks,spatial,frame,context,plan.candidate,&plan,&error),"input/output alias refuses before clearing input");
 auto empty=frame;empty.intents.clear();empty.ui={};check(lower(empty)&&plan.candidate==input,"empty accepted frame needs no invented cue");
 binding.reset();check(!lower(frame),"expired model scope refuses actor commands");scope.reset();check(!lower(empty),"expired capture scope even with no sounds refuses");
 std::printf("{\"startup_capture_command_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
