#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_actor_dimensions.hpp"
#include "tests/unit/startup_actor_setup_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"battle dimensions: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;const auto bytes=startup_actor_setup_fixture::profile();StartupActorSetupProfile setup;check(startup_actor_setup_load({bytes.data(),bytes.size()},1234,&setup,&error),"owned setup");
    BattleActorScalarEffects effects;for(unsigned i=0;i<15;++i)effects.values850[i]=f32(i)-5.5f;
    BattleActorDimensions result;check(battle_actor_fresh_dimensions(setup,effects,2,&result,&error),"fresh dimension subset");
    check(result.field668==160&&result.field66c==60&&result.field670==10&&result.field674==40&&result.field770==90,"shared55DCC scale products");
    check(result.size7fc==1&&result.offset6d0==0&&result.cached87==0&&result.input334==32,"unscaled68004 size and reached fresh/human branches");
    check(result.values814==effects.values850&&result.values678==std::array<f32,3>{-1,0,1},"source field copies");
    check(!battle_actor_fresh_dimensions(setup,effects,-1,&result,&error)&&result.writes.empty(),"invalid scale refuses without candidate writes");
    effects.values850[0]=bits_to_f32(0x7fc00000);check(!battle_actor_fresh_dimensions(setup,effects,1,&result,&error),"nonfinite current effect vector refuses");
    std::printf("battle actor dimensions: %u checks PASS\n",checks);
}
