#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_setup_presentation.hpp"
#include "tests/unit/challenge_force_visual_fixture.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"second-box edges: %s\n",why);std::abort();}}
struct Save final:gf::ISaveBackend {unsigned reads=0,writes=0;bool load(gf::u32,std::vector<gf::u8>&)override{++reads;return false;}bool store(gf::u32,const gf::u8*,std::size_t)override{++writes;return false;}bool erase(gf::u32)override{return false;}};
}
int main(){
    using namespace gf;const auto catalog=force_inventory_fixture::catalog();ForceInventory first,second,neighbor;check(first.create(catalog)&&second.load_state(catalog,*first.state())&&neighbor.create(catalog),"distinct owned inventories");
    ChallengeSetupPresentation unopened;ChallengeSetupChoice choice;for(unsigned player:{0u,1u,2u,0xffffffffu})check(!unopened.take_choice(player,&choice)&&choice.kind()==ChallengeSetupChoiceKind::None&&!unopened.player_inventory(player),"invalid/unready candidate outputs bounded and empty");check(!unopened.take_choice(0,nullptr)&&!choice.current(first),"null output/default candidate refuses");
    ChallengeForceVisual visual;const auto art=challenge_force_visual_fixture::art();const auto cards=challenge_force_visual_fixture::cards();check(visual.open(art,cards,first)&&visual.step(first,{true,0,1000}),"owned visual binding");check(!visual.switch_inventory(first,first)&&!visual.ready()&&visual.draws().empty(),"second owner cannot alias first inventory");check(visual.open(art,cards,first)&&!visual.switch_inventory(neighbor,second),"foreign prior owner cannot authorize switch");ForceInventory missing;check(visual.open(art,cards,first)&&!visual.switch_inventory(first,missing),"unready second inventory refuses");
    Save a,b;ForceInventoryStore a0(a),a0alias(a),a1(a,1),b0(b);check(a0.same_namespace(a0alias)&&!a0.same_namespace(a1)&&!a0.same_namespace(b0),"namespace identity is exact backend/slot pair, not wrapper address");
    ChallengeSetupPagesProfile pages{1,1,1,1,1};ChallengeLoadBox page;ChallengeBoxEvent event;std::array<ChallengeSetupButtons,4> history{};std::array<ChallengeSetupPad,4> pads{};check(page.open(pages,2,0,1,history)&&page.step(true,pads,nullptr,&event)&&page.step(true,pads,nullptr,&event),"owned second box opens");pads[1]={0x100,true};check(page.step(true,pads,nullptr,&event)&&page.step(true,pads,nullptr,&event)&&event.request_load,"actual second load request");ChallengeBoxLoadDriver driver;std::string error;check(!driver.begin(event.load,catalog,a0,&a0alias,&error)&&!a.reads&&!b.reads,"same namespace refuses before any native read");check(driver.begin(event.load,catalog,a0,&a1,&error)&&driver.notice()==ChallengeNativeLoadNotice::Invalid&&!a.reads&&!b.reads,"misconfigured OPTION slot1 is refused by unchanged save codec before backend IO");check(!a.writes&&!b.writes,"no save writes");
    std::printf("second-box edges: %u owned checks PASS\n",checks);
}
