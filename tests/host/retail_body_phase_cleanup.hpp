// Continue the original phase-3 dispatcher without calling the disc backend.
#pragma once
#include "retail_contact_effect_archive.hpp"

namespace retail_body_phase_cleanup {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace archive=retail_contact_effect_archive;
inline constexpr u32 victim=contact::receiver,scene=contact::scene,seat=1,side=scene+60;
inline constexpr u32 wrapper=0x80054070,presentation=0x8005A580,loader=0x80041BF0,disc=0x8000B07C;
inline constexpr u32 phase3=0x8005514C,configure=0x80056CE0;
inline constexpr u32 view_count=3120,view=0x803BB4C0+view_count*seat,view_owner=view+0x600;
inline constexpr u32 pending_model=scene+16+2*seat,pending_color=scene+160+seat;
inline constexpr u32 ordinal=scene+84+seat,length=scene+90+seat,cursor=scene+5576+seat;
inline constexpr u32 queue=scene+488+840*seat,active_mask=scene+5593,active_count=scene+5594;
inline constexpr u32 resources=0x803C4D94+4*seat,slot_bank=0x803C4D7C+4*seat;
inline constexpr u32 repository=0x81060000,pools=0x81063000,header=0x81064000,pool=pools+28;
inline constexpr u32 source_basis=0x8043B5A0-21352,source_pool=0x8036F5D4;
inline constexpr u16 next=0x0106,terminal_model=0x0F07;

class Probe {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    unsigned byte(u32 a){return at(a)[0];}
    unsigned body(unsigned off){return byte(victim+off);}
    void set(u32 a,unsigned x){at(a)[0]=u8(x);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void short_store(u32 a,u16 x){put16(at(a),x);}
    u32 call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;
        mod.run(cpu,ram,entry,4096);
        return cpu.gpr[3];
    }
    void tick(){
        CPUState cpu{};cpu.gpr[3]=victim;
        CallObservation update{phase3};
        const auto cut=mod.run_to(cpu,ram,wrapper,{presentation,loader,disc},&update);
        if(cut!=presentation||cpu.gpr[3]!=victim||cpu.lr!=0x80054144||update.count!=1||update.arguments[0]!=victim||
           update.caller!=0x800540C8)
            fail("phase-3 actor did not use the original wrapper dispatch");
    }
    void prepare(unsigned kind){
        if(!body(0)||body(0x18)!=3||body(0x19)||body(0x3E4)!=seat||body(0x3E7)!=seat||body(0x3E6)||
           half(victim+0x1C6)||half(victim+0x20C)!=2||word(scene+0xB8)!=victim||word(side+0x118))
            fail("phase-3 inlet did not retain the accepted normal body");
        set(active_mask,3);set(active_count,2);
        set(length,2);set(cursor,1);set(ordinal,0);
        set(victim+0x491,kind?2:0);set(victim+0x492,0);
        short_store(queue,contact::sasuke.model);short_store(queue+28,next);
        short_store(pending_model,contact::sasuke.model);
        set(pending_color,0);
        set(scene+203+seat,1);
        if(kind==2){
            set(side+246,1);
            set(word(0x8043B5A0-31856)+50,1); // Declared sentinel branch after original mode-0 KO.
        }
    }
    void install_empty_owner(bool initialize){
        if(initialize){
            archive::Setup setup(mod,ram);
            setup.init_heap();
            store(source_basis,repository);store(0x8043B5A0-21024,pools);
            store(pool,header-repository);store(pool+4,32);
            std::memcpy(at(pool+16),at(source_pool+16),12);
            call(0x80197950,1);
        }
        if(call(0x80197A0C,1)!=header||word(pool+8)!=header||word(pool+12)||half(pool+18)||
           byte(header+9)!=1||!word(header+12)||half(header+26))
            fail("phase-3 source owner did not allocate its empty archive context");
        store(view_owner,header);
    }
    bool full_hp_accounting() {
        return !half(victim+0x1C6)&&half(victim+0x20C)==2&&word(victim+0x41C)==16&&
            word(side+0x118)==0&&body(0x435)==1;
    }
    void request(unsigned caster,unsigned kind){
        const auto prepared=ram;
        const auto wanted=kind==2?terminal_model:next;
        CPUState selected{};selected.gpr[3]=victim;
        mod.run_to(selected,ram,wrapper,{loader});
        if(selected.lr!=0x80055344||selected.gpr[3]!=victim||half(selected.gpr[4])!=wanted||
           selected.gpr[5]||half(pending_model)!=wanted||byte(pending_color)||body(0x19)!=1||
           half(victim+0x1C)!=60||!body(0)||word(view_owner)!=header)
            fail("phase-3 model request crossed clock, identity or resource ownership");
        std::memcpy(ram.data(),prepared.data(),ram.size());
        CPUState cut{};cut.gpr[3]=victim;
        mod.run_to(cut,ram,wrapper,{disc});
        if(cut.lr!=0x80041EE4||cut.gpr[4]!=0x803B07AC+56||word(view_owner)||word(resources)||word(slot_bank)||
           word(pool+8)||word(pool+12)!=header||half(pool+18)!=1||byte(header+9)||
           word(archive::arena+4)!=archive::block||word(archive::arena+8)||
           word(archive::block+8)!=archive::arena_end-archive::block||
           half(pending_model)!=wanted||body(0x19)!=1||!body(0)||half(victim+0x1C)!=60||
           byte(cursor)!=(kind==2?1:0)||byte(ordinal)!=(kind==2?0:1)||
           byte(queue+22)!=unsigned(kind==0)||byte(queue+24)!=unsigned(kind==0)||
           byte(queue+28+23)!=unsigned(kind==0)||!full_hp_accounting())
            fail("phase-3 original empty resource release or queue preparation changed");
        std::printf("body-cleanup-request: %u %u %04X %u %u %u %u %u %u %u %u %u %u\n",
                    caster,kind,half(pending_model),cut.gpr[3],byte(cursor),byte(ordinal),byte(queue+22),byte(queue+24),
                    byte(queue+28+23),half(pool+18),body(0),body(0x19),half(victim+0x1C));
    }
    void reentry(unsigned caster){
        if(word(view_owner))fail("phase-zero readiness control already had an owner");
        CPUState ready{};ready.gpr[3]=victim;
        mod.run_to(ready,ram,wrapper,{presentation});
        if(ready.lr!=0x80054144||body(0x18)||!body(0)||half(victim+0x1C6))
            fail("phase-zero actor admitted without its model resource");
        install_empty_owner(false);
        const auto generation=body(0x3F0);
        CPUState enter{};enter.gpr[3]=victim;
        mod.run_to(enter,ram,wrapper,{configure});
        if(enter.lr!=0x80054328||enter.gpr[3]!=victim||body(0x18)!=1||body(0x19)||
           body(0x3F0)!=generation+1||half(victim+0x3E8)!=next||half(victim+0x1C6)||word(view_owner)!=header)
            fail("phase-zero owner gate did not reach the original next-model setup cut");
        std::printf("body-cleanup-ready: %u %u %04X %u %u\n",caster,body(0x18),half(victim+0x3E8),
                    body(0x3F0)-generation,half(victim+0x1C6));
    }
public:
    Probe(const Module& m,const std::vector<u8>& data,unsigned caster,unsigned kind):mod(m),ram(data){
        prepare(kind);
        install_empty_owner(true);
        if(kind!=1)request(caster,kind);
        else tick();
        if(body(0x18)!=3||body(0x19)!=1||half(victim+0x1C)!=60||!body(0)||!full_hp_accounting())
            fail("phase-3 entry did not arm the 60-visit wait");
        // In the load cases these are declared separate callback visits
        // after the resource frontier. The unresolved disc proc is not resumed.
        for(unsigned n=0;n<2;++n){
            set(scene+31,1);
            tick();
            if(half(victim+0x1C)!=60||body(0x19)!=1||!body(0)||!full_hp_accounting())
                fail("phase-3 scene allow-mask advanced an excluded player");
        }
        for(unsigned n=1;n<=60;++n){
            set(scene+31,n==1?2:0);
            tick();
            if(half(victim+0x1C)!=60-n||!full_hp_accounting()||
               (n<60&&(!body(0)||body(0x18)!=3||body(0x19)!=1||byte(active_mask)!=3||byte(active_count)!=2)))
                fail("phase-3 wait did not count its allowed isolated visits");
        }
        const bool gone=kind==1;
        const auto wanted=kind==2?terminal_model:gone?contact::sasuke.model:next;
        if(body(0)!=unsigned(!gone)||body(0x18)!=(gone?3:0)||body(0x19)!=(gone?2:0)||
           byte(active_mask)!=(gone?1:3)||byte(active_count)!=(gone?1:2)||half(pending_model)!=wanted||
           word(view_owner)!=(gone?header:0)||word(pool+8)!=(gone?header:0)||half(pool+18)!=unsigned(!gone))
            fail("phase-3 expiry mixed active slot reuse and normal actor retirement");
        std::printf("body-cleanup-expiry: %u %u %u %u %u %u %u %u %u %u %u\n",caster,kind,body(0),body(0x18),
                    body(0x19),byte(active_mask),byte(active_count),half(victim+0x20C),half(victim+0x1C),
                    unsigned(word(view_owner)!=0),half(pool+18));
        if(!kind)reentry(caster);
    }
};
} // namespace retail_body_phase_cleanup
