#pragma once
#include "engine/gameplay/solid_pair_displacement.hpp"

namespace gf {
struct SolidDisplacementActor {
    std::array<f32,3> position{},accumulated1cc{};
    u8 active00=0,links1dc=0,moved1dd=0;
    bool operator==(const SolidDisplacementActor&)const=default;
};
struct SolidDisplacementSnapshot {
    std::array<SolidDisplacementActor,6> slots{};
    bool declared=false;
    bool operator==(const SolidDisplacementSnapshot&)const=default;
};
struct SolidDisplacementApplication {
    SolidDisplacementSnapshot snapshot;
    // Each 35CA4 call can write its target, six direct links, and 36
    // second-level links. Repeated slots are significant, including order.
    std::array<u8,2*(1+6+36)> write_order{};
    unsigned writes=0;
    bool operator==(const SolidDisplacementApplication&)const=default;
};

namespace solid_displacement_detail {
inline bool step(SolidDisplacementApplication& app,u8 target,const std::array<f32,3>& delta){
    using namespace source_intersection_detail;
    auto& slots=app.snapshot.slots;
    const auto write=[&](u8 index){
        auto& actor=slots[index];
        const auto base=sub(actor.position,actor.accumulated1cc);
        for(unsigned k=0;k<3;++k){
            actor.accumulated1cc[k]=f32(actor.accumulated1cc[k]+delta[k]);
            actor.position[k]=f32(base[k]+actor.accumulated1cc[k]);
        }
        if(!valid(actor.position)||!valid(actor.accumulated1cc)||app.writes>=app.write_order.size())return false;
        actor.moved1dd|=1;
        app.write_order[app.writes++]=index;
        return true;
    };
    if(!write(target))return false;
    const u8 links=slots[target].links1dc;
    std::array<bool,6> direct{};
    for(u8 i=0;i<6;++i)if(slots[i].active00&&(links&(1u<<i))){
        if(!write(i))return false;
        direct[i]=true;
    }
    for(u8 i=0;i<6;++i)if(direct[i]){
        for(u8 j=0;j<6;++j)if(slots[j].active00&&(slots[i].links1dc&(1u<<j))){
            if(!write(j))return false;
        }
    }
    return true;
}
}

// 35CA4's bounded write order on an explicitly supplied six-slot snapshot.
// This does not infer the link masks, resolve actual owners, or publish a live
// solid response. The caller must protect the actual actor/slot lifetime.
inline bool solid_pair_apply_displacements(const SolidDisplacementSnapshot& before,const std::array<u8,2>& owners,
                                           const SolidPairDisplacementPlan& plan,SolidDisplacementApplication* out,
                                           std::string* error){
    using namespace source_intersection_detail;
    auto fail=[&](const char* why){if(error)*error=std::string("solid displacement application: ")+why;return false;};
    if(!out||!before.declared||owners[0]>=6||owners[1]>=6||owners[0]==owners[1]||
       !before.slots[owners[0]].active00||!before.slots[owners[1]].active00)
        return fail("distinct active owners in a declared snapshot required");
    if(plan.count>2||(plan.count==2&&(plan.steps[0].target!=SolidPairSide::Second||plan.steps[1].target!=SolidPairSide::First)))
        return fail("source displacement order required");
    for(const auto& actor:before.slots)if(actor.active00&&(!valid(actor.position)||!valid(actor.accumulated1cc)))
        return fail("active slot has invalid numeric state");
    SolidDisplacementApplication next;next.snapshot=before;
    for(unsigned i=0;i<plan.count;++i){
        const auto& s=plan.steps[i];const unsigned side=unsigned(s.target);
        if(side>=2||!valid(s.delta)||!solid_displacement_detail::step(next,owners[side],s.delta))
            return fail("displacement outside numeric or owner domain");
    }
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
