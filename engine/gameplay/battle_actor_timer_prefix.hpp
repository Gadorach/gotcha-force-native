#pragma once
#include <vector>
#include "engine/assets/battle_actor_timers.hpp"
#include "engine/gameplay/source_byte_rng.hpp"
namespace gf {
struct BattleActorInitFieldWrite {u16 offset=0;u8 bytes=0;u16 value=0;bool operator==(const BattleActorInitFieldWrite&)const=default;};
struct BattleActorTimerPrefixInput {
    bool declared=false;u16 model=0;u8 selector438=0,level439=0;s16 field1c6=0;
    bool override_enabled=false,override_fields_declared=false;
    std::array<u8,5> override_fields{}; //source per-slot339/352/37F/1E9/1EA
};
struct BattleActorTimerPrefixPlan {
    std::vector<BattleActorInitFieldWrite> writes;std::array<u8,6> draws{};SourceByteRngState after;
    bool target_selection_pending=false,suffix_pending=false;
};
//541AC→1C80C through263EC, BEFORE2532C's target/peer/RNG consumers. Writes are
//ordered candidates, not a source actor image, current owner or committed RNG.
//The two admitted models take263EC's no-write return. No guessed target, fake
//registry miss, fabricated peer list, or seeded/restarted global random stream.
inline bool battle_actor_timer_prefix(const BattleActorTimerProfile& profile,const BattleActorTimerPrefixInput& in,SourceByteRngState rng,BattleActorTimerPrefixPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle actor timer prefix: declared two-normal fields/table domain required";return false;};
    if(!out||!profile.ready()||!in.declared||(in.model!=0x615&&in.model!=10)||in.selector438>=18||in.level439>=16||(in.override_enabled&&!in.override_fields_declared))return fail();
    BattleActorTimerPrefixPlan next;next.after=rng;
    auto put=[&](unsigned offset,unsigned bytes,u16 value){next.writes.push_back({u16(offset),u8(bytes),value});};
    for(unsigned at:{0x2e4u,0x2e5u,0x2e6u,0x2e7u,0x32cu,0x32du,0x337u,0x338u,0x336u,0x347u})put(at,1,0);
    for(unsigned at:{0x33eu,0x340u,0x33au})put(at,2,0);
    next.draws[0]=source_byte_rng_step(next.after);put(0x33c,2,next.draws[0]);put(0x308,2,0);put(0x328,2,0);put(0x30f,1,0);
    constexpr std::array<unsigned,5> targets{0x32e,0x320,0x322,0x324,0x326};
    for(unsigned i=0;i<targets.size();++i){next.draws[i+1]=source_byte_rng_step(next.after);s16 timer=0;if(!profile.value(i,in.selector438,in.level439,next.draws[i+1]&7u,&timer))return fail();put(targets[i],2,std::bit_cast<u16>(timer));}
    for(unsigned at:{0x346u,0x335u,0x366u,0x367u})put(at,1,0);
    put(0x368,2,0);put(0x36a,2,0);
    for(unsigned at:{0x373u,0x374u,0x375u,0x377u,0x37eu,0x38du,0x38eu,0x38fu})put(at,1,0);
    put(0x37c,2,std::bit_cast<u16>(in.field1c6));put(0x30a,1,0);put(0x30b,1,0);
    constexpr std::array<unsigned,5> overrides{0x339,0x352,0x37f,0x1e9,0x1ea};
    for(unsigned i=0;i<overrides.size();++i)put(overrides[i],1,in.override_enabled?in.override_fields[i]:0);
    if(!in.override_enabled&&in.level439==0)put(0x339,1,0x10);
    next.target_selection_pending=next.suffix_pending=true;*out=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
