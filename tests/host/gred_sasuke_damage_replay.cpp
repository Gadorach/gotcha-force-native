// Source tables and a fresh-sphere bench. Neither the spheres, lists nor
// scalar inputs are live actor observations or license an HP write.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_damage_tables.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_fresh_pair.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_selected_hit.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_post_hit.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_ko_prefix.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_ko_tally.hpp"

int main(int argc,char** argv){
    using namespace gf;
    if(argc!=2){std::fprintf(stderr,"usage: %s ORDINARY.GFDA\n",argv[0]);return 2;}
    std::error_code ec;
    const std::filesystem::path path(argv[1]);
    if(!std::filesystem::is_regular_file(path,ec)||std::filesystem::file_size(path,ec)!=GRedSasukeDamageTable::byte_size||ec){
        std::fputs("exact local GFDA profile required\n",stderr);return 1;
    }
    std::array<u8,GRedSasukeDamageTable::byte_size> bytes;
    std::ifstream file(path,std::ios::binary);
    GRedSasukeDamageTable table;
    if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))||
       file.peek()!=std::char_traits<char>::eof()||file.bad()||!table.load({bytes.data(),bytes.size()})){
        std::fputs("GFDA profile refused\n",stderr);return 1;
    }

    constexpr NormalGRedNeutralBSource red{0x0615,0,0};
    constexpr GRedBeamShapeOperands beam{{4,48,49,26,0,false},0,15,0,0,true,0,-2,20};
    GRedSasukeFreshPairInput pair;
    auto& ordered=pair.contact_fields;
    ordered.receiver_model=10;ordered.receiver_slot=ordered.receiver_credit_slot=1;
    ordered.receiver_descriptor=9;ordered.receiver_hit_row=0;ordered.beam_token26c=1;
    pair.opposing_direct_scope_declared=pair.lists_declared=true;
    pair.attack_list.lists=1u<<unsigned(CollisionRegistrationList::L477C);
    pair.receiver_list.lists=1u<<unsigned(CollisionRegistrationList::L417C);
    // Declared first-phase spheres at unit scale, with the exact local
    // radius operands; not a moving beam or the retail actor's live pose.
    pair.beam={true,1,0,{42,0,0},20,20};pair.body={true,1,0,{},22,22};
    GRedSasukeFreshPairPlan overlap,other;
    if(!normal_gred_beam_sasuke_fresh_pair(red,beam,pair,&overlap)||overlap.route!=GRedSasukeFreshPairRoute::Overlap||
       overlap.point!=std::array<f32,3>{22,0,0})return 1;
    GRedSasukeMovingReceiver moving;
    static_cast<GRedSasukeOrderedPairSample&>(moving)=pair;moving.body=pair.body;
    GRedSasukeMovingScanInput scan;
    scan.complete_reviewed_list_declared=true;scan.contact_update100=0x800c44a8;
    scan.beam={true,{-60,0,0},{-60,0,0},{},1};
    scan.carrier={{-61,0,0},{},{80,0,0},80,61};
    if(!normal_gred_beam_scale_seed(red,1,scan.carrier.distance180,&scan.stored_scale))return 1;
    const std::array receivers{moving};
    GRedSasukeMovingSelection selected;
    if(!normal_gred_beam_sasuke_select_moving(red,beam,scan,receivers,&selected)||!selected.selected||selected.slot!=1||
       !selected.refresh114||selected.point!=std::array<f32,3>{-22,0,0}||
       selected.carrier.position20!=selected.point||selected.carrier.distance180!=61)return 1;
    pair.beam.center64[0]=43;
    if(!normal_gred_beam_sasuke_fresh_pair(red,beam,pair,&other)||other.route!=GRedSasukeFreshPairRoute::BroadMiss)return 1;
    pair.beam.center64[0]=42;ordered.receiver_tokens20c=overlap.prefix.receiver_tokens20c;
    if(!normal_gred_beam_sasuke_fresh_pair(red,beam,pair,&other)||other.route!=GRedSasukeFreshPairRoute::RepeatToken)return 1;
    GRedBeamSasukeTableSample sample;
    sample.declared=true;sample.credit_slot=0;sample.receiver_slot=1;
    sample.credit_level=sample.receiver_level=sample.context32=sample.credit_team88=0;sample.receiver_team88=1;
    sample.credit_cap1c4=200;sample.credit_hp_snapshot=200;sample.receiver_hp_snapshot=180;
    sample.credit_force_cap=sample.receiver_force_cap=100;
    sample.credit_difficulty=sample.receiver_difficulty=sample.credit_rank43a=sample.receiver_rank43a=sample.receiver_tier6ca=0;
    GRedBeamSasukeHealthInput in;
    in.reviewed_ordinary=true;in.receiver_slot=1;in.beam_relative_c4=in.receiver_b4=1;
    in.current1c6=in.max1c4=180;in.previous1c8=29;
    GRedBeamSasukeHealthPlan plan;
    if(!normal_gred_beam_sasuke_health_from_tables(red,overlap.prefix,in,sample,table,&plan)||
       plan.amount!=19||!plan.write_health||plan.current1c6!=161||plan.previous1c8!=180||plan.reaches_zero){
        std::fputs("ordinary source table fixture differed\n",stderr);return 1;
    }
    std::printf("declared level-zero 0615/0 -> 000A/0: source-table amount %d, proposed HP %d -> %d\n",
        int(plan.amount),int(plan.previous1c8),int(plan.current1c6));
    std::puts("Fresh sphere fixture: tangency at (22,0,0); one-unit gap and repeated token refused. No live HP was written.");
    GRedBeamSasukePostHitInput after;
    after.reviewed_post_contact_declared=true;after.credit_total418=100;after.receiver_total41c=30;
    after.receiver.restore6be=90;after.receiver.remaining6c2=100;after.receiver.threshold6c4=50;
    after.receiver.impact6c6=120;after.receiver.flags6fd=selected.contact.receiver_flags6fd;
    GRedBeamSasukePostHitPlan moving_hit,lethal,held;
    if(!normal_gred_beam_sasuke_post_hit(red,selected.contact,in,sample,table,after,&moving_hit)||
       moving_hit.health.amount!=19||moving_hit.health.current1c6!=161||
       moving_hit.credit_total418!=119||moving_hit.receiver_total41c!=49||!moving_hit.reaction.visited||
       moving_hit.reaction.fields.impact6c6!=70||moving_hit.reaction.fields.remaining6c2!=50)return 1;
    in.current1c6=7;
    if(!normal_gred_beam_sasuke_post_hit(red,selected.contact,in,sample,table,after,&lethal)||!lethal.request_ko_continuation||
       lethal.health.current1c6||lethal.credit_total418!=119||lethal.reaction.visited)return 1;
    GRedBeamSasukeKoInput ko_input;
    ko_input.direct_mode_zero_declared=true;ko_input.credit_slot=0;ko_input.receiver_slot=1;
    ko_input.credit_team88=0;ko_input.receiver_team88=1;ko_input.context32=0;
    // Normal Sasuke's ordinary full-cost source operand. In a scene
    // this must come from the receiving actor's runtime +4AA.
    ko_input.receiver_cost4aa=150;ko_input.receiver_team_remaining118=140;ko_input.debit108=1;
    GRedBeamSasukeKoPlan ko;
    if(!normal_gred_beam_sasuke_ko_prefix(red,lethal,ko_input,&ko)||!ko.team_debited||
       ko.receiver_team_remaining118||ko.credit_ko_cost420!=150||ko.scene_credit_b4!=0||ko.scene_receiver_b8!=1||
       !ko.observer_scan_pending)return 1;
    GRedSasukeKoObserverSnapshot observers;
    observers.six_physical_slots_declared=true;
    observers.slots[0].active00=1;observers.slots[0].team88=0;observers.slots[0].cursor484=7;
    observers.slots[1].active00=1;observers.slots[1].team88=1;observers.slots[1].slot3e4=1;
    GRedSasukeKoObserverPlan writes;
    if(!normal_gred_beam_sasuke_ko_observers(red,ko,observers,&writes)||writes.queue_write_mask!=1||
       writes.ally_stat_mask||writes.slots[0].queue464[7]!=GRedSasukeKoObserverEvent{3,1}||
       writes.slots[0].cursor484||writes.slots[1]!=observers.slots[1])return 1;
    StoryRewardCounters tally;
    GRedSasukeKoTallyInput tally_in;
    tally_in.caller_and_rng_declared=true;tally_in.credit_slot=0;tally_in.receiver_slot=1;
    tally_in.context32=tally_in.credit_selector3e6=tally_in.credit_kind83=tally_in.receiver_kind83=0;
    tally_in.receiver_link4a0=tally_in.receiver_variant3ed=0;tally_in.receiver_model3ea=0x000a;tally_in.rng={5,1};
    GRedSasukeKoTallyPlan tally_plan;
    if(!normal_gred_beam_sasuke_ko_tally(red,writes,tally,tally_in,&tally_plan)||!tally_plan.appended||
       tally_plan.active_index||tally_plan.active_count!=1||tally_plan.stored_word!=~0x000a0009u||
       tally_plan.rng!=SourceByteRngState{8,3}||tally_plan.receiver_credit5a4_slot!=0||tally.active_count)return 1;
    in.current1c6=180;in.scene_hp_hold1f=true;
    if(!normal_gred_beam_sasuke_post_hit(red,selected.contact,in,sample,table,after,&held)||held.stat_write||
       held.health.write_health||!held.reaction.visited)return 1;
    std::puts("Separate moving shape-2 fixture: selected (-22,0,0), moved and rebuilt candidate matrix, proposed HP 180 -> 161.");
    std::puts("Moving post-hit alternatives: surviving counters kept separate; 7 -> 0 selects KO continuation; scene hold skips HP and stats.");
    std::puts("Detached KO ledger: declared cost 150, team budget 140 -> 0, credited slot 0.");
    std::puts("Observer fixture: slot 0 writes (3,1) at cursor 7 -> 0; fallen slot skipped. No event delivered.");
    std::puts("Story tally fixture: proposed normal Sasuke key +9 and credit slot 0. No reward or save write.");
}
