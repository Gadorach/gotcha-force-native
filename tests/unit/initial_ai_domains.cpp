#include <cstdio>
#include <cstdlib>
#include "tests/unit/startup_actor_setup_fixture.hpp"
#include "engine/assets/character_model_parts.hpp"
#include "engine/assets/battle_initial_ai.hpp"
#include "engine/gameplay/battle_actor_level_cost.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"initial AI domains: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;auto setup_bytes=startup_actor_setup_fixture::profile();StartupActorSetupProfile setup;
    store_be16(setup_bytes.data()+6,0x70b);check(!startup_actor_setup_load({setup_bytes.data(),setup_bytes.size()},1234,&setup,&error),"GFCS1 keeps its normal-pair domain");
    store_be16(setup_bytes.data()+4,2);check(startup_actor_setup_load({setup_bytes.data(),setup_bytes.size()},1234,&setup,&error)&&setup.domain==BattleModelDomain::InitialAi,"explicit V2 numeric binding");
    for(u16 model:{u16(0x615),u16(10),u16(0x10a),u16(0x70c)}){store_be16(setup_bytes.data()+6,model);check(!startup_actor_setup_load({setup_bytes.data(),setup_bytes.size()},1234,&setup,&error),"V2 does not broaden to adjacent or legacy classes");}
    std::array<u8,24> bytes{};store_be32(bytes.data(),0x47465054);store_be16(bytes.data()+4,2);store_be16(bytes.data()+6,0x70b);store_be32(bytes.data()+8,1234);store_be32(bytes.data()+12,24);
    CharacterModelPartsProfile parts;check(character_model_parts_load({bytes.data(),bytes.size()},1234,&parts,&error),"source empty-table domain");
    CharacterModelPartSelection selected;check(character_model_parts_select(parts,{255,255},0,0xffffffffu,&selected)&&!selected.visible[0]&&!selected.visible[1],"empty sentinel does not read selectors or fabricate a body attachment");
    store_be16(bytes.data()+4,1);check(!character_model_parts_load({bytes.data(),bytes.size()},1234,&parts,&error),"GFPT1 still refuses empty tables");
    store_be16(bytes.data()+4,2);store_be16(bytes.data()+6,0x615);check(!character_model_parts_load({bytes.data(),bytes.size()},1234,&parts,&error),"empty V2 is not an omission path for normal-pair parts");
    BattleActorLevelRow row;row.model=0x70b;row.model_hash=1234;row.full_cost=140;row.values[0]=200;BattleActorLevelCostPlan cost;
    check(!battle_actor_level_cost(row,1,0,&cost,&error),"legacy declared row cannot select AI domain");
    row.domain=BattleModelDomain::InitialAi;check(battle_actor_level_cost(row,1,0,&cost,&error)&&cost.cost4aa==93&&cost.shorts1c4_1c6_1c8==std::array<s16,3>{200,200,200},"AI uses the shared exact two-thirds cost operation");
    std::array<u8,80> ai{};const std::array<u32,15> words{0x4746414a,1,80,0,0x80027a44,0x802cfaa0,0x8019c60c,0x801985b0,0x800c627c,0x801fcc50,0x800c6394,0x800c6724,0x801fcda8,0x801fd09c,0x461c4000};
    for(unsigned i=0;i<words.size();++i)store_be32(ai.data()+4*i,words[i]);
    const std::array<u8,18> selectors{31,16,12,30,12,6,8,29,9,14,12,12,12,29,0,0,0,0};std::copy(selectors.begin(),selectors.end(),ai.begin()+60);
    BattleInitialAiProfile profile;check(profile.load({ai.data(),ai.size()},&error),"qualified global AI source operands");u8 value=255;
    check(profile.input(14,&value)&&value==0&&profile.input(0,&value)&&value==31&&!profile.input(18,&value),"table-driven AI control, bounded to actual selectors");
    ai[20]^=1;check(!profile.load({ai.data(),ai.size()},&error)&&!profile.input(0,&value),"altered source operands cannot survive failed reload");
    std::printf("initial AI domains: %u checks PASS\n",checks);
}
