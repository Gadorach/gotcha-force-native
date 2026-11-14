#pragma once
// Shared native selection/edit/launch handoff. Source 80196030/80196188 returns
// the editor's selected force to selection; battle consumes the same inventory.
#include "engine/gameplay/force_inventory.hpp"
namespace gf {
class ForceSelection {
public:
    bool open(const ForceInventory& owner,unsigned force,s32 budget,ForceCostMode mode){
        *this={};if(!owner.ready()||force>=kForceCount||(mode!=ForceCostMode::Full&&mode!=ForceCostMode::TwoThirds))return false;
        scope_=owner.instance_binding();force_=force;budget_=budget;mode_=mode;ready_=true;return true;
    }
    bool choose(const ForceInventory& owner,unsigned force){if(!bound(owner)||editing_||force>=kForceCount)return false;force_=force;launch_={};return true;}
    bool begin_edit(const ForceInventory& owner,unsigned* force){
        if(!force||!bound(owner)||editing_)return false;
        editing_=true;launch_={};*force=force_;return true;
    }
    bool return_from_edit(const ForceInventory& owner,unsigned force){
        if(!bound(owner)||!editing_||force>=kForceCount)return false;
        force_=force;editing_=false;launch_={};return true;
    }
    // 801905E0 rebuilds the nonempty-slot list, then maps the requested slot to
    // its row. Missing requested slots select row0, not literal force slot0.
    // Explicit entry keeps the old direct-slot return contract unchanged.
    bool return_from_edit_nonempty(const ForceInventory& owner,unsigned force){
        if(!bound(owner)||!editing_||force>=kForceCount)return false;
        unsigned selected=kForceCount;
        for(unsigned i=0;i<kForceCount;++i)if(owner.state()->forces[i].count){if(selected==kForceCount)selected=i;if(i==force){selected=i;break;}}
        return selected<kForceCount&&return_from_edit(owner,selected);
    }
    bool choose_nonempty(const ForceInventory& owner,int direction){
        if(!bound(owner)||editing_||(direction!=-1&&direction!=1))return false;
        for(unsigned offset=1;offset<=kForceCount;++offset){const auto candidate=unsigned((int(force_)+direction*int(offset)+int(kForceCount))%int(kForceCount));if(owner.state()->forces[candidate].count)return choose(owner,candidate);}
        return false;
    }
    bool preview(const ForceInventory& owner,ForceBattleRoster* out)const{
        if(!out)return false;
        *out={};return bound(owner)&&!editing_&&owner.battle_roster(force_,budget_,mode_,out);
    }
    bool confirm(const ForceInventory& owner){launch_={};return preview(owner,&launch_);}
    // One-shot transfer to the battle owner. Rechecking current() at consumption
    // prevents an edited/reloaded roster from launching after confirmation.
    bool take(const ForceInventory& owner,ForceBattleRoster* out){
        if(!out)return false;
        *out={};const bool valid=bound(owner)&&!editing_&&owner.current(launch_);
        if(valid)*out=launch_;
        launch_={};return valid;
    }
    unsigned force()const{return force_;}bool editing()const{return editing_;}
private:
    bool bound(const ForceInventory& owner)const{return ready_&&owner.ready()&&scope_.lock()==owner.instance_binding().lock();}
    bool ready_=false,editing_=false;unsigned force_=0;s32 budget_=0;ForceCostMode mode_=ForceCostMode::Full;
    std::weak_ptr<const ForceInventoryScope> scope_;ForceBattleRoster launch_;
};
} // namespace gf
