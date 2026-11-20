#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_ko_observers.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke KO observers: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,0};
    GRedBeamSasukeKoPlan ko;
    ko.credit_slot=0;ko.receiver_slot=3;ko.receiver_cost4aa=150;
    GRedSasukeKoObserverSnapshot input;
    input.six_physical_slots_declared=true;
    auto& slots=input.slots;
    for(auto& s:slots)s.queue464.fill({-1,-1});
    slots[0].active00=1;slots[0].team88=0;slots[0].cursor484=7;
    slots[1].active00=255;slots[1].team88=1;slots[1].cursor484=4;
    slots[1].ally_loss437=255;slots[1].ally_loss_cost430=0xffffffffu-149;
    slots[3].active00=1;slots[3].team88=1;slots[3].slot3e4=5;slots[3].cursor484=255;
    slots[5].active00=1;slots[5].team88=0;slots[5].cursor484=0;
    GRedSasukeKoObserverPlan p;
    check(!normal_gred_beam_sasuke_ko_observers(red,ko,input,&p),"unknown KO scan boundary refused");
    ko.observer_scan_pending=true;
    check(normal_gred_beam_sasuke_ko_observers(red,ko,input,&p)&&p.queue_write_mask==0x23&&p.ally_stat_mask==2&&
        p.slots[0].queue464[7]==GRedSasukeKoObserverEvent{3,5}&&p.slots[0].cursor484==0&&p.slots[0].queue464[6]==slots[0].queue464[6]&&
        p.slots[5].queue464[0]==GRedSasukeKoObserverEvent{3,5}&&p.slots[5].cursor484==1,
        "opposing slots queue code 3 and raw subject, not the physical record index");
    check(p.slots[1].queue464[4]==GRedSasukeKoObserverEvent{4,5}&&p.slots[1].cursor484==5&&
        !p.slots[1].ally_loss437&&!p.slots[1].ally_loss_cost430,"same team queues code 4 and wraps allied counters");
    check(p.slots[3]==slots[3]&&p.slots[2]==slots[2]&&p.slots[4]==slots[4],"fallen and inactive records untouched");
    const auto accepted=p;
    slots[5].cursor484=8;
    check(!normal_gred_beam_sasuke_ko_observers(red,ko,input,&p)&&p.slots==accepted.slots&&
        p.queue_write_mask==accepted.queue_write_mask&&p.ally_stat_mask==accepted.ally_stat_mask,
        "later invalid cursor refuses complete proposal");
    slots[5].cursor484=0;slots[3].slot3e4=255;
    check(!normal_gred_beam_sasuke_ko_observers(red,ko,input,&p),"unknown subject is not substituted with the physical slot");
    slots[3].slot3e4=5;slots[0].team88=1;
    check(!normal_gred_beam_sasuke_ko_observers(red,ko,input,&p),"credit team's snapshot must agree");
    std::puts("normal G-Red/Sasuke KO observers: six-slot queue writes and refusal PASS");
}
