// Run one original GG4E08 helper against synthetic actor data in a local
// unmodified recomp module. No original code or fixtures are embedded.
#include "retail_recomp_fixture.hpp"
#include <cstdint>

namespace {
using namespace retail_fixture;
constexpr u32 entry=0x80066838;

struct Case {
    const char* name;
    u32 counter;
    u8 flags;
    float x,y,z,scale,limit;
    int32_t result;
};
constexpr Case cases[]{
    {"zero counter before forced flag",0,0x80,3,0,4,1,10,-1},
    {"zero counter without flag",0,0,0,0,0,1,0,-1},
    {"force while outside",1,0x80,3,0,4,1,1,1},
    {"neighbor flag does not force",1,0x40,3,0,4,1,1,0},
    {"outside horizontal threshold",1,0,3,0,4,1,4,0},
    {"inside horizontal threshold",1,0,3,500,4,1,6,1},
    {"actor scale doubles threshold",1,0,3,500,4,2,3,1},
    {"actor scale halves threshold",1,0,3,500,4,.5f,6,0},
    {"vertical separation alone is ignored",1,0,0,500,0,1,0,1},
    {"finite nonzero equality",1,0,3,0,4,1,5,1},
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: phase-gate-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2]);
    for(const auto& test:cases){
        auto ram=original;
        auto* obj=ram.data()+actor-GC_RAM_BASE;
        put32(obj+0xCC,test.counter);
        obj[0x1DD]=test.flags;
        put_float(obj+0xB4,test.scale);
        // Deliberately use nonzero base position so an origin-distance
        // calculation cannot accidentally pass the same trials.
        put_float(obj+0x20,10);put_float(obj+0x24,20);put_float(obj+0x28,30);
        put_float(obj+0x518,10+test.x);put_float(obj+0x51C,20+test.y);put_float(obj+0x520,30+test.z);
        CPUState cpu{};
        cpu.gpr[3]=actor;cpu.fpr[1]=test.limit;
        mod.run(cpu,ram,entry);
        const auto result=std::bit_cast<int32_t>(cpu.gpr[3]);
        if(result!=test.result){
            std::fprintf(stderr,"%s: got %d, expected %d\n",test.name,result,test.result);
            return 1;
        }
    }
    std::printf("phase gate: %zu synthetic actor cases passed\n",sizeof(cases)/sizeof(cases[0]));
}
