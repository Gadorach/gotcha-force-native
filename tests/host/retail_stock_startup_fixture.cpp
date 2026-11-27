// Real normal installer, size initializer and level-stock writes up to the resource frontier.
#include "retail_recomp_fixture.hpp"
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 role0=0x81014000,scene=0x81018000,resources=0x803C4D94;
constexpr unsigned records=8,record_bytes=432,roster=206,levels=10;
constexpr u32 start=0x8005614C,equipment=0x8013BD80,material=0x80006238;

u16 half(const u8* p){return u16(u16(p[0])<<8|p[1]);}

void inspect(const Module& mod,std::vector<u8>& ram,unsigned model,u32 installer,unsigned extent){
    const auto at=[&](u32 a){return ram.data()+a-GC_RAM_BASE;};
    auto* obj=at(actor);
    put32(at(resources),role0);
    put32(at(0x8043B5A0-31852),scene);
    put16(obj+0x3E8,u16(model)); // exact normal model, standalone/variant/resource indices are zero
    CPUState install{};install.gpr[3]=actor;
    mod.run(install,ram,installer);
    const auto selected=be32(obj+0x4AC);
    if(selected<role0||selected>=role0+extent||((selected-role0)%record_bytes))
        fail("normal stock startup installer selected an unprovided actor-data row");
    std::array<u8,0x1E00> pristine{};
    std::memcpy(pristine.data(),obj,pristine.size());
    for(unsigned level=0;level<levels;++level){
        // Before these cuts, the initialization bodies write actor fields
        // only. Restore the original installed actor before each level.
        std::memcpy(obj,pristine.data(),pristine.size());
        obj[0x3EE]=u8(level);
        for(unsigned i=0;i<8;++i){obj[0x77C+i]=u8(0xA0+i);obj[0x794+i]=u8(0xB0+i);}
        CPUState cpu{};cpu.gpr[3]=actor;
        const auto cut=mod.run_to(cpu,ram,start,{equipment,material});
        if((cut==equipment?cpu.lr!=0x80056604:cpu.lr!=0x80056824)||be32(obj+0x4AC)!=selected||
           half(obj+0x1C6)||obj[0x3EE]!=level)
            fail("normal stock startup crossed the declared resource or actor boundary");
        for(unsigned i=0;i<8;++i)
            if(obj[0x77C+i]!=u8(0xA0+i)||obj[0x794+i]!=u8(0xB0+i))
                fail("normal level stock overwrote stock channel 1");
        std::printf("stock-start: %04X %u %u %u %04X %04X %08X %08X %08X %08X",
                    model,level+1,(selected-role0)/record_bytes,unsigned(cut==equipment),
                    half(obj+0x59C),half(obj+0x59E),be32(obj+0x74A),be32(obj+0xB4),be32(obj+0xB8),be32(obj+0x1DD0));
        for(unsigned i:{0u,2u}){
            const auto* value=obj+0x774+8*i;
            const auto* def=obj+0x78C+8*i;
            std::printf(" %u %u %u %u %u %u %08X",half(def),half(def+2),half(def+4),half(def+6),
                        half(value),half(value+2),be32(value+4));
        }
        std::putchar('\n');
    }
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: stock-startup-fixture qualified.dol unmodified-recomp.so < qualified-plan");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    int previous=-1;
    for(unsigned i=0;i<roster;++i){
        std::array<u8,8> header{};
        if(std::fread(header.data(),1,header.size(),stdin)!=header.size())fail("stock-startup header extent");
        const auto model=half(header.data());
        const auto entry=be32(header.data()+2);
        const auto count=header[6];
        if(int(model)<=previous||model>=0x8000||!count||count>records||header[7]||
           entry<0x80003100||entry>=0x802B0160||(entry&3))fail("stock-startup header");
        previous=model;
        auto ram=original;
        const unsigned bytes=count*record_bytes;
        if(std::fread(ram.data()+role0-GC_RAM_BASE,1,bytes,stdin)!=bytes)fail("stock-startup records extent");
        inspect(mod,ram,model,entry,bytes);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra stock-startup rows");
}
