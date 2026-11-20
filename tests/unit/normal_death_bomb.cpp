#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_death_bomb.hpp"
#include "engine/gameplay/borgs/normal_death_icbm.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Death Bomb game controller: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using P=GameImpactProbeResult;
    using Q=GameImpactQuery;
    constexpr NormalDeathBombSource source{0x0d07,0,2};
    DeathBombEntryPlan entry;
    check(normal_death_bomb_entry(source,2,&entry)&&entry.selector==6&&entry.upper_state==0&&entry.lower_state==7&&entry.program==13,
        "entry code two selects actual fall, independent of placement");
    check(normal_death_bomb_entry(source,0,&entry)&&entry.selector==0&&entry.program==8&&
        normal_death_bomb_entry(source,1,&entry)&&entry.selector==0&&entry.program==0,
        "ordinary idle start doesn't inherit a forced fall");
    DeathBombFallSteering steering;
    check(normal_death_bomb_fall_steering(source,1,2,0x40,10,-20,0x50,true,&steering)&&steering.write_motion&&
        steering.speed44==11&&steering.vertical48==-2&&steering.heading_history5ae==0x50,
        "ordinary pre-class steering: blend toward own speed and clamp falling sample");
    check(normal_death_bomb_fall_steering(source,1,2,0x43,10,-20,0x50,false,&steering)&&!steering.write_motion,
        "class-owned action doesn't receive ordinary steering");
    check(!normal_death_bomb_fall_steering(source,1,2,0x40,10,-20,0x50,false,&steering)&&!steering.write_motion,
        "missing direction result cannot be supplied by this tail");
    DeathBombDropRequest drop;
    check(normal_death_bomb_drop_request(source,0x80,0,true,&drop)&&drop.requested&&drop.action_group==2&&drop.selector==45&&
        !drop.class_state580&&!drop.substate581,"conditioned neutral X is a stockless descent request");
    check(normal_death_bomb_drop_request(source,0x80,4,true,&drop)&&!drop.requested,"X inhibitor retained");
    DeathBombDropMotion motion;
    check(normal_death_bomb_drop_motion(source,0,10,true,&motion)&&motion.phase540==1&&motion.request_entry_presentation&&
        motion.speed44==9&&motion.vertical_change50==-0.2f&&motion.turn_motion_due,"Drop starts a distinct motion branch, not impact");
    check(normal_death_bomb_drop_motion(source,1,10,false,&motion)&&!motion.request_entry_presentation&&!motion.turn_motion_due,
        "continued phase does not invent a repeated entry cue");
    DeathBombHistoryPlan history;
    check(normal_death_bomb_early_history(source,2,2,{12,3010,34},true,&history)&&history.restore_history&&
        history.root==std::array<f32,3>{12,3010,34}&&!history.vertical48,"early/defeated prefix restores XYZ, not zero Y");
    check(normal_death_bomb_early_history(source,1,2,{},false,&history)&&!history.restore_history,"active visit does not use history restore");

    GameContactImpactInput input{true,0,1,2,0,0,0,P::Clear,P::Clear,true,{10,-17,15}};
    GameContactImpactPlan impact;
    check(normal_death_bomb_game_impact(source,input,&impact)&&impact==GameContactImpactPlan{},
        "clear contact does not trigger impact after Drop");
    input.floor=P::Unresolved;
    check(normal_death_bomb_game_impact(source,input,&impact)&&impact.next_query==Q::Floor,"floor result still needed");
    input.floor=P::Clear;input.scene=P::Contact;
    check(normal_death_bomb_game_impact(source,input,&impact)&&impact.request_area_object&&impact.object_type==5&&impact.argument==17&&
        impact.owner_slot==2&&impact.pending663==1&&impact.request_position==input.matrix_translation,
        "separate game impact emits Bomb's exact object request and pending defeat");
    const auto accepted=impact;
    check(!normal_death_bomb_game_impact({0x0d06,0,2},input,&impact)&&impact==accepted,"model identity cannot substitute result");
    check(normal_death_icbm_game_impact({0x0d06,0,2},input,&impact)&&impact.object_type==73&&!impact.argument,
        "same gate still selects ICBM-specific object");
    input.lifecycle18=2;
    check(normal_death_bomb_game_impact(source,input,&impact)&&impact==GameContactImpactPlan{},
        "already-defeated actor cannot request late impact");
    GameContactDefeatPlan defeat;
    check(normal_death_bomb_consume_terminal(source,1,accepted.pending663,0x10,true,&defeat)&&
        defeat==GameContactDefeatPlan{true,2,0x11,0},"pending defeat consumer doesn't depend on object allocation");
    std::printf("normal Death Bomb separate Drop, history and shared game impact: %u checks PASS\n",checks);
}
