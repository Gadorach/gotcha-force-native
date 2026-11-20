#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_damage_tables.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"G-Red/Sasuke tables: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    GRedBeamSasukeTableSample sample;
    GRedBeamSasukeTableIndices indices;
    check(!normal_gred_beam_sasuke_table_indices(sample,&indices),"missing scene/table inputs do not default");
    sample.declared=true;sample.credit_slot=0;sample.receiver_slot=1;
    sample.credit_level=sample.receiver_level=sample.context32=sample.credit_team88=0;sample.receiver_team88=1;
    sample.credit_cap1c4=200;sample.credit_hp_snapshot=200;sample.receiver_hp_snapshot=180;
    sample.credit_force_cap=sample.receiver_force_cap=100;
    sample.credit_difficulty=sample.receiver_difficulty=sample.credit_rank43a=sample.receiver_rank43a=sample.receiver_tier6ca=0;
    check(normal_gred_beam_sasuke_table_indices(sample,&indices)&&indices.credit_hp==0&&indices.receiver_hp==4&&
        indices.credit_force==31&&indices.receiver_force==31,"exact projected receiver HP and both force-zero indices");
    sample.credit_hp_snapshot=0;sample.receiver_hp_snapshot=20;
    sample.credit_force=99;sample.receiver_force=100;
    check(normal_gred_beam_sasuke_table_indices(sample,&indices)&&indices.credit_hp==31&&indices.receiver_hp==31&&
        indices.credit_force==1&&indices.receiver_force==0,"integer ratios then source upper index clamp");
    sample.credit_force=17;sample.credit_force_cap=43;
    check(normal_gred_beam_sasuke_table_indices(sample,&indices)&&indices.credit_force==20,"unequal declared denominator");
    sample.receiver_tier6ca=4;
    check(!normal_gred_beam_sasuke_table_indices(sample,&indices)&&indices.credit_force==20,"unreviewed tier leaves result unchanged");
    sample.receiver_tier6ca=0;sample.credit_6fc=1;
    check(!normal_gred_beam_sasuke_table_indices(sample,&indices),"active modifiers refused by this lookup");
    sample.credit_6fc=0;sample.credit_hp_snapshot=201;
    check(!normal_gred_beam_sasuke_table_indices(sample,&indices),"history outside reviewed cap is not clamped into a row");
    sample.credit_hp_snapshot=200;sample.credit_force_cap=0;
    check(!normal_gred_beam_sasuke_table_indices(sample,&indices),"force cap cannot be absent");
    sample.credit_force_cap=43;

    GRedSasukeDamageTable table;
    std::array<u8,GRedSasukeDamageTable::byte_size> zeros{};
    check(!table.load({zeros.data(),zeros.size()})&&!table.ready(),"foreign coefficients rejected");
    const NormalGRedNeutralBSource red{0x0615,0,0};
    GRedBeamSasukeContactPrefix contact;
    contact.beam_receiver290=contact.credit_receiver290=1;contact.receiver_reaction702=4;contact.receiver_binding71a=2;
    GRedBeamSasukeHealthInput in;
    in.reviewed_ordinary=true;in.receiver_slot=1;in.current1c6=in.max1c4=180;in.beam_relative_c4=in.receiver_b4=1;
    in.factors={true,1,1,1,1,1,1,1,1,1};
    GRedBeamSasukeHealthPlan plan;
    check(!normal_gred_beam_sasuke_health_from_tables(red,contact,in,sample,table,&plan),
        "bound entry refuses to substitute manually supplied factors");
    in.receiver_mask5a0=4;
    check(normal_gred_beam_sasuke_health_from_tables(red,contact,in,{},table,&plan)&&
        plan.write_health&&!plan.amount&&plan.current1c6==180&&plan.previous1c8==180,
        "source zero branch uses no table sample but still reaches its writer");
    std::puts("G-Red/Sasuke tables: scoped integer selection and refusal PASS");
}
