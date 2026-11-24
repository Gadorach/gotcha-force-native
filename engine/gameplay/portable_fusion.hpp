#pragma once
#include <array>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include "engine/assets/portable_battle_keys.hpp"

namespace gf {
struct PortableBattleForm {
    PortableBattleBorg borg;
    std::string form;
    bool operator==(const PortableBattleForm&)const=default;
};
inline bool portable_battle_form_valid(const PortableBattleForm& key){
    return portable_battle_borg_valid(key.borg)&&borg_catalog_variant_id_valid(key.form)&&key.form!="unresolved";
}

// Only the form binding belongs here. HP, position, save identity, and
// input stay owned by the existing battle actor. The host's actual form
// loader is responsible for constructing and pinning a verified binding.
template<class Binding> struct PortableFormSlot {
    u64 instance=0,revision=0;
    u8 team=0;
    PortableBattleForm key;
    std::shared_ptr<const Binding> loaded;
    bool operator==(const PortableFormSlot&)const=default;
};

enum class PortableFusionStep {Waiting,Committed,Expired,Restored,Complete,Refused,Stale};

// One round-scoped paired swap of already prepared form bindings. A span
// is supplied on each visit; this class does not retain pointers into the
// roster or publish resource callbacks. Reserve it with the actor owner.
template<class Binding> class PortableFusion {
public:
    using Slot=PortableFormSlot<Binding>;
    PortableFusionStep prepare(std::span<const Slot> slots,std::array<unsigned,2> order,
                               std::array<PortableBattleForm,2> expected,std::array<Slot,2> loaded,u16 ticks){
        if(phase_!=Phase::Empty||slots.size()>6||order[0]>=slots.size()||order[1]>=slots.size()||
           order[0]==order[1]||!ticks||ticks>3600)return PortableFusionStep::Refused;
        const auto& first=slots[order[0]];const auto& second=slots[order[1]];
        if(first.instance==second.instance||first.team!=second.team)return PortableFusionStep::Refused;
        for(unsigned i=0;i<2;++i){
            const auto& old=slots[order[i]];const auto& next=loaded[i];
            if(!valid(old)||old.revision>std::numeric_limits<u64>::max()-2||old.key!=expected[i]||
               !next.loaded||!portable_battle_form_valid(next.key)||old.key==next.key||
               next.instance!=old.instance||next.team!=old.team||next.revision!=old.revision+1)
                return PortableFusionStep::Refused;
        }
        if(loaded[0].key==loaded[1].key)return PortableFusionStep::Refused;
        before_={first,second};after_=std::move(loaded);order_=order;remaining_=ticks;
        phase_=Phase::Prepared;
        return PortableFusionStep::Waiting;
    }
    PortableFusionStep commit(std::span<Slot> slots,std::array<bool,2> accepted_cues){
        if(phase_!=Phase::Prepared)return PortableFusionStep::Refused;
        for(unsigned i=0;i<2;++i)if(order_[i]>=slots.size()||slots[order_[i]]!=before_[i]){
            clear();return PortableFusionStep::Stale;
        }
        if(!accepted_cues[0]||!accepted_cues[1])return PortableFusionStep::Waiting;
        // All validation precedes publication. Move the prepared keys;
        // retain separate receipts for guarded restoration.
        auto planned=after_;
        static_assert(std::is_nothrow_move_assignable_v<Slot>);
        for(unsigned i=0;i<2;++i)slots[order_[i]]=std::move(planned[i]);
        phase_=Phase::Active;
        return PortableFusionStep::Committed;
    }
    PortableFusionStep tick(bool eligible){
        if(phase_!=Phase::Active)return PortableFusionStep::Refused;
        if(eligible&&!--remaining_){phase_=Phase::Returning;return PortableFusionStep::Expired;}
        return PortableFusionStep::Waiting;
    }
    PortableFusionStep restore(std::span<Slot> slots,unsigned role,bool accepted){
        if(phase_!=Phase::Returning||role>=2||finished_[role])return PortableFusionStep::Refused;
        const auto index=order_[role];
        if(index>=slots.size()||slots[index]!=after_[role]){
            finish(role);return PortableFusionStep::Stale;
        }
        if(!accepted)return PortableFusionStep::Waiting;
        auto previous=std::move(before_[role]);
        previous.revision=after_[role].revision+1;
        slots[index]=std::move(previous);
        const bool done=finish(role);
        return done?PortableFusionStep::Complete:PortableFusionStep::Restored;
    }
    // Close only a pending preparation. An active pair must be restored
    // by the world or discarded with the entire round.
    void cancel(){if(phase_==Phase::Prepared)clear();}
    u16 remaining()const{return remaining_;}
private:
    enum class Phase {Empty,Prepared,Active,Returning};
    static bool valid(const Slot& slot){
        return slot.instance&&slot.revision&&slot.team<4&&slot.loaded&&portable_battle_form_valid(slot.key);
    }
    bool finish(unsigned i){
        finished_[i]=true;
        before_[i]={};after_[i]={};
        if(!finished_[0]||!finished_[1])return false;
        clear();return true;
    }
    void clear(){*this={};}
    Phase phase_=Phase::Empty;
    std::array<Slot,2> before_{},after_{};
    std::array<unsigned,2> order_{};
    std::array<bool,2> finished_{};
    u16 remaining_=0;
};
} // namespace gf
