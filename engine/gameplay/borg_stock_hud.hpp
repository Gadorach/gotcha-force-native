#pragma once

#include "engine/gameplay/borg_gameplay.hpp"

namespace gf {

enum class BorgStockHudPhase:u8 {Ready,Empty,Refill,Return};

struct BorgStockHudRow {
    u8 channel=0,buttons=0;
    u16 current=0,capacity=0;
    BorgStockHudPhase phase=BorgStockHudPhase::Ready;
    bool operator==(const BorgStockHudRow&)const=default;
};

struct BorgStockHud {
    std::array<BorgStockHudRow,kBorgBattleStocks> rows{};
    unsigned count=0;
    bool operator==(const BorgStockHud&)const=default;
};

inline const char* borg_stock_hud_buttons(u8 buttons){
    return buttons==BorgBattleInputs::mask?"B/X":buttons==BorgBattleInputs::b?"B":buttons==BorgBattleInputs::x?"X":"-";
}
inline const char* borg_stock_hud_phase(BorgStockHudPhase phase){
    switch(phase){
    case BorgStockHudPhase::Ready:return "ready";
    case BorgStockHudPhase::Empty:return "empty";
    case BorgStockHudPhase::Refill:return "refill";
    case BorgStockHudPhase::Return:return "return";
    }
    return "-";
}

// Observe declared pools in stable channel order. The mask is a label
// for their authored uses, not a prediction of what a press will do.
inline BorgStockHud borg_stock_hud(const BorgGameplayProfile& profile,const BorgBattleActionState& action){
    const auto channels=std::min(profile.stocks.size(),action.stocks.size());
    std::array<u8,kBorgBattleStocks> buttons{};
    for(const auto& move:profile.moves){
        const auto bit=BorgBattleInputs::button(move.button);
        const auto attach=[&](unsigned channel,unsigned cost){if(cost&&channel<channels)buttons[channel]|=bit;};
        if(move.entry_stock)attach(move.entry_stock->stock,move.entry_stock->minimum);
        if(move.held_fire)attach(move.held_fire->stock,move.held_fire->cost);
        for(const auto& release:move.releases)attach(release.stock,release.cost);
        for(const auto& anchor:move.anchors)attach(anchor.stock,anchor.cost);
        if(move.field&&move.field->debit){
            const auto& debit=*move.field->debit;
            for(unsigned i=0;i<channels;++i)if(profile.stocks[i].id==debit.stock){attach(i,debit.cost);break;}
        }
    }
    BorgStockHud out;
    for(unsigned i=0;i<channels;++i)if(buttons[i]){
        const auto& spec=profile.stocks[i];
        const auto& state=action.stocks[i];
        if(!spec.capacity||state.current<0||state.current>spec.capacity||!std::isfinite(state.refill_clock))continue;
        auto phase=BorgStockHudPhase::Ready;
        if(state.current<spec.capacity){
            // A live clock does not imply a specific next-ammo ETA in all
            // stock modes. Show only its activity and the actual count.
            if(!spec.refill_ticks)phase=BorgStockHudPhase::Return;
            else if(state.refill_clock>0)phase=BorgStockHudPhase::Refill;
            else if(!state.current)phase=BorgStockHudPhase::Empty;
        }
        out.rows[out.count++]={u8(i),buttons[i],u16(state.current),spec.capacity,phase};
    }
    return out;
}

} // namespace gf
