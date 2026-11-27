// Original actor tail, next shape publication, and accepted pair kept separate.
#pragma once
#include "retail_contact_frame_prefix.hpp"

namespace retail_actor_frame_admission {
using namespace retail_contact_pose_clock;
namespace prefix=retail_contact_frame_prefix;
inline constexpr u32 eligibility_flags=0x5E0,attack_closed=0x276,close_bit=0x20000000;
inline constexpr u32 attack_count=0x8043B5A0-21328,attack_registry=0x803C477C;

inline void observe(const Module& mod,std::vector<u8>& ram,unsigned model,const std::array<u32,2>& shapes,
                    unsigned count,u32 table,u32 amount,unsigned seed,unsigned damage,float pose_frame){
    constexpr u32 body=contact::receiver;
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto spec=word(actor+0x4AC);
    const bool tau=model==0x0807;
    if((!tau&&model!=1)||!spec||damage!=(tau?20u:41u)||half(actor+0x3E8)!=model||half(attack_count)!=count||
       count!=(tau?1u:2u)||at(actor+0x18)[0]||
       half(body+0x1C6)!=180||half(body+0x20C)||half(actor+0x26C)!=1)
        fail("actor frame needs the admitted original melee queue");
    const unsigned active_loss=tau?17:35;
    // Amount-control forks, not two hits in the retained publication.
    for(unsigned actor_phase=0;actor_phase<2;++actor_phase){
        auto fork=ram;
        contact::at(fork,actor+0x18)[0]=u8(actor_phase);
        CPUState pair{};CallObservation requested{contact::amount_entry};
        mod.run_observed(pair,fork,contact::pair_pass,requested,4096);
        const auto expected=actor_phase?active_loss:damage;
        const auto hp=contact::half(fork,body+0x1C6);
        if(requested.count!=1||requested.caller!=0x8002F108||requested.arguments[0]!=amount||
           requested.arguments[1]!=actor||requested.arguments[2]!=body||hp!=180-expected||
           be32(contact::at(fork,actor+0x418))!=expected||be32(contact::at(fork,body+0x41C))!=expected||
           contact::at(fork,actor+timer)[0]!=seed||contact::at(fork,body+timer)[0]!=seed){
            std::fprintf(stderr,"phase-only amount model=%04X phase=%u hp=%u (expected %u) calls=%u\n",
                         model,actor_phase,hp,180-expected,requested.count);
            fail("phase-only amount control did not reproduce the exact amount boundary");
        }
        std::printf("frame-amount: %u %u %u %u\n",model,actor_phase,expected,hp);
    }
    put32(at(0x803C4D94),spec);
    at(actor+0x3E7)[0]=0;
    at(actor+0x18)[0]=1;
    CPUState install{};install.gpr[3]=actor;
    mod.run(install,ram,tau?0x801A04F0:0x800EA500);
    if(word(actor+0x4AC)!=spec||word(actor+0x27C)!=table||
       word(actor+0x4BC)!=(tau?0x801A05F0:0x800EA61C)||
       word(actor+0x4C0)!=(tau?0x801A05F4:0x800EA6A0)||word(actor+0x4C8)!=(tau?0x801A0610:0x800EA6B8))
        fail("actor frame source model installed different class callbacks");

    // Neutral lanes and finite stage bounds. No world scheduler or controller.
    put32(at(0x8043B5A0-21316),prefix::stage);
    for(unsigned off:{52u,56u,64u})put_float(at(prefix::stage+off),10000.f);
    for(unsigned off:{60u,68u})put_float(at(prefix::stage+off),-10000.f);
    at(contact::scene+0x1829)[0]=1;
    at(actor+0x5E4)[0]=at(actor+0x5E5)[0]=9;
    at(actor+0x5DA)[0]=at(actor+0x5DB)[0]=0;
    at(actor+0x73F)[0]=1;
    for(unsigned lane=0;lane<4;++lane)at(actor+0x1AD4+64*lane+55)[0]=0xFF;
    factors(ram,actor);
    const auto root=std::array{word(actor+0x20),word(actor+0x24),word(actor+0x28)};
    const auto free_before=half(contact::free_count+8),token=half(actor+0x26C);
    const auto shape_owners_stable=[&](){
        if(half(contact::free_count+8)!=free_before||half(actor+0x26C)!=token)return false;
        for(unsigned i=0;i<count;++i){
            if(!at(shapes[i])[0]||at(shapes[i]+0x10)[0]!=1||word(shapes[i]+0x20)!=actor||
               word(shapes[i]+0x24)!=actor||word(shapes[i]+0x2C)!=table)return false;
        }
        return true;
    };
    for(unsigned phase=0;phase<2;++phase){
        const bool closed=phase==0;
        const unsigned stale=closed?0:1,before_publish=closed?count:0;
        const auto flags=(word(actor+eligibility_flags)&~close_bit)|(closed?close_bit:0);
        put32(at(actor+eligibility_flags),flags);
        at(actor+attack_closed)[0]=u8(stale);
        const auto cut_check=half(body+0x1C6);
        CPUState update{};update.gpr[3]=actor;
        CallObservation post{word(actor+0x4C0)};
        mod.run_observed(update,ram,prefix::frame,post,4096);
        if(post.count!=1||post.caller!=0x80055630||post.arguments[0]!=actor||
           at(actor+attack_closed)[0]!=unsigned(closed)||(word(actor+eligibility_flags)&close_bit)!=(flags&close_bit)||
           !shape_owners_stable()||half(attack_count)!=before_publish||half(body+0x1C6)!=cut_check||
           word(actor+0x20)!=root[0]||word(actor+0x24)!=root[1]||word(actor+0x28)!=root[2]||
           word(actor+0x1CDC)!=std::bit_cast<u32>(pose_frame)){
            std::fprintf(stderr,"admission frame %04X phase=%u flag=%08X projected=%u post=%u/%08X queue=%u framebits=%08X\n",
                         model,phase,word(actor+eligibility_flags),at(actor+attack_closed)[0],post.count,post.caller,
                         half(attack_count),word(actor+0x1CDC));
            fail("complete actor wrapper did not project the late attack eligibility");
        }
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState next{};mod.run(next,ram,entry,1024);
        }
        const auto published=closed?0:count;
        if(half(attack_count)!=published||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=contact::body||!shape_owners_stable())
            fail("next collision publication changed ownership or used a stale eligibility flag");
        unsigned seen=0;
        for(unsigned i=0;i<published;++i){
            const auto cell=word(attack_registry+4*i);
            const unsigned id=cell==shapes[0]?0:cell==shapes[1]&&count==2?1:2;
            if(id>=count||(seen&(1u<<id)))fail("republication did not use the same melee cells");
            seen|=1u<<id;
        }
        CPUState pair{};CallObservation requested{contact::amount_entry};
        mod.run_observed(pair,ram,contact::pair_pass,requested,4096);
        const auto lost=closed?0:active_loss;
        if(requested.count!=unsigned(!closed)||(!closed&&(requested.caller!=0x8002F108||requested.arguments[0]!=amount||
           requested.arguments[1]!=actor||requested.arguments[2]!=body))||
           half(body+0x1C6)!=180-lost||word(actor+0x418)!=lost||word(body+0x41C)!=lost||
           half(body+0x20C)!=unsigned(!closed)||at(actor+timer)[0]!=(closed?0:seed)||at(body+timer)[0]!=(closed?0:seed)){
            std::fprintf(stderr,"admission pair %04X phase=%u calls=%u hp=%u ledgers=%u/%u timers=%u/%u shape z=%g body z=%g radius=%g flags=%08X\n",
                         model,phase,requested.count,half(body+0x1C6),word(actor+0x418),word(body+0x41C),
                         at(actor+timer)[0],at(body+timer)[0],double(std::bit_cast<float>(word(shapes[0]+0x6C))),
                         double(std::bit_cast<float>(word(contact::body+0x6C))),
                         double(std::bit_cast<float>(word(shapes[0]+0x7C))),word(actor+eligibility_flags));
            fail("original pair consumed a withheld shape or changed the retained accepted hit");
        }
        std::printf("frame-admission: %u %u %u %u %u %u %u %u %u %u %u\n",
                    model,phase,unsigned(closed),stale,at(actor+attack_closed)[0],before_publish,published,
                    count,token,requested.count,half(body+0x1C6));
    }
}
}
