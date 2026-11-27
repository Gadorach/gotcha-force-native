// Replay one original menu update from declared processed-pad packets.
#include "retail_recomp_fixture.hpp"
#include <array>
#include <bit>

namespace {
using namespace retail_fixture;
constexpr u32 mode=0x81018000,scene=0x81019000,menu=0x81022000,pads=0x803C727C;
constexpr u32 update=0x80190038,audio=0x800F0468,transition=0x800437B0;
constexpr unsigned no_port=255;

struct Input {u32 word=0;u16 previous_held=0;u8 status=1;};
struct Case {
    const char* id;
    u8 menu_mode,current,mask;
    std::array<Input,4> inputs;
};

constexpr std::array<Case,13> cases{{
    {"first-match",2,0,1,{{{0x100},{0x100},{0x100},{}}}},
    {"held-ignored",2,0,1,{{{},{0x100,0x100},{0x100},{}}}},
    {"status-zero",2,0,1,{{{},{0x100,0,0},{0x100},{}}}},
    {"status-nonzero",2,0,1,{{{},{0x100,0,255},{0x100},{}}}},
    {"current-excluded",2,0,1,{{{0x100},{},{},{}}}},
    {"held-no-edge",2,0,1,{{{},{0x100,0x100},{},{}}}},
    {"alternate-bit",2,0,1,{{{},{},{0x1000},{}}}},
    {"edge-priority",2,0,1,{{{},{0x1000},{0x100},{}}}},
    {"upper-bit-priority",2,0,1,{{{},{0x100},{0x200},{0x1000}}}},
    {"single-port",1,0,1,{{{0x100},{0x100},{},{}}}},
    {"single-port-alternate",1,0,1,{{{0x1000},{0x100},{},{}}}},
    {"nonzero-current",2,2,4,{{{0x100},{0x100},{0x100},{}}}},
    {"already-enrolled",2,2,5,{{{0x100},{0x100},{0x100},{}}}},
}};

void replay(const Module& mod,const std::vector<u8>& original,const Case& c){
    auto ram=original;
    const auto ptr=[&](u32 va){return ram.data()+va-GC_RAM_BASE;};
    const auto half=[&](u32 va){const auto* b=ptr(va);return unsigned(b[0])*256+b[1];};
    const auto slot_state=[](unsigned i){return menu+256+128*i;};
    put32(ptr(0x8043B5A0-31856),mode);
    put32(ptr(0x8043B5A0-31836),menu);
    auto* m=ptr(mode);
    auto* cfg=ptr(menu);
    m[2]=c.mask;m[3]=u8(std::popcount(unsigned(c.mask)));m[50]=2;
    cfg[23]=c.menu_mode;cfg[20]=c.current;cfg[25]=cfg[110]=no_port;
    put16(cfg+4,4);put16(cfg+6,8);
    // Sentinel Scene state, not installed in guest globals: no Scene work
    // is permitted to be part of this menu-only execution.
    auto* sc=ptr(scene);
    sc[5593]=5;sc[5594]=2;
    for(unsigned i=0;i<c.inputs.size();++i){
        const auto& input=c.inputs[i];
        put32(ptr(pads+32*i),input.word);
        ptr(pads+32*i)[21]=input.status;
        put16(ptr(slot_state(i)+8),input.previous_held);
        // The current port word is explicitly cleared in the multi-port
        // branch. This sentinel distinguishes clearing from a skipped read.
        put16(ptr(slot_state(i)+14),0x40);
        sc[62+i]=u8(i+3);
    }
    CPUState cpu{};
    const auto reached=mod.run_to_or_return(cpu,ram,update,{audio,transition});
    unsigned cut=0;
    if(reached==audio){
        if(cpu.gpr[3]!=0||cpu.gpr[5]!=0)fail("unexpected menu audio arguments");
        if(cpu.lr==0x80190188&&cpu.gpr[4]==123)cut=1;
        else if(cpu.lr==0x801901F4&&cpu.gpr[4]==122)cut=2;
        else fail("unexpected menu audio caller");
    }else if(reached==transition){
        if(cpu.lr!=0x8019025C||cpu.gpr[3]!=0)fail("unexpected menu transition caller");
        cut=3;
    }
    if(cfg[23]!=c.menu_mode||m[50]!=2||sc[5593]!=5||sc[5594]!=2)
        fail("menu update changed fixture mode or scene");
    unsigned banks=0;
    for(unsigned i=0;i<c.inputs.size();++i){
        if(sc[62+i]!=i+3)fail("menu update changed fixture rank");
        auto* a=ptr(actor);
        a[0x3E4]=u8(i);a[0x3E5]=u8(1u<<i);a[0x88]=u8(i&1u);a[0x3E6]=255;
        CPUState bank{};bank.gpr[31]=actor;
        mod.run_to(bank,ram,0x8005438C,{0x80055F90});
        const unsigned expected=(m[2]&(1u<<i))==0;
        if(bank.lr!=0x80054418||a[0x3E6]!=expected||a[0x488]!=expected||a[0x3F3]||a[0x88]!=(i&1u))
            fail("menu-produced input mask disagrees with the ordinary bank selector");
        banks|=unsigned(a[0x3E6])<<i;
    }
    std::printf("%s: %u %u %u %u %u %u %u %u %u %u %u %u %u\n",c.id,
                half(slot_state(0)+14),half(slot_state(1)+14),
                half(slot_state(2)+14),half(slot_state(3)+14),
                cut,cfg[25],cfg[110],cfg[20],m[2],m[3],banks,
                half(menu+4),half(menu+6));
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: menu-port-input qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(const auto& c:cases)replay(mod,original,c);
}
