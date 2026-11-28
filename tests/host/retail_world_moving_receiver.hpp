// Two loaded Tau actors, with motion and the world pair pass in the same original schedule.
#pragma once
#include "retail_scene_entry.hpp"

namespace retail_world_moving_receiver {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace schedule=retail_scene_entry;
inline constexpr u32 actor0=0x81020000,actor1=actor0+0x1E00,stage=0x81048000,pad=0x803C727C;
inline constexpr u32 pool=0x81070000,stride=0x200,attack_count=0x8043B5A0-21328,attacks=0x803C477C;
inline constexpr u32 world_pairs=0x8002DB58,sphere_sphere=0x80030500,sphere_capsule=0x80030A3C;

using Memory=std::vector<u8>;
using Point=std::array<u32,3>;
inline u8* at(Memory& ram,u32 a){return contact::at(ram,a);}
inline u32 word(Memory& ram,u32 a){return be32(at(ram,a));}
inline unsigned half(Memory& ram,u32 a){return contact::half(ram,a);}
inline Point point(Memory& ram,u32 addr){return {word(ram,addr),word(ram,addr+4),word(ram,addr+8)};}
inline Point matrix_translation(Memory& ram,u32 m){return {word(ram,m+12),word(ram,m+28),word(ram,m+44)};}
inline void print(Point p){for(const auto w:p)std::printf(" %08X",w);}
inline unsigned slot(u32 cell){
    if(cell<pool||cell>=pool+48*stride||(cell-pool)%stride)fail("moving receiver left the fixture's shape pool");
    return (cell-pool)/stride;
}
inline std::array<u32,2> receivers(Memory& ram){
    if(half(ram,contact::receiver_count)!=4)fail("moving receiver requires both ordinary Tau shape pairs");
    std::array<u32,2> out{};
    const auto bank=word(ram,actor1+0x3FC)+32;
    for(unsigned i=0;i<4;++i){
        const auto cell=word(ram,contact::receiver_registry+4*i);
        if(word(ram,cell+0x20)!=actor1)continue;
        const auto desc=word(ram,cell+0x28);
        if(desc!=bank+80&&desc!=bank+160)fail("moving receiver used the wrong Tau receiving descriptor");
        const unsigned index=(desc==bank+160);
        const auto matrix=actor1+(index?0x8D4+3*0x30:0x1AA4);
        if(out[index]||word(ram,cell+0x60)!=matrix||word(ram,cell+0x58)||*at(ram,cell+0x13))
            fail("moving receiver lost its original sphere or matrix owner");
        slot(cell);out[index]=cell;
    }
    if(!out[0]||!out[1])fail("moving receiver registry missing a descriptor");
    return out;
}

inline u32 sample_narrow(const CPUState& cpu){
    const auto receive=cpu.gpr[26];
    slot(receive);
    const auto read=[&](u32 a){
        if(a<GC_RAM_BASE||a-GC_RAM_BASE>cpu.ram_size-4)fail("moving receiver scratch extent");
        return be32(cpu.ram+a-GC_RAM_BASE);
    };
    const auto scratch=cpu.lr==0x800304E4?cpu.gpr[3]:cpu.gpr[4];
    if((cpu.lr!=0x800304E4&&cpu.lr!=0x800304F0)||read(receive+0x20)!=actor1||scratch!=0x803B0368)
        fail("ordinary narrow dispatcher entered a foreign receiver");
    for(unsigned axis=0;axis<3;++axis)
        if(read(scratch+12+4*axis)!=read(receive+0x64+4*axis))
            fail("ordinary narrow dispatcher did not consume the current sphere center");
    return receive;
}

inline void visit(const Module& mod,Memory& ram,unsigned arm,unsigned n,CallObservation* observation=nullptr){
    const auto turns=word(ram,schedule::scene+168),world=word(ram,schedule::scene+176),combat=word(ram,schedule::scene+172);
    CPUState cpu{};
    const auto cut=mod.run_to_or_return(cpu,ram,schedule::dispatch,{0x80264290},observation,1u<<20);
    if(cut||word(ram,schedule::scene+168)!=turns+1||word(ram,schedule::scene+172)!=combat+1||
       word(ram,schedule::scene+176)!=world+1){
        std::fprintf(stderr,"moving receiver arm=%u visit=%u cut=%08X lr=%08X\n",arm,n,cut,cpu.lr);
        fail("moving receiver did not return through the original scene schedule");
    }
}

inline u32 sample_visit(const Module& mod,Memory& ram,unsigned arm,unsigned n){
    const auto old=receivers(ram);
    auto posed=ram,listed=ram;
    CallObservation publisher{0x800465FC};CPUState before{},world{};
    if(mod.run_to_or_return(before,posed,schedule::dispatch,{0x80087CF0,0x80264290},&publisher,1u<<20)!=0x80087CF0||
       before.lr!=0x80029490||publisher.count!=2||publisher.caller!=0x80054160||publisher.first_arguments[0]!=actor0||
       publisher.first_arguments[1]!=actor1||
       mod.run_to_or_return(world,listed,schedule::dispatch,{0x8002BB14,0x80264290},nullptr,1u<<20)!=0x8002BB14||
       world.lr!=0x800294F8||receivers(listed)!=old)
        fail("ordinary pose publication must precede the original list rebuild and contact");
    if(half(listed,attack_count)!=(n==27?1u:0u)||half(listed,actor1+0x1C6)!=60)
        fail("moving receiver changed the shared attack publication and pre-HP cut");
    const auto root_before=point(ram,actor1+0x20),root_after=point(posed,actor1+0x20);
    if(root_before==root_after||root_after!=point(listed,actor1+0x20)||*at(posed,actor1+0x5E4)!=13)
        fail("control requires motion on the same lower dash handler in both arms");
    std::printf("world-receiver-pose: %u %u %u %u %08X",arm,n,*at(posed,actor1+0x5E4),
                half(listed,attack_count),word(posed,actor1+0x1CDC));
    print(root_before);print(root_after);std::putchar('\n');
    for(unsigned i=0;i<2;++i){
        const auto cell=old[i],matrix=word(listed,cell+0x60);
        const auto previous=point(ram,cell+0x64),current=point(listed,cell+0x64);
        if(point(posed,cell+0x64)!=previous||point(listed,cell+0x70)!=previous||current==previous||
           matrix_translation(posed,matrix)!=matrix_translation(listed,matrix)||
           matrix_translation(ram,matrix)==matrix_translation(posed,matrix))
            fail("receiver history did not track an actual pose followed by fresh shape publication");
        std::printf("world-receiver-shape: %u %u %u %u",arm,n,i+1,slot(cell));
        print(matrix_translation(posed,matrix));print(previous);print(current);std::putchar('\n');
    }
    if(n!=27)return 0;

    const auto attack=word(listed,attacks);
    if(word(listed,attack+0x20)!=actor0||word(listed,attack+0x24)!=actor0||
       word(listed,attack+0x28)!=word(listed,actor0+0x3FC)+0xAA0+2*80||half(listed,actor0+0x26C)!=1)
        fail("moving receiver lost Tau's real close-B shape and token");
    auto pair=ram;CPUState inlet{};
    if(mod.run_to_or_return(inlet,pair,schedule::dispatch,{world_pairs,0x80264290},nullptr,1u<<20)!=world_pairs||
       inlet.lr!=0x8002C4D0||receivers(pair)!=old||word(pair,attacks)!=attack||half(pair,actor1+0x1C6)!=60)
        fail("world did not carry the registry into the ordinary pair pass");
    const auto kind=word(pair,attack+0x58);
    if(kind&&kind!=2)fail("moving receiver uses an unqualified attack shape");
    std::printf("world-receiver-pair: %u %u %u %u",arm,n,slot(attack),kind);
    print(point(pair,actor1+0x1CC));print(point(pair,attack+0x64));
    for(const auto cell:old)print(point(pair,cell+0x64));
    std::putchar('\n');

    const u32 kernel=kind==2?sphere_capsule:sphere_sphere;
    auto scratch=pair;
    CPUState narrow{};CallObservation attempts{kernel};attempts.sample=sample_narrow;
    mod.run_observed(narrow,scratch,world_pairs,attempts,65536);
    unsigned tried=0;
    if(attempts.count>1)fail("moving receiver escaped the qualified singleton narrow attempt");
    for(unsigned i=0;i<attempts.count;++i){
        const auto cell=attempts.samples[i];
        if(cell!=old[0]&&cell!=old[1])fail("moving receiver used a foreign narrow shape");
        tried|=cell==old[0]?1:2;
    }
    unsigned eligible=0,broad=0;
    for(unsigned i=0;i<old.size();++i){
        CPUState test{};test.gpr[3]=attack;test.gpr[4]=old[i];test.gpr[5]=2;
        mod.run(test,pair,0x8002FD7C,4096);
        if(test.gpr[3])eligible|=1u<<i;
        test={};test.gpr[3]=attack+0x58;test.gpr[4]=old[i]+0x58;
        mod.run(test,pair,0x80039F6C,4096);
        if(test.gpr[3])broad|=1u<<i;
    }
    unsigned picked=0;
    if(tried){
        CPUState inlet{};CallObservation cut_observation{kernel};cut_observation.sample=sample_narrow;
        if(mod.run_to_or_return(inlet,pair,world_pairs,{kernel},&cut_observation,65536)!=kernel||
           cut_observation.count!=1||cut_observation.samples[0]!=attempts.samples[0])
            fail("moving receiver no longer has the same original scratch inlet");
        CPUState exact{};exact.gpr[3]=inlet.gpr[3];exact.gpr[4]=inlet.gpr[4];
        mod.run(exact,pair,kernel,4096);
        if(exact.gpr[3])picked=tried;
    }
    if(eligible!=3||(tried&~broad)||half(scratch,actor1+0x1C6)!=(arm?60u:43u)||bool(picked)==bool(arm)){
        std::fprintf(stderr,"moving receiver arm=%u kind=%u eligible=%u broad=%u tried=%u picked=%u hp=%u\n",
            arm,kind,eligible,broad,tried,picked,half(scratch,actor1+0x1C6));
        fail("the observed narrow pair did not explain the same original HP result");
    }
    std::printf("world-receiver-narrow: %u %u %u %u %u %u %u\n",
                arm,n,eligible,broad,tried,picked,half(scratch,actor1+0x1C6));
    return attack;
}

inline void observe(const Module& mod,Memory& ram,unsigned opening){
    std::printf("world-receiver-open: %u %u %u\n",opening,half(ram,actor0+0x1C6),half(ram,actor1+0x1C6));
    for(unsigned n=1;n<=21;++n){
        put32(at(ram,pad),n==8?0x200:0);
        visit(mod,ram,0,n);
        if(half(ram,attack_count)||half(ram,actor1+0x1C6)!=60)fail("shared prefix already published contact");
    }
    const auto saved=ram;
    for(unsigned arm=0;arm<2;++arm){
        ram=saved;
        u32 cell=0;
        for(unsigned n=22;n<=32;++n){
            *at(ram,pad+32+22)=u8(arm?-63:63);
            if(n==26)sample_visit(mod,ram,arm,n);
            if(n==27)cell=sample_visit(mod,ram,arm,n);
            CallObservation writer{contact::hp_write};
            visit(mod,ram,arm,n,&writer);
            const unsigned expected=unsigned(!arm&&n==27);
            if(writer.count!=expected||(expected&&(writer.caller!=0x8002F1D8||
               writer.arguments[0]!=actor1||writer.arguments[1]!=17))||
               half(ram,actor1+0x1C6)!=(!arm&&n>=27?43u:60u)||half(ram,actor0+0x1C6)!=60)
                fail("the actual world did not preserve the pair fork's HP and owner result");
            bool returned=false;
            if(n==32){
                for(u32 free=word(ram,contact::free_head+16),hops=0;free;free=word(ram,free+4)){
                    slot(free);
                    if(++hops>48)fail("moving receiver crossed the shape free list");
                    if(free==cell){if(*at(ram,free))fail("returned strike is still active");returned=true;break;}
                }
                if(!returned)fail("moving receiver did not return the same attack cell");
            }
            if(n==27||n==28||n==32)
                std::printf("world-receiver-end: %u %u %u %u %u %u %u %u %u\n",arm,n,
                    writer.count,half(ram,actor1+0x1C6),half(ram,actor1+0x20C),word(ram,actor0+0x418),
                    word(ram,actor1+0x41C),half(ram,attack_count),unsigned(returned));
        }
    }
}
} // namespace retail_world_moving_receiver
