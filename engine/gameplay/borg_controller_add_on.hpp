#pragma once
#include "engine/assets/borg_controller_profile.hpp"
#include "engine/gameplay/borgs/normal_death_icbm.hpp"
#include "engine/gameplay/borgs/normal_switching_ninja_exchange.hpp"
#include "engine/gameplay/borgs/normal_teleport_ninja.hpp"

namespace gf {
enum class BorgControllerAddOnStatus:u8 {
    NoController,Unresolved,None,NeedsSourceObservation,Refused,DetachedPlanner
};

// Supplied by the caller that owns the source snapshot and slot, independently
// of GFBORG metadata. A declaration, evidence label, display name or decoded
// preview is not such an observation. These views are not retained.
struct BorgControllerSourceObservation {
    std::string_view package_id,variant,form;
    BorgControllerSourceKey source;
    u8 slot=255;
};

// Detached planning only: no actor, geometry query, allocation, commit, or
// gameplay admission. Keeps package selection by value, not into profile text.
class BorgDetachedControllerAddOn {
public:
    static BorgDetachedControllerAddOn bind(const BorgIdentity& identity,const BorgControllerProfile& profile,
                                            std::string_view variant,std::string_view form,
                                            const BorgControllerSourceObservation* observed=nullptr){
        using S=BorgControllerAddOnStatus;
        BorgDetachedControllerAddOn next;
        const auto* row=profile.find(variant,form);
        if(!row)return next;
        next.kind_=row->add_on;
        if(!borg_controller_detail::add_on_scope(*row))next.status_=S::Refused;
        else if(row->add_on==BorgControllerAddOn::Unresolved)next.status_=S::Unresolved;
        else if(row->add_on==BorgControllerAddOn::None)next.status_=S::None;
        else if(!observed)next.status_=S::NeedsSourceObservation;
        else {
            const auto& source=borg_controller_detail::add_on_spec(row->add_on)->source;
            if(!borg_package_id_valid(identity.id)||observed->package_id!=identity.id||observed->variant!=variant||
               observed->form!=form||observed->source!=source||observed->slot>=6)next.status_=S::Refused;
            else {
                next.identity_=identity;
                next.variant_=variant;
                next.form_=form;
                next.model_=source.model;
                next.source_variant_=source.variant;
                next.slot_=observed->slot;
                next.status_=S::DetachedPlanner;
            }
        }
        return next;
    }
    BorgControllerAddOnStatus status()const{return status_;}
    BorgControllerAddOn kind()const{return kind_;}
    const BorgIdentity& identity()const{return identity_;}
    std::string_view variant()const{return variant_;}
    std::string_view form()const{return form_;}

    // The existing history/stock/travel/finish helpers consume only caster
    // samples. A directed-target setup still refuses in the history planner.
    std::optional<NormalTeleportNinjaSource> teleport_source(u8 caster_slot)const{
        if(!has(BorgControllerAddOn::TeleportNinjaHistory)||caster_slot!=slot_)return {};
        return NormalTeleportNinjaSource{model_,source_variant_};
    }
    bool plan_teleport_history(u8 caster_slot,const TeleportPosition& position,const TeleportPosition& history5e8,
                               u32 flags5e0,f32 directed_window144,bool has_target_snapshot,TeleportSetupPlan* out)const{
        const auto source=teleport_source(caster_slot);
        return source&&normal_teleport_history_setup(*source,position,history5e8,flags5e0,directed_window144,
                                                    has_target_snapshot,out);
    }
    bool stage_exchange(const SwitchingNinjaContact& in,SwitchingNinjaStagedExchange* out)const{
        // Existing contact planner checks the exact carrier, hit, model and
        // retail variant. Each resulting leg is consumed independently.
        return has(BorgControllerAddOn::SwitchingNinjaExchange)&&in.caster.slot==slot_&&
            normal_switching_ninja_stage_exchange(in,out);
    }
    bool plan_impact(u8 caster_slot,const IcbmGameImpactInput& in,IcbmGameImpactPlan* out)const{
        return has(BorgControllerAddOn::DeathIcbmImpact)&&caster_slot==slot_&&
            normal_death_icbm_game_impact({model_,source_variant_,slot_},in,out);
    }
    bool consume_terminal(u8 caster_slot,u8 lifecycle18,u8 pending663,u8 received1da,bool observed_active_slot_pass,
                          IcbmGameDefeatPlan* out)const{
        return has(BorgControllerAddOn::DeathIcbmImpact)&&caster_slot==slot_&&
            normal_death_icbm_consume_terminal({model_,source_variant_,slot_},lifecycle18,pending663,
                                               received1da,observed_active_slot_pass,out);
    }
private:
    bool has(BorgControllerAddOn kind)const{
        return status_==BorgControllerAddOnStatus::DetachedPlanner&&kind_==kind&&!identity_.id.empty();
    }
    BorgControllerAddOnStatus status_=BorgControllerAddOnStatus::NoController;
    BorgControllerAddOn kind_=BorgControllerAddOn::Unresolved;
    BorgIdentity identity_;
    std::string variant_,form_;
    u16 model_=0;
    u8 source_variant_=0,slot_=255;
};
}
