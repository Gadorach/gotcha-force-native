// Loaded Tau team comparison using the ordinary pose/list/pair schedule.
#pragma once
#include "retail_world_moving_receiver.hpp"

namespace retail_world_team_pair {
using namespace retail_world_moving_receiver;
inline constexpr u32 actor2=actor1+0x1E00;
inline constexpr std::array<u32,3> roster{actor0,actor1,actor2};
inline constexpr float receiver_z=240.f;

inline unsigned seat(u32 body){
    for(unsigned i=0;i<roster.size();++i)if(roster[i]==body)return i;
    fail("team pair left the three physical Tau seats");
}

inline std::array<u32,6> receivers(Memory& ram){
    if(half(ram,contact::receiver_count)!=6)fail("three loaded Taus must publish six receiving shapes");
    std::array<u32,6> out{};
    for(unsigned i=0;i<6;++i){
        const auto cell=word(ram,contact::receiver_registry+4*i),body=word(ram,cell+0x20);
        const auto owner=seat(body),bank=word(ram,body+0x3FC)+32,desc=word(ram,cell+0x28);
        const unsigned j=desc==bank+160;
        if((desc!=bank+80&&desc!=bank+160)||word(ram,cell+0x58)||word(ram,cell+0x24)!=body||
           word(ram,cell+0x60)!=body+(j?0x8D4+3*0x30:0x1AA4)||out[owner*2+j])
            fail("team pair lost a Tau receiver descriptor, category or owner");
        slot(cell);out[owner*2+j]=cell;
    }
    return out;
}

inline u32 narrow_cell(const CPUState& cpu){
    const auto cell=cpu.gpr[26];
    slot(cell);
    const auto rd=[&](u32 p){
        if(p<GC_RAM_BASE||p-GC_RAM_BASE>cpu.ram_size-4)fail("team pair scratch extent");
        return be32(cpu.ram+p-GC_RAM_BASE);
    };
    const auto scratch=cpu.lr==0x800304E4?cpu.gpr[3]:cpu.gpr[4];
    if((cpu.lr!=0x800304E4&&cpu.lr!=0x800304F0)||scratch!=0x803B0368||
       !seat(rd(cell+0x20)))fail("team pair used a foreign narrow kernel inlet");
    for(unsigned a=0;a<3;++a)
        if(rd(scratch+12+4*a)!=rd(cell+0x64+4*a))fail("team pair narrow kernel did not use the current receiver");
    return cell;
}

inline void print_result(const char* tag,unsigned arm,unsigned n,Memory& ram,const CallObservation& write){
    if(write.count>2||(write.count&&write.caller!=0x8002F1D8))fail("team pair HP writer changed");
    std::printf("world-team-%s: %u %u %u",tag,arm,n,write.count);
    for(unsigned i=0;i<write.count;++i)std::printf(" %u %u",seat(write.first_arguments[i]),write.second_arguments[i]);
    for(auto body:roster)std::printf(" %u %u %u",half(ram,body+0x1C6),half(ram,body+0x20C),word(ram,body+0x41C));
    std::printf(" %u %u %u\n",word(ram,actor0+0x418),word(ram,actor0+0x42C),half(ram,attack_count));
}

inline u32 compare_pair(const Module& mod,Memory& ram,unsigned ally){
    const auto old=receivers(ram);
    auto pose=ram,pair=ram;
    CPUState posed{},inlet{};CallObservation publisher{0x800465FC};
    if(mod.run_to_or_return(posed,pose,schedule::dispatch,{0x80087CF0,0x80264290},&publisher,1u<<20)!=0x80087CF0||
       posed.lr!=0x80029490||publisher.count!=3||publisher.caller!=0x80054160||
       mod.run_to_or_return(inlet,pair,schedule::dispatch,{world_pairs,0x80264290},nullptr,1u<<20)!=world_pairs||
       inlet.lr!=0x8002C4D0||receivers(pair)!=old||half(pair,attack_count)!=1)
        fail("three original poses and list did not precede the ordinary world pair");
    const auto attack=word(pair,attacks);
    const auto row=word(pair,attack+0x2C)+24u*(*at(pair,attack+0x11));
    if(word(pair,attack+0x20)!=actor0||word(pair,attack+0x24)!=actor0||word(pair,attack+0x58)||
       word(pair,attack+0x28)!=word(pair,actor0+0x3FC)+0xAA0+2*80||
       half(pair,actor0+0x26C)!=1||half(pair,row+16)!=1)
        fail("team pair needs exact normal Tau close-B descriptor, category and token");
    unsigned order=0,seen=0;
    for(unsigned i=0;i<3;++i){
        const auto body=roster[i];
        const auto published=seat(publisher.first_arguments[i]);
        if(seen&(1u<<published))fail("team pair repeated a body publisher");
        seen|=1u<<published;
        order=10*order+published;
        if(half(pair,body+0x1C6)!=60||half(pair,body+0x20C)||
           point(pose,body+0x20)!=point(pair,body+0x20)||point(ram,body+0x20)==point(pose,body+0x20)||
           *at(pair,body+0x88)!=unsigned(i!=0&&i!=ally)){
            std::fprintf(stderr,"team pair seat=%u publisher=%08X hp=%u token=%u team=%u moved=%d same-root=%d\n",
                         i,publisher.first_arguments[i],half(pair,body+0x1C6),half(pair,body+0x20C),*at(pair,body+0x88),
                         point(ram,body+0x20)!=point(pose,body+0x20),point(pose,body+0x20)==point(pair,body+0x20));
            fail("team pair lost pose motion or original battle team");
        }
    }
    std::printf("world-team-pose: %u 27 %03u\n",ally,order);
    unsigned broad=0,eligible=0;
    for(unsigned j=0;j<old.size();++j){
        const auto cell=old[j];
        if(point(pose,cell+0x64)!=point(ram,cell+0x64)||point(pair,cell+0x70)!=point(ram,cell+0x64)||
           point(pair,cell+0x64)==point(ram,cell+0x64))fail("team pair shape history did not follow pose publication");
        CPUState rel{};rel.gpr[3]=attack;rel.gpr[4]=cell;rel.gpr[5]=2;
        mod.run(rel,pair,0x8002FD7C,4096);
        if(rel.gpr[3])eligible|=1u<<j;
        CPUState geometry{};geometry.gpr[3]=attack+0x58;geometry.gpr[4]=cell+0x58;
        mod.run(geometry,pair,0x80039F6C,4096);
        if(geometry.gpr[3])broad|=1u<<j;
    }
    if(eligible!=60)fail("both other bodies must be independently pair-eligible");
    for(unsigned variant=0;variant<=(ally==2?3u:0u);++variant){
        auto fork=pair;
        // Pair-only counterfactuals: do not steer or republish a shape.
        const auto change=[&]{
            if(variant==1)*at(fork,actor2+0x88)=1;
            if(variant==2)*at(fork,actor1+0x88)=0;
            if(variant==3)put16(at(fork,row+16),0x81);
        };
        change();
        unsigned admitted=0;
        for(unsigned j=0;j<old.size();++j){
            CPUState rel{};rel.gpr[3]=attack;rel.gpr[4]=old[j];rel.gpr[5]=2;
            mod.run(rel,fork,0x8002FD7C,4096);
            if(rel.gpr[3])admitted|=1u<<j;
        }
        CallObservation narrow{sphere_sphere};narrow.sample=narrow_cell;
        CPUState test{};mod.run_observed(test,fork,world_pairs,narrow,65536);
        if(narrow.count>4)fail("team pair narrow extent");
        unsigned checked=0;
        for(unsigned i=0;i<narrow.count;++i){
            unsigned j=0;
            while(j<old.size()&&old[j]!=narrow.samples[i])++j;
            if(j==old.size()||(checked&(1u<<j)))fail("team pair narrow pass reused or crossed a receiving shape");
            checked|=1u<<j;
        }
        if(checked&~(broad&admitted))fail("team pair tested an ineligible or broad-rejected shape");
        std::printf("world-team-pair: %u %u %u %u %u %u\n",ally,variant,broad,admitted,checked,slot(attack));
        fork=pair;
        change();
        CallObservation write{contact::hp_write};test={};mod.run_observed(test,fork,world_pairs,write,65536);
        print_result("fork",ally,variant,fork,write);
    }
    return attack;
}

inline void observe(const Module& mod,Memory& ram,unsigned opening,unsigned ally){
    if(opening!=174||(ally!=1&&ally!=2)||*at(ram,schedule::scene+5594)!=3)fail("three loaded Tau opening changed");
    const auto bodies=receivers(ram);
    for(unsigned i=0;i<3;++i){
        const auto body=roster[i],archive=word(ram,0x803BB4C0+3120*i+0x600);
        if(!archive||!*at(ram,archive+9)||half(ram,body+0x3E8)!=0x0807||half(ram,body+0x1C6)!=60||
           *at(ram,body+0x3F0)!=1||*at(ram,body+0x3E6)||*at(ram,body+0x88)!=unsigned(i!=0&&i!=ally)||
           !bodies[2*i]||!bodies[2*i+1])fail("three independently loaded Tau actors did not open");
    }
    std::printf("world-team-open: %u %u %u 3 6\n",opening,ally,unsigned(receiver_z));
    u32 attack=0;
    for(unsigned n=1;n<=32;++n){
        put32(at(ram,pad),n==8?0x200:0);
        *at(ram,pad+32+22)=n>=22?63:0;
        *at(ram,pad+64+22)=n>=22?63:0;
        if(n==27)attack=compare_pair(mod,ram,ally);
        CallObservation writer{contact::hp_write};
        visit(mod,ram,ally,n,&writer);
        if(writer.count!=(n==27?ally:0u)||half(ram,attack_count)!=unsigned(n>=27&&n<32))
            fail("only visit 27 may write HP; the original strike window returns at 32");
        if(n==27||n==28||n==32)print_result("visit",ally,n,ram,writer);
        if(n==32){
            bool returned=false;
            for(u32 cell=word(ram,contact::free_head+16),hops=0;cell;cell=word(ram,cell+4)){
                slot(cell);
                if(++hops>48)fail("team pair shape free-list extent");
                if(cell==attack){if(*at(ram,cell))fail("returned strike cell is still active");returned=true;}
            }
            if(!returned)fail("team pair did not return its exact strike cell");
            std::printf("world-team-return: %u 32 %u\n",ally,slot(attack));
        }
    }
}
}
