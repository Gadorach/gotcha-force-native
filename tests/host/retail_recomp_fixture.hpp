// Private-input CPU-only harness for named original GG4E08 entries.
#pragma once

#include "StaticRecompABI.h"

#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <vector>

namespace retail_fixture {
inline constexpr u32 actor=0x81010000, stack=0x81100000, stop=0x81810000;

inline u32 be32(const u8* p){return u32(p[0])<<24|u32(p[1])<<16|u32(p[2])<<8|p[3];}
inline void put16(u8* p,u16 v){p[0]=u8(v>>8);p[1]=u8(v);}
inline void put32(u8* p,u32 v){p[0]=u8(v>>24);p[1]=u8(v>>16);p[2]=u8(v>>8);p[3]=u8(v);}
inline void put_float(u8* p,float v){put32(p,std::bit_cast<u32>(v));}

[[noreturn]] inline void fail(const char* reason){std::fprintf(stderr,"%s\n",reason);std::exit(2);}
inline u64 read_external(CPUState*,u32,u8){fail("unexpected external read");}
inline void write_external(CPUState*,u32,u64,u8){fail("unexpected external write");}
inline void fallback(CPUState*,u32,u32){fail("unexpected instruction fallback");}

inline std::vector<u8> load_ram(const char* path){
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

class Module {
    void* library=nullptr;
    const StaticRecompModuleDesc* mod=nullptr;
public:
    explicit Module(const char* path){
        library=dlopen(path,RTLD_NOW|RTLD_LOCAL);
        if(!library)fail(dlerror());
        const auto get=reinterpret_cast<StaticRecompGetModuleFn>(dlsym(library,STATICRECOMP_GET_MODULE_SYMBOL));
        mod=get?get():nullptr;
        if(!mod||mod->abi_version!=STATICRECOMP_ABI_VERSION||mod->cpu_abi_version!=GXRUNTIME_CPU_ABI_VERSION||
           mod->cpu_state_size!=sizeof(CPUState)||!mod->dispatch)fail("recomp module ABI");
    }
    Module(const Module&)=delete;
    Module& operator=(const Module&)=delete;
    ~Module(){dlclose(library);}
    void run(CPUState& cpu,std::vector<u8>& ram,u32 entry)const{
        cpu.pc=entry;cpu.lr=stop;cpu.ram=ram.data();cpu.ram_size=GC_MAIN_RAM_SIZE;
        cpu.gpr[1]=stack;cpu.gpr[2]=0x8043EA20;cpu.gpr[13]=0x8043B5A0;
        cpu.msr=PPC_MSR_FP;cpu.hid2=PPC_HID2_PSE|PPC_HID2_LSQE;
        cpu.external_read=read_external;cpu.external_write=write_external;cpu.instruction_fallback=fallback;
        if(mod->on_state_loaded)mod->on_state_loaded(&cpu);
        for(unsigned n=0;n<64&&cpu.pc!=stop;++n){
            cpu.downcount=256;
            if(!mod->dispatch(&cpu,cpu.pc)||cpu.exception){
                std::fprintf(stderr,"entry=%08X pc=%08X exception=%08X\n",entry,cpu.pc,cpu.exception);
                fail("fixture dispatch or exception");
            }
        }
        if(cpu.pc!=stop||cpu.gpr[1]!=stack)fail("fixture did not return to sentinel");
    }
};
}
