#pragma once
#include <optional>
#include "engine/gameplay/challenge_initial_party.hpp"
namespace gf {
//Known consumed fields, NOT a 28-byte memory image. Unknown optional values stay
//unknown until their source branch reads them; word8 is NOT XP or acquisition.
struct BattleRosterMember {
    ChallengePartyMember selected;
    std::optional<u32> word8;
    std::optional<std::array<s16,3>> shorts16_18_20;
    std::optional<u8> flag22,flag24;
};
enum class BattleMemberWriteTarget:u8 {Actor,Roster};
struct BattleMemberWrite {BattleMemberWriteTarget target=BattleMemberWriteTarget::Actor;u16 offset=0,extent=0;u32 value=0;bool operator==(const BattleMemberWrite&)const=default;};
struct BattleMemberContext {
    std::optional<u8> common50,context1f;
    u8 selected84=255;
    std::span<const BattleRosterMember> members;
};
struct BattleMemberInitialization {
    u8 selected492=255,level3ee=0;u32 acquired3f4=0,experience3f8=0;
    std::optional<u32> roster_word8;
    std::optional<std::array<s16,3>> actor_shorts1c4_1c6_1c8;
    std::vector<BattleMemberWrite> writes;
};
//80068344, pure declared-input seam. Native construction must additionally bind
//the context/member to its exact current allocation; this plan is no capability.
inline bool battle_actor_member_initialize(const BattleMemberContext& c,BattleMemberInitialization* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle member: required branch-local field/index unknown or invalid";return false;};
    if(!out||!c.common50||c.members.size()>30||c.selected84>=c.members.size())return fail();
    const auto& row=c.members[c.selected84];if(!row.flag22)return fail();
    BattleMemberInitialization p;p.selected492=c.selected84;p.level3ee=row.selected.fields.level;p.acquired3f4=row.selected.fields.acquired;p.experience3f8=row.selected.fields.experience;
    using T=BattleMemberWriteTarget;auto put=[&](T target,u16 at,u16 extent,u32 value){p.writes.push_back({target,at,extent,value});};
    put(T::Actor,0x492,1,p.selected492);put(T::Actor,0x3ee,1,p.level3ee);put(T::Actor,0x3f4,4,p.acquired3f4);put(T::Actor,0x3f8,4,p.experience3f8);
    if(*c.common50==0){if(!c.context1f)return fail();if(*c.context1f==0){p.roster_word8=2;put(T::Roster,8,4,2);}}
    if(*row.flag22!=0){if(!row.shorts16_18_20)return fail();p.actor_shorts1c4_1c6_1c8=row.shorts16_18_20;for(unsigned i=0;i<3;++i)put(T::Actor,u16(0x1c4+2*i),2,u16((*row.shorts16_18_20)[i]));}
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
