#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_knight_samurai_actions.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Knight/Samurai actions: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    constexpr KnightSamuraiSource knight{0x020a,0,2},samurai{0x0700,0,3};
    KnightSamuraiActionRequest action;
    check(normal_knight_samurai_action_request(knight,0x80,0,0,true,&action)&&action.requested&&action.channel==2&&
        action.class_state580==0&&action.selector==44,"neutral Knight X, not hit registration");
    check(normal_knight_samurai_action_request(knight,0x80,4,8,true,&action)&&!action.requested,"inhibited X leaves no request");
    check(normal_knight_samurai_action_request(samurai,0x40,0,0,true,&action)&&!action.requested,"missing near classification");
    check(normal_knight_samurai_action_request(samurai,0x40,0,8,true,&action)&&action.requested&&action.channel==1&&
        action.class_state580==1&&action.selector==44,"near input selects close B, not a contact receipt");
    check(!normal_knight_samurai_action_request(samurai,0x40,0,8,false,&action)&&action.requested,"earlier actor paths remain required");

    const WeaponStockState loaded{1,7,0,2.5f},empty{0,7,0,2.5f};
    KnightShieldEventPlan shield;
    check(normal_knight_shield_event(knight,255,0,loaded,&shield)&&shield.latch144==1&&shield.event1cef==0&&
        !shield.attempted_debit&&!shield.request_factory&&shield.stock==loaded,"signed negative arms, never spends or launches");
    check(normal_knight_shield_event(knight,128,2,loaded,&shield)&&shield.event1cef==128&&shield.latch144==2&&
        !shield.request_factory,"armed or away latch doesn't consume another negative event");
    check(normal_knight_shield_event(knight,1,2,loaded,&shield)&&!shield.attempted_debit&&shield.stock==loaded,
        "shield already away blocks repeat release");
    check(normal_knight_shield_event(knight,1,1,empty,&shield)&&shield.attempted_debit&&!shield.request_factory&&
        shield.event1cef==1&&!shield.latch144&&shield.stock==empty,"failed debit clears latch without a cast");
    check(normal_knight_shield_event(knight,1,1,loaded,&shield)&&shield.attempted_debit&&!shield.event1cef&&
        shield.request_factory&&shield.owner_slot==2&&shield.object_type==24&&shield.argument==1&&
        shield.latch144==1&&shield.write_latch_after_factory&&shield.post_factory_latch144==2&&
        shield.stock==WeaponStockState{0,1,0,0},"separate launch proposal and caller's post-factory latch write");
    check(!normal_knight_shield_event(samurai,1,1,loaded,&shield)&&shield.request_factory,"different class cannot borrow the guard latch");
    KnightShieldOwnerStock returned;
    check(normal_knight_shield_owner_hook(knight,2,true,shield.stock,1,&returned)&&!returned.return_depleted_stock&&
        returned.default_group582==1&&!returned.stock.current,"no refund while latched away");
    check(normal_knight_shield_owner_hook(knight,0,true,shield.stock,1,&returned)&&returned.return_depleted_stock&&
        !returned.default_group582&&returned.stock==WeaponStockState{1,1,0,0},"separate owner hook can restore depleted stock");

    SamuraiMeleeShapeRequest shape;
    check(normal_samurai_melee_shape(samurai,2,true,&shape)&&!shape.requested,"wrong lane doesn't register the first strike");
    check(normal_samurai_melee_shape(samurai,15,true,&shape)&&shape.requested&&shape.group==1&&
        shape.carrier_slot==3&&shape.credited_owner_slot==3,"actor-owned melee shapes, not visual or stock-driven shot");
    check(!normal_samurai_melee_shape(knight,1,true,&shape)&&shape.group==1,"no actor class substitution");
    SamuraiPresentationRequest visual;
    check(normal_samurai_melee_presentation(samurai,1,false,true,&visual)&&!visual.requested,"optional presentation hook missing");
    check(normal_samurai_melee_presentation(samurai,1,true,true,&visual)&&visual.requested&&visual.visual_key==6&&
        visual.owner_slot==3&&shape.group==1,"presentation output never replaces melee group or owner");
    std::printf("normal Knight/Samurai signed events and distinct hit ownership: %u checks PASS\n",checks);
}
