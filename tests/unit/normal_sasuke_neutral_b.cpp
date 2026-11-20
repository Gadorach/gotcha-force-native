#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>
#include "engine/gameplay/borgs/normal_gred_neutral_b.hpp"
#include "engine/gameplay/borgs/normal_sasuke_neutral_b.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Sasuke neutral B: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using R=SasukeNeutralBRoute;
    static_assert(!std::is_same_v<SasukeNeutralBReleaseGate,GRedNeutralBReleaseGate>);
    constexpr NormalSasukeNeutralBSource sasuke{0x000a,0,4};
    constexpr NormalGRedNeutralBSource red{0x0615,0,4};
    const WeaponStockModeZeroState last{1,7,90,0},empty{0,7,90,0};
    SasukeNeutralBChoice row;
    check(normal_sasuke_neutral_b_choice(sasuke,0x460,0,8,true,true,&row)&&row.route==R::Close&&
        row.channel==1&&row.selector==44&&row.class_state580==1,"close wins; injected charge does not");
    check(normal_sasuke_neutral_b_choice(sasuke,0x400,0,0,true,true,&row)&&row.route==R::None,
        "no charged-B action");
    check(normal_sasuke_neutral_b_choice(sasuke,0x460,2,8,true,true,&row)&&row.route==R::Shot&&
        row.selector==44&&!row.channel&&!row.class_state580,"inhibited close leaves the no-target shot");
    check(!normal_sasuke_neutral_b_choice(sasuke,0x20,0,0,true,false,&row)&&
        !normal_sasuke_neutral_b_choice(sasuke,0x20,1,0,true,true,&row)&&
        !normal_sasuke_neutral_b_choice(sasuke,0x20,0,0,false,true,&row)&&row.route==R::Shot,
        "unsupported target/override/arbitration leaves output untouched");
    WeaponStockModeZeroPlan available;
    check(normal_sasuke_neutral_b_availability(sasuke,row,last,&available)&&available.accepted&&
        available.state==last&&available.operation==WeaponStockOperation::CheckOnly,"entry check does not pay");
    check(normal_sasuke_neutral_b_availability(sasuke,row,empty,&available)&&!available.accepted&&
        available.state==empty,"unavailable is separate from staging the request");

    SasukeNeutralBStartup start;
    check(normal_sasuke_neutral_b_start(sasuke,true,0,&start)&&start.aim_clock560==30&&start.phase540==1&&
        start.attempts6ef==5&&start.group==2&&start.upper_mask==1&&!start.upper_program&&start.lower_mask==2&&
        start.lower_program==9,"startup separates upper and lower programs");
    check(!normal_sasuke_neutral_b_start({0x000a,1,4},true,0,&start)&&
        !normal_sasuke_neutral_b_start({0x000a,0,6},true,0,&start)&&
        !normal_sasuke_neutral_b_start({0x0615,0,4},true,0,&start)&&
        !normal_sasuke_neutral_b_start(sasuke,false,0,&start)&&
        !normal_sasuke_neutral_b_start(sasuke,true,1,&start)&&start.aim_clock560==30,"exact normal fresh-idle only");
    bool visit=false;
    f32 red_step=1;
    check(normal_sasuke_neutral_b_should_visit_vm(sasuke,0,&visit)&&visit&&
        normal_sasuke_neutral_b_should_visit_vm(sasuke,255,&visit)&&!visit&&
        normal_gred_neutral_b_vm_step(red,255,1,&red_step)&&red_step==0,"skip is distinct from visiting with zero step");
    SasukeNeutralBReleaseGate gate;
    check(normal_sasuke_neutral_b_release_gate(sasuke,0,false,0,1,&gate)&&gate.aim_clock560==-1&&
        !gate.release_due&&gate.phase540==1,"no cue means no release after expiry");
    check(normal_sasuke_neutral_b_release_gate(sasuke,1,false,2,1,&gate)&&gate.aim_clock560==1&&
        !gate.release_due,"cue alone does not authorize release");
    check(normal_sasuke_neutral_b_release_gate(sasuke,1,false,1,1,&gate)&&!gate.aim_clock560&&
        gate.release_due&&gate.phase540==2,"cue and exact zero authorize release");
    check(!normal_sasuke_neutral_b_release_gate(sasuke,1,true,std::numeric_limits<f32>::quiet_NaN(),1,&gate)&&
        gate.release_due&&!gate.aim_clock560,"invalid scalar leaves output untouched");

    SasukeNeutralBReleasePlan release;
    check(!normal_sasuke_neutral_b_debit({0x000a,0,2},gate,1,last,&release)&&!release.request_shuriken,
        "gate from another slot cannot authorize the debit");
    check(normal_sasuke_neutral_b_debit(sasuke,gate,5,last,&release)&&release.stock==WeaponStockModeZeroState{0,1,90,90}&&
        release.local_pose6ee==1&&release.attempts6ef==4&&release.request_shuriken&&release.owner_slot==4&&
        release.object_type==14&&release.argument==4,"paid release requests one exact object and arms empty refill");
    check(normal_sasuke_neutral_b_debit(sasuke,gate,1,empty,&release)&&release.stock==empty&&release.local_pose6ee==1&&
        !release.attempts6ef&&!release.request_shuriken,"failed debit still spends the local attempt");
    check(!normal_sasuke_neutral_b_debit(sasuke,gate,0,last,&release)&&!release.request_shuriken,
        "exhausted attempts are outside this release path");

    SasukeNeutralBRepeatPlan repeat;
    check(normal_sasuke_neutral_b_repeat(sasuke,true,0x200,4,false,last,&repeat)&&repeat.checked_stock&&repeat.request_repeat&&
        !repeat.request_cleanup&&repeat.phase540==1&&repeat.mask==1&&repeat.group==2&&repeat.program==1&&last.current==1,
        "held B and positive attempts request upper repeat without spending");
    check(normal_sasuke_neutral_b_repeat(sasuke,true,0x200,4,false,empty,&repeat)&&repeat.checked_stock&&
        !repeat.request_repeat&&repeat.request_cleanup,"empty stock cleans up rather than waiting to reload");
    check(normal_sasuke_neutral_b_repeat(sasuke,true,0x200,255,false,last,&repeat)&&repeat.checked_stock&&
        !repeat.request_repeat&&repeat.request_cleanup,"negative signed attempts cannot repeat");
    check(normal_sasuke_neutral_b_repeat(sasuke,true,0,4,false,last,&repeat)&&!repeat.checked_stock&&
        !repeat.request_repeat&&!repeat.request_cleanup,"released B waits for completion");
    check(normal_sasuke_neutral_b_repeat(sasuke,true,0,4,true,last,&repeat)&&!repeat.checked_stock&&
        repeat.request_cleanup,"released B and completed lane request shared cleanup");
    check(!normal_sasuke_neutral_b_repeat(sasuke,false,0x200,4,false,last,&repeat)&&repeat.request_cleanup,
        "air or other unreviewed phase leaves output unchanged");
    std::printf("normal Sasuke neutral B scoped release and repeat: %u checks PASS\n",checks);
}
