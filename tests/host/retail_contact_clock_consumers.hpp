// Feed original body-clock outputs to separately scoped movement and VM helpers.
#pragma once
#include "retail_contact_pose_clock.hpp"

namespace retail_contact_clock_consumers {
using namespace retail_contact_pose_clock;
inline constexpr u32 wait_command=0x802BD766,clip_command=0x802C2F56,clip_cursor=0x81034020;
inline constexpr u32 lane=0x1CD4,lane_clock=0x1AE0,remaining=0x5AA,heading=0x72;
inline constexpr u32 lane_interpret=0x8004C050,lane_visit=0x8004CD24,clip=0x8004C298,clip_frontier=0x80048648;
inline constexpr u32 motion=0x800676D8,turn=0x80066E40;

inline float clip_frame(const Module& mod,std::vector<u8> fork,u32 body,float frame){
    auto* cursor=contact::at(fork,clip_cursor);
    put32(cursor,clip_command);
    CPUState next{};
    next.gpr[3]=body;next.gpr[4]=1;next.gpr[5]=clip_cursor;next.gpr[6]=next.gpr[7]=0xFFFFFFFF;
    next.fpr[1]=frame;
    mod.run_to(next,fork,clip,{clip_frontier});
    if(next.lr!=0x8004C3A8||next.gpr[3]!=body||next.gpr[4]||next.gpr[5]||next.gpr[6]!=3||
       next.gpr[7]||next.gpr[8]||be32(cursor)!=clip_command+8)
        fail("contact rate clip request changed its bounded lane, clip or cursor");
    return float(next.fpr[1]);
}

template<class Stable>
void observe(const Module& mod,std::vector<u8>& ram,u32 body,unsigned model,unsigned scenario,unsigned armed,
             Stable hp_stable){
    const auto at=[&](u32 o){return contact::at(ram,body+o);};
    const auto word=[&](u32 o){return be32(at(o));};
    const auto scalar=[&](u32 o){return std::bit_cast<float>(word(o));};
    const auto half=[&](u32 o){return contact::half(ram,body+o);};
    const auto spec=word(0x4AC);
    if(!spec||be32(contact::at(ram,spec+48))!=std::bit_cast<u32>(0.04f)||word(lane)||at(timer)[0]!=armed)
        fail("contact rate consumers need the normal Sasuke inlet and its existing timer");
    if(be32(contact::at(ram,wait_command))!=0x00090001||
       be32(contact::at(ram,clip_command))!=0x01000300||be32(contact::at(ram,clip_command+4))!=0x0000FFFF)
        fail("contact rate indexed wait or clip command changed");
    const auto initial_x=word(0x20),initial_y=word(0x24);
    at(0x579)[0]=1;
    put_float(at(lane_clock),6.f);
    put_float(at(0x5F8),1.f);
    put_float(at(0x664),1.f);
    put16(at(remaining),1024);
    put16(at(heading),0);
    CPUState start{};start.gpr[3]=body;start.gpr[4]=1;start.gpr[5]=wait_command;
    start.gpr[6]=start.gpr[7]=0xFFFFFFFF; start.fpr[1]=-1;
    mod.run(start,ram,lane_interpret);
    if(word(lane)!=wait_command||word(lane+4)!=wait_command+4||scalar(lane+20)!=1.f||at(lane+26)[0])
        fail("original wait command did not install a one-unit terminal lane");
    CPUState unselected{};unselected.gpr[3]=body;unselected.gpr[4]=2;
    mod.run(unselected,ram,lane_visit);
    if(unselected.gpr[3]||scalar(lane+20)!=1.f||at(lane+26)[0])
        fail("an excluded lane consumed its clock");

    const auto other_timer=contact::at(ram,actor+timer)[0];
    unsigned gap=1024,angle=0;
    float wait=1.f,position=scalar(0x28);
    for(unsigned visit=1;visit<=std::max(3u,armed+2);++visit){
        tick(mod,ram,body);
        const unsigned left=visit<armed?armed-visit:0;
        const float p=left?0.03f:1.f,s=left?0.03f:0.375f;
        check_clocks(ram,body,left,unit,left?slow:unit,left?slow:neutral_secondary);
        const auto sampled=clip_frame(mod,ram,body,-1.f);
        const auto explicit_sample=clip_frame(mod,ram,body,2.5f);
        if(sampled!=6.f+s||explicit_sample!=2.5f)
            fail("original clip frontier did not preserve secondary-rate or explicit-frame selection");

        CPUState walk{};walk.gpr[3]=body;walk.gpr[4]=0;walk.fpr[1]=100.;
        mod.run(walk,ram,motion);
        position+=100.f*p;
        if(word(0x20)!=initial_x||word(0x24)!=initial_y||scalar(0x28)!=position)
            fail("original planar displacement did not follow the primary rate");

        CPUState face{};face.gpr[3]=body;face.gpr[4]=0;
        mod.run(face,ram,turn);
        const auto step=unsigned((float(gap)*0.04f)*s);
        gap-=step;angle+=step;
        if(half(0x7E)!=step||half(heading)!=angle||half(remaining)!=gap)
            fail("original heading step did not follow the secondary rate");

        CPUState move{};move.gpr[3]=body;move.gpr[4]=1;
        mod.run(move,ram,lane_visit);
        wait=std::max(0.f,wait-s);
        const auto done=unsigned(wait==0.f);
        if(word(lane)!=wait_command||scalar(lane+20)!=wait||at(lane+26)[0]!=done||
           move.gpr[3]!=done||at(timer)[0]!=left||scalar(lane_clock)!=6.f||!hp_stable()||
           contact::at(ram,actor+timer)[0]!=other_timer)
            fail("original wait or helper changed the timer, HP, or other lane state");
        std::printf("clock-consumers: %u %u %u %u %08X %08X %08X %u %.0f %u %u %08X %08X\n",
                    model,scenario,visit,left,word(primary),word(secondary),word(lane+20),done,
                    double(scalar(0x28)),step,gap,std::bit_cast<u32>(sampled),std::bit_cast<u32>(explicit_sample));
    }
}
}
