// Run one original GG4E08 helper against synthetic actor data in a local
// unmodified recomp module. No original code or fixtures are embedded.
#include "StaticRecompABI.h"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <vector>

namespace {
constexpr u32 entry=0x80066838, actor=0x81010000, stack=0x81100000, stop=0x81810000;

u32 be32(const u8* p){return u32(p[0])<<24|u32(p[1])<<16|u32(p[2])<<8|p[3];}
void put32(u8* p,u32 v){p[0]=u8(v>>24);p[1]=u8(v>>16);p[2]=u8(v>>8);p[3]=u8(v);}
void put_float(u8* p,float v){put32(p,std::bit_cast<u32>(v));}

[[noreturn]] void fail(const char* reason){std::fprintf(stderr,"%s\n",reason);std::exit(2);}
u64 read_external(CPUState*,u32,u8){fail("unexpected external read");}
void write_external(CPUState*,u32,u64,u8){fail("unexpected external write");}
void fallback(CPUState*,u32,u32){fail("unexpected instruction fallback");}

std::vector<u8> load_ram(const char* path){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    const auto size=file.tellg();
    if(size<256||size>(8<<20))fail("DOL budget");
    std::vector<u8> dol(static_cast<size_t>(size)),ram(GC_MAIN_RAM_SIZE);
    file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(dol.data()),size))fail("short DOL read");
    for(unsigned i=0;i<18;++i){
        const u32 offset=be32(dol.data()+4*i),addr=be32(dol.data()+0x48+4*i),n=be32(dol.data()+0x90+4*i);
        if(!n)continue;
        if(addr<GC_RAM_BASE||addr>=actor||n>actor-addr||
           offset<256||offset>dol.size()||n>dol.size()-offset)fail("DOL section extent");
        std::memcpy(ram.data()+addr-GC_RAM_BASE,dol.data()+offset,n);
    }
    return ram;
}

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
    void* library=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);
    if(!library)fail(dlerror());
    const auto get=reinterpret_cast<StaticRecompGetModuleFn>(dlsym(library,STATICRECOMP_GET_MODULE_SYMBOL));
    const auto* mod=get?get():nullptr;
    if(!mod||mod->abi_version!=STATICRECOMP_ABI_VERSION||mod->cpu_abi_version!=GXRUNTIME_CPU_ABI_VERSION||
       mod->cpu_state_size!=sizeof(CPUState)||!mod->dispatch)fail("recomp module ABI");
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
        cpu.pc=entry;cpu.lr=stop;cpu.ram=ram.data();cpu.ram_size=GC_MAIN_RAM_SIZE;
        cpu.gpr[1]=stack;cpu.gpr[2]=0x8043EA20;cpu.gpr[3]=actor;cpu.fpr[1]=test.limit;
        cpu.msr=PPC_MSR_FP;cpu.hid2=PPC_HID2_PSE|PPC_HID2_LSQE;
        cpu.external_read=read_external;cpu.external_write=write_external;cpu.instruction_fallback=fallback;
        if(mod->on_state_loaded)mod->on_state_loaded(&cpu);
        for(unsigned n=0;n<16&&cpu.pc!=stop;++n){
            cpu.downcount=256;
            if(!mod->dispatch(&cpu,cpu.pc)||cpu.exception)fail("phase gate dispatch or exception");
        }
        if(cpu.pc!=stop||cpu.gpr[1]!=stack)fail("phase gate did not return to sentinel");
        const auto result=std::bit_cast<int32_t>(cpu.gpr[3]);
        if(result!=test.result){
            std::fprintf(stderr,"%s: got %d, expected %d\n",test.name,result,test.result);
            return 1;
        }
    }
    dlclose(library);
    std::printf("phase gate: %zu synthetic actor cases passed\n",sizeof(cases)/sizeof(cases[0]));
}
