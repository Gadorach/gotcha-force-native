// Normal Blue Striker's installed meter, request leaves and lower prefixes.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
constexpr u32 record=0x81014000,scene=0x81018000;
constexpr u32 selector=0x5E4,local_phase=0x544,lower=0x5DB,flags=0x5E0,latch=0x588;
constexpr u32 value=0x736,delay=0x73A,clock568=0x568,held=0x5B4,pulse=0x5D4;
constexpr unsigned cruise=27,boost=28;

class Replay {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 addr){return retail_stationary_contact::at(ram,addr);}
    u8* obj(){return at(actor);}
    unsigned half(u32 off){return retail_stationary_contact::half(ram,actor+off);}
    float clock(){return std::bit_cast<float>(be32(obj()+clock568));}
    bool boosted(){return be32(obj()+flags)&0x80;}
    void write(u32 off,u32 n){put32(obj()+off,n);}
    CPUState call(u32 entry,unsigned arg=0){
        CPUState cpu{};cpu.gpr[3]=actor;cpu.gpr[4]=arg;
        mod.run(cpu,ram,entry,4096);return cpu;
    }
    void prefix(u32 entry,u32 callee,u32 ret){
        CPUState cpu{};cpu.gpr[3]=actor;
        const auto cut=mod.run_to_or_return(cpu,ram,entry,{callee,0x80264290},nullptr,4096);
        if(cut!=callee||cpu.lr!=ret||cpu.gpr[3]!=actor){
            std::fprintf(stderr,"boost prefix entry=%08X boundary=%08X lr=%08X\n",entry,cut,cpu.lr);
            fail("boost replay crossed its original prefix");
        }
    }
    void lower_prefix(){
        const bool active=obj()[selector]==boost;
        const u32 entry=active?0x80063644:0x800633C8;
        if((!active&&obj()[selector]!=cruise)||obj()[lower]!=(active?34:33)||
           be32(at(0x802D37A8+4*obj()[lower]))!=entry)
            fail("boost replay leaves the exact cruise/boost pair");
        // Both lower handlers preserve real resource and phase writes; the
        // cut precedes heading, integration, floor query and presentation.
        prefix(entry,0x80066C58,active?0x80063764:0x8006344C);
        if(boosted()!=active||obj()[0x87]!=1)fail("boost/cruise lower handler state");
    }
    void install(){
        put32(at(0x803C4D94),record);
        put32(at(0x8043B5A0-31852),scene);
        put16(obj()+0x3E8,0x0D00);
        call(0x800F7580);
        if(be32(obj()+0x4AC)!=record||be32(obj()+0x4B0)!=0x8031CD78)fail("Blue Striker installer chose a different record/config");
        CPUState cpu{};cpu.gpr[3]=actor;
        const auto cut=mod.run_to(cpu,ram,0x8005614C,{0x8013BD80,0x80006238});
        if(cpu.lr!=(cut==0x8013BD80?0x80056604:0x80056824)||obj()[0x661]!=3||obj()[0x7CD]!=1||
           half(value)!=10000||half(delay)||be32(obj()+0x4AC)!=record)
            fail("Blue Striker normal resource initialization changed");
        put_float(obj()+0x5F4,1.f); // the branch witness declares its unit rate
        // Cue 19 occupies slot 6. Supply an already-held original request
        // (as in the stock replay) so the cue refreshes without playback.
        if(obj()[0x96])fail("declared cue slot needs audio seat zero");
        put32(at(0x803D3AE8+6*16+88),be32(at(0x8031B694+19*4)));
        if(call(0x8006A750,cruise).gpr[3]!=1)fail("initial normal cruise refused");
        lower_prefix();
    }
    void run(unsigned route){
        bool prior_down=false;
        for(unsigned visit=1;visit<=(route==0?49u:route==1?54u:250u);++visit){
            const bool down=route==0?visit==1:route==1?visit<37:visit<=201||visit>=218;
            const bool fresh=down&&!prior_down;
            const auto before=half(value),delay_before=half(delay);
            const unsigned old=obj()[selector],ready=obj()[latch],phase=obj()[local_phase];
            const bool was_boost=boosted();
            const float old_clock=clock();
            // The real resource pass runs before the conditioned selector
            // leaves. Raw pad polling and the outer action-owner checks are
            // not in this scoped schedule.
            prefix(0x80055568,0x80059068,0x8005559C);
            const auto selected_meter=half(value),selected_delay=half(delay);
            write(held,down?0x100:0);write(pulse,fresh?2:0);
            const unsigned changed=call(old==boost?0x800698C0:0x80069860).gpr[3];
            const unsigned next=obj()[selector];
            if(changed!=unsigned(next!=old)||half(value)!=selected_meter||half(delay)!=selected_delay)
                fail("boost request is not a debit or refill");
            lower_prefix();
            if((old==boost)!=was_boost||(ready&&phase!=2)||(!was_boost&&(ready||phase!=1))||
               (was_boost&&phase==1&&next==boost&&clock()!=old_clock-1.f)||clock()<-1.f||clock()>30.f||
               (next==boost&&old==cruise&&(clock()!=30.f||obj()[latch]||obj()[local_phase]!=1)))
                fail("boost replay did not separate the clock seed from continuation");
            const unsigned post_clock=next==boost?unsigned(clock()):0;
            std::printf("blue-boost: %u %u %u %u %u %u %u %u %u %u %u %u %u\n",route,visit,
                        unsigned(down),unsigned(fresh),old,ready,before,delay_before,next,
                        half(value),half(delay),obj()[latch],post_clock);
            prior_down=down;
        }
    }
public:
    Replay(const Module& maker,const std::vector<u8>& initial):mod(maker),ram(initial){}
    void observe(){
        install();
        const auto initial=ram;
        for(unsigned route=0;route<3;++route){ram=initial;run(route);}
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: blue-striker-boost-fixture qualified.dol unmodified-recomp.so < qualified-role0");
    auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    std::array<u8,432> spec{};
    if(std::fread(spec.data(),1,spec.size(),stdin)!=spec.size()||std::getc(stdin)!=EOF||std::ferror(stdin))
        fail("Blue Striker boost spec extent");
    std::memcpy(retail_stationary_contact::at(original,record),spec.data(),spec.size());
    Replay(mod,original).observe();
}
