// A one-row Tau queue through ordinary KO, Force exhaustion and the scene exit.
#pragma once
#include "retail_world_body_exit.hpp"

namespace retail_world_match_end {
using namespace retail_world_body_exit;
inline constexpr u32 side1=scene+60,force=side1+0x118,force_limit=side1+0x114,force_gate=side1+0x108;
inline constexpr u32 audio_tables=0x803D3E40;

inline void roster(Memory& ram){
    *at(ram,scene+91)=1;*at(ram,scene+85)=0;*at(ram,scene+5577)=0;
    put16(at(ram,queue),0x0807);
    put32(at(ram,0x8043B5A0-21068),requests);
    put32(at(ram,force_limit),90);put32(at(ram,force),90);*at(ram,force_gate)=1;
}

inline unsigned returned_cells(Memory& ram){
    unsigned mask=0,n=0;
    for(u32 cell=word(ram,contact::free_head+16);cell;cell=word(ram,cell+4)){
        if(++n>48)fail("one-row queue shape free-list extent");
        const auto i=slot(cell);
        if(i>=3&&i<=5){
            if(*at(ram,cell))fail("one-row queue returned a live old body cell");
            mask|=1u<<(i-3);
        }
    }
    return mask;
}

// Only visit 28 of the Force-exhausting arm may use this callback. It does
// not return data or resume the source. Any other external read still fails.
[[noreturn]] inline u64 audio_frontier(CPUState* cpu,u32 address,u8 size){
    if(cpu->pc!=0x800F0FDC||cpu->lr!=0x800EFC94||address!=0x766||size!=1||
       cpu->gpr[19]!=118||cpu->gpr[30]!=audio_tables+4||cpu->ram_size!=GC_MAIN_RAM_SIZE)
        read_external(cpu,address,size);
    const auto bytes=[&](u32 a){return cpu->ram+a-GC_RAM_BASE;};
    if(be32(bytes(audio_tables+4))||*bytes(scene)!=3||*bytes(scene+1)!=1||
       *bytes(scene+8)||*bytes(scene+9)!=90||*bytes(scene+69)!=5||*bytes(scene+31)!=1||
       !*bytes(actor1)||*bytes(actor1+0x18)!=2||!be32(bytes(owner)))
        fail("ending audio frontier did not retain the loaded defeated body");
    std::printf("world-end-frontier: 28 3 1 5 90 1 2 1 %08X %08X %08X %08X %u\n",
                cpu->pc,cpu->lr,address,cpu->gpr[30],cpu->gpr[19]);
    std::exit(0);
}

inline void observe(const Module& mod,Memory& ram,unsigned opening){
    for(unsigned n=1;n<=26;++n){
        put32(at(ram,pad),n==8?0x200:0);
        *at(ram,pad+32+22)=n>=22?63:0;
        visit(mod,ram,0,n);
    }
    const auto original=ram;
    const auto old=word(ram,owner),old_spec=word(ram,resources),old_motion=word(ram,motion);
    if(opening!=174||!old||!*at(ram,old+9)||!old_spec||!old_motion||half(ram,old+26)!=2||
       half(ram,actor1+0x4AA)!=90||half(ram,actor1+0x1C6)!=60||
       word(ram,force)!=90||word(ram,force_limit)!=90||!*at(ram,force_gate)||
       *at(ram,scene+91)!=1||half(ram,queue)!=0x0807||half(ram,queue+28)||receiving(ram)!=6)
        fail("one-row queue needs the retained loaded Tau and declared Force cost");
    std::array<u32,4> wrappers{};
    for(unsigned i=0;i<4;++i){
        wrappers[i]=word(ram,view+(i<2?1800:1928)+4*(i%2));
        if(!wrappers[i]||!*at(ram,wrappers[i]+9))fail("old one-row view wrapper was not loaded");
    }
    std::printf("world-end-open: %u %u %u %u\n",opening,word(ram,force),half(ram,actor1+0x4AA),*at(ram,scene+91));
    for(unsigned arm:{0,2,1}){
        ram=original;
        if(arm)put16(at(ram,actor1+0x1C6),1);
        if(arm==2)*at(ram,force_gate)=0;
        for(unsigned n=27;n<=(arm==0?35u:arm==2?130u:28u);++n){
            *at(ram,pad+32+22)=n==27?63:0;
            if(n==27){
                auto fork=ram;CPUState hp{};
                const unsigned amount=arm?14:17;
                if(mod.run_to_or_return(hp,fork,schedule::dispatch,{contact::hp_write,0x80264290},nullptr,1u<<20)
                        !=contact::hp_write||hp.lr!=0x8002F1DC||hp.gpr[3]!=actor1||hp.gpr[4]!=amount||
                   word(fork,actor0+0x418)!=amount||word(fork,actor1+0x41C)!=amount)
                    fail("one-row queue lost the ordinary HP writer and ledgers");
                mod.finish_stopped_leaf(hp,contact::hp_write,164);
                if(hp.gpr[3]!=unsigned(arm!=0)||half(fork,actor1+0x1C6)!=(arm?0u:43u)||
                   *at(fork,actor1+0x18)!=1||word(fork,force)!=90||*at(fork,scene+31))
                    fail("HP writer itself must not select a result or debit Force");
            }
            const auto ticks=word(ram,scene+168),combat=word(ram,scene+172),world=word(ram,scene+176);
            CPUState cpu{};CallObservation write{contact::hp_write};
            const bool ending=arm==1,inactive=arm==2&&n>=125;
            const auto cut=mod.run_to_or_return(cpu,ram,schedule::dispatch,{disc,0x80264290},&write,1u<<20,
                                              ending&&n==28?audio_frontier:read_external);
            if(ending&&n==28)fail("first ending visit did not hit its exact missing audio owner");
            const unsigned phase=!arm?1:n<64?2:3,sub=!arm?2:n<65?0:inactive?2:1;
            const auto cells=returned_cells(ram),receivers=receiving(ram);
            if(cut||write.count!=unsigned(n==27)||(n==27&&(write.caller!=0x8002F1D8||
               write.arguments[0]!=actor1||write.arguments[1]!=(arm?14u:17u)))||
               word(ram,scene+168)!=ticks+1||word(ram,scene+172)!=combat+1||word(ram,scene+176)!=world+1||
               *at(ram,scene)!=(ending?3u:2u)||*at(ram,scene+1)!=(ending?0u:3u)||
               *at(ram,scene+69)!=4||*at(ram,scene+31)!=unsigned(ending)||word(ram,force)!=(ending?0u:90u)||
               *at(ram,scene+258)!=unsigned(ending)||*at(ram,scene+318)||
               *at(ram,actor1)!=unsigned(!inactive)||*at(ram,scene+5594)!=(inactive?1u:2u)||
               *at(ram,actor1+0x18)!=phase||*at(ram,actor1+0x19)!=sub||*at(ram,actor1+0x3F0)!=1||
               half(ram,actor1+0x1C)!=(arm==2&&n>=65&&!inactive?125u-n:0u)||
               half(ram,actor1+0x1C6)!=(arm?0u:43u)||half(ram,actor0+0x1C6)!=60||
               half(ram,actor1+0x272)!=(arm?0x443:0)||receivers!=((!arm||n==27)?6u:0u)||cells!=(arm&&n>=63?7u:0u)||
               word(ram,owner)!=old||!*at(ram,old+9)||word(ram,resources)!=old_spec||word(ram,motion)!=old_motion||
               *at(ram,scene+91)!=1||*at(ram,scene+5577)||half(ram,queue)!=0x0807||half(ram,queue+28)){
                std::fprintf(stderr,"one-row arm=%u visit=%u winner-flags=%u/%u turns=%u/%u/%u queued=%u/%u\n",
                    arm,n,*at(ram,scene+258),*at(ram,scene+318),
                    word(ram,scene+168)-ticks,word(ram,scene+172)-combat,word(ram,scene+176)-world,
                    *at(ram,scene+91),*at(ram,scene+5577));
                fail("one-row queue changed its KO, Force, result or retirement boundary");
            }
            for(auto wrapper:wrappers)if(!*at(ram,wrapper+9))fail("one-row queue returned a retained wrapper");
            if(n==27||n==28||n==35||n==63||n==64||n==65||n==124||n==125||n==130)
                std::printf("world-end-step: %u %u %u %u %u %u %u %u %u %u %u %u %04X %u 1\n",
                    arm,n,*at(ram,scene),*at(ram,scene+31),word(ram,force),half(ram,actor1+0x1C6),
                    write.arguments[1],*at(ram,actor1),*at(ram,scene+5594),phase,sub,half(ram,actor1+0x1C),
                    receivers,cells);
        }
    }
}
} // namespace retail_world_match_end
