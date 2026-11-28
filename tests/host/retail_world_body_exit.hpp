// Same loaded Tau inlet as the moving-receiver world contrast, through KO.
#pragma once
#include "retail_world_moving_receiver.hpp"

namespace retail_world_body_exit {
using namespace retail_world_moving_receiver;
inline constexpr u32 view=0x803BB4C0+3120,owner=view+0x600,resources=0x803C4D94+4,motion=0x803C4D7C+4;
inline constexpr u32 scene=schedule::scene,queue=scene+488+840,disc=0x8000B07C;
inline constexpr u32 requests=0x81050000,pools=0x81063000;
struct Delivery {
    contact::ActorSpec spec;
    contact::HitBlock hit;
    std::array<u8,435104> motion;
    std::array<u8,171236> view;
};
inline void read(Delivery& assets){
    const auto role=[](auto& bytes){
        if(std::fread(bytes.data(),1,bytes.size(),stdin)!=bytes.size())fail("replacement role extent");
    };
    role(assets.spec);role(assets.hit);role(assets.motion);role(assets.view);
}

inline void roster(Memory& ram){
    *at(ram,scene+91)=2;*at(ram,scene+85)=0;*at(ram,scene+5577)=1;
    put16(at(ram,queue),0x0807);put16(at(ram,queue+28),0x0106);
    put32(at(ram,0x8043B5A0-21068),requests); // empty original KO model request buffer
}

inline void deliver(const Module& mod,Memory& ram,const Delivery& next,u32 old){
    constexpr u32 dest=0x803B07AC+56,role0=0x812C9400,role2=0x81765C00,role3=0x81547C00,role4=0x81323C00;
    if(word(ram,dest)!=role0||word(ram,dest+4)!=0xF0000000||word(ram,dest+8)!=role2||
       word(ram,dest+12)!=role3||word(ram,dest+16)!=role4||word(ram,dest+20)!=0xF0000000||
       word(ram,owner)||half(ram,scene+18)!=0x0106)
        fail("loaded replacement role mapping changed");
    std::memcpy(at(ram,role0),next.spec.data(),next.spec.size());
    std::memcpy(at(ram,role2),next.hit.data(),next.hit.size());
    std::memcpy(at(ram,role3),next.motion.data(),next.motion.size());
    std::memcpy(at(ram,role4),next.view.data(),next.view.size());
    CPUState complete{};complete.gpr[3]=1;
    mod.run(complete,ram,0x800412C4,4096);
    if(word(ram,owner)!=old||!*at(ram,old+9)||word(ram,resources)!=role0||word(ram,scene+132)!=role2||
       word(ram,motion)!=role3||word(ram,0x803BB4A0+4)!=role4)fail("replacement delivery crossed its seat or archive identity");
}

inline unsigned receiving(Memory& ram){
    const auto count=half(ram,contact::receiver_count);
    if(count>18)fail("queued body receiver extent");
    unsigned mask=0;
    for(unsigned i=0;i<count;++i){
        const auto cell=word(ram,contact::receiver_registry+4*i);
        const auto who=word(ram,cell+0x20);
        if(who==actor0)continue;
        if(who!=actor1)fail("queued body receiver crossed its physical seat");
        const auto first=word(ram,actor1+0x3FC)+32,desc=word(ram,cell+0x28);
        if(desc<first||desc>=first+16*80||(desc-first)%80)fail("queued receiving cell has an unrelated descriptor");
        const auto bit=1u<<((desc-first)/80);
        if(mask&bit)fail("queued body duplicated a receiving descriptor");
        mask|=bit;
    }
    return mask;
}

inline void trace(Memory& ram,const char* tag,unsigned n,unsigned polls){
    std::printf("world-ko-%s: %u %u %u %u %u %u %04X %04X %u %04X %u %u %u %u\n",tag,n,
            *at(ram,actor1),*at(ram,actor1+0x18),*at(ram,actor1+0x19),half(ram,actor1+0x1C),
            *at(ram,actor1+0x3F0),half(ram,actor1+0x3E8),half(ram,actor1+0x272),half(ram,actor1+0x1C6),
            receiving(ram),unsigned(word(ram,owner)!=0),polls,*at(ram,actor1+0x743),*at(ram,scene+69));
}

inline u32 first_poll(const CallObservation& poll){
    if(!poll.count||poll.count>2||poll.first_arguments[0]!=actor0||(poll.count==2&&poll.first_arguments[1]!=actor1))
        fail("world KO did not preserve source seat order");
    return poll.count==2;
}

inline void finish_request(const Module& mod,Memory& ram,unsigned after,const Delivery& next,u32 old){
    for(unsigned i=1;i<=63;++i){
        CallObservation poll{0x80056900};
        visit(mod,ram,1,after+i,&poll);
        if(first_poll(poll)||word(ram,owner)||*at(ram,actor1+0x3F0)!=1||
           !*at(ram,actor1)||*at(ram,actor1+0x18)!=(i<60?3u:0u)||half(ram,actor1+0x1C6)||
           half(ram,actor1+0x1C)!=(i<60?60u-i:0u)||receiving(ram))
            fail("queued phase-zero body became ready without the replacement owner");
        if(i==1||i==59||i==60||i==63)trace(ram,"wait",i,0);
    }
    deliver(mod,ram,next,old);
    trace(ram,"delivery",0,0);
    for(unsigned i=1;i<=141;++i){
        if(i==83){
            // The neutral accepted path is retained. This fork shows the
            // first eligible processed pad can reach a paid source request.
            auto fork=ram;put32(at(fork,pad+32),0x200);
            CPUState cpu{};
            if(mod.run_to_or_return(cpu,fork,schedule::dispatch,{0x800E058C,0x80264290},nullptr,1u<<20)!=0x800E058C||
               cpu.lr!=0x80130584||cpu.gpr[3]!=actor1||cpu.gpr[4]!=2||cpu.gpr[5]!=10||
               half(fork,actor1+0x774)!=9||half(ram,actor1+0x774)!=10||receiving(ram))
                fail("replacement input must precede its ordinary receiver publication");
            std::puts("world-ko-input: 83 2 10 9");
        }
        CallObservation poll{0x80056900};
        visit(mod,ram,1,after+63+i,&poll);
        if(first_poll(poll)!=unsigned(i>=83)||!*at(ram,actor1)||*at(ram,actor1+0x18)!=1||
           *at(ram,actor1+0x3F0)!=2||half(ram,actor1+0x3E8)!=0x0106||half(ram,actor1+0x1C6)!=220||
           half(ram,actor1+0x774)!=10||half(ram,actor1+0x784)!=3||word(ram,owner)!=old||
           receiving(ram)!=(i>=140?0x3EF0u:0u)||*at(ram,actor1+0x743)!=unsigned(i>=81)||
           half(ram,actor1+0x272)!=(i<21?0xEF:i<82?0xC3:0))
            fail("loaded replacement actor lost its original readiness/receiver sequence");
        if(i==1){
            unsigned descriptors=0;
            for(unsigned j=0;j<48;++j){
                const auto cell=pool+j*stride;
                if(!*at(ram,cell)||word(ram,cell+0x20)!=actor1)continue;
                const auto bank=word(ram,actor1+0x3FC)+32,desc=word(ram,cell+0x28);
                if(desc<bank||desc>=bank+16*80||(desc-bank)%80)
                    fail("replacement body retained a foreign descriptor");
                const unsigned bit=1u<<((desc-bank)/80);
                if(descriptors&bit)fail("replacement body bound a descriptor twice");
                descriptors|=bit;
            }
            if(descriptors!=0xFFFF||half(ram,old+26)!=4)fail("loaded Time Bomber does not own its complete body/view wrappers");
            for(unsigned j=0;j<48;++j)
                if(half(ram,actor1+0x20C+2*j))fail("loaded replacement retained an old contact token");
            std::puts("world-ko-bound: 1 16 4 10 3 48");
        }
        if(i==1||i==21||i==80||i==81||i==82||i==83||i==139||i==140)
            trace(ram,"restart",i,poll.count-1);
    }
}

inline void observe(const Module& mod,Memory& ram,unsigned opening,const Delivery& next){
    for(unsigned n=1;n<=26;++n){
        put32(at(ram,pad),n==8?0x200:0);
        *at(ram,pad+32+22)=n>=22?63:0;
        visit(mod,ram,0,n);
    }
    const auto saved=ram;
    const auto old=word(ram,owner);
    if(!old)fail("world KO needs the original loaded view owner");
    const unsigned roots=half(ram,old+26);
    if(!*at(ram,old+9)||!roots||roots>32||half(ram,actor1+0x4AA)!=90||receiving(ram)!=6)
        fail("loaded Tau KO requires the retained rendering and collision owners");
    std::array<u32,64> wrappers{};
    for(unsigned i=0;i<roots;++i)for(unsigned j=0;j<2;++j){
        const auto ptr=word(ram,view+(j?1928:1800)+4*i);
        if(ptr<0x81066000||ptr>=0x81066000+44*64||(ptr-0x81066000)%44||!*at(ram,ptr+9))
            fail("old Tau view pointer is not a live wrapper");
        wrappers[2*i+j]=ptr;
    }
    std::printf("world-ko-open: %u %u %u 90\n",opening,roots,half(ram,contact::free_count+8));
    for(unsigned arm=0;arm<2;++arm){
        ram=saved;
        if(arm)put16(at(ram,actor1+0x1C6),1);
        for(unsigned n=27;n<=(arm?65u:35u);++n){
            *at(ram,pad+32+22)=n==27?63:0;
            if(n==27){
                auto fork=ram;CPUState hp{};
                const unsigned before=arm?1:60,request=arm?14:17;
                if(mod.run_to_or_return(hp,fork,schedule::dispatch,{contact::hp_write,0x80264290},nullptr,1u<<20)!=contact::hp_write||
                   hp.lr!=0x8002F1DC||hp.gpr[3]!=actor1||hp.gpr[4]!=request||
                   half(fork,actor1+0x1C6)!=before||word(fork,actor0+0x418)!=request||
                   word(fork,actor1+0x41C)!=request)fail("KO writer inlet lost its amount, credit, or receiver");
                mod.finish_stopped_leaf(hp,contact::hp_write,164);
                if(hp.gpr[3]!=arm||half(fork,actor1+0x1C6)!=(arm?0u:43u)||*at(fork,actor1+0x18)!=1||
                   word(fork,owner)!=old||word(fork,scene+60+0x118)!=300)
                    fail("the HP leaf itself did more than change health");
                std::printf("world-ko-writer: %u %u %u %u %u\n",arm,n,before,request,arm?1u:17u);
            }
            unsigned view_free=0,owner_free=0;
            if(n==65){
                auto queued=ram;CPUState loader{};
                if(mod.run_to_or_return(loader,queued,schedule::dispatch,{0x80041BF0,0x80264290},nullptr,1u<<20)!=0x80041BF0||
                   loader.lr!=0x80055344||loader.gpr[3]!=actor1||half(queued,loader.gpr[4])!=0x0106||
                   loader.gpr[5]||word(queued,owner)!=old||half(queued,actor1+0x1C)!=60)
                    fail("replacement request did not come from the exact retained actor/queue");
                view_free=half(queued,pools+18);owner_free=half(queued,pools+28+18);
            }
            const auto turns=word(ram,scene+168),world=word(ram,scene+176),combat=word(ram,scene+172);
            CPUState cpu{};CallObservation write{contact::hp_write};
            const auto cut=mod.run_to_or_return(cpu,ram,schedule::dispatch,{disc,0x80264290},&write,1u<<20);
            if(cut!=(n==65?disc:0)||write.count!=unsigned(n==27)||half(ram,actor1+0x1C6)!=(arm?0u:43u)||
               (n==27&&(write.caller!=0x8002F1D8||write.arguments[0]!=actor1||write.arguments[1]!=(arm?14u:17u)))||
               (!cut&&(word(ram,scene+168)!=turns+1||word(ram,scene+172)!=combat+1||word(ram,scene+176)!=world+1))||
               !*at(ram,actor1)||half(ram,actor0+0x1C6)!=60||word(ram,owner)!=(n==65?0:old)||*at(ram,scene+69)!=4)
                fail("world KO left its complete or declared disc boundary");
            unsigned freed=0;
            for(u32 f=word(ram,contact::free_head+16),hops=0;f;f=word(ram,f+4)){
                const auto i=slot(f);
                if(++hops>48)fail("KO old body free-list extent");
                if(i>=3&&i<=5){
                    if(*at(ram,f))fail("freed old Tau body cell still active");
                    freed|=1u<<(i-3);
                }
            }
            if(freed!=(arm&&n>=63?7u:0u))fail("old body cells did not unlink at their own boundary");
            if(n==27||n==28||n==32||n==63||n==64||n==65){
                std::printf("world-ko-old: %u %u %u %04X %04X %u %u %u %u %u %u %u %u\n",
                    arm,n,*at(ram,actor1+0x18),half(ram,actor1+0x272),receiving(ram),freed,
                    unsigned(word(ram,owner)!=0),*at(ram,actor1+0x3F0),half(ram,actor1+0x1C6),
                    word(ram,actor0+0x418),word(ram,actor1+0x41C),*at(ram,actor1+0x435),*at(ram,scene+5594));
            }
            if(cut==disc){
                if(cpu.lr!=0x80041EE4||cpu.gpr[3]!=504||cpu.gpr[4]!=0x803B07AC+56||
                   word(ram,resources)||word(ram,motion)||*at(ram,old+9)||
                   half(ram,pools+18)!=view_free+2*roots||half(ram,pools+28+18)!=owner_free+1||
                   *at(ram,scene+5577)||*at(ram,scene+85)!=1||
                   *at(ram,queue+22)!=1||*at(ram,queue+24)!=1||*at(ram,queue+28+23)!=1)
                    fail("loaded Tau archive and view wrapper release did not precede its real next-model request");
                for(unsigned i=0;i<2*roots;++i)
                    if(*at(ram,wrappers[i]+9))fail("old loaded view wrapper is still active after archive release");
                std::printf("world-ko-request: 65 504 %u 1 0 1\n",2*roots);
                finish_request(mod,ram,n,next,old);break;
            }
        }
    }
}
} // namespace retail_world_body_exit
