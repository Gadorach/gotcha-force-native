#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_preparation_bootstrap.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"side archive: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;std::vector<u8> bytes(8192);
    store_be32(bytes.data(),3);store_be32(bytes.data()+4,1);store_be32(bytes.data()+8,1);store_be32(bytes.data()+12,1);
    store_be32(bytes.data()+4096,41);store_be32(bytes.data()+6144,2039);
    auto view=[&](){return ByteView{bytes.data(),bytes.size()};};BattleSideArchiveLayout layout;
    check(battle_side_archive_layout(view(),&layout,&error)&&layout.count==2&&layout.placement.offset==2048&&layout.placement.bytes==2048&&layout.models[0].offset==4096&&layout.models[0].bytes==41&&layout.models[1].offset==6144&&layout.models[1].bytes==2039,"placement skipped; each HSD excludes sector padding");
    const auto original=bytes;
    for(const auto [offset,value]:std::array<std::array<u32,2>,8>{{{0,0},{0,10},{0,0xffffffffu},{4,0},{8,0x40000001},{12,2},{4096,31},{6144,2049}}}){
        bytes=original;store_be32(bytes.data()+offset,value);
        check(!battle_side_archive_layout(view(),&layout,&error)&&!layout.count&&!layout.placement.bytes,"invalid counts/flags/extents publish no slices");
    }
    bytes=original;bytes.resize(10240);check(!battle_side_archive_layout(view(),&layout,&error),"unaccounted sectors refuse");
    bytes=original;check(!battle_side_archive_layout({bytes.data(),bytes.size()-1},&layout,&error)&&!battle_side_archive_layout({nullptr,8192},&layout,&error),"short/null input refuses");
    bytes.resize(4096);store_be32(bytes.data(),1);check(battle_side_archive_layout(view(),&layout,&error)&&!layout.count&&layout.placement.bytes==2048,"placement-only archive is not absent payload");
    SceneDeferredCallbacks closed;check(!closed.initialized()&&!closed.battle_cleanup_pending()&&!closed.word(0),"default ring is not a final-callback reset");
    BattlePreparationBootstrap bootstrap;std::array<const ForceInventory*,2> owners{};
    check(!bootstrap.initialize_side_resources(owners,&error)&&!bootstrap.side_resources(owners)&&!bootstrap.deferred_callbacks(owners),"closed bootstrap cannot allocate or register cleanup");
    std::printf("battle side archive: %u checks PASS\n",checks);
}
