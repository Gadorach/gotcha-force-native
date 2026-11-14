#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_sound_commands_fixture.hpp"
#include "tests/unit/startup_sound_spatial_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup sound commands: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(){
    using namespace gf;std::string error;auto raw=startup_sound_commands_fixture::tones(),pr=startup_sound_commands_fixture::profile(raw);StartupSoundCommandProfile p;StartupSoundToneBank bank;
    check(startup_sound_command_load({pr.data(),pr.size()},&p,&error)&&startup_sound_tones_load({raw.data(),raw.size()},p,&bank,&error),"owned source-gain/TSBD fixture");
    for(unsigned i=0;i<pr.size();++i)if(i<16||i>=20){auto bad=pr;bad[i]^=0x80;StartupSoundCommandProfile r;check(!startup_sound_command_load({bad.data(),bad.size()},&r,&error),"changed profile header/gain/reserved refuses");}
    auto badp=p;badp.tone_hash^=1;StartupSoundToneBank rejected;
    check(!startup_sound_tones_load({raw.data(),raw.size()},badp,&rejected,&error)&&!rejected.loaded,"tone FNV binding is enforced, not authentication");
    for(unsigned i=0;i<16;++i){auto bad=raw;bad[i]^=1;check(!startup_sound_tones_load({bad.data(),bad.size()},p,&rejected,&error),"malformed TSB/binding");}
    auto altered=bank;altered.tones[40][8]^=1;check(!startup_sound_tones_valid(p,altered),"mutable metadata cannot retain old identity");
    StartupSoundCommandQueue empty;StartupSoundCommandObservation observation{true,true,0,4};StartupSoundScalarCommand call;call.cue=40;call.volume=127;call.pan=63;StartupSoundCommandPlan plan;
    check(startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.count==1&&plan.candidate.start_calls==1,"eligible source start counter and packet append");
    const auto packet=plan.candidate.packets[0];check(load_be32(packet.bytes.data())==0x60002885&&packet.known==0x3fff,"start opcode6/identity, only14known bytes NOTfull16byte equality");
    check(packet.bytes[4]==1&&packet.bytes[5]==27&&packet.bytes[8]==90&&packet.bytes[9]==63&&load_be16(packet.bytes.data()+10)==8192&&packet.bytes[13]==100,"tone fields/gain/pan/pitch encoding");
    check(empty==StartupSoundCommandQueue{}&&packet.bytes[14]==0&&packet.bytes[15]==0&&!(packet.known&0xc000),"canonical unknown values are masked NOTclaimedoriginalzeros");
    call.actor96=5;call.update=true;
    check(startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error)&&load_be32(plan.candidate.packets[0].bytes.data())==0x570028ad&&plan.candidate.start_calls==0,"EFCB8 update opcode57, owner96*8 added into low7identity");
    call.actor96=0;call.cue=41;
    check(startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.packets[0].known==0x3f&&load_be32(plan.candidate.packets[0].bytes.data())==0x37002985,"control cue41opcode37 onlysixknown bytes even for update");
    call.cue=40;call.update=false;observation.sfx_index=0;
    check(startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.packets[0].bytes[8]==53,"F32settings127*.6 trunc76 BEFOREtone90/127 trunc53");
    auto chained=raw;chained[16+40*16+15]=42;auto* second=chained.data()+16+42*16;const auto* first=chained.data()+16+40*16;for(unsigned i=0;i<16;++i)second[i]=first[i];second[8]=64;second[9]=80;store_be16(second+10,100);second[15]=43;
    auto* control=chained.data()+16+43*16;for(unsigned i=0;i<16;++i)control[i]=raw[16+41*16+i];
    auto cp=startup_sound_commands_fixture::profile(chained);StartupSoundCommandProfile chain_profile;StartupSoundToneBank chain_bank;
    check(startup_sound_command_load({cp.data(),cp.size()},&chain_profile,&error)&&startup_sound_tones_load({chained.data(),chained.size()},chain_profile,&chain_bank,&error),"owned linked tone descriptors");observation.sfx_index=4;
    check(startup_sound_command_plan(chain_profile,&chain_bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.count==3&&plan.outcomes[0].visited==3,"chain order threepackets");
    const auto& middle=plan.candidate.packets[1];const auto& last=plan.candidate.packets[2];
    check(middle.bytes[8]==45&&middle.bytes[9]==79&&load_be16(middle.bytes.data()+10)==8292,"volume/pan/pitch accumulate INPLACE across chain, not reset from original arguments");
    check(last.known==0x3fff&&last.bytes[8]==45&&last.bytes[9]==79&&load_be16(last.bytes.data()+10)==8292,"control inherits same-invocation known scratch tail");
    call.cue=43;check(startup_sound_command_plan(chain_profile,&chain_bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.packets[0].known==0x3f,"new invocation never inherits prior call's scratch tail");call.cue=40;
    StartupSoundCommandQueue almost;almost.count=31;almost.start_calls=0xffffffffu;
    check(startup_sound_command_plan(chain_profile,&chain_bank,almost,observation,{&call,1},&plan,&error)&&plan.candidate.count==32&&plan.outcomes[0].added==1&&plan.outcomes[0].visited==2&&plan.outcomes[0].full&&plan.candidate.start_calls==0,"queue full retains admitted chainprefix/stops on second packet, start counter wraps");
    std::array<StartupSoundScalarCommand,3> calls={call,call,call};calls[1].cue=41;calls[2].update=true;
    check(startup_sound_command_plan(p,&bank,almost,observation,calls,&plan,&error)&&plan.candidate.count==32&&plan.outcomes[1].full&&plan.outcomes[2].full&&plan.count==3&&plan.candidate.start_calls==1,"outercalls continue after queuefull; everyeligibleSTARTincrements, UPDATEnot");
    StartupSoundCommandQueue full;full.count=32;call.cue=0;
    check(startup_sound_command_plan(p,&bank,full,observation,{&call,1},&plan,&error)&&plan.candidate.count==32&&!plan.outcomes[0].full&&plan.candidate.start_calls==1,"FFno-op terminates without capacityattempt, still countsstart");
    auto bad_chain=chained;bad_chain[16+42*16+12]=1;auto bp=startup_sound_commands_fixture::profile(bad_chain);StartupSoundCommandProfile bad_profile;StartupSoundToneBank bad_bank;
    check(startup_sound_command_load({bp.data(),bp.size()},&bad_profile,&error)&&startup_sound_tones_load({bad_chain.data(),bad_chain.size()},bad_profile,&bad_bank,&error),"randomized metadata parses but not admitted for execution");call.cue=40;
    check(!startup_sound_command_plan(bad_profile,&bad_bank,empty,observation,{&call,1},&plan,&error)&&plan.count==0&&plan.candidate==StartupSoundCommandQueue{},"late randomizedtone failure rolls back provisional prefix, NOTsourcefull drop");
    call.cue=42;check(!startup_sound_command_plan(bad_profile,&bad_bank,full,observation,{&call,1},&plan,&error),"random dependency occurs BEFOREcapacitycheck, not bypassed when full");call.cue=40;
    auto cyclic=raw;cyclic[16+40*16+15]=40;auto cycpr=startup_sound_commands_fixture::profile(cyclic);StartupSoundCommandProfile cycp;StartupSoundToneBank cycb;
    check(startup_sound_command_load({cycpr.data(),cycpr.size()},&cycp,&error)&&startup_sound_tones_load({cyclic.data(),cyclic.size()},cycp,&cycb,&error),"cycle metadata retained");
    check(!startup_sound_command_plan(cycp,&cycb,empty,observation,{&call,1},&plan,&error)&&plan.count==0,"cyclic reachable chain refuses bounded subset, no forced originalterminationclaim");
    auto limit=raw;limit[16+40*16+15]=128;auto lp=startup_sound_commands_fixture::profile(limit);StartupSoundCommandProfile limitp;StartupSoundToneBank limitb;
    check(startup_sound_command_load({lp.data(),lp.size()},&limitp,&error)&&startup_sound_tones_load({limit.data(),limit.size()},limitp,&limitb,&error),"indexlimit fixture");
    check(startup_sound_command_plan(limitp,&limitb,empty,observation,{&call,1},&plan,&error)&&plan.outcomes[0].index_limit&&plan.candidate.count==1,"source tone>=128 exits with earlier admittedprefix, no invalid dereference");
    observation={true,false,1,-1};call.cue=-1;check(startup_sound_command_plan(p,nullptr,empty,observation,{&call,1},&plan,&error)&&plan.outcomes[0].ignored&&plan.candidate.start_calls==0,"disabledgate no settings/bank/scalarreads");
    observation.disabled=0;call.cue=384;check(startup_sound_command_plan(p,nullptr,empty,observation,{&call,1},&plan,&error)&&plan.outcomes[0].ignored,"uppercueguard before settings/bank");call.cue=-1;
    check(!startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error),"enablednegativecue refuses unsafe source index domain");call.cue=40;
    observation={true,true,0,5};check(!startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error),"only sourcegainindices0..4");observation.sfx_index=4;call.cue=128;
    check(!startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error),"bank1/2 not silently redirected to common01");call.cue=40;observation.gate_declared=false;
    check(!startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error),"missing gate");observation.gate_declared=true;
    using namespace startup_sound_commands_detail;Parameters params;params.tone=40;params.update=true;params.replace=false;params.volume=params.pan=params.pitch=-1;StartupSoundCommandQueue q;StartupSoundCommandOutcome outcome;
    check(chain(bank,params,&q,&outcome,&error)&&load_be32(q.packets[0].bytes.data())==0x47002885&&q.packets[0].bytes[8]==255&&q.packets[0].bytes[9]==255&&load_be16(q.packets[0].bytes.data()+10)==65535,"bounded lower F1238 param8zero opcode47, sentinelFF/FFFF preserved in update");
    params.update=false;q={};outcome={};check(chain(bank,params,&q,&outcome,&error)&&q.packets[0].bytes[8]==90&&q.packets[0].bytes[9]==64&&load_be16(q.packets[0].bytes.data()+10)==8192,"start sentinels use tone/defaultvalues unlike update");
    q={};outcome={};params.tone=41;params.key_offset=2;check(chain(bank,params,&q,&outcome,&error)&&q.packets[0].bytes[4]==3,"control program byte includes keyoffset modulo256");
    auto invalid=empty;invalid.count=33;check(!startup_sound_command_plan(p,&bank,invalid,observation,{&call,1},&plan,&error),"queuecapacity domain");invalid={};invalid.packets[31].bytes[15]=1;
    check(!startup_sound_command_plan(p,&bank,invalid,observation,{&call,1},&plan,&error),"unknownbytes canonicalzero representation guard");
    auto spr=startup_sound_spatial_fixture::profile();StartupSoundSpatialProfile sp;check(startup_sound_spatial_load({spr.data(),spr.size()},&sp,&error),"spatialfixture");auto snapshot=startup_sound_spatial_fixture::snapshot();std::array<StartupSoundOwnerObservation,1> owners={startup_sound_spatial_fixture::owner()};StartupSoundPendingCall pending{StartupSoundCallKind::StartBiased,40,101,sp.bias};
    check(startup_sound_spatial_resolve(sp,pending,snapshot,owners,&call,&error)&&startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.count==1,"pending spatial→scalar→candidatepacket, stillNOvoice/PCM/actorcommit");
    const unsigned owned=checks;auto source_profile=read("local/content/startup-sound-commands/commands.gfsc"),source_tones=read("local/content/startup-sound-commands/common01.tsb");
    if(!source_profile.empty()){check(startup_sound_command_load({source_profile.data(),source_profile.size()},&p,&error)&&startup_sound_tones_load({source_tones.data(),source_tones.size()},p,&bank,&error),"original loose normal common01 tones/sourcegains");
        call={};call.cue=40;call.volume=127;call.pan=64;check(startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.packets[0].bytes[8]==90,"actual cue40 startmetadata");call.cue=41;check(startup_sound_command_plan(p,&bank,empty,observation,{&call,1},&plan,&error)&&plan.candidate.packets[0].known==63,"actual cue41 controlmetadata NOTsecondPCM sample");
    }
    std::printf("{\"startup_sound_command_checks\":%u,\"owned_checks\":%u,\"tone_queue_candidate_only\":true,\"voice_PCM_or_actor_admission\":false,\"all_passed\":true}\n",checks,owned);
}
