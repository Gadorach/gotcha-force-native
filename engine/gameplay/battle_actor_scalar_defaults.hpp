#pragma once
#include "engine/gameplay/startup_actor_setup.hpp"
#include "engine/assets/challenge_player_defaults.hpp"
#include <vector>
namespace gf {
struct BattleActorScalarDefaultWrite {u16 offset=0;u8 extent=0;u32 value=0;bool operator==(const BattleActorScalarDefaultWrite&)const=default;};
struct BattleActorScalarDefaults {
    f32 field5f4=0,field768=0,scale_b4=0,fieldc0=0,fieldc4=0;std::array<f32,3> actor_scale{};
    s16 field736=0,field738=0;ChallengePlayerDefaults player;StartupActorRankState rank;u8 upper74d=0,lower74c=0;
    std::vector<BattleActorScalarDefaultWrite> writes;
};
//56180 fresh ordinary zero489/48A/500 branch ONLY. No562B8 effects/reloads,
//55DCC dimensions, local4BB14, world, healing or actor readiness is implied.
inline bool battle_actor_scalar_defaults(const StartupActorSetupProfile& profile,const ChallengePlayerDefaults& player,BattleActorScalarDefaults* out,std::string* error){
    if(out)*out={};
    if(!out||!startup_actor_setup_detail::valid(profile)){if(error)*error="battle actor defaults: qualified same-model numeric profile required";return false;}
    BattleActorScalarDefaults next;next.field5f4=next.field768=profile.one;next.player=player;
    const auto data=startup_actor_setup_detail::data(profile);next.field736=next.field738=std::bit_cast<s16>(data.be16(4));next.upper74d=data.data[0xa1];next.lower74c=data.data[0xa2];
    if(!startup_actor_rank_select(profile,16,&next.rank)){if(error)*error="battle actor defaults: rank selection";return false;}
    next.scale_b4=next.fieldc0=next.fieldc4=next.rank.scale_b8;next.actor_scale.fill(next.scale_b4);
    auto put=[&](u16 at,u8 size,u32 value){next.writes.push_back({at,size,value});};auto fl=[&](u16 at,f32 v){put(at,4,f32_to_bits(v));};
    fl(0x5f4,next.field5f4);fl(0x768,next.field768);put(0x738,2,u16(next.field738));put(0x736,2,u16(next.field736));
    put(0x438,1,player.field20);put(0x439,1,player.field32);put(0x43a,1,player.field3e);put(0x74a,1,16);put(0x74d,1,next.upper74d);put(0x74c,1,next.lower74c);
    put(0x74b,1,next.rank.selected);fl(0xb8,next.rank.scale_b8);fl(0x1dd0,next.rank.field1dd0);fl(0x5f8,next.rank.field5f8);
    for(u16 at:{u16(0xb4),u16(0xc4),u16(0xc0),u16(0x60),u16(0x5c),u16(0x58)})fl(at,next.scale_b4);
    *out=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
