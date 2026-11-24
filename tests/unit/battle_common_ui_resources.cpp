#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_common_ui_resources.hpp"
#include "tests/unit/scene_archive_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"common UI resources: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;const auto bytes=scene_archive_fixture::bank();std::array<u8,40> raw{};
    const std::array<u32,10> words{0x47465549,1,40,2497,u32(bytes.size()),challenge_payload_hash({bytes.data(),bytes.size()}),1,0x944400,0x700000,0};
    for(unsigned i=0;i<10;++i){store_be32(raw.data()+4*i,words[i]);}BattleCommonUiProfile profile;check(profile.load({raw.data(),raw.size()},&error),"source recipe");
    for(unsigned route=0;route<5;++route){
        auto domain=scene_object_fixture::owner(32);auto pool=std::make_shared<SceneArchivePool>();check(pool->begin_fresh(scene_archive_fixture::profile(4),domain,&error),"shared archive pool");
        preparation_model_fixture::Files files;files.data=bytes;BattleCommonUiResources owner,moved;
        if(route==1){files.on_read=[&](){owner.retire();};}if(route==2){files.on_read=[&](){pool->retire();};}if(route==3){files.on_read=[&](){moved=std::move(owner);};}if(route==4){files.data.back()^=1;}
        const bool loaded=owner.load(profile,files,"owned",pool,&error);check(loaded==(route==0)&&files.reads==1&&!files.writes,"success/corrupt/retire/move branches");
        check(!owner.load(profile,files,"owned",pool,&error)&&files.reads==1,"no source read replay");
        if(route){check(!owner.current()&&!owner.bank()&&!moved.current(),"no late publication after refusal");continue;}
        check(pool->snapshot()->free_count==3&&owner.bank()&&owner.archive_record()->bank_loaded&&owner.bank()->records()==1,"actual shared HSD decode and archive allocation");
        const auto* bank=owner.bank();moved=std::move(owner);check(moved.current()&&moved.bank()==bank&&!owner.current(),"same immutable bank after move");pool->retire();check(!moved.current()&&!moved.bank(),"pool retirement closes input authority");
    }
    for(unsigned at:{0u,4u,8u,12u,16u,24u,28u,32u,36u}){auto bad=raw;bad[at]^=1;BattleCommonUiProfile invalid;check(!invalid.load({bad.data(),bad.size()},&error),"wrong source recipe/extent refused");}
    std::printf("battle common UI resources: %u checks PASS\n",checks);
}
