#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include "engine/gameplay/borgs/normal_gunman_beta_basic_shot.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Gunman/Beta II basic shot: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using S=NormalGunmanBetaShotSource;
    using Op=WeaponStockOperation;
    for(const auto model:{S::gunman,S::beta_ii}){
        const S source{model,0};
        const bool gunman=model==S::gunman;
        GunmanBetaShotTraits traits;
        check(normal_gunman_beta_shot_traits(source,&traits)&&traits.program_group==2&&traits.program==(gunman?4:0)&&
            traits.base_projectile==(gunman?0:56)&&traits.level0_stock==(gunman?20:5)&&traits.level0_refill_preset==(gunman?120:60),
            "only the exact pose, projectile and stock data are bound");
        GunmanBetaShotAction action;
        check(normal_gunman_beta_shot_request(source,{0,0,0,true},&action)&&!action.request,"held/raw input isn't a conditioned pulse");
        check(normal_gunman_beta_shot_request(source,{0x20,0,0,true},&action)&&action.request&&
            !action.group&&!action.class_state580&&!action.substate581,"B pulse selects reviewed ordinary class action");
        check(!normal_gunman_beta_shot_request(source,{0x20,0,0,false},&action)&&
            !normal_gunman_beta_shot_request(source,{0x20,2,0,true},&action)&&
            !normal_gunman_beta_shot_request(source,{0x20,0,1,true},&action)&&action.request,
            "target/priority and unsupported B contexts do not fall through");
        GunmanBetaShotReleasePlan release;
        for(u8 event:{0,128,255})
            check(normal_gunman_beta_shot_release(source,1,event,&release)&&release.next_substate540==1&&!release.stock.amount,
                "zero and signed-negative program events cannot release");
        check(normal_gunman_beta_shot_release(source,1,1,&release)&&release.next_substate540==2&&
            release.stock.channel==0&&release.stock.operation==Op::Debit&&release.stock.amount==1,
            "positive animation event requests debit, not a button release or guaranteed outcome");

        const WeaponStockModeZeroState loaded{1,3,traits.level0_refill_preset,2.5f};
        GunmanBetaShotStockRequest repeat;
        WeaponStockModeZeroPlan stock;
        GunmanBetaProjectileRequest shot;
        check(normal_gunman_beta_shot_repeat(source,0,&repeat)&&!repeat.amount,"no new conditioned B, no check");
        check(normal_gunman_beta_shot_repeat(source,0x20,&repeat)&&repeat.operation==Op::CheckOnly&&repeat.amount==1&&
            weapon_stock_mode_zero_plan(loaded,repeat.operation,repeat.amount,&stock)&&stock.accepted&&stock.state==loaded,
            "repeat checks stock without spending or starting refill");
        check(!normal_gunman_beta_projectile_request(source,3,0,stock,&shot)&&!shot.requested,"check-only result cannot publish a shot");
        check(weapon_stock_mode_zero_plan(loaded,release.stock.operation,release.stock.amount,&stock)&&stock.accepted&&
            stock.state==WeaponStockModeZeroState{0,1,traits.level0_refill_preset,f32(traits.level0_refill_preset)},
            "last round retains previous count and seeds only the mode-zero refill clock");
        check(normal_gunman_beta_projectile_request(source,3,0,stock,&shot)&&shot==GunmanBetaProjectileRequest{true,3,traits.base_projectile},
            "successful debit names one projectile and the supplied firing slot");
        check(normal_gunman_beta_projectile_request(source,1,128,stock,&shot)&&shot.firing_slot==1&&
            shot.projectile_key==traits.base_projectile+1,"nonzero latched side uses its distinct key");
        const auto spent=stock.state;
        check(weapon_stock_mode_zero_plan(spent,Op::Debit,1,&stock)&&!stock.accepted&&stock.state==spent&&
            normal_gunman_beta_projectile_request(source,3,0,stock,&shot)&&!shot.requested,
            "empty release does not rearm the refill clock or create a request");
        check(!normal_gunman_beta_shot_traits({model,1},&traits)&&
            !normal_gunman_beta_shot_release({0x010a,0},1,1,&release),"alternate or different class refuses");
    }
    WeaponStockModeZeroPlan stock;
    const WeaponStockModeZeroState loaded{4,8,100,7.75f};
    check(weapon_stock_mode_zero_plan(loaded,Op::Debit,2,&stock)&&stock.accepted&&
        stock.state==WeaponStockModeZeroState{2,4,100,7.75f},"nonfinal debit preserves the old clock");
    const auto retained=stock;
    check(!weapon_stock_mode_zero_plan(loaded,Op(255),1,&stock)&&
        !weapon_stock_mode_zero_plan({4,8,100,std::numeric_limits<f32>::infinity()},Op::Debit,1,&stock)&&stock==retained,
        "out-of-domain inputs cannot replace approved snapshot");
    std::printf("normal Gunman/Beta II shot requests and shared mode-zero stock: %u checks PASS\n",checks);
}
