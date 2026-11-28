// Original battle coordinator through its next scheduled actor pass.
#pragma once
#include "retail_stationary_contact.hpp"
#include <cstdint>

namespace retail_scene_entry {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
inline constexpr u32 scene=contact::scene, mode_ptr=0x8043B5A0-31856, hud_ptr=0x8043B5A0-21512;
inline constexpr u32 hud=0x81076000, dispatch=0x80028264, actor_pass=0x80053A2C;
inline constexpr std::array<u32,4> handlers{0x80029218,0x80029388,0x800293DC,0x80029408};

inline void prepare(std::vector<u8>& ram,bool fast=false,bool long_intro=false){
    const auto at=[&](u32 p){return contact::at(ram,p);};
    const auto mode=be32(at(mode_ptr));
    if(!mode)fail("scene startup needs a declared mode descriptor");
    put32(at(0x8043B5A0-31852),scene);
    put32(at(hud_ptr),hud);
    at(hud+13)[0]=1;
    at(mode+64)[0]=u8(fast);
    at(scene+5597)[0]=u8(long_intro);
    at(scene)[0]=2;at(scene+1)[0]=0;
    put16(at(scene+8),0);
    // Leave unrelated spawn/camera promotion lists empty; the physical
    // actor records used by readiness remain exactly in their seat scan.
    at(scene+0xEA)[0]=0;
}

struct Visit {unsigned handler,status,substep;int timer;unsigned hud_held;};
inline Visit advance(const Module& mod,std::vector<u8>& ram){
    const auto at=[&](u32 p){return contact::at(ram,p);};
    const auto ending=at(scene)[0]==3;
    const auto substep=at(scene+1)[0];
    if((!ending&&(at(scene)[0]!=2||substep>=handlers.size()))||(ending&&substep!=1))
        fail("normal scene coordinator inlet outside its tested table");
    const auto entry=ending?0x800289FC:handlers[substep];
    CPUState cpu{};CallObservation choice{entry};
    mod.run_to(cpu,ram,dispatch,{actor_pass},&choice);
    if(cpu.lr!=0x8002948C||choice.count!=1||
       choice.caller!=(ending?0x8002829C:0x80029190)||at(be32(at(mode_ptr))+66)[0]!=1)
        fail("normal scene did not select exactly one original handler before its world update");
    return {entry,at(scene+69)[0],at(scene+1)[0],
            std::bit_cast<std::int16_t>(contact::half(ram,scene+8)),at(hud+13)[0]};
}
} // namespace retail_scene_entry
