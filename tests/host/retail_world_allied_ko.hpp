// The loaded three-Tau inlet through opposing and allied lethal contact.
#pragma once
#include "retail_world_team_pair.hpp"

namespace retail_world_allied_ko {
using namespace retail_world_team_pair;
inline constexpr u32 scene=schedule::scene, requests=0x81050000;
inline constexpr u32 attack_rule=0x80377AD8,hp_mirror=0x803B069C+4;
inline constexpr u32 force(unsigned team){return scene+60*team+0x118;}
inline constexpr u32 archive_slot(unsigned i){return 0x803BB4C0+3120*i+0x600;}

inline void budget(Memory& ram,unsigned pool){
    for(unsigned i=0;i<2;++i){
        if(*at(ram,force(i)-15)||*at(ram,force(i)-14))fail("allied KO contrast has no reserve spend");
        put32(at(ram,force(i)-4),pool);
        put32(at(ram,force(i)),pool);
        *at(ram,force(i)-16)=1;
    }
    put32(at(ram,0x8043B5A0-21068),requests); // Declared empty KO model request buffer.
}

inline void pad_visit(Memory& ram,unsigned n){
    put32(at(ram,pad),n==8?0x200:0);
    *at(ram,pad+32+22)=n>=22&&n<=27?63:0;
    *at(ram,pad+64+22)=n>=22&&n<=27?63:0;
}

inline u32 pre_write(const CPUState& cpu){
    const auto rd=[&](u32 addr){return cpu.ram+addr-GC_RAM_BASE;};
    const auto recipient=seat(cpu.gpr[3]);
    if(!recipient||cpu.ram_size!=GC_MAIN_RAM_SIZE||be32(rd(force(0)))>300||be32(rd(force(1)))>300)
        fail("unexpected allied KO HP inlet");
    const auto hp=rd(roster[recipient]+0x1C6);
    const auto mirror=rd(0x803B069C+2*recipient);
    if(hp[0]||hp[1]>60||mirror[0]!=hp[0]||mirror[1]!=hp[1]||
       be32(rd(0x803B068C))!=be32(rd(force(0)-4))||be32(rd(0x803B068C+4))!=be32(rd(force(1)-4)))
        fail("allied KO amount must still see the visit's original HP and Force mirrors");
    return unsigned(hp[0])*256+hp[1]+(be32(rd(force(0)))<<8)+(be32(rd(force(1)))<<17);
}

inline void observe_amount(const Module& mod,Memory& ram){
    for(unsigned hp=1;hp<=61;++hp){
        auto pair=ram;
        const bool late=hp==61;
        put16(at(pair,actor2+0x1C6),late?60:hp);
        CPUState cpu{};
        if(mod.run_to_or_return(cpu,pair,schedule::dispatch,{world_pairs,0x80264290},nullptr,1u<<20)!=world_pairs||
           cpu.lr!=0x8002C4D0||half(pair,attack_count)!=1||half(pair,hp_mirror)!=(late?60:hp)||
           retail_world_team_pair::receivers(pair)[5]==0||half(pair,attack_rule)!=20||half(pair,attack_rule+16)!=1)
            fail("allied KO did not reach the published pair");
        const auto attack=word(pair,attacks);
        if(word(pair,attack+0x20)!=actor0||word(pair,attack+0x24)!=actor0||
           word(pair,attack+0x2C)+24u*(*at(pair,attack+0x11))!=attack_rule)
            fail("allied KO changed the exact Tau strike row");
        if(late)put16(at(pair,actor2+0x1C6),1);
        unsigned values[3]{};
        for(unsigned i=0;i<3;++i){
            *at(pair,actor2+0x88)=u8(i==0);
            put16(at(pair,attack_rule+16),i==2?0x1001:1); // Amount-only branch contrast, before any HP write.
            CPUState amount{};amount.gpr[3]=attack_rule;amount.gpr[4]=actor0;amount.gpr[5]=actor2;
            mod.run(amount,pair,contact::amount_entry,4096);
            values[i]=amount.gpr[3];
        }
        std::printf("ally-ko-%s: %u %u %u %u %u\n",late?"stale":"amount",late?1:hp,half(pair,hp_mirror),
                    values[0],values[1],values[2]);
    }
}

inline void observe(const Module& mod,Memory& ram,unsigned opening){
    if(opening!=174||*at(ram,actor1+0x88)!=1||*at(ram,actor2+0x88)||*at(ram,scene+5594)!=3)
        fail("allied KO requires the stock three-Tau team assignment");
    for(unsigned n=1;n<=26;++n){pad_visit(ram,n);visit(mod,ram,0,n);}
    std::array<u32,3> archive{};
    for(unsigned i=0;i<3;++i){
        archive[i]=word(ram,archive_slot(i));
        if(half(ram,roster[i]+0x4AA)!=90||half(ram,roster[i]+0x1C4)!=60||
           half(ram,roster[i]+0x1C6)!=60||!archive[i]||!*at(ram,archive[i]+9))
            fail("allied KO requires each exact loaded Tau and cached Force cost");
    }
    std::printf("ally-ko-open: %u 26 3 90\n",opening);
    const auto original=ram;
    for(unsigned arm=0;arm<5;++arm){
        ram=original;
        const unsigned victim=arm?1+(arm-1)%2:0,pool=arm>=3?90:180;
        budget(ram,pool);
        if(victim)put16(at(ram,roster[victim]+0x1C6),1);
        if(arm==0){pad_visit(ram,27);observe_amount(mod,ram);}
        for(unsigned n=27;n<=(arm>=3?27:35u);++n){
            pad_visit(ram,n);
            CallObservation write{contact::hp_write};write.sample=pre_write;
            visit(mod,ram,arm,n,&write);
            if(write.count!=(n==27?2u:0u)||(write.count&&(write.caller!=0x8002F1D8||
                write.first_arguments[0]!=actor1||write.first_arguments[1]!=actor2))||
               *at(ram,scene+5594)!=3||half(ram,actor0+0x1C6)!=60||*at(ram,actor0+0x18)!=1||
               half(ram,contact::receiver_count)!=(victim&&n>=28?4:6)||
               half(ram,attack_count)!=unsigned(n<32))
                fail("the same strike did not preserve two writer recipients and the loaded actor schedule");
            for(unsigned i=0;i<3;++i)
                if(!*at(ram,roster[i])||word(ram,archive_slot(i))!=archive[i]||!*at(ram,archive[i]+9))
                    fail("no archive or physical seat may retire at the allied KO boundary");
            if(n!=27&&n!=28&&n!=32&&n!=35)continue;
            std::printf("ally-ko-visit: %u %u %u %u %u %u %u %u %u",arm,n,*at(ram,scene),*at(ram,scene+31),
                word(ram,force(0)),word(ram,force(1)),half(ram,contact::receiver_count),half(ram,attack_count),write.count);
            for(unsigned i=0;i<write.count&&i<8;++i)
                std::printf(" %u %u %u %u %u",seat(write.first_arguments[i]),write.second_arguments[i],
                    write.samples[i]&255,(write.samples[i]>>8)&511,write.samples[i]>>17);
            for(auto obj:roster)
                std::printf(" %u %u %04X %u %u %u %u %u %u %u",half(ram,obj+0x1C6),*at(ram,obj+0x18),
                    half(ram,obj+0x272),word(ram,obj+0x41C),word(ram,obj+0x420),word(ram,obj+0x424),
                    word(ram,obj+0x430),*at(ram,obj+0x434),*at(ram,obj+0x435),*at(ram,obj+0x437));
            const auto finish=[&](u32 addr){return word(ram,addr)?seat(word(ram,addr))+1:0u;};
            std::printf(" %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                word(ram,actor0+0x418),word(ram,actor0+0x42C),finish(scene+0xB4),finish(scene+0xB8),
                word(ram,requests+0x2D18),*at(ram,scene+258),*at(ram,scene+318),finish(actor1+0x5A4),finish(actor2+0x5A4),
                *at(ram,actor0+0x484),half(ram,actor0+0x464),half(ram,actor0+0x468),half(ram,actor0+0x46C));
        }
    }
}
}
