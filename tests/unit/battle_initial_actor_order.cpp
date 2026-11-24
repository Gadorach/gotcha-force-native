#include <cstdio>
#include <cstdlib>
#include <set>
#include "engine/gameplay/battle_initial_actor_order.hpp"
namespace {unsigned checks=0;void check(bool b){++checks;if(!b)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,64> bytes{};
    const std::array<u32,16> words{0x4746414f,1,64,0,0x80053b30,0x80053a2c,0x80054070,0x800055fc,0x803c4e6c,0x80436260,0x803c6dec,0x80436264,6,0,0x47435000,0x3f800000};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleInitialActorOrderProfile profile;check(profile.load({bytes.data(),bytes.size()},&error));
    BattleInitialActorOrderPlan plan;check(battle_initial_actor_order(profile,{0,1,0,1,0,0},{91,47},&plan,&error));
    check(plan.state.draws==std::array<u8,2>{233,149}&&plan.after==SourceByteRngState{149,172});
    check(plan.state.scan==std::array<u8,6>{4,3,2,1,0,5}&&plan.state.step==-1&&plan.state.count==2);
    check(plan.state.updates[0]==ScenePhysicalLink{0,3}&&plan.state.updates[1]==ScenePhysicalLink{0,1}&&!plan.state.updates[2]);
    check(battle_initial_actor_order(profile,{1,1,1,1,1,1},{255,255},&plan,&error)&&plan.state.scan==std::array<u8,6>{3,4,5,0,1,2}&&plan.state.step==1);
    for(unsigned mask=0;mask<64;++mask)for(const SourceByteRngState seed:std::array<SourceByteRngState,3>{{{0,0},{91,47},{255,255}}}){
        std::array<u8,6> active{};for(unsigned i=0;i<6;++i)active[i]=u8((mask>>i)&1);
        check(battle_initial_actor_order(profile,active,seed,&plan,&error));
        std::set<unsigned> selected;unsigned visited=0;
        for(u8 slot:plan.state.scan){check(slot<6&&!(visited&(1u<<slot)));visited|=1u<<slot;}
        for(unsigned i=0;i<6;++i){
            check(plan.state.factory_writes[i]==BattleInitialFactoryOrderFields{0,0,1});
            if(i<plan.state.count){check(plan.state.updates[i]&&plan.state.updates[i]->physical==0&&active[unsigned(plan.state.updates[i]->index)]);selected.insert(unsigned(plan.state.updates[i]->index));}
            else check(!plan.state.updates[i]); //unwritten tail is unknown, not zeroed
        }
        check(selected.size()==plan.state.count&&selected.size()==unsigned(std::popcount(mask))&&visited==63);
        check(plan.state.team_counts==std::array<u8,4>{});
        for(const auto& team:plan.state.teams)check(team==std::array<ScenePhysicalLink,6>{});
    }
    check(!battle_initial_actor_order(profile,{0,2,0,0,0,0},{91,47},&plan,&error)&&plan.state.count==0);
    check(!battle_initial_actor_order(profile,{},{91,47},nullptr,&error));
    bytes[16]^=1;check(!profile.load({bytes.data(),bytes.size()},&error)&&!battle_initial_actor_order(profile,{},{91,47},&plan,&error));
    std::printf("initial factory actor order: %u checks PASS\n",checks);
}
