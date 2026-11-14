#pragma once
#include "engine/assets/story_map_navigation.hpp"
namespace gf {
struct StoryMapNavigationState {u8 current=10,previous=10;bool operator==(const StoryMapNavigationState&)const=default;};
struct StoryMapNavigationPlan {StoryMapNavigationState state{};bool changed=false;u16 sound=0;};
inline bool story_map_navigation_plan(const StoryMapNavigationProfile& profile,
                                     const StoryMapNavigationState& before,u32 available,u32 pressed,
                                     StoryMapNavigationPlan* out){
    if(out)*out={};
    if(!out||!profile.loaded||!profile.map_fingerprint||before.current>=kStoryMapNodes||
       before.previous>=kStoryMapNodes||(available&~0x7fffu))return false;
    //Recheck public metadata before using it. Mutable caller copies cannot
    //introduce shifts by negative/out-of-range node IDs or replace priority.
    constexpr std::array<u32,4> order{0x40004,0x80008,0x20002,0x10001};
    if(profile.directions!=order)return false;
    const auto& row=profile.neighbors[before.current];
    for(auto node:row)if(node < -1 || node>=s8(kStoryMapNodes))return false;
    StoryMapNavigationPlan next;next.state=before;
    for(unsigned direction=0;direction<4;++direction){
        if(!(pressed&order[direction]))continue;
        const s8 primary=row[direction],secondary=row[direction+4];
        s8 chosen=-1;
        //Source previous-location preference precedes both unlock/current-node
        //tests. Do not "repair" it by applying the availability filter first.
        if(primary==s8(before.previous))chosen=primary;
        else if(secondary==s8(before.previous))chosen=secondary;
        else if(primary>=0&&primary!=s8(before.current)&&(available&(1u<<unsigned(primary))))chosen=primary;
        else if(secondary>=0&&secondary!=s8(before.current)&&(available&(1u<<unsigned(secondary))))chosen=secondary;
        if(chosen>=0){
            next.state.previous=before.current;next.state.current=u8(chosen);
            next.changed=true;next.sound=119;break;
        }
        //A pressed but unavailable direction does NOT consume later directions.
    }
    *out=next;return true;
}
} //namespace gf
