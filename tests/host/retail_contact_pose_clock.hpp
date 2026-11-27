// Bounded original post-contact body clock: no frame-loop or pose advancement.
#pragma once
#include "retail_stationary_contact.hpp"
#include <algorithm>

namespace retail_contact_pose_clock {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
inline constexpr u32 clock_entry=0x8005A378,timer=0x71C,base=0x768,primary=0x5F4,secondary=0x1DC8;
inline constexpr u32 scale_a=0x1DCC,scale_b=0x1DD0,override_flag=0x4A1,link_kind=0x4A0,link=0x4A4;
inline constexpr u32 unit=0x3F800000,boost=0x3FC00000,slow=0x3CF5C28F;
inline constexpr u32 neutral_secondary=0x3EC00000,boosted_secondary=0x3F100000;

inline void tick(const Module& mod,std::vector<u8>& ram,u32 body){
    CPUState state{};state.gpr[3]=body;
    mod.run(state,ram,clock_entry);
}
inline void factors(std::vector<u8>& ram,u32 body){
    put_float(contact::at(ram,body+scale_a),0.5f);
    put_float(contact::at(ram,body+scale_b),0.75f);
}
inline void check_clocks(std::vector<u8>& ram,u32 body,unsigned expected_timer,u32 b,u32 p,u32 s){
    const auto at=[&](u32 o){return contact::at(ram,body+o);};
    if(at(timer)[0]!=expected_timer||be32(at(base))!=b||be32(at(primary))!=p||be32(at(secondary))!=s){
        std::fprintf(stderr,"body clock: addr=%08X timer=%u/%u rates=%08X/%08X/%08X expected=%08X/%08X/%08X\n",
                     body,at(timer)[0],expected_timer,be32(at(base)),be32(at(primary)),be32(at(secondary)),b,p,s);
        fail("body clock did not follow the declared source state");
    }
}

inline void projectile(const Module& mod,std::vector<u8>& ram,unsigned kind,u16 model,u32 attack,u32 record,
                       unsigned seed,unsigned prior,unsigned damage){
    constexpr auto body=contact::receiver;
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto carrier_kind=at(attack+0x83)[0];
    if(!carrier_kind||half(body+0x1C6)!=180||half(body+0x20C)||!half(attack+0x26C)||prior>8||seed>8||
       at(body+0x83)[0]||at(actor+timer)[0]||at(body+timer)[0]||at(body+override_flag)[0]||
       at(body+link_kind)[0]||at(body+0x2AF)[0]||half(contact::free_count+4)||
       at(record+8)[0]!=seed||(half(record+18)&0x0FC0))
        fail("contact clock needs the admitted ordinary projectile and normal body");
    factors(ram,body);
    tick(mod,ram,body); // Bootstrap the normal rate fields, not the held inlet.
    check_clocks(ram,body,0,unit,unit,neutral_secondary);
    at(body+timer)[0]=u8(prior);
    const auto token=half(attack+0x26C);
    const auto armed=std::max(seed,prior);
    {
        auto fork=ram;
        CPUState inlet{};CallObservation amount{contact::amount_entry};
        mod.run_to(inlet,fork,contact::pair_pass,{contact::amount_entry},&amount);
        if(amount.count!=1||amount.caller!=0x8002F108||amount.arguments[0]!=record||
           amount.arguments[1]!=attack||amount.arguments[2]!=body||
           contact::half(fork,body+0x1C6)!=180||contact::at(fork,body+0x2AF)[0]||
           contact::at(fork,actor+timer)[0])
            fail("contact clock amount cut changed HP, presentation or credit prematurely");
        check_clocks(fork,body,armed,unit,unit,neutral_secondary);
    }
    CPUState full{};
    mod.run(full,ram,contact::pair_pass,4096);
    const auto hp_unchanged=[&](){
        return half(body+0x1C6)==180-damage&&half(body+0x20C)==token&&word(body+0x41C)==damage&&
               word(actor+0x418)==damage&&at(body+0x2AF)[0]==4&&at(actor+timer)[0]==0;
    };
    if(!hp_unchanged()||at(body+timer)[0]!=armed||word(body+primary)!=unit||word(body+secondary)!=neutral_secondary)
        fail("contact hit did not arm the expected clock independently of its credited actor and rates");
    unsigned slow_visits=0;
    for(unsigned visit=1;visit<=armed+2;++visit){
        tick(mod,ram,body);
        const auto left=visit<armed?armed-visit:0;
        check_clocks(ram,body,left,unit,left?slow:unit,left?slow:neutral_secondary);
        if(!hp_unchanged())fail("body clock visit changed the hit, visual gate or credited actor");
        slow_visits+=unsigned(left!=0);
    }
    std::printf("pause-pair: %u %u %u %u %u %u %u %u %u %u\n",kind,model,carrier_kind,seed,prior,armed,
                unsigned(at(actor+timer)[0]),slow_visits,damage,half(body+0x1C6));
}
}
