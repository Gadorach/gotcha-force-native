#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include "tests/unit/startup_sound_spatial_fixture.hpp"
#include "tests/unit/startup_sound_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup sound spatial: %s\n",why);std::abort();}}
bool close(float a,float b){return std::abs(a-b)<1e-4f;}
}
int main(){
    using namespace gf;std::string error;auto raw=startup_sound_spatial_fixture::profile();StartupSoundSpatialProfile p;
    check(startup_sound_spatial_load({raw.data(),raw.size()},&p,&error),"owned source operand profile");
    for(unsigned at=0;at<raw.size();++at){auto bad=raw;bad[at]^=0x80;StartupSoundSpatialProfile result;check(!startup_sound_spatial_load({bad.data(),bad.size()},&result,&error)&&!result.loaded,"changed header/source/reserved byte refuses");}
    StartupSoundSpatialProfile rejected;check(!startup_sound_spatial_load({raw.data(),63},&rejected,&error),"truncated");raw.push_back(0);check(!startup_sound_spatial_load({raw.data(),raw.size()},&rejected,&error),"trailing");
    auto snapshot=startup_sound_spatial_fixture::snapshot();std::array<StartupSoundOwnerObservation,1> owners={startup_sound_spatial_fixture::owner()};
    StartupSoundPendingCall call{StartupSoundCallKind::StartDirect,40,101,0};StartupSoundScalarCommand command;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.volume==127&&command.pan==63&&command.screen_x==320&&command.nearest_distance==10,"source mid-screen320 gives pan63 by truncation, NOTinvented64center");
    check(command.position==owners[0].position20&&!command.update&&command.pitch==8192&&!command.bias_used&&!command.reflection_used,"direct command identity/pitch and actor20 position");
    call.kind=StartupSoundCallKind::UpdateDirect;check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.update,"update is NOTrestart");
    call.kind=StartupSoundCallKind::StartBiased;call.viewer_bias=p.bias;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.bias_used&&command.position[2]==8&&command.reflection_used&&close(command.pan_position[2],12),"literal bias viewer+.2*(viewer-actor) THEN localZ reflection before pan");
    snapshot.viewers[0].actor2e5=1;snapshot.viewers[0].position_declared=false;
    check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"mismatched bias avoids position there, but volume still needs its viewer position");
    snapshot.viewers0c[0]=-1;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&!command.bias_used&&command.position[2]==20&&command.volume==127&&command.pan==64,"bias mismatch falls backdirect, negative head bypasses all distance/projection reads");
    snapshot.bias_selection_declared=false;check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"biased call must observe selector even when volume/pan bypass");
    call={StartupSoundCallKind::StartDirect,41,0,0};StartupSoundSpatialSnapshot absent;
    check(startup_sound_spatial_resolve(p,call,absent,{},&command,&error)&&command.volume==127&&command.pan==64&&command.actor96==0&&!command.position_used,"null actor default requires NOviewer/camera/sourceposition");
    call={StartupSoundCallKind::UpdateBiased,40,0,p.bias};check(startup_sound_spatial_resolve(p,call,absent,{},&command,&error)&&command.update,"null biased update delegatesdirect null defaults");
    call={StartupSoundCallKind::PauseCentered,40,101,0};owners[0].position_declared=false;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.update&&command.volume==0&&command.pan==64&&!command.position_used,"pause only reads live owner signed96, not absent scene/positions");call.owner=0;
    check(!startup_sound_spatial_resolve(p,call,absent,{},&command,&error),"pause null owner refuses source dereference");
    call={StartupSoundCallKind::StartDirect,40,101,0};snapshot=startup_sound_spatial_fixture::snapshot();owners[0]=startup_sound_spatial_fixture::owner();
    owners[0].epoch=78;check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"stale owner snapshot refuses");owners[0].epoch=77;owners[0].live=false;
    check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"released owner refuses");owners[0].live=true;
    std::array<StartupSoundOwnerObservation,2> duplicate={owners[0],owners[0]};check(!startup_sound_spatial_resolve(p,call,snapshot,duplicate,&command,&error),"ambiguous generation refuses");
    snapshot.viewers[0].epoch=78;check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"stale selected viewer refuses");snapshot.viewers[0].epoch=77;
    for(int slot=-1;slot<=6;++slot){owners[0].actor96=s8(slot);check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)==(slot>=0&&slot<6),"signed owner slot admission");}owners[0].actor96=0;
    snapshot.viewers[0].position2e8=owners[0].position20;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.volume==32&&command.nearest_distance==-1,"zero viewer distance is ignored, not replaced by full volume");
    snapshot.viewers[0].matrix364.at(2,3)=0;snapshot.viewers[0].position_declared=false;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.volume==32,"matrix-origin magnitude<=source.01 yields invalid facing; no distance position read");
    snapshot=startup_sound_spatial_fixture::snapshot();snapshot.mode29=1;snapshot.viewers0c={0,1,2,3};snapshot.projection_declared=false;
    snapshot.viewers[0].position2e8={0,0,4000};snapshot.viewers[1].position2e8={0,0,120};snapshot.viewers[2].position2e8={0,0,60};snapshot.viewers[3].position2e8={0,0,21};
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.volume==125&&command.nearest_distance==100&&command.pan==64,"mode1 uses TWO nearest-positive viewers and bypasses pan projection");snapshot.mode29=2;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.volume==127&&command.nearest_distance==1,"mode2 uses FOURviewers");snapshot.override33=1;
    check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"override1 reduces to one but reenables required pan projection");snapshot.projection_declared=true;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.volume==32&&command.nearest_distance==3980,"single farviewer attenuation caps95 giving floor32");
    snapshot=startup_sound_spatial_fixture::snapshot();snapshot.viewport[2]=1280;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.screen_x==640&&command.pan==127,"pan clamp remains source640, not scaled to declared viewportwidth");
    snapshot=startup_sound_spatial_fixture::snapshot();snapshot.projection[0]=1;snapshot.projection[1]=.01f;owners[0].position20[0]=-101;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.pan==0,"orthographic negative screenX clamps0");owners[0].position20[0]=101;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.pan==127,"orthographic positive screenX clamps127");
    snapshot=startup_sound_spatial_fixture::snapshot();owners[0].position20={14.25f,1.5f,0};auto& m=snapshot.viewers[0].matrix364;m.at(0,0)=2;m.at(0,1)=.5f;m.at(0,2)=.25f;m.at(0,3)=10;m.at(1,1)=3;m.at(1,2)=.5f;m.at(1,3)=-5;m.at(2,2)=4;m.at(2,3)=20;
    snapshot.viewers[0].matrix394.at(2,3)=-100;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error)&&command.reflection_used&&close(command.pan_position[0],16.75f)&&close(command.pan_position[1],6.5f)&&close(command.pan_position[2],40),"behind reflection uses FULLaffine inverse incl shear/translation, not transpose/negateworldZ");
    snapshot=startup_sound_spatial_fixture::snapshot();owners[0]=startup_sound_spatial_fixture::owner();snapshot.viewers[0].matrix364.at(0,0)=0;
    check(startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"singular facing matrix is not inverted in FRONTpath");owners[0].position20[2]=5;
    check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"singular BEHIND inverse refuses no invented reflection");
    snapshot=startup_sound_spatial_fixture::snapshot();owners[0].position20[2]=30;
    check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"perspective Zzero refuses, not centered fallback");owners[0].position20[2]=10;
    check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"zero normalized view delta refuses instead of claiming hardwareNaN parity");
    snapshot=startup_sound_spatial_fixture::snapshot();owners[0]=startup_sound_spatial_fixture::owner();owners[0].position20[0]=std::numeric_limits<f32>::quiet_NaN();
    check(!startup_sound_spatial_resolve(p,call,snapshot,owners,&command,&error),"nonfinite source position refuses");owners[0]=startup_sound_spatial_fixture::owner();
    auto mr=startup_sound_fixture::profile();StartupSoundProfile manager_profile;check(startup_sound_load({mr.data(),mr.size()},&manager_profile,&error),"manager fixture");StartupSoundOperation operation;operation.owner=101;operation.tick_declared=true;StartupSoundPlan plan;
    check(startup_sound_plan(manager_profile,{},operation,&plan,&error),"pending manager start");StartupSoundSpatialBatch batch;
    check(startup_sound_spatial_resolve_batch(p,{plan.pending.data(),plan.pending_count},snapshot,owners,&batch,&error)&&batch.count==1&&batch.commands[0].bias_used,"ordered manager pending start resolves to scalar params, NOTtonequeue/voicecommit");
    std::array<StartupSoundPendingCall,2> calls={StartupSoundPendingCall{StartupSoundCallKind::StartDirect,40,0,0},call};owners[0].position_declared=false;
    check(!startup_sound_spatial_resolve_batch(p,calls,snapshot,owners,&batch,&error)&&batch.count==0,"late missing actor position discards earlier scalar command atomically");owners[0].position_declared=true;
    std::array<StartupSoundPendingCall,49> oversized{};check(!startup_sound_spatial_resolve_batch(p,oversized,snapshot,owners,&batch,&error),"batchbudget48 is NOToriginal32packetqueueadmission");
    const unsigned owned=checks;std::ifstream f("local/content/startup-sound-spatial/spatial.gfsp",std::ios::binary);std::vector<u8> source{std::istreambuf_iterator<char>(f),{}};
    if(!source.empty()){check(source==startup_sound_spatial_fixture::profile(),"loose original operands equal owned bits");check(startup_sound_spatial_load({source.data(),source.size()},&p,&error),"source profile");}
    std::printf("{\"startup_sound_spatial_checks\":%u,\"owned_checks\":%u,\"source_scene_producers_voice_PCM_or_actor_admission\":false,\"all_passed\":true}\n",checks,owned);
}
