// Observe the selected tone left in r3 by the original disabled mixer path.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 choose_sound=0x8002FC18,reset_gate=0x8002CDA4;
constexpr u32 counters=0x803D3AE8,audio=0x8103E000,point=0x8103D000;
constexpr u32 first=contact::receiver,second=0x81020000,separate=0x81024000;
constexpr u32 alias_a=0x81028000,alias_b=0x8102C000,quiet=0x81034000;
constexpr unsigned sound_gate=0x2AE;
constexpr float upper(float value){return std::bit_cast<float>(std::bit_cast<u32>(value)+1);}
constexpr float sizes[]={0.f,1.5f,upper(1.5f),2.5f,upper(2.5f),3.5f,upper(3.5f)};
constexpr unsigned bands[]={0,0,1,1,2,2,3};
constexpr unsigned codes[]={0,0x80,0x84,0x88,0xC0,0xC8};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 address){return contact::at(ram,address);}
public:
    Check(const Module& module,const std::vector<u8>& original):mod(module),ram(original){
        put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
        at(counters+852)[0]=1;
        for(unsigned i=0;i<3;++i)put_float(at(point+4*i),std::array{120.f,160.f,280.f}[i]);
    }

    void place(u32 target,unsigned partition,float size){
        at(target)[0]=1;at(target+0x96)[0]=u8(partition);
        put_float(at(target+0x7FC),size);
    }

    unsigned call(u32 target,unsigned code,unsigned expected){
        CPUState cpu{};cpu.gpr[3]=actor;cpu.gpr[4]=target;cpu.gpr[5]=point;cpu.gpr[6]=code;
        const auto played=be32(at(counters+848));
        mod.run(cpu,ram,choose_sound,1024);
        // The disabled branch in 800EFB3C returns without changing r3
        // or entering the backend. Held and silent calls retain the input.
        if(cpu.gpr[3]!=(expected?expected:actor)||be32(at(counters+848))!=played){
            std::fprintf(stderr,"impact sound code=%u expected=%u r3=%u gate=%u\n",
                         code,expected,cpu.gpr[3],at(target+sound_gate)[0]);
            fail("disabled impact sound path did not retain the selected tone");
        }
        return expected?cpu.gpr[3]:0;
    }

    void band(unsigned code,unsigned sample){
        const auto lane=(code&0x3C)+((code&0x80)?bands[sample]:0);
        const bool special=code&0x40;
        const unsigned slot=lane+(special?12:0),expected=48+2*lane+(special?24:0);
        place(first,0,sizes[sample]);
        const auto tone=call(first,code,expected);
        if(at(first+sound_gate)[0]!=4||at(first+0x2AF)[0])fail("impact sound changed the wrong gate");
        for(unsigned i=0;i<26;++i)
            if(at(counters+i)[0]!=unsigned(i==slot))fail("impact sound incremented the wrong slot");
        std::printf("impact-band: %u %u %u %u %u %u\n",code,sample,slot,tone,at(first+sound_gate)[0],at(counters+slot)[0]);
    }

    void sharing(){
        place(first,0,1.5f);place(second,0,1.5f);place(separate,1,1.5f);
        place(alias_a,0,upper(2.5f));place(alias_b,1,upper(1.5f));place(quiet,1,0.f);
        at(counters+4)[0]=255;
        const auto observe=[&](unsigned i,u32 target,unsigned code,unsigned slot,unsigned after_count,unsigned tone){
            const auto prior=slot==255?0u:unsigned(at(counters+slot)[0]);
            const auto before=unsigned(at(target+sound_gate)[0]);
            const auto next=call(target,code,tone);
            if(at(target+sound_gate)[0]!=(tone?4:before)||at(target+0x2AF)[0]||
               (slot!=255&&at(counters+slot)[0]!=after_count))
                fail("impact sound alias, wrap or held gate changed");
            std::printf("impact-counter: %u %u %u %u %u %u %u %u\n",i,code,slot,prior,after_count,before,
                        at(target+sound_gate)[0],next);
        };
        observe(0,first,0x84,4,0,57);       // 255 -> 0
        observe(1,second,0x84,4,1,56);      // same partition, separate gate
        observe(2,first,0x84,4,1,0);        // held gate did not increment
        observe(3,separate,0x84,17,1,56);   // independent partition
        for(unsigned i=0;i<4;++i){
            CPUState reset{};reset.gpr[3]=first;
            mod.run(reset,ram,reset_gate);
            if(at(first+sound_gate)[0]!=3-i)fail("impact sound did not reopen after four reset visits");
        }
        observe(4,first,0x84,4,2,57);
        observe(5,alias_a,0xC8,22,1,92);    // special slot 22 in partition zero
        observe(6,alias_b,0x88,22,2,67);    // ordinary slot 9 in the next partition
        const auto before=ram;
        observe(7,quiet,0xFF,255,0,0);     // silent code does not arm the gate
        if(std::memcmp(at(counters),before.data()+counters-GC_RAM_BASE,26))
            fail("silent impact sound touched a counter");
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: impact-sound-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(unsigned code:codes)
        for(unsigned sample=0;sample<std::size(sizes);++sample)
            Check(mod,original).band(code,sample);
    Check(mod,original).sharing();
}
