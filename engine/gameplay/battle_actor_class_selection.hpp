#pragma once
#include "engine/assets/battle_normal_class.hpp"
#include "engine/assets/startup_actor_setup.hpp"
namespace gf {
//Source addresses below are non-callable provenance keys. Actual native source
//views are privately bound by the construction owner, never host pointers in u32.
struct BattleActorClassFields {
    std::array<u32,16> keys4b4_to_4f0{};std::array<u32,4> program_keys1d80{};
    u32 config_key4b0=0,parameters_key27c=0,initializer_key=0;bool installed=false;
};
enum class BattleActorClassWriteKind:u8 {SourceKey,ActorDataResource};
struct BattleActorClassWrite {BattleActorClassWriteKind kind=BattleActorClassWriteKind::SourceKey;u16 offset=0;u32 value=0;bool operator==(const BattleActorClassWrite&)const=default;};
struct BattleActorClassSelectionPlan {BattleActorClassFields fields;std::vector<BattleActorClassWrite> writes;};
//56CE0 selects the class-field installer (18CCFC/6F4F8), NOT the later4B8
//initializer. Two normal models only. No model construction/program execution.
inline bool battle_actor_class_select(const BattleNormalClassRow& row,const StartupActorSetupProfile& setup,u8 resource_slot,BattleActorClassSelectionPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="battle class selection: two-normal current source/config domain";return false;};
    if(!out||resource_slot>=6||!startup_actor_setup_detail::valid(setup)||!row.model_hash||row.model_hash!=setup.model_hash||row.model!=setup.model_id||row.variant||(row.model!=0x615&&row.model!=10))return fail();
    const u32 expected_initializer=row.model==0x615?0x8018ccfcu:0x8006f4f8u,expected_config=row.model==0x615?0x80365ac8u:0x802d3ee0u;
    if(row.initializer_key!=expected_initializer||row.config_key!=expected_config||row.config_hash!=challenge_payload_hash(startup_actor_setup_detail::config(setup))||row.actor_data_hash!=challenge_payload_hash(startup_actor_setup_detail::data(setup)))return fail();
    auto key=[](u32 value){return !value||(value>=0x80000000u&&value<0x81800000u);};
    for(u32 value:row.fields4b4_to_4f0)if(!key(value))return fail();
    for(u32 value:row.program_table_keys)if(!value||!key(value))return fail();
    if(!row.fields4b4_to_4f0[1]||!row.parameters27c_key||!key(row.parameters27c_key))return fail();
    BattleActorClassSelectionPlan p;p.fields={row.fields4b4_to_4f0,row.program_table_keys,row.config_key,row.parameters27c_key,row.initializer_key,true};
    using K=BattleActorClassWriteKind;auto put=[&](K kind,unsigned at,u32 value){p.writes.push_back({kind,u16(at),value});};
    for(unsigned i=0;i<16;++i)put(K::SourceKey,0x4b4+4*i,row.fields4b4_to_4f0[i]);
    put(K::ActorDataResource,0x4ac,resource_slot);
    put(K::SourceKey,0x4b0,row.model==10?0x802d3dc8u:row.config_key);
    for(unsigned i=0;i<4;++i)put(K::SourceKey,0x1d80+4*i,row.program_table_keys[i]);
    put(K::SourceKey,0x27c,row.parameters27c_key);
    if(row.model==10)put(K::SourceKey,0x4b0,row.config_key); //000A override AFTER default stores
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
