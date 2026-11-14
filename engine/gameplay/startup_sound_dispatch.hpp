#pragma once
// ED058 transport prefix + EE800/EEC5C/EED24/EEE10/EEAA8/EF1A8/EE9AC/EEB84.
// Source 48-slot ring/64 event slots; EDA40 event/voice tail NOT executed.
// Pure candidates retain UNKNOWN packet bytes and do not acknowledge playback.
#include "engine/gameplay/startup_sound_commands.hpp"
namespace gf {
struct StartupSoundRing {
    std::array<StartupSoundPacket,48> packets{};u32 read=0,write=0,updates=0;
    bool operator==(const StartupSoundRing&)const=default;
};
struct StartupSoundTransferPlan {
    StartupSoundCommandQueue staging;StartupSoundRing ring;
    unsigned forwarded=0,skipped=0,dropped=0,first_dropped=32;bool requires_eda40=false;
};
struct StartupSoundEvent {
    bool allocated=false;u8 flag01=0,flag02=0,busy03=0;
    s32 current04=0,requested08=0;u16 phase0c=0;s16 previous0e=-1;u16 slot10=0;s16 next12=-1,child14=-1;
    u8 volume28=0,pan29=0;u16 pitch2a=0,limit2c=0,limit2e=0,dirty30=0;
    u8 flags34=0,bank35=0,tone36=0,group37=0,program38=0,key39=0;u16 flags3a=0;u8 priority3c=0;
    u16 count58=0,wait5a=0;
    bool operator==(const StartupSoundEvent&)const=default;
};
struct StartupSoundEventPool {
    std::array<StartupSoundEvent,64> events{};std::array<u8,64> free{};u32 free_count=0;s16 head=-1,tail=-1;
    bool operator==(const StartupSoundEventPool&)const=default;
};
inline StartupSoundEventPool startup_sound_event_pool_fresh(){
    // ED1CC's pool ordering/links/self slots, not a full source memory memset.
    StartupSoundEventPool p;p.free_count=64;for(unsigned i=0;i<64;++i){p.free[i]=u8(63-i);p.events[i].slot10=u16(i);}return p;
}
struct StartupSoundDispatchOutcome {
    u8 operation=0;unsigned matches=0,changed=0,lowered_others=0;s16 allocated_slot=-1;
    bool priority_blocked=false,pool_exhausted=false,ignored=false;
};
struct StartupSoundDispatchPlan {
    StartupSoundRing ring;StartupSoundEventPool pool;std::array<StartupSoundDispatchOutcome,47> outcomes{};unsigned count=0;
    bool requires_event_voice_tail=false;
};
namespace startup_sound_dispatch_detail {
inline bool packet_valid(const StartupSoundPacket& p){for(unsigned i=0;i<16;++i)if(!(p.known&(1u<<i))&&p.bytes[i])return false;return true;}
inline bool ring_valid(const StartupSoundRing& r){if(r.read>=48||r.write>=48)return false;for(const auto& p:r.packets)if(!packet_valid(p))return false;return true;}
inline bool known(const StartupSoundPacket& p,u16 mask){return (p.known&mask)==mask;}
inline bool pool_valid(const StartupSoundEventPool& p){
    if(p.free_count>64||p.head<-1||p.head>=64||p.tail<-1||p.tail>=64||(p.head==-1)!=(p.tail==-1))return false;
    std::array<bool,64> seen{};s16 previous=-1,current=p.head;unsigned active=0;
    while(current!=-1){
        if(current<0||current>=64||seen[unsigned(current)]||++active>64)return false;
        const auto& e=p.events[unsigned(current)];if(!e.allocated||e.slot10!=u16(current)||e.previous0e!=previous||e.bank35>127||e.tone36>127||e.group37>127||e.flags34>7)return false;
        seen[unsigned(current)]=true;previous=current;current=e.next12;
    }
    if(previous!=p.tail||active+p.free_count!=64)return false;
    for(unsigned i=0;i<p.free_count;++i){const unsigned id=p.free[i];if(id>=64||seen[id])return false;const auto& e=p.events[id];if(e.allocated||e.slot10!=id||e.previous0e!=-1||e.next12!=-1||e.child14!=-1)return false;seen[id]=true;}
    return true;
}
inline u16 identity_bytes(u8 selection){return u16(1u|((selection&4u)?2u:0u)|((selection&2u)?4u:0u)|((selection&1u)?8u:0u));}
inline u32 selection_mask(u8 selection){return ((selection&4u)?0x7f0000u:0u)|((selection&2u)?0x7f00u:0u)|((selection&1u)?0x7fu:0u);}
inline u32 identity(const StartupSoundEvent& e){return u32(e.bank35)<<16|u32(e.tone36)<<8|e.group37;}
inline u8 flags(u32 word){return u8((word>>7&1u)|(word>>21&4u)|(word>>14&2u));}
inline bool same_group(const StartupSoundEvent& e,u32 word,u8 f){
    const u32 mask=0x7f0000u|((f&1u)?0x7fu:0u)|((f&2u)?0x7f00u:0u);return (identity(e)&mask)==(word&mask);
}
inline bool process(const StartupSoundPacket& packet,StartupSoundEventPool& pool,StartupSoundDispatchOutcome* outcome,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound dispatch: ")+why;return false;};
    if(!known(packet,1))return fail("operation byte unknown");
    const u8 operation=packet.bytes[0]&0x70u;outcome->operation=operation;
    if(operation==0||operation==0x70){outcome->ignored=true;return true;}
    const u8 selection=packet.bytes[0]&7u;const u32 word=load_be32(packet.bytes.data());
    if(operation==0x10||operation==0x20){
        if(pool.head==-1)return true;
        if(!known(packet,identity_bytes(selection)))return fail("selected stop identity unknown");
        const auto mask=selection_mask(selection);const s32 target=operation;
        for(s16 id=pool.head;id!=-1;id=pool.events[unsigned(id)].next12){auto& e=pool.events[unsigned(id)];if((word&mask)==(identity(e)&mask)){++outcome->matches;if(target<e.requested08){e.requested08=target;++outcome->changed;}}}
        return true;
    }
    if(operation==0x30){
        if(pool.head==-1)return true;
        // EED24 always matches bank, conditionally group/program/key; no tail read.
        const u16 required=u16(3u|((selection&1u)?8u:0u)|((selection&2u)?0x30u:0u));
        if(!known(packet,required))return fail("selected control fields unknown");
        const u32 mask=((selection&1u)?0x7fu:0u)|((selection&2u)?0x7f7f00u:0u);
        const u32 control=u32(packet.bytes[4])<<16|u32(packet.bytes[5])<<8|(word&127u);
        for(s16 id=pool.head;id!=-1;id=pool.events[unsigned(id)].next12){auto& e=pool.events[unsigned(id)];const u32 key=u32(e.program38)<<16|u32(e.key39)<<8|e.group37;
            if(e.bank35==((word>>16)&127u)&&(control&mask)==(key&mask)){++outcome->matches;if(0x20<e.requested08){e.requested08=0x20;++outcome->changed;}}
        }
        return true;
    }
    if(operation==0x40||operation==0x50){
        // EE800 loads8..11 before EEE10, even if its active list is empty.
        if(!known(packet,0x0f01))return fail("update scalars unknown");
        if(pool.head!=-1&&!known(packet,identity_bytes(selection)))return fail("selected update identity unknown");
        const u32 mask=selection_mask(selection);const u8 volume=packet.bytes[8],pan=packet.bytes[9];const u16 pitch=load_be16(packet.bytes.data()+10);
        for(s16 id=pool.head;id!=-1;id=pool.events[unsigned(id)].next12){auto& e=pool.events[unsigned(id)];if((word&mask)!=(identity(e)&mask)||e.requested08<=0x40)continue;
            ++outcome->matches;bool write=false;
            if(volume!=255){e.volume28=volume;e.dirty30|=1;write=true;}if(pan!=255){e.pan29=pan;e.dirty30|=2;write=true;}if(pitch!=65535){e.pitch2a=pitch;e.dirty30|=4;write=true;}outcome->changed+=write;
        }
        if(operation==0x40||outcome->matches)return true;
    }
    // Opcode60, or opcode50 with EEE10 return0: priority query then allocation.
    if(!known(packet,0x200f))return fail("create identity/priority unknown");
    const u8 f=flags(word),priority=packet.bytes[13];
    if(f)for(s16 id=pool.head;id!=-1;id=pool.events[unsigned(id)].next12){const auto& e=pool.events[unsigned(id)];if(same_group(e,word,f)&&priority<e.priority3c){outcome->priority_blocked=true;return true;}}
    if(!pool.free_count){outcome->pool_exhausted=true;return true;}
    if(!known(packet,0x3fff))return fail("allocated event initializer fields unknown");
    const auto id=pool.free[--pool.free_count];auto& e=pool.events[id];e.busy03=1;
    if(pool.head==-1){pool.head=pool.tail=s16(id);}else{pool.events[unsigned(pool.tail)].next12=s16(id);e.previous0e=pool.tail;pool.tail=s16(id);}
    e.allocated=true;e.flag01=e.flag02=0;e.phase0c=0;e.current04=e.requested08=0x60;e.limit2c=e.limit2e=0x7fff;e.dirty30=0xffff;
    e.volume28=packet.bytes[8]==255?127:packet.bytes[8];e.pan29=packet.bytes[9]==255?64:packet.bytes[9];const u16 pitch=load_be16(packet.bytes.data()+10);e.pitch2a=pitch==65535?8192:pitch;
    e.flags34=f;e.bank35=u8(word>>16&127u);e.tone36=u8(word>>8&127u);e.group37=u8(word&127u);e.program38=u8(u32(packet.bytes[4])+packet.bytes[12]);e.key39=packet.bytes[5];e.flags3a=load_be16(packet.bytes.data()+6);e.priority3c=priority;e.count58=e.wait5a=0;
    outcome->allocated_slot=s16(id);
    if(f)for(s16 other=pool.head;other!=-1;other=pool.events[unsigned(other)].next12){auto& previous=pool.events[unsigned(other)];if(other!=s16(id)&&same_group(previous,word,f)&&0x10<previous.requested08){previous.requested08=0x10;++outcome->lowered_others;}}
    return true;
}
} // namespace startup_sound_dispatch_detail
inline bool startup_sound_ring_transfer(const StartupSoundCommandQueue& staging,const StartupSoundRing& ring,StartupSoundTransferPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound transfer: ")+why;return false;};
    using namespace startup_sound_dispatch_detail;
    if(!out||!startup_sound_commands_detail::queue_valid(staging)||!ring_valid(ring))return fail("staging/ring domain");
    StartupSoundTransferPlan plan;plan.staging=staging;plan.ring=ring;
    for(unsigned i=0;i<staging.count;++i){const auto& p=staging.packets[i];if(!known(p,1))return fail("source skip flag unknown");
        if(p.bytes[0]&128u){++plan.skipped;continue;}
        if(plan.ring.write==(ring.read+47u)%48u){plan.dropped=staging.count-i;plan.first_dropped=i;break;}
        plan.ring.packets[plan.ring.write]=p;plan.ring.write=(plan.ring.write+1u)%48u;++plan.forwarded;
    }
    plan.staging.count=0;plan.ring.updates+=1u;plan.requires_eda40=true;*out=plan;if(error)error->clear();return true;
}
inline bool startup_sound_dispatch_prefix(const StartupSoundRing& ring,const StartupSoundEventPool& pool,StartupSoundDispatchPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound dispatch prefix: ")+why;return false;};
    using namespace startup_sound_dispatch_detail;
    if(!out||!ring_valid(ring)||!pool_valid(pool))return fail("ring/pool topology domain");
    StartupSoundDispatchPlan plan;plan.ring=ring;plan.pool=pool;const auto end=ring.write;
    while(plan.ring.read!=end){if(plan.count>=47)return fail("ring budget");
        if(!process(plan.ring.packets[plan.ring.read],plan.pool,&plan.outcomes[plan.count],error))return false;
        ++plan.count;plan.ring.read=(plan.ring.read+1u)%48u;
    }
    if(!pool_valid(plan.pool))return fail("derived pool topology");
    plan.requires_event_voice_tail=true;*out=plan;if(error)error->clear();return true;
}
} // namespace gf
