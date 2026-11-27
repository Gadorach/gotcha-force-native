// Original type-4/key-0 lifecycle with declared ownership and empty render records.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 parent=0x81028000,effect=0x81030000,descriptor=0x81032000,descriptor_row=0x81033000;
constexpr u32 shape=0x81034000,origin=0x81035000,scene=contact::scene;
constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty67=visual+0x400,empty23=visual+0x480;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 factory=0x8001BDA8,allocate=0x80088AA0,bucket_entry=0x80089A94,bucket=25,attack_count=0x8043B5A0-21328;

void observe(const Module& mod,std::vector<u8> ram,unsigned scenario){
    const auto at=[&](u32 va){return contact::at(ram,va);};
    const auto word=[&](u32 va){return be32(at(va));};
    const auto half=[&](u32 va){return contact::half(ram,va);};
    const auto scalar=[&](u32 va){return std::bit_cast<float>(word(va));};
    at(actor)[0]=1;at(actor)[0x18]=1;
    at(parent)[0]=1;at(parent)[0x18]=2;
    put16(at(actor+0x3E8),0x0D01);put16(at(parent+0x94),0x0D01);
    put32(at(actor+0x90),actor);put32(at(parent+0x90),actor);
    put_float(at(actor+0x1DC8),scenario==1?0.5f:1.f);
    put_float(at(parent+0xB4),0.75f);put_float(at(parent+0xB8),1.f);
    for(unsigned i=0;i<3;++i)put_float(at(origin+4*i),std::array{160.f,0.f,400.f}[i]);
    put32(at(0x8043B5A0-31852),scene);
    put32(at(0x803C4E84),actor);
    for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);
    put32(at(table+4*67),empty67);put32(at(table+4*23),empty23);
    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),descriptor);put16(at(descriptor_row+18),1);
    put32(at(free_head+8),effect);put32(at(free_tail+8),effect);put16(at(free_count+4),1);at(effect)[2]=2;
    const bool shape_supplied=scenario!=2;
    if(shape_supplied){
        put32(at(free_head+16),shape);put32(at(free_tail+16),shape);put16(at(free_count+8),1);at(shape)[2]=4;
    }
    CPUState make{};make.gpr[3]=parent;make.gpr[4]=origin;make.gpr[5]=0;
    CallObservation alloc{allocate};
    mod.run_observed(make,ram,factory,alloc);
    if(make.gpr[3]!=effect||at(effect)[0x10]!=4||at(effect)[0x11]||at(effect)[0x18]||
       word(effect+0x8C)!=parent||word(effect+0x90)!=actor||word(effect+0x0C)!=0x8001BEC8||
       std::memcmp(at(effect+0x20),at(origin),12)||alloc.count!=1||alloc.caller!=0x8001BDE0||
       alloc.arguments[0]!=actor||alloc.arguments[1]!=2||alloc.arguments[2]!=24||alloc.arguments[3]||
       alloc.arguments[4]!=1||half(free_count+4)||half(descriptor_row+18)||word(active_head+4*bucket)!=effect)
        fail("type-4 key-0 allocated identity or class-2 resources changed");

    // The allocated effect starts after the declared parent changes scale.
    // The last case leaves that storage intact but marks the parent inactive.
    put_float(at(parent+0xB4),2.f);
    if(scenario==3)at(parent)[0]=0;
    float opening_scale=0,switch_scale=0,terminal_scale=0;
    unsigned switched=0,phase2=0,phase3=0,returned=0;
    for(unsigned visit=1;visit<=41;++visit){
        CPUState tick{};tick.gpr[3]=24;
        CallObservation new_cell{allocate};
        mod.run_observed(tick,ram,bucket_entry,new_cell,4096);
        for(u32 entry:{0x80089554u,0x8008A28Cu,0x80089AECu}){
            CPUState pass{};mod.run(pass,ram,entry,1024);
        }
        const bool retired=visit==41;
        const unsigned phase=visit<39?1:visit==39?2:3,local_stage=visit<9?0:visit<39?1:2;
        const unsigned elapsed=visit<9?visit-1:visit<39?visit-9:30;
        const auto scale=scalar(effect+0x58);
        if(new_cell.count||half(attack_count)||half(contact::receiver_count)||word(active_head+49*4)||
           half(free_count+8)!=unsigned(shape_supplied)||word(free_head+16)!=(shape_supplied?shape:0))
            fail("type-4 key-0 changed a collision resource or registry");
        if(bool(at(effect)[0])==retired||at(effect)[0x18]!=phase||at(effect)[0x12]!=local_stage||
           half(effect+0x1C)!=elapsed||word(descriptor+20)!=(visit<9?67:23)||
           half(free_count+4)!=unsigned(retired)||half(descriptor_row+18)!=unsigned(retired)||
           word(free_head+8)!=(retired?effect:0)||word(free_tail+8)!=(retired?effect:0)||
           word(descriptor_row+12)!=(retired?descriptor:0)||word(active_head+4*bucket)!=(retired?0:effect)||
           word(effect+0x8C)!=parent||word(effect+0x90)!=actor||std::memcmp(at(effect+0x20),at(origin),12)||
           !(scale>=2.f&&scale<=3.125f)||scalar(effect+0x5C)!=scale||scalar(effect+0x60)!=scale)
            fail("type-4 key-0 stage, scale or pool return no longer matches");
        if(visit==1){opening_scale=scale;put_float(at(parent+0xB4),7.f);}
        if(local_stage==1&&!switched){switched=visit;switch_scale=scale;}
        if(phase==2&&!phase2){phase2=visit;terminal_scale=scale;}
        if(phase==3&&!phase3)phase3=visit;
        if(retired)returned=visit;
    }
    std::printf("type4-life: %u %u %u %u %u %.5f %.5f %.5f\n",scenario,switched,phase2,phase3,returned,
                double(opening_scale),double(switch_scale),double(terminal_scale));
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type4-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(unsigned i=0;i<4;++i)observe(mod,original,i);
}
