#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_initial_common_auxiliary.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,240> bytes{};
    const std::array<u32,20> header{0x47464341,1,240,0,0x800c7664,0x800c76a0,0x80089100,0x80006fb4,0x80042954,0x80040eb4,0x80006c88,0x800c74ec,0x800c79f8,0x800c7954,0x800c77ec,0x800c790c,3,0x615,0x6dd5d3b2,2};
    const std::array<u32,40> data{0x47464155,0x00010002,160,0x6dd5d3b2,0x25ac8,0x00030004,0x28000200,0x3303071c,0x2aaa0000,0x41900000,0x41700000,0xc1d80000,0x3ecccccd,0x3303071c,0x2aaa0000,0xc1900000,0x41700000,0xc1d80000,0x3ecccccd,0x29020200,0x33030000,0x0e380000,0x41900000,0x41700000,0xc1d80000,0x3ecccccd,0x33030000,0x0e380000,0xc1900000,0x41700000,0xc1d80000,0x3ecccccd,0x3f4ccccd,0x3f800000,0x3f4ccccd,0x3f800000,0x3f4ccccd,0x3f800000,0x38c90fdb,0x06150033};
    for(unsigned i=0;i<header.size();++i)store_be32(bytes.data()+4*i,header[i]);
    for(unsigned i=0;i<data.size();++i)store_be32(bytes.data()+80+4*i,data[i]);
    BattleInitialCommonAuxiliaryProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready()&&profile.auxiliary()->model_hash==0x6dd5d3b2);
    StartupAuxiliaryState input;input.allocated=true;input.captured94=0x615;StartupAuxiliaryState out;
    check(battle_initial_common_auxiliary_state(profile,3,input,&out,&error)&&out.phase==1&&out.render_group==1&&out.allocated&&!out.matrices_valid&&!out.counter&&!out.cached_a8&&!out.previous_a9);
    for(const auto& matrix:out.matrices)for(float value:matrix.m)check(value==0); //storage zeroes, NOT identity/worlds
    for(unsigned slot:{0u,1u,2u,4u,5u})check(!battle_initial_common_auxiliary_state(profile,u8(slot),input,&out,&error));
    input.phase=1;check(!battle_initial_common_auxiliary_state(profile,3,input,&out,&error));input.phase=0;
    input.matrices_valid=true;check(!battle_initial_common_auxiliary_state(profile,3,input,&out,&error));input.matrices_valid=false;
    for(unsigned at=0;at<bytes.size();at+=4){auto bad=bytes;bad[at]^=1;BattleInitialCommonAuxiliaryProfile p;check(!p.load({bad.data(),bad.size()},&error)&&!p.ready()&&!p.auxiliary());}
    for(unsigned n=0;n<bytes.size();++n){BattleInitialCommonAuxiliaryProfile p;check(!p.load({bytes.data(),n},&error));}
    std::printf("source split-effect common auxiliary/inline storage: %u checks PASS\n",checks);
}
