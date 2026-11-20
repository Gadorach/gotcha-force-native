#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_ko_prefix.hpp"

namespace gf {
struct GRedSasukeKoObserverEvent {
    s16 code=0,subject3e4=0;
    bool operator==(const GRedSasukeKoObserverEvent&)const=default;
};
struct GRedSasukeKoObserverSlot {
    u8 active00=0,team88=255,slot3e4=255,cursor484=0,ally_loss437=0;
    u32 ally_loss_cost430=0;
    std::array<GRedSasukeKoObserverEvent,8> queue464{};
    bool operator==(const GRedSasukeKoObserverSlot&)const=default;
};
struct GRedSasukeKoObserverSnapshot {
    bool six_physical_slots_declared=false;
    std::array<GRedSasukeKoObserverSlot,6> slots{};
};
struct GRedSasukeKoObserverPlan {
    std::array<GRedSasukeKoObserverSlot,6> slots{};
    u8 queue_write_mask=0,ally_stat_mask=0;
    u8 credit_slot=255,receiver_slot=255;
};
// 2FB14..2FC14, exact ordinary mode-zero opposing pair. A declared
// physical-slot queue snapshot is not a live actor or a notification.
// This only proposes the writer, not consumption or lifecycle changes.
inline bool normal_gred_beam_sasuke_ko_observers(NormalGRedNeutralBSource source,
    const GRedBeamSasukeKoPlan& ko,const GRedSasukeKoObserverSnapshot& in,GRedSasukeKoObserverPlan* out){
    if(!out||!source.reviewed()||!ko.observer_scan_pending||ko.credit_slot!=source.slot||
       ko.receiver_slot>=6||ko.receiver_slot==source.slot||ko.receiver_cost4aa<0||!in.six_physical_slots_declared||
       in.slots[ko.credit_slot].team88!=0||in.slots[ko.receiver_slot].team88!=1)return false;
    // The source compares record addresses, but queues the separate 3E4 byte.
    const u8 subject=in.slots[ko.receiver_slot].slot3e4;
    if(subject>=in.slots.size())return false;
    GRedSasukeKoObserverPlan next;next.slots=in.slots;
    next.credit_slot=ko.credit_slot;next.receiver_slot=ko.receiver_slot;
    for(unsigned i=0;i<next.slots.size();++i){
        auto& observer=next.slots[i];
        if(!observer.active00||i==ko.receiver_slot)continue;
        if(observer.team88>1||observer.cursor484>=observer.queue464.size())return false;
        const bool ally=observer.team88==1;
        observer.queue464[observer.cursor484]={s16(ally?4:3),s16(subject)};
        observer.cursor484=u8((observer.cursor484+1u)%observer.queue464.size());
        const u8 bit=u8(1u<<i);next.queue_write_mask|=bit;
        if(ally){
            observer.ally_loss437=u8(observer.ally_loss437+1u);
            observer.ally_loss_cost430+=u32(ko.receiver_cost4aa);
            next.ally_stat_mask|=bit;
        }
    }
    *out=next;return true;
}
}
