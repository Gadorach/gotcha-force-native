// Bounded normal Sasuke body and original amount/HP observation cuts.
#pragma once
#include "retail_recomp_fixture.hpp"
#include <array>

namespace retail_stationary_contact {
using namespace retail_fixture;
using HitBlock=std::array<u8,7840>;
using ActorSpec=std::array<u8,432>;
using Point=std::array<float,3>;
struct StationaryBody {u16 model,hp;u8 step,joint;u32 table;Point offset;};
inline constexpr StationaryBody sasuke{0x000A,180,9,2,0x802D4160,{0,-14,0}};
inline constexpr u32 receiver=0x8101C000,body=0x81044000,body_block=0x81045000,scene=0x81018000;
inline constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
inline constexpr u32 receiver_count=0x8043B5A0-21330,receiver_registry=0x803C417C;
inline constexpr u32 pair_pass=0x8002DB58,amount_entry=0x8003CD5C,hp_write=0x8003D344;

inline u8* at(std::vector<u8>& ram,u32 address){
    if(address<GC_RAM_BASE||address-GC_RAM_BASE>ram.size()-4)fail("contact scratch extent");
    return ram.data()+address-GC_RAM_BASE;
}
inline u16 half(std::vector<u8>& ram,u32 address){
    const auto* p=at(ram,address);return u16(u16(p[0])<<8|p[1]);
}
inline void place_receiver(std::vector<u8>& ram,const Point& center,u32 receiver_address=receiver){
    auto* target=at(ram,receiver_address);
    auto* joint=target+0x8D4+2*0x30;
    put_float(joint+12,center[0]);put_float(joint+28,center[1]+14.f);put_float(joint+44,center[2]);
    for(unsigned axis=0;axis<3;++axis)put_float(target+0x20+4*axis,center[axis]);
}
inline void bind_stationary(const Module& mod,std::vector<u8>& ram,const Point& center,const HitBlock& hit,
                            const ActorSpec& attacker,const ActorSpec& defender,unsigned owner_hp,StationaryBody spec,
                            u32 receiver_address=receiver,u32 stage_cell=0){
    if(stage_cell&&spec.model!=sasuke.model)fail("declared stage sphere requires the normal Sasuke bank");
    auto* target=at(ram,receiver_address);
    target[0]=1;target[0x18]=1;target[0x88]=1;target[0x3E4]=1;
    put32(at(ram,0x803C4E84+4),receiver_address);
    std::memcpy(at(ram,body_block),hit.data(),hit.size());
    at(ram,body_block)[0]=stage_cell?1:spec.step;at(ram,body_block)[1]=stage_cell?spec.step:255;
    if(stage_cell)at(ram,body_block)[2]=255;
    put16(target+0x3E8,spec.model);put32(target+0x3FC,body_block);
    put32(target+0x27C,spec.table);put32(target+0x76C,(1u<<spec.joint)|unsigned(stage_cell!=0));
    put_float(target+0xC0,1.f);
    auto* joint=target+0x8D4+spec.joint*0x30;
    for(unsigned axis=0;axis<3;++axis){
        put_float(joint+20*axis,1.f);
        put_float(joint+12+16*axis,center[axis]-spec.offset[axis]);
        put_float(target+0x20+4*axis,center[axis]);
        if(stage_cell){
            put_float(target+0x1AA4+20*axis,1.f);
            put_float(target+0x1AA4+12+16*axis,center[axis]);
        }
    }
    if(half(ram,free_count+8)||be32(at(ram,free_head+16)))fail("contact needs a distinct body slot");
    at(ram,body)[2]=4;
    if(stage_cell){at(ram,stage_cell)[2]=4;put32(at(ram,stage_cell+4),body);}
    put32(at(ram,free_head+16),stage_cell?stage_cell:body);put32(at(ram,free_tail+16),body);
    put16(at(ram,free_count+8),stage_cell?2:1);
    CPUState bind_body{};bind_body.gpr[3]=receiver_address;
    mod.run(bind_body,ram,0x8008AB30);
    if(be32(at(ram,active_head+50*4))!=body||be32(at(ram,body+0x28))!=body_block+32+spec.step*80||half(ram,free_count+8))
        fail("contact body did not bind its selected descriptor");
    if(stage_cell&&(be32(at(ram,stage_cell+0x20))!=receiver_address||be32(at(ram,stage_cell+0x28))!=body_block+32+80))
        fail("contact body did not bind Sasuke's original stage sphere");

    constexpr u32 owner_spec=0x81015000,target_spec=0x81015400,difficulty=0x81019600;
    std::memcpy(at(ram,owner_spec),attacker.data(),attacker.size());
    std::memcpy(at(ram,target_spec),defender.data(),defender.size());
    put32(at(ram,actor+0x4AC),owner_spec);put32(target+0x4AC,target_spec);
    put32(at(ram,0x8043B5A0-31824),difficulty);put32(at(ram,0x8043B5A0-31852),scene);
    for(unsigned team=0;team<2;++team){
        const auto hp=u16(team?spec.hp:owner_hp);
        const auto cell=team?receiver_address:actor;
        auto* fighter=at(ram,cell);
        put32(fighter+0x90,cell);
        put_float(fighter+0xB4,1.f);put_float(fighter+0xC4,1.f);
        put16(fighter+0x1C4,hp);put16(fighter+0x1C6,hp);put16(fighter+0x1C8,0xFFFF);
        put16(at(ram,0x803B069C+2*team),hp);
        put32(at(ram,0x803B068C+4*team),100);
        put32(at(ram,scene+team*60+0x114),100);put32(at(ram,scene+team*60+0x118),100);
    }
}
inline void bind_sasuke(const Module& mod,std::vector<u8>& ram,const Point& center,const HitBlock& hit,
                        const ActorSpec& attacker,const ActorSpec& defender,unsigned owner_hp){
    bind_stationary(mod,ram,center,hit,attacker,defender,owner_hp,sasuke);
}
inline unsigned pair_hp(const Module& mod,std::vector<u8>& ram,u32 attack,u32 record,bool expected){
    const auto hp=half(ram,receiver+0x1C6);
    const auto before=expected?ram:std::vector<u8>{};
    CPUState amount{};
    const auto cut=mod.run_to_or_return(amount,ram,pair_pass,{amount_entry,hp_write});
    if(half(ram,receiver+0x1C6)!=hp||(!expected&&cut)||
       (expected&&(cut!=amount_entry||amount.lr!=0x8002F10C||amount.gpr[3]!=record||
                  amount.gpr[4]!=attack||amount.gpr[5]!=receiver))){
        std::fprintf(stderr,"contact pair attack=%08X expected=%u cut=%08X record=%08X expected_record=%08X\n",
                     attack,unsigned(expected),cut,amount.gpr[3],record);
        fail("contact pair reached the wrong damage path");
    }
    if(!expected)return 0;
    std::memcpy(ram.data(),before.data(),ram.size());
    CPUState writer{};
    mod.run_to(writer,ram,pair_pass,{hp_write});
    if(writer.lr!=0x8002F1DC||writer.gpr[3]!=receiver||half(ram,receiver+0x1C6)!=hp)
        fail("contact pair reached the wrong HP writer");
    const unsigned damage=writer.gpr[4];
    mod.finish_stopped_leaf(writer,hp_write,164);
    return damage;
}
}
