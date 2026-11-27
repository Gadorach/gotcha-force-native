// Original type-9 allocation and common shape admission, before presentation.
#include "retail_recomp_fixture.hpp"
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 parent=0x81028000,effect=0x81030000,shape=0x81032000,descriptor=0x81033000;
constexpr u32 descriptor_row=0x81033800,point=0x81034000,scene=0x81040000,common=0x81048000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 factory=0x800FC6A0,init=0x800FC810,alloc=0x80088AA0,bind=0x8008AE3C;
constexpr u32 presentation_frontier=0x800055FC,attack_count=0x8043B5A0-21328,amount_table=0x802C4760;
using CommonBank=std::array<u8,21504>;

void observe(const Module& mod,std::vector<u8> ram,const CommonBank& bank,unsigned key,bool slot){
    const auto at=[&](u32 address){return ram.data()+address-GC_RAM_BASE;};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return unsigned(at(address)[0])*256+at(address)[1];};
    at(actor)[0]=1;at(parent)[0]=1;
    put32(at(actor+0x90),actor);put32(at(parent+0x90),actor);
    put_float(at(parent+0xB4),1.f);put_float(at(parent+0xB8),1.f);
    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),descriptor);put16(at(descriptor_row+18),1);
    put32(at(free_head+12),effect);put32(at(free_tail+12),effect);put16(at(free_count+6),1);
    at(effect)[2]=3;
    for(unsigned i=0;i<3;++i)put_float(at(point+4*i),std::array<float,3>{160,0,400}[i]);
    put32(at(0x8043B5A0-31852),scene);put32(at(scene+152),common);
    std::memcpy(at(common),bank.data(),bank.size());
    if(slot){
        at(shape)[2]=4;
        put32(at(free_head+16),shape);put32(at(free_tail+16),shape);put16(at(free_count+8),1);
    }
    CPUState create{};create.gpr[3]=parent;create.gpr[4]=point;create.gpr[5]=key;
    CallObservation allocate{alloc};
    mod.run_observed(create,ram,factory,allocate);
    if(allocate.count!=1||allocate.caller!=0x800FC6D8||allocate.arguments[0]!=actor||
       allocate.arguments[1]!=3||allocate.arguments[2]!=8||allocate.arguments[3]!=1||allocate.arguments[4]!=1||
       create.gpr[3]!=effect||at(effect)[0x10]!=9||at(effect)[0x11]!=key||at(effect)[0x18]||
       word(effect+0x0C)!=0x800FC7C0||word(effect+0x8C)!=parent||word(effect+0x90)!=actor||
       std::memcmp(at(effect+0x20),at(point),12)||half(free_count+6)||half(descriptor_row+18)||
       word(active_head+49*4))
        fail("type-9 constructor did not allocate with separate parent and credit");
    CPUState opening{};opening.gpr[3]=effect;
    CallObservation group{bind};
    const auto frontier=mod.run_to_or_return(opening,ram,init,{presentation_frontier},&group);
    if(group.count!=1||group.caller!=0x800FC870||group.arguments[0]!=actor||group.arguments[1]!=effect||
       group.arguments[2]!=84||frontier!=(slot?presentation_frontier:0)||
       at(effect)[0x18]!=(slot?1:2)||half(free_count+8)||half(attack_count)||
       word(active_head+49*4)!=(slot?shape:0))
        fail("type-9 initializer did not separate common shape admission from refusal");
    if(slot&&(opening.lr!=0x800FC8B8||word(shape+0x20)!=effect||word(shape+0x24)!=actor||
              word(shape+0x28)!=common+0x400+122*80||word(shape+0x2C)!=amount_table||
              at(shape)[0x11]!=9||half(shape+0x48)||half(effect+0x1E)!=30))
        fail("type-9 shape did not retain the common descriptor, amount index or owner");
    std::printf("type9-bind: %u %u %u %u %u %u %u\n",key,unsigned(slot),group.arguments[2],
                unsigned(frontier!=0),unsigned(at(effect)[0x18]),slot?122u:0u,half(attack_count));
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type9-bind-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    CommonBank bank{};
    if(std::fread(bank.data(),1,bank.size(),stdin)!=bank.size()||std::getc(stdin)!=EOF)
        fail("type-9 common bank payload extent");
    for(unsigned key=0;key<7;++key)
        for(bool slot:{false,true})
            observe(mod,original,bank,key,slot);
}
