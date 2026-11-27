// Execute original class installers on isolated slot-0 actor inputs.
#include "retail_recomp_fixture.hpp"

namespace {
using namespace retail_fixture;
constexpr unsigned roster_size=206;
constexpr u32 role0=0x81014000, resources=0x803C4D94;
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: installer-fixture qualified.dol unmodified-recomp.so < plan");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    char row[32]{};
    int previous=-1;
    for(unsigned i=0;i<roster_size;++i){
        unsigned model=0, entry=0;int read=0;
        if(!std::fgets(row,sizeof(row),stdin)||std::sscanf(row,"%4x %8x%n",&model,&entry,&read)!=2||
           read!=13||row[read]!='\n'||row[read+1]!='\0'||model>=0x8000||int(model)<=previous||
           entry<0x80003100||entry>=0x802B0160||(entry&3))fail("invalid installer plan row");
        previous=int(model);
        auto ram=original;
        auto* obj=ram.data()+actor-GC_RAM_BASE;
        put16(obj+0x3E8,u16(model));
        put32(ram.data()+resources-GC_RAM_BASE,role0);
        CPUState cpu{};cpu.gpr[3]=actor;
        mod.run(cpu,ram,entry);
        const auto pointer=be32(obj+0x4AC);
        if(pointer<role0||pointer>=role0+8*432)fail("installer resource pointer outside role 0");
        std::printf("%04X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    model,entry,be32(obj+0x4B0),be32(obj+0x4EC),be32(obj+0x4F0),be32(obj+0x4B4),
                    be32(obj+0x4BC),be32(obj+0x4C0),pointer-role0,be32(obj+0x1D80),
                    be32(obj+0x1D84),be32(obj+0x1D88),be32(obj+0x1D8C));
    }
    if(std::fgetc(stdin)!=EOF)fail("extra installer plan rows");
}
