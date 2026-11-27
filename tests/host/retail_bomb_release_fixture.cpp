// Bounded original bomb release leaves, with declared stock, state and free lists.
#include "retail_recomp_fixture.hpp"
#include <algorithm>
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 spec=0x81014000,scene=0x81017000,mode=0x81019000;
constexpr u32 descriptor_row=0x81030000,descriptors=0x81030100,cells=0x81032000;
constexpr u32 descriptor_stride=0x80,cell_stride=0x800;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC;
constexpr u32 active_head=0x803C706C,active_tail=0x803C6F6C;
constexpr u32 factory=0x8007DB5C, powered=0x8016711C, ninja=0x800715F8, alpha=0x801A0458;
constexpr u32 audio=0x800F036C, normal_x_helper=0x8016D584, movement_tail=0x800B22F4;

struct Case {
    u16 model;
    u8 state, extra, stock, carriers, descriptors;
};
constexpr std::array cases{
    Case{0x0101,3,0,1,4,4}, Case{0x0101,3,0,0,4,4},
    Case{0x0101,3,0,1,2,4}, Case{0x0101,3,0,1,4,2},
    Case{0x0101,3,0,1,0,4}, Case{0x0101,3,0,1,4,0},
    Case{0x0101,2,0,1,4,4}, Case{0x0101,2,0,0,4,4},
    Case{0x0000,2,0,1,1,1}, Case{0x0000,2,0,0,1,1},
    Case{0x0003,2,0,1,2,2}, Case{0x0003,2,1,1,2,1}, Case{0x0003,2,1,1,2,2},
    Case{0x000A,2,0,1,0,1}, Case{0x000A,2,0,2,1,1},
    Case{0x000C,2,0,2,1,1},
};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){
        if(va<GC_RAM_BASE||va-GC_RAM_BASE>ram.size()-4)fail("bomb fixture address extent");
        return ram.data()+va-GC_RAM_BASE;
    }
    u16 half(u32 va){const auto* b=at(va);return u16(u16(b[0])<<8|b[1]);}
    float fp(u32 va){return std::bit_cast<float>(be32(at(va)));}
    static u32 cell(unsigned i){return cells+i*cell_stride;}
    static u32 descriptor(unsigned i){return descriptors+i*descriptor_stride;}
    void free_pools(unsigned objects,unsigned capacity){
        if(objects>4||capacity>4)fail("bomb pool capacity extent");
        put32(at(0x80436380),descriptor_row);
        put16(at(descriptor_row+18),u16(capacity));
        put32(at(descriptor_row+12),capacity?descriptor(0):0);
        for(unsigned i=0;i<capacity;++i)
            put32(at(descriptor(i)),i+1<capacity?descriptor(i+1):0);
        put16(at(free_count+6),u16(objects));
        put32(at(free_head+12),objects?cell(0):0);
        put32(at(free_tail+12),objects?cell(objects-1):0);
        for(unsigned i=0;i<objects;++i){
            at(cell(i))[2]=3;
            put32(at(cell(i)+4),i+1<objects?cell(i+1):0);
        }
    }
    void owner(const Case& c){
        auto* a=at(actor);
        a[0]=1;a[0x18]=1;
        put16(a+0x3E8,c.model);put16(a+0x94,c.model);
        a[0x580]=c.state;a[0x7C8]=c.extra;
        put32(a+0x90,actor);put32(a+0x4AC,spec);
        put16(a+0x784,c.stock);put16(a+0x786,c.stock);
        put16(a+0x79E,c.model==0x0101?1:5);put16(a+0x7A2,c.model==0x0101?120:300);
        put_float(a+0xB4,1.f);put_float(a+0xB8,1.f);put_float(a+0xC0,1.f);put_float(a+0xC4,1.f);
        // Keep the Powered Gunman nonrelease path at the movement-call
        // frontier too; no synthetic result or guest epilogue is supplied.
        put_float(a+0x38,300.f);
        put32(at(0x8043B5A0-31852),scene);put32(at(0x8043B5A0-31856),mode);
        // Three distinct rigid sockets. The launcher uses the original
        // matrix math on these inlets; no scene or pose producer runs.
        for(const unsigned joint:{7u,11u}){
            auto* m=a+0x8D4+joint*48;
            for(unsigned axis=0;axis<3;++axis)put_float(m+20*axis,1.f);
            put_float(m+12,float(10*joint));put_float(m+28,200.f);put_float(m+44,300.f);
        }
        put_float(a+0x7B0,23.f); // distinct declared add-parent-offset operand for key 2
    }
    void check_pools(const Case& c,const std::array<u8,4>& keys,unsigned made){
        unsigned seen=0;
        u32 prior=0;
        for(u32 obj=be32(at(active_head+36));obj;obj=be32(at(obj+8))){
            if(seen>=made)fail("bomb active-list length");
            const unsigned index=made-1-seen; // Original factory prepends each accepted cell.
            if(obj!=cell(index)||be32(at(obj+4))!=prior||
               !at(obj)[0]||at(obj)[1]!=9||at(obj)[2]!=3||at(obj)[0x10]!=15||
               at(obj)[0x11]!=keys[index]||at(obj)[0x83]!=0x10||
               be32(at(obj+0x8C))!=actor||be32(at(obj+0x90))!=actor||
               be32(at(obj+0xE0))!=descriptor(index)){
                std::fprintf(stderr,"bomb list model=%04X index=%u obj=%08X prev=%08X next=%08X "
                             "on=%u bucket=%u kind=%u type=%u key=%u tag=%u owner=%08X credit=%08X descriptor=%08X\n",
                             c.model,index,obj,be32(at(obj+4)),be32(at(obj+8)),at(obj)[0],at(obj)[1],at(obj)[2],
                             at(obj)[0x10],at(obj)[0x11],at(obj)[0x83],be32(at(obj+0x8C)),be32(at(obj+0x90)),
                             be32(at(obj+0xE0)));
                fail("bomb active-list order, header or descriptor association");
            }
            prior=obj;
            ++seen;
        }
        if(seen!=made||be32(at(active_tail+36))!=prior||half(free_count+6)!=c.carriers-made||
           half(descriptor_row+18)!=c.descriptors-made||
           be32(at(free_head+12))!=(c.carriers>made?cell(made):0)||
           be32(at(descriptor_row+12))!=(c.descriptors>made?descriptor(made):0))
            fail("bomb allocator did not preserve the unused pools");
    }
public:
    Check(const Module& module,const std::vector<u8>& original):mod(module),ram(original){}
    void run_case(const Case& c){
        owner(c);free_pools(c.carriers,c.descriptors);
        const bool gun=c.model==0x0101,charged=gun&&c.state==3;
        const bool ordinary_paid=!charged&&c.stock;
        std::array<u8,4> keys{};
        unsigned count=0;
        if(charged){keys={8,9,6,7};count=4;}
        else if(ordinary_paid&&!gun){
            keys[0]=c.model==3?1:c.model==0xA?3:c.model==0xC?5:0;
            count=1;
            if(c.model==3&&c.extra){keys[1]=2;count=2;}
        }
        // The Gunman slice starts after the separate cue and movement
        // owners. Observe the actual state split and commit below it.
        CPUState cpu{};cpu.gpr[3]=actor;cpu.gpr[31]=actor;
        CallObservation observation{factory};
        const auto boundary=mod.run_to_or_return(cpu,ram,gun?powered:c.model==0xC?alpha:ninja,
                                                  {audio,normal_x_helper,movement_tail},&observation);
        const u32 expected_boundary=charged?audio:gun&&ordinary_paid?normal_x_helper:gun?movement_tail:0;
        if(boundary!=expected_boundary||observation.count!=count||
           (boundary==audio&&(cpu.lr!=0x801671C4||cpu.gpr[3]!=actor||cpu.gpr[4]!=6))||
           (boundary==normal_x_helper&&(cpu.lr!=0x80167154||cpu.gpr[3]!=actor||cpu.gpr[4]!=0))||
           (boundary==movement_tail&&(cpu.lr!=0x801671E0||cpu.gpr[3]!=actor))){
            std::fprintf(stderr,"bomb model=%04X state=%u boundary=%08X lr=%08X calls=%u\n",
                         c.model,c.state,boundary,cpu.lr,observation.count);
            fail("bomb release left its reviewed continuation");
        }
        unsigned attempted=0,published=0;
        const auto made=std::min({count,unsigned(c.carriers),unsigned(c.descriptors)});
        for(unsigned i=0;i<count;++i){
            if(observation.second_arguments[i]!=keys[i])fail("bomb requested the wrong factory key");
            attempted=10*attempted+keys[i];
            if(i>=made)continue;
            published=10*published+keys[i];
            const bool lower=keys[i]==8||keys[i]==9,left=keys[i]==8||keys[i]==6;
            const float x=gun?(left?70.f-118.f:110.f+118.f):165.f+(keys[i]==2?23.f:0.f);
            const float y=gun?(lower?178.f:236.f):200.f,z=gun?330.f:300.f;
            if(fp(cell(i)+0x20)!=x||fp(cell(i)+0x24)!=y||fp(cell(i)+0x28)!=z)
                fail("bomb did not sample its exact constructor socket");
        }
        check_pools(c,keys,made);
        const auto remaining=half(actor+0x784);
        const auto clock=fp(actor+0x788);
        if(remaining!=(ordinary_paid?c.stock-1:c.stock)||
           clock!=(ordinary_paid&&!remaining?(gun?120.f:300.f):0.f))
            fail("bomb stock debit or refill-clock mismatch");
        std::printf("bomb: %u %u %u %u %u %u %u %u %u %u %u %.0f %u\n",
                    c.model,c.state,c.extra,c.stock,c.carriers,c.descriptors,count,attempted,
                    made,published,remaining,double(clock),boundary==audio?1:boundary==normal_x_helper?2:boundary==movement_tail?3:0);
    }
    void wrong_parent(){
        // A reviewed charged key, offered to a different normal actor.
        const Case c{0x000A,2,0,1,1,1};
        owner(c);free_pools(1,1);
        CPUState cpu{};cpu.gpr[3]=actor;cpu.gpr[4]=8;
        mod.run(cpu,ram,factory);
        check_pools(c,{},0);
        if(half(actor+0x784)!=1)fail("wrong-parent bomb changed stock");
        std::puts("bomb-wrong-parent: 10 8 0 1 1");
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: bomb-release-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module module(argv[2],original);
    for(const auto& c:cases){Check test(module,original);test.run_case(c);}
    Check wrong(module,original);wrong.wrong_parent();
}
