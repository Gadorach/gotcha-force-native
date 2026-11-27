// Original indexed type-89 phase-two requests and factory admission.
#include "retail_recomp_fixture.hpp"
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 update=0x800B8D6C,factory=0x8016CC24;
constexpr u32 objects=0x81030000,object_stride=0x400,descriptors=0x81034000,desc_stride=0x80;
constexpr u32 descriptor_row=0x81035000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr unsigned max_cells=8;
struct Capacity {unsigned objects,descriptors;};
constexpr Capacity capacities[]={{8,8},{1,8},{8,1},{0,8},{8,0},{0,0}};
constexpr unsigned refill_case=std::size(capacities)-1;
constexpr u16 pilots[]={0x0503,0x0506,0x050F,0x0512};
constexpr unsigned singles[]={3,6,4,7,5,8};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 address){
        if(address<GC_RAM_BASE||address-GC_RAM_BASE>ram.size()-4)fail("type-89 cast scratch extent");
        return ram.data()+address-GC_RAM_BASE;
    }
    u32 word(u32 address){return be32(at(address));}
    u16 half(u32 address){const auto* p=at(address);return u16(u16(p[0])<<8|p[1]);}
    void pool(Capacity cap){
        if(cap.objects>max_cells||cap.descriptors>max_cells)fail("type-89 cast pool extent");
        put32(at(free_head+12),cap.objects?objects:0);
        put32(at(free_tail+12),cap.objects?objects+(cap.objects-1)*object_stride:0);
        put16(at(free_count+6),u16(cap.objects));
        for(unsigned i=0;i<cap.objects;++i){
            const auto cell=objects+i*object_stride;
            at(cell)[2]=3;
            put32(at(cell+4),i+1<cap.objects?cell+object_stride:0);
        }
        put32(at(0x80436380),descriptor_row);
        put32(at(descriptor_row+12),cap.descriptors?descriptors:0);
        put16(at(descriptor_row+18),u16(cap.descriptors));
        for(unsigned i=0;i<cap.descriptors;++i)
            put32(at(descriptors+i*desc_stride),i+1<cap.descriptors?descriptors+(i+1)*desc_stride:0);
    }
public:
    Check(const Module& m,const std::vector<u8>& input):mod(m),ram(input){}
    void observe(u16 model,unsigned scenario,Capacity cap){
        pool(cap);
        auto* owner=at(actor);
        owner[0]=1;owner[0x18]=1;owner[0x540]=2;
        // No actor animation cue or floor snapping is needed by the supplied phase.
        owner[0x1DB]=1;owner[0x7C9]=1;
        put16(owner+0x3E8,model);put16(owner+0x94,model);
        put32(owner+0x90,actor);
        for(unsigned offset:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+offset,1.f);
        put16(owner+0x774,5);
        put_float(owner+0x558,2.5f);
        const bool pair_bank=model==0x0503||model==0x050F;
        const unsigned steps=6+unsigned(pair_bank);
        unsigned cursor=0,allocated=0;
        float clock=2.5f;
        std::array<unsigned,max_cells> keys{};
        for(unsigned visit=1;visit<=81;++visit){
            if(scenario==refill_case&&visit==5){
                // Both pools gain one cell between the first timed step and the next update.
                cap={1,1};
                pool(cap);
            }
            CPUState cpu{};cpu.gpr[3]=actor;
            CallObservation requests{factory};
            const auto before=allocated;
            const bool end=visit==4+11*steps;
            if(end){
                // Stop at the original actor-return request after the negative pair.
                mod.run_to(cpu,ram,update,{0x8006A474},&requests);
                if(cpu.lr!=0x800B8EE8||cpu.gpr[3]!=actor||owner[0x1CEF])
                    fail("type-89 cast sentinel did not reach the scoped return request");
            }else{
                mod.run_observed(cpu,ram,update,requests,1024);
            }
            if(clock>0){
                clock-=1.f;
                if(requests.count)fail("type-89 cast escaped positive timer");
            }else{
                clock=10.f;
                const bool sentinel=cursor==steps,pair=pair_bank&&cursor==0;
                const unsigned count=sentinel?0:pair?2:1;
                if(requests.count!=count)fail("type-89 cast pair length or sentinel mismatch");
                for(unsigned k=0;k<count;++k){
                    const unsigned key=pair?9+k:singles[cursor-unsigned(pair_bank)];
                    if(requests.first_arguments[k]!=actor||requests.second_arguments[k]!=key||
                       requests.caller!=0x800B8E84)
                        fail("type-89 cast order or input mismatch");
                    if(allocated<cap.objects&&allocated<cap.descriptors)keys[allocated++]=key;
                }
                std::printf("type89-cast: %u %u %u %u %u %u %u %u\n",model,scenario,visit,cursor,count,
                            count?requests.second_arguments[0]:0,count==2?requests.second_arguments[1]:0,allocated-before);
                if(!sentinel)++cursor;
            }
            if(owner[0x6EB]!=cursor||word(actor+0x558)!=std::bit_cast<u32>(clock)||half(actor+0x774)!=5||
               half(free_count+6)!=cap.objects-allocated||half(descriptor_row+18)!=cap.descriptors-allocated||
               word(active_head+9*4)!=(allocated?objects+(allocated-1)*object_stride:0)||word(active_head+49*4))
                fail("type-89 cast cursor, timer, stock or pool mismatch");
            for(unsigned i=0;i<allocated;++i){
                const auto cell=objects+i*object_stride;
                const auto* child=at(cell);
                if(child[0x10]!=89||child[0x11]!=keys[i]||child[0x18]||child[0x17D]||
                   word(cell+0xC)!=0x8016CD1C||word(cell+0x8C)!=actor||word(cell+0x90)!=actor||
                   word(cell+0xE0)!=descriptors+i*desc_stride)
                    fail("type-89 admitted carrier identity, ownership or order mismatch");
            }
            if(end){
                if(cursor!=steps||requests.count)fail("type-89 cast did not stop at sentinel");
                return;
            }
        }
        fail("type-89 cast visit budget");
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type89-cast-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(const auto model:pilots)
        for(unsigned i=0;i<std::size(capacities);++i){
            Check test(mod,original);
            test.observe(model,i,capacities[i]);
        }
}
