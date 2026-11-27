#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_stock_hud.hpp"

namespace {
using namespace gf;
void check(bool result,const char* message){
    if(!result){std::fprintf(stderr,"stock HUD: %s\n",message);std::exit(1);}
}
}

int main(){
    BorgGameplayProfile profile;
    profile.stocks={{"main",4,60},{"remote",2,0},{"anchor",1,0},{"unused",3,60},{"field",2,60},{"gate",3,60}};
    BorgBattleMove fire,held,x_release,place,field;
    fire.entry_stock=BorgBattleEntryStock{5,1};
    BorgBattleRelease paid;paid.stock=0;paid.cost=1;
    fire.releases={paid,paid,{}};
    held.button=BorgBattleButton::X;
    held.held_fire=BorgBattleHeldFire{0,1,0,6,3,2,0};
    x_release.button=BorgBattleButton::X;paid.stock=1;x_release.releases={paid};
    BorgBattleAnchorMarker mark;mark.stock=2;mark.cost=1;place.anchors={mark};
    field.button=BorgBattleButton::X;field.field=BorgField{};
    field.field->debit=BorgFieldDebit{"field",1};
    profile.moves={fire,held,x_release,place,field};
    BorgBattleActionState action;
    action.reset(profile);
    action.stocks[0].current=0;action.stocks[0].refill_clock=42;
    action.stocks[1].current=1;
    action.stocks[2].current=0;
    action.stocks[4].current=1; // seed-on-depletion mode has not started its clock
    action.stocks[5].current=0;
    const auto hud=borg_stock_hud(profile,action);
    check(hud.count==5,"unreferenced pools and free releases are hidden");
    check(hud.rows[0]==BorgStockHudRow{0,BorgBattleInputs::mask,0,4,BorgStockHudPhase::Refill},
          "shared B/X channel appears once with its actual refill status");
    check(hud.rows[1]==BorgStockHudRow{1,BorgBattleInputs::x,1,2,BorgStockHudPhase::Return}&&
          hud.rows[2]==BorgStockHudRow{2,BorgBattleInputs::b,0,1,BorgStockHudPhase::Return},
          "paid release and anchor pools expose return-only counts");
    check(hud.rows[3]==BorgStockHudRow{4,BorgBattleInputs::x,1,2,BorgStockHudPhase::Ready}&&
          hud.rows[4]==BorgStockHudRow{5,BorgBattleInputs::b,0,3,BorgStockHudPhase::Empty},
          "field debit and entry-only gate are named; a quiet clock is not refilling");
    action.stocks[0].refill_clock-=1;
    check(borg_stock_hud(profile,action)==hud,"clock progress alone does not change the readout");
    action.reset(profile);
    const auto full=borg_stock_hud(profile,action);
    check(full.count==hud.count&&std::all_of(full.rows.begin(),full.rows.begin()+full.count,[](const auto& row){
        return row.current==row.capacity&&row.phase==BorgStockHudPhase::Ready;
    }),"restart restores the displayed pools without changing their bindings");
    action.stocks[0].current=-1;
    check(borg_stock_hud(profile,action).count==full.count-1,"invalid stock is not silently clamped and displayed");
}
