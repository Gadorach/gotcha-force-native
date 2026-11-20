#pragma once
#include "engine/assets/gred_sasuke_damage.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_health.hpp"

namespace gf {
struct GRedBeamSasukeTableSample {
    bool declared=false;
    u8 credit_slot=255,receiver_slot=255,credit_level=255,receiver_level=255;
    u8 context32=255,credit_team88=255,receiver_team88=255;
    u8 credit_selector3e6=0,receiver_selector3e6=0,credit_6fc=0,receiver_6fc=0;
    u16 credit_cap1c4=0,credit_hp_snapshot=0,receiver_hp_snapshot=0;
    u16 credit_force=0,credit_force_cap=0,receiver_force=0,receiver_force_cap=0;
    u8 credit_difficulty=255,receiver_difficulty=255,credit_rank43a=255,receiver_rank43a=255,receiver_tier6ca=255;
};
struct GRedBeamSasukeTableIndices {
    u8 credit_hp=0,receiver_hp=0,credit_force=0,receiver_force=0;
};

// Scoped 3CD5C integer selection for the two level-zero caps, declared
// ordinary unbuffed 0/1 team inputs. HP history here is not current HP.
inline bool normal_gred_beam_sasuke_table_indices(const GRedBeamSasukeTableSample& sample,
                                                  GRedBeamSasukeTableIndices* out){
    if(!out||!sample.declared||sample.credit_slot>=6||sample.receiver_slot>=6||
       sample.credit_slot==sample.receiver_slot||sample.credit_level||sample.receiver_level||
       sample.context32||sample.credit_team88||sample.receiver_team88!=1||
       sample.credit_selector3e6||sample.receiver_selector3e6||sample.credit_6fc||sample.receiver_6fc||
       sample.credit_cap1c4!=200||sample.credit_hp_snapshot>200||sample.receiver_hp_snapshot>180||
       !sample.credit_force_cap||sample.credit_force>sample.credit_force_cap||
       !sample.receiver_force_cap||sample.receiver_force>sample.receiver_force_cap||
       sample.credit_difficulty>2||sample.receiver_difficulty>2||sample.credit_rank43a>3||
       sample.receiver_rank43a>3||sample.receiver_tier6ca>3)return false;
    const auto index=[](unsigned sample,unsigned cap){
        const unsigned inverse=32u-(32u*sample)/cap;
        return u8(inverse>31?31:inverse);
    };
    const unsigned receiver_projected=sample.receiver_hp_snapshot<=20?0u:sample.receiver_hp_snapshot-20u;
    *out={index(sample.credit_hp_snapshot,200),index(receiver_projected,180),
          index(sample.credit_force,sample.credit_force_cap),index(sample.receiver_force,sample.receiver_force_cap)};
    return true;
}

inline bool normal_gred_beam_sasuke_table_factors(const GRedBeamSasukeTableSample& sample,
    const GRedSasukeDamageTable& table,GRedBeamSasukeHealthFactors* out){
    using R=GRedSasukeDamageRow;
    GRedBeamSasukeTableIndices i;
    GRedBeamSasukeHealthFactors f;
    if(!out||!normal_gred_beam_sasuke_table_indices(sample,&i)||
       !table.factor(R::CreditDifficulty,sample.credit_difficulty,&f.credit_difficulty)||
       !table.factor(R::CreditHealth,i.credit_hp,&f.credit_health)||
       !table.factor(R::CreditForce,i.credit_force,&f.credit_force)||
       !table.factor(R::CreditRank,sample.credit_rank43a,&f.credit_rank)||
       !table.factor(R::ReceiverDifficulty,sample.receiver_difficulty,&f.receiver_difficulty)||
       !table.factor(R::ReceiverHealth,i.receiver_hp,&f.receiver_health)||
       !table.factor(R::ReceiverForce,i.receiver_force,&f.receiver_force)||
       !table.factor(R::ReceiverRank,sample.receiver_rank43a,&f.receiver_rank)||
       !table.factor(R::ReceiverTier,sample.receiver_tier6ca,&f.receiver_tier))return false;
    f.declared=true;*out=f;return true;
}

// The bound entry never uses manually supplied factors. A source zero
// branch still bypasses the table reads; the downstream HP writer is
// governed independently. No live factor capture or HP write is added.
inline bool normal_gred_beam_sasuke_health_from_tables(NormalGRedNeutralBSource source,
    const GRedBeamSasukeContactPrefix& contact,const GRedBeamSasukeHealthInput& input,
    const GRedBeamSasukeTableSample& sample,const GRedSasukeDamageTable& table,GRedBeamSasukeHealthPlan* out){
    if(!out||!source.reviewed()||input.max1c4!=180)return false;
    auto in=input;in.factors={};
    if(!(in.receiver_flags1da&2u)&&!(in.receiver_mask5a0&4u)&&
       (sample.credit_slot!=source.slot||sample.receiver_slot!=in.receiver_slot||
        !normal_gred_beam_sasuke_table_factors(sample,table,&in.factors)))return false;
    return normal_gred_beam_sasuke_health_plan(source,contact,in,out);
}
}
