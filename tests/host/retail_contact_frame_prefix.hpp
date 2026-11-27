// Original Sasuke frame prefix through its selected lower-lane visit.
#pragma once
#include "retail_contact_clock_consumers.hpp"

namespace retail_contact_frame_prefix {
using namespace retail_contact_pose_clock;
namespace consumers=retail_contact_clock_consumers;
inline constexpr u32 installer=0x8006F4F8,frame=0x80055568,post_callback=0x8006F88C;
inline constexpr u32 stage=0x8103C000,float_count=0x694,int_count=0x6B8;

template<class Stable>
void observe(const Module& mod,std::vector<u8>& ram,u32 body,unsigned model,unsigned scenario,unsigned armed,
             Stable hp_stable){
    const auto pos=[&](u32 address){return contact::at(ram,address);};
    const auto at=[&](u32 offset){return pos(body+offset);};
    const auto word=[&](u32 offset){return be32(at(offset));};
    const auto half=[&](u32 offset){return contact::half(ram,body+offset);};
    const auto clock=[&](u32 offset){return std::bit_cast<float>(word(offset));};
    const auto spec=word(0x4AC);
    if(!spec||half(0x3E8)!=0x000A||!at(0)[0]||at(timer)[0]!=armed||
       word(0x5E0)&3||word(consumers::lane)||at(override_flag)[0]||at(link_kind)[0])
        fail("frame prefix needs the normal Sasuke body retained from melee");
    put32(pos(0x803C4D94+4),spec);
    at(0x3E7)[0]=1;
    at(0x3E4)[0]=1;
    put32(pos(0x803C4E84+4),body);
    CPUState install{};install.gpr[3]=body;
    mod.run(install,ram,installer);
    if(word(0x4AC)!=spec||word(0x4B0)!=0x802D3EE0||word(0x4BC)!=0x8006F7C0||
       word(0x4C0)!=0x8006F88C||word(0x4C8)!=0x8006F950)
        fail("frame prefix did not keep the exact Sasuke class handlers");

    // No linked contact, live input source, or world update in this
    // prefix. These selections use genuine original table entries.
    put32(pos(0x8043B5A0-21316),stage);
    for(unsigned off:{52u,56u,64u})put_float(pos(stage+off),10000.f);
    for(unsigned off:{60u,68u})put_float(pos(stage+off),-10000.f);
    pos(contact::scene+0x1829)[0]=1; // Suppress the separate input-accumulation path.
    put_float(at(0xB8),1.f);
    at(0x579)[0]=3;
    at(0x5E4)[0]=9; // Empty source prepass table entry.
    at(0x5E5)[0]=9; // Same previous mode: no transition resets either lane selector.
    at(0x5DA)[0]=3;  // Lower slot 3 calls the original wait updater.
    at(0x5DB)[0]=0;
    at(0x73F)[0]=1; // Leave the unrelated 80010514 request path out of scope.
    put_float(at(float_count),4.f);put16(at(int_count),8);
    CPUState start{};start.gpr[3]=body;start.gpr[4]=1;start.gpr[5]=consumers::wait_command;
    start.gpr[6]=start.gpr[7]=0xFFFFFFFF;start.fpr[1]=-1.;
    mod.run(start,ram,consumers::lane_interpret);
    if(clock(consumers::lane+20)!=1.f||at(consumers::lane+26)[0])
        fail("frame prefix origin terminal wait did not arm");

    float deferred=4.f,wait=1.f;
    unsigned integer=8;
    for(unsigned visit=1;visit<=std::max(3u,armed+2);++visit){
        const unsigned before=at(timer)[0];
        const auto incoming=clock(secondary);
        deferred-=incoming;
        if(!before)--integer;
        const unsigned left=visit<armed?armed-visit:0;
        const float next=left?.03f:.375f;
        wait=std::max(0.f,wait-next);
        CPUState update{};update.gpr[3]=body;
        CallObservation vm{consumers::lane_visit};
        const auto frontier=mod.run_to_or_return(update,ram,frame,{post_callback},&vm,4096);
        if(frontier!=post_callback||update.lr!=0x80055634||update.gpr[3]!=body||vm.count!=1||
           vm.caller!=0x8005F564||vm.arguments[0]!=body||vm.arguments[1]!=13){
            std::fprintf(stderr,"prefix model=%04X case=%u visit=%u cut=%08X lr=%08X lane=%u from=%08X state=%u/%u/%u wait=%g\n",
                         model,scenario,visit,frontier,update.lr,vm.count,vm.caller,at(0x5E4)[0],at(0x5DA)[0],
                         at(0x661)[0],double(clock(consumers::lane+20)));
            fail("frame prefix did not reach the scheduled original lane visit");
        }
        check_clocks(ram,body,left,unit,left?slow:unit,left?slow:neutral_secondary);
        if(clock(float_count)!=deferred||half(int_count)!=integer||
           clock(consumers::lane+20)!=wait||at(consumers::lane+26)[0]!=unsigned(wait==0.f)||
           at(0x5E4)[0]!=9||!hp_stable()){
            std::fprintf(stderr,"prefix model=%04X case=%u visit=%u rate=%g/%g counters=%g/%u expected=%g/%u wait=%g/%g flags=%08X\n",
                         model,scenario,visit,double(incoming),double(next),double(clock(float_count)),half(int_count),
                         double(deferred),integer,double(clock(consumers::lane+20)),double(wait),word(0x5E0));
            fail("original frame prefix consumed a different rate or mutated the retained hit");
        }
        std::printf("frame-clock: %u %u %u %u %u %08X %08X %08X %u %08X %u\n",
                    model,scenario,visit,before,left,std::bit_cast<u32>(incoming),word(secondary),word(float_count),
                    half(int_count),word(consumers::lane+20),at(consumers::lane+26)[0]);
    }
}
}
