#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_initial_shape_continuation.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,212> bytes{};
    const std::array<u32,4> header{0x47465348,1,212,0};
    const std::array<u32,29> light{0x47465353,1,116,0,0x8001c42c,0x8001c468,0x8001c524,0x80089100,0x80008a94,0x80245f20,0x80040e94,0x802442dc,0x80245a8c,0x80245acc,0x8020bab8,0x8020bb14,0x070b,0xbdb1d4f4,0xbdf8e542,2,0,0,19,0x3f800000,0x3a83126f,0x38100000,0,0x10,1};
    const std::array<u32,20> rows{0,0x070b,0xbdb1d4f4,16,19,1,0x000a,0xeab3632a,16,19,2,0x0105,0x4fb1157d,16,19,3,0x0615,0xe2b4439a,16,19};
    for(unsigned i=0;i<header.size();++i)store_be32(bytes.data()+4*i,header[i]);
    for(unsigned i=0;i<light.size();++i)store_be32(bytes.data()+16+4*i,light[i]);
    for(unsigned i=0;i<rows.size();++i)store_be32(bytes.data()+132+4*i,rows[i]);
    BattleInitialShapeContinuationProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready()&&profile.light_recipe());
    for(unsigned slot=0;slot<4;++slot){const auto row=profile.owner(slot);check(row&&row->slot==slot&&row->model==rows[5*slot+1]&&row->hash==rows[5*slot+2]&&row->bones==std::array<u8,2>{16,19});}
    check(!profile.owner(4)&&!profile.owner(5)&&!profile.owner(255));
    for(unsigned at=0;at<bytes.size();at+=4){auto bad=bytes;bad[at]^=1;BattleInitialShapeContinuationProfile p;check(!p.load({bad.data(),bad.size()},&error)&&!p.ready()&&!p.light_recipe()&&!p.owner(0));}
    for(unsigned n=0;n<bytes.size();++n){BattleInitialShapeContinuationProfile p;check(!p.load({bytes.data(),n},&error));}
    std::printf("source shape slot/model continuation profile: %u checks PASS\n",checks);
}
