#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_kung_fu_master.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Kung-Fu Master: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using R=KungFuBRoute;
    constexpr NormalKungFuSource kung{0x0800,0,3};
    KungFuBRequest selected;
    check(normal_kung_fu_b_request(kung,0x60,0,8,true,&selected)&&selected.route==R::Close&&
        selected.action_channel==1&&selected.class_state580==1&&selected.selector==44,"eligible close request wins same edge");
    WeaponStockPlan availability;
    const WeaponStockState stock{2,1,300,320},empty{0,7,300,0};
    check(!normal_kung_fu_shot_availability(kung,selected,stock,&availability),
        "close request isn't a channel-zero availability debit");
    check(normal_kung_fu_b_request(kung,0x60,1,0,true,&selected)&&selected.route==R::Close,
        "exact low-bit context can bypass the near classifier");
    check(normal_kung_fu_b_request(kung,0x60,2,8,true,&selected)&&selected.route==R::Shot&&
        !selected.action_channel&&!selected.class_state580,"close inhibitor takes the ordinary shot fallback");
    check(normal_kung_fu_shot_availability(kung,selected,stock,&availability)&&availability.accepted&&
        availability.operation==WeaponStockOperation::CheckOnly&&availability.state==stock,
        "entry check doesn't spend or rewrite clock");
    check(normal_kung_fu_shot_availability(kung,selected,empty,&availability)&&!availability.accepted&&
        availability.state==empty&&selected.route==R::Shot,"refused check doesn't undo previously staged action");
    check(!normal_kung_fu_b_request(kung,0x20,1,0,true,&selected)&&selected.route==R::Shot,
        "unsupported B-altering context is not a neutral fallback");

    KungFuShotRelease release;
    check(normal_kung_fu_shot_release(kung,0,3,0,3,stock,&release)&&!release.attempted_debit&&!release.subphase541,
        "both exact release bytes required");
    check(normal_kung_fu_shot_release(kung,0,3,1,3,stock,&release)&&release.attempted_debit&&release.subphase541==1&&
        release.stock==WeaponStockState{1,2,300,620}&&release.request_projectile&&release.owner_slot==3&&release.projectile_key==12,
        "paid release adds mode-one stock clock and requests the exact projectile");
    check(normal_kung_fu_shot_release(kung,0,3,1,3,empty,&release)&&release.attempted_debit&&release.subphase541==1&&
        release.stock==empty&&!release.request_projectile,"refused release still advances, not a retry or object");
    check(normal_kung_fu_shot_release(kung,1,255,0,0x43,stock,&release)&&release.subphase541==2&&release.clear_action73f&&
        release.flags5e0==0x40&&!release.attempted_debit&&release.stock==stock,"later signed -1 ends action without new debit");
    KungFuHandPresentation hand;
    check(normal_kung_fu_shot_hand(kung,1,&hand)&&hand.requested&&hand.owner_slot==3&&hand.key==85&&
        normal_kung_fu_shot_hand(kung,4,&hand)&&!hand.requested,"hand presentation is distinct from hit owner and cue");

    WeaponStockState refill;
    check(weapon_stock_mode_one_refill({3,7,300,301},5,1,false,&refill)&&refill==WeaponStockState{3,7,300,300},
        "exact whole interval and unchanged count retain previous sample");
    check(weapon_stock_mode_one_refill(refill,5,1,false,&refill)&&refill==WeaponStockState{4,4,300,299},
        "crossing the interval updates current and previous together");
    check(weapon_stock_mode_one_refill(refill,5,1,true,&refill)&&refill.refill_clock==299&&refill.current==4,
        "paused stock visit doesn't use projectile lifetime");
    check(weapon_stock_mode_one_refill({4,4,300,0.5f},5,1,false,&refill)&&refill==WeaponStockState{5,5,300,-0.5f},
        "mode-one expiry restores stock but keeps signed overshoot");
    check(weapon_stock_plan(refill,WeaponStockRefillMode::AddOnDebit,WeaponStockOperation::Debit,1,&availability)&&
        availability.state==WeaponStockState{4,5,300,299.5f}&&
        !weapon_stock_plan(refill,WeaponStockRefillMode::SeedOnDepletion,WeaponStockOperation::Debit,1,&availability),
        "only mode one can carry that negative remainder into the next debit");
    std::printf("normal Kung-Fu Master scoped close/shot selection and mode-one stock: %u checks PASS\n",checks);
}
