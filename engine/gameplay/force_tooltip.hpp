#pragma once
#include "engine/assets/force_tooltip_art.hpp"
#include "engine/gameplay/force_editor_grid.hpp"
namespace gf {
struct ForceTooltipSnapshot {std::string name;u16 model=0;u8 variant=0,level=0; s16 cost=0;unsigned corner=0,remaining=0;};
class ForceTooltip {
public:
    bool bind(const ForceInventory& owner,const ForceTooltipArt& art){*this={};if(!owner.ready()||art.lifetime!=10||!std::isfinite(art.split_x)||!std::isfinite(art.split_z))return false;scope_=owner.instance_binding();art_=art;ready_=true;return true;}
    // Owner-corrected presentation: screen-space X selects the opposite bottom
    // corner. Keep bind() as the explicit recovered four-position component API.
    bool bind_bottom(const ForceInventory& owner,const ForceTooltipArt& art,f32 width){
        if(!std::isfinite(width)||width<=0||width>16384){*this={};return false;}
        if(!bind(owner,art))return false;
        bottom_=true;midpoint_=width*.5f;return true;
    }
    bool bound(const ForceInventory& owner)const{return ready_&&owner.ready()&&scope_.lock()==owner.instance_binding().lock();}
    bool step(const ForceInventory& owner,unsigned force,s16 held,ForceEditorHit hit,const std::array<f32,2>& cursor,bool enabled){
        if(!bound(owner)||force>=kForceCount||held<-1||held>=s16(kBoxBorgs)||hit.box<-1||hit.box>=s16(kBoxBorgs)||hit.member<-1||hit.member>=s8(kForceSlots)||(hit.box>=0&&hit.member>=0)||!std::isfinite(cursor[0])||!std::isfinite(cursor[1])){value_={};return false;}
        if(!enabled){value_={};return true;}
        if(held>=0&&!owner.state()->box[unsigned(held)].value.present()){value_={};return false;}
        s16 slot=held;if(slot<0){if(hit.member>=0)slot=owner.state()->forces[force].slots[unsigned(hit.member)];else if(hit.box>=0&&!(owner.state()->box[unsigned(hit.box)].memberships&(1u<<force)))slot=hit.box;}
        if(slot<0||!owner.state()->box[unsigned(slot)].value.present()){if(value_.remaining)--value_.remaining;return true;}
        const auto& borg=owner.state()->box[unsigned(slot)].value;const auto* info=owner.catalog()->find(u16(borg.model));if(!info){value_={};return false;}
        const unsigned corner=bottom_?(cursor[0]<midpoint_?1u:3u):unsigned(cursor[1]<art_.split_z)+2*unsigned(cursor[0]>art_.split_x);
        value_={info->source_name,u16(borg.model),borg.variant,u8(borg.level+1),s16(force_borg_cost(*owner.catalog(),borg)),corner,art_.lifetime};return true;
    }
    const ForceTooltipSnapshot* visible(const ForceInventory& owner)const{return bound(owner)&&value_.remaining?&value_:nullptr;}
private:
    bool ready_=false;std::weak_ptr<const ForceInventoryScope> scope_;ForceTooltipArt art_;ForceTooltipSnapshot value_;
    bool bottom_=false;f32 midpoint_=0;
};
} // namespace gf
