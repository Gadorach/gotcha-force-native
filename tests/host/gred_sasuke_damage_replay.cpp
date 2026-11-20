// Source tables and a fresh-sphere bench. Neither the spheres, lists nor
// scalar inputs are live actor observations or license an HP write.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_damage_tables.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_fresh_pair.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_swept_pair.hpp"

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
    GRedSasukeSweptPairInput moving;
    static_cast<GRedSasukeOrderedPairSample&>(moving)=pair;
    moving.body=pair.body;
    moving.beam={true,{-60,0,0},{-60,0,0},{},1};
    GRedSasukeSweptPairPlan swept;
    if(!normal_gred_beam_sasuke_swept_pair(red,beam,moving,&swept)||swept.route!=GRedSasukeSweptRoute::RayCrossing||
       swept.point!=std::array<f32,3>{-22,0,0})return 1;
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
    std::puts("Separate moving shape-2 fixture: geometric candidate at (-22,0,0); no selection or callback.");
}
