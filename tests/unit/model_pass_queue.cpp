#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/model_pass_queue.hpp"
#include "tests/unit/scene_pass_fixture.hpp"
namespace {unsigned checks=0;void check(bool b){++checks;if(!b)std::abort();}}
int main(){
    using namespace gf;std::array<u8,56> raw{};store_be32(raw.data(),0x47465150);store_be32(raw.data()+4,1);store_be32(raw.data()+8,56);
    const std::array<s32,8> groups{0,1,2,-1,3,-1,-1,-1};const std::array<u8,6> passes{1,4,2,2,2,2};
    for(unsigned i=0;i<8;++i){store_be32(raw.data()+16+4*i,u32(groups[i]));}std::copy(passes.begin(),passes.end(),raw.data()+48);
    ModelPassQueueProfile profile;std::string error;check(model_pass_queue_profile_load({raw.data(),raw.size()},&profile,&error));
    for(unsigned i=0;i<raw.size();++i){auto bad=raw;bad[i]^=1;ModelPassQueueProfile invalid;check(!model_pass_queue_profile_load({bad.data(),bad.size()},&invalid,&error));}
    ModelPassQueue queue;std::vector<ModelPassVisit> plan;check(queue.load_global(profile)&&!queue.plan(&plan)&&queue.begin(1));
    check(queue.register_model(244,0x47,true)&&queue.register_model(245,0x47,true)&&queue.register_model(251,0x47,true)&&queue.plan(&plan)&&plan.size()==9);
    for(unsigned i=0;i<9;++i)check(plan[i].model==std::array<u32,3>{251,245,244}[i%3]&&plan[i].queue==std::array<u8,3>{0,1,5}[i/3]&&plan[i].pass==passes[plan[i].queue]);
    check(!queue.load_global(profile)&&!queue.load(scene_pass_fixture::profile())&&queue.plan(&plan)&&plan.size()==9);
    check(queue.begin(2)&&queue.plan(&plan)&&plan.empty());check(!queue.register_model(1,0x34,true)&&!queue.ready());
    ModelPassQueue legacy;check(legacy.load(scene_pass_fixture::profile())&&legacy.begin(1)&&legacy.register_model(1,7,true)&&!legacy.load_global(profile)&&legacy.plan(&plan)&&plan.size()==3);
    std::printf("global model pass queues: %u checks PASS\n",checks);
}
