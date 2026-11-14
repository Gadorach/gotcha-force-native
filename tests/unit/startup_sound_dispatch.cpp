#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_sound_dispatch_fixture.hpp"
#include "tests/unit/startup_sound_commands_fixture.hpp"
namespace {unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup sound dispatch: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace startup_sound_dispatch_fixture;std::string error;auto fresh=startup_sound_event_pool_fresh();
    check(startup_sound_dispatch_detail::pool_valid(fresh)&&fresh.free_count==64&&fresh.head==-1&&fresh.tail==-1,"source64-slot pool init");
    for(unsigned i=0;i<64;++i)check(fresh.free[i]==63-i&&fresh.events[i].slot10==i&&fresh.events[i].child14==-1,"source free stack reversed so allocation starts0, no blanket fullmemoryzero claim");
    StartupSoundCommandQueue staging;staging.count=3;staging.start_calls=22;for(unsigned i=0;i<3;++i)staging.packets[i]=packet();StartupSoundRing r;r.read=r.write=47;r.updates=0xffffffffu;StartupSoundTransferPlan transfer;
    check(startup_sound_ring_transfer(staging,r,&transfer,&error)&&transfer.forwarded==3&&transfer.ring.write==2&&transfer.ring.read==47&&transfer.ring.updates==0&&transfer.staging.count==0&&transfer.requires_eda40,"transferwrap/counterwrap/clearcount, explicit UNFULFILLEDEDA40tail");
    check(transfer.staging.packets==staging.packets&&transfer.staging.start_calls==22&&r.write==47&&staging.count==3,"partialsourceclear retainspacketstorage/startcounter and inputs");
    for(unsigned read=0;read<48;++read)for(unsigned write=0;write<48;++write){r={};r.read=read;r.write=write;const unsigned free=47-(write+48-read)%48;
        check(startup_sound_ring_transfer(staging,r,&transfer,&error)&&transfer.forwarded==std::min(3u,free)&&transfer.dropped==(free<3?3-free:0)&&transfer.ring.write==(write+std::min(3u,free))%48,"all2304ringpositions have47usable slots, preserveprefix onfull");}
    r={};r.write=47;staging.packets[0]=packet(0xe0);staging.packets[1]=packet();mask(staging.packets[2],0);
    check(startup_sound_ring_transfer(staging,r,&transfer,&error)&&transfer.skipped==1&&transfer.dropped==2&&transfer.first_dropped==1&&transfer.staging.count==0,"skipflag BEFOREfullcheck; full drops entireunreadsuffix includingunknownoperation");
    mask(staging.packets[0],0);check(!startup_sound_ring_transfer(staging,r,&transfer,&error)&&transfer.forwarded==0&&!transfer.requires_eda40,"needed skipflag unknown REFUSES, unlike knownfullcapacity outcome");
    staging={};r={};check(startup_sound_ring_transfer(staging,r,&transfer,&error)&&transfer.ring.updates==1&&transfer.requires_eda40,"ED058increments/callstail evenempty");
    auto start=packet();StartupSoundDispatchPlan plan;
    check(startup_sound_dispatch_prefix(ring({&start,1},47),fresh,&plan,&error)&&plan.count==1&&plan.ring.read==0&&plan.requires_event_voice_tail,"dispatchprefix takes snapshotwrite and wrapsread, NOTcompletedEDA40");
    auto pool=plan.pool;const auto& e=pool.events[0];
    check(e.allocated&&pool.head==0&&pool.tail==0&&pool.free_count==63&&e.previous0e==-1&&e.next12==-1&&e.busy03==1,"actual native eventslot0/LIFO/appendbusy retained untiltail");
    check(e.current04==96&&e.requested08==96&&e.phase0c==0&&e.dirty30==65535&&e.limit2c==32767&&e.limit2e==32767&&e.child14==-1,"EE9ACconstructor defaults inclSOURCEdirtyFFFF, notzero");
    check(e.flags34==1&&e.bank35==0&&e.tone36==40&&e.group37==5&&e.program38==1&&e.key39==27&&e.priority3c==100&&e.volume28==90&&e.pan29==64&&e.pitch2a==8192,"initializer consumesknown0..13, not unknown14..15");
    auto update=packet(0x57);update.bytes[8]=10;update.bytes[9]=30;store_be16(update.bytes.data()+10,1234);pool.events[0].dirty30=0x8000;
    check(startup_sound_dispatch_prefix(ring({&update,1}),pool,&plan,&error)&&plan.outcomes[0].matches==1&&plan.outcomes[0].allocated_slot==-1&&plan.pool.free_count==63&&plan.pool.events[0].volume28==10&&plan.pool.events[0].pan29==30&&plan.pool.events[0].pitch2a==1234&&plan.pool.events[0].dirty30==0x8007,"opcode57 qualifyingmatch updatesinstead ofallocating, dirtyORpreservesotherbits");
    update.bytes[8]=update.bytes[9]=255;store_be16(update.bytes.data()+10,65535);
    check(startup_sound_dispatch_prefix(ring({&update,1}),pool,&plan,&error)&&plan.outcomes[0].matches==1&&!plan.outcomes[0].changed&&plan.pool==pool,"ALLsentinels stillreturnMATCH, no falsefallbackallocation");
    check(startup_sound_dispatch_prefix(ring({&update,1}),fresh,&plan,&error)&&plan.outcomes[0].allocated_slot==0&&plan.pool.events[0].volume28==127&&plan.pool.events[0].pan29==64&&plan.pool.events[0].pitch2a==8192,"opcode57noqualifyingmatch FALLSBACKallocate withFF/FFFFdefaults");
    auto update_only=update;update_only.bytes[0]=0x47;
    check(startup_sound_dispatch_prefix(ring({&update_only,1}),fresh,&plan,&error)&&plan.pool==fresh&&plan.outcomes[0].allocated_slot==-1,"opcode47nevercreateswhenunmatched");
    pool.events[0].requested08=64;
    check(startup_sound_dispatch_prefix(ring({&update,1}),pool,&plan,&error)&&!plan.outcomes[0].matches&&plan.outcomes[0].allocated_slot==1&&plan.pool.events[0].requested08==16,"thresholdrequested08>64 STRICT;fallbacknewlowersoldrequest16withoutfreeingit");
    auto repeated=start;pool=fresh;
    for(unsigned i=0;i<64;++i){check(startup_sound_dispatch_prefix(ring({&repeated,1}),pool,&plan,&error)&&plan.outcomes[0].allocated_slot==s16(i),"equalpriority permitsreplacementandsequentialallocation");pool=plan.pool;}
    check(pool.free_count==0&&pool.head==0&&pool.tail==63&&pool.events[0].requested08==16,"supersededrequest loweringNOTrelease: all64slotsremainallocated");
    check(startup_sound_dispatch_prefix(ring({&start,1}),pool,&plan,&error)&&plan.outcomes[0].pool_exhausted&&plan.ring.read==plan.ring.write&&plan.pool==pool,"poolfulldropscreateandCONSUMESpacket, notguardfailure orautomaticrelease");
    auto low=start;low.bytes[13]=99;check(startup_sound_dispatch_prefix(ring({&low,1}),pool,&plan,&error)&&plan.outcomes[0].priority_blocked&&!plan.outcomes[0].pool_exhausted,"strictpriorityquery precedespoolallocation, includespendingstops");
    mask(low,0x200f);check(startup_sound_dispatch_prefix(ring({&low,1}),pool,&plan,&error)&&plan.outcomes[0].priority_blocked,"blockedprioritydoesnotreadunknowninitializerfields");
    low.bytes[13]=100;check(startup_sound_dispatch_prefix(ring({&low,1}),pool,&plan,&error)&&plan.outcomes[0].pool_exhausted,"exhaustedpooldoesnotreadinitializerfields");
    check(!startup_sound_dispatch_prefix(ring({&low,1}),fresh,&plan,&error)&&plan.count==0&&!plan.requires_event_voice_tail,"availableallocationneedsall0..13-known; nofakezeros");
    auto control=packet(0x37);mask(control,0x3f);
    check(startup_sound_dispatch_prefix(ring({&control,1}),pool,&plan,&error)&&plan.outcomes[0].matches==64&&plan.pool.events[63].requested08==32&&plan.pool.events[0].requested08==16&&plan.pool.free_count==0,"controlmatchesPROGRAM/key/group/bank, monotonicminimum32 notraiseold16, nofree");
    control.bytes[4]=2;check(startup_sound_dispatch_prefix(ring({&control,1}),pool,&plan,&error)&&plan.outcomes[0].matches==0,"controlprogrammismatchignoresSAMEtoneidentity");control.bytes[4]=129;
    check(startup_sound_dispatch_prefix(ring({&control,1}),pool,&plan,&error)&&plan.outcomes[0].matches==64,"controlprogram/keymaskis7bit, notfullbytecomparison");
    auto stop=packet(0x10);mask(stop,1);check(startup_sound_dispatch_prefix(ring({&stop,1}),pool,&plan,&error)&&plan.outcomes[0].matches==64&&plan.pool.events[63].requested08==16,"stopwithselector0broadcastswithoutreadingunknownidentitybytes");
    stop.bytes[0]=0x11;check(!startup_sound_dispatch_prefix(ring({&stop,1}),pool,&plan,&error)&&plan.count==0,"selectedgroup requiresknownidentitybyte");
    auto ignored=packet(0x70);mask(ignored,1);check(startup_sound_dispatch_prefix(ring({&ignored,1}),fresh,&plan,&error)&&plan.outcomes[0].ignored&&plan.pool==fresh,"unsupportedsourceopcode70isactualno-opconsumed, notinventedvoicecall");
    auto cancellation=start;cancellation.bytes[0]=0xe0;check(startup_sound_dispatch_prefix(ring({&cancellation,1}),fresh,&plan,&error)&&plan.outcomes[0].allocated_slot==0,"EE800mask70doesNOTrepeattransfer80skipcheckonexistingring");
    auto initializer=start;initializer.bytes[4]=250;initializer.bytes[12]=10;initializer.bytes[8]=initializer.bytes[9]=255;store_be16(initializer.bytes.data()+10,65535);
    check(startup_sound_dispatch_prefix(ring({&initializer,1}),fresh,&plan,&error)&&plan.pool.events[0].program38==4,"initializerprogram+keyoffsetbyteWRAP notclamp");
    std::array<StartupSoundPacket,2> late={start,start};mask(late[1],0);
    check(!startup_sound_dispatch_prefix(ring(late),fresh,&plan,&error)&&plan.count==0&&plan.pool.free_count==0&&!plan.requires_event_voice_tail,"lateunknownpacket clearsentireoutput androllsbackfirstallocation; inputfreshnotmodified");check(fresh.free_count==64&&!fresh.events[0].allocated,"inputpooluntouched");
    auto invalid=fresh;invalid.free[63]=invalid.free[62];check(!startup_sound_dispatch_prefix({},invalid,&plan,&error),"duplicatefreeentries refuse");invalid=pool;invalid.events[0].next12=0;
    check(!startup_sound_dispatch_prefix({},invalid,&plan,&error),"activecycle refuse");invalid=pool;invalid.head=-1;check(!startup_sound_dispatch_prefix({},invalid,&plan,&error),"inconsistentheadtail refuse ratherthanrepair sourcepool");
    auto raw=startup_sound_commands_fixture::tones(),pr=startup_sound_commands_fixture::profile(raw);StartupSoundCommandProfile profile;StartupSoundToneBank bank;StartupSoundCommandPlan commands;
    check(startup_sound_command_load({pr.data(),pr.size()},&profile,&error)&&startup_sound_tones_load({raw.data(),raw.size()},profile,&bank,&error),"ownedcommandinput");StartupSoundCommandObservation obs{true,true,0,4};StartupSoundScalarCommand scalar;scalar.cue=40;scalar.volume=127;scalar.pan=64;
    check(startup_sound_command_plan(profile,&bank,{},obs,{&scalar,1},&commands,&error)&&startup_sound_ring_transfer(commands.candidate,{},&transfer,&error)&&startup_sound_dispatch_prefix(transfer.ring,fresh,&plan,&error)&&plan.pool.events[0].tone36==40,"startup/UIsharedcommand→ring→eventprefix, tailNOTacknowledged");
    const unsigned owned=checks;auto read=[](const char* path){std::ifstream f(path,std::ios::binary);return std::vector<u8>{std::istreambuf_iterator<char>(f),{}};};auto real_profile=read("local/content/startup-sound-commands/commands.gfsc"),real_tones=read("local/content/startup-sound-commands/common01.tsb");
    if(!real_profile.empty()){check(startup_sound_command_load({real_profile.data(),real_profile.size()},&profile,&error)&&startup_sound_tones_load({real_tones.data(),real_tones.size()},profile,&bank,&error),"retained original common01 data");
        for(unsigned cue:{119u,124u,123u}){scalar.cue=s16(cue);check(startup_sound_command_plan(profile,&bank,{},obs,{&scalar,1},&commands,&error)&&startup_sound_ring_transfer(commands.candidate,{},&transfer,&error)&&startup_sound_dispatch_prefix(transfer.ring,fresh,&plan,&error)&&plan.pool.events[0].bank35==0&&plan.pool.events[0].tone36==cue,"original navigation/selection/cancel119/124/123 COMMAND-to-eventprefix, not playbackproof");}
    }
    std::printf("{\"startup_sound_dispatch_checks\":%u,\"owned_checks\":%u,\"source_slots\":64,\"ring_usable\":47,\"event_voice_tail_or_actor_admission\":false,\"all_passed\":true}\n",checks,owned);
}
