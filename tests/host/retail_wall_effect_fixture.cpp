// Bounded lifetime of the fixed-bucket wall helper's selector-0 effect.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 parent=0x81028000,effect=0x81030000,descriptor=0x81032000,descriptor_row=0x81033000;
constexpr u32 shape=0x81034000,origin=0x81035000,scene=contact::scene;
constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 helper=0x80019550,allocator=0x8008893C,bucket_visit=0x80089A94,bucket=31;
constexpr u32 attack_count=0x8043B5A0-21328;

void observe(const Module& mod,std::vector<u8> ram,unsigned scenario){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto scalar=[&](u32 address){return std::bit_cast<float>(word(address));};
    const bool objects=scenario!=4,descriptors=scenario!=5,shapes=scenario!=2;
    at(actor)[0]=1;at(actor)[0x18]=1;at(parent)[0]=1;at(parent)[0x18]=2;
    put16(at(actor+0x3E8),0x0D01);put16(at(parent+0x94),0x0D01);
    put32(at(actor+0x90),actor);put32(at(parent+0x90),actor);
    put_float(at(actor+0x1DC8),scenario==1?0.5f:1.f);
    put_float(at(parent+0xB4),0.75f);put_float(at(parent+0xB8),1.f);
    constexpr contact::Point point{200.f,100.f,1000.f};
    for(unsigned i=0;i<point.size();++i)put_float(at(origin+4*i),point[i]);
    put32(at(0x8043B5A0-31852),scene);
    put32(at(0x803C4E84),actor);
    for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);put32(at(table+4*21),empty);
    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),descriptors?descriptor:0);put16(at(descriptor_row+18),u16(descriptors));
    put32(at(free_head+8),objects?effect:0);put32(at(free_tail+8),objects?effect:0);
    put16(at(free_count+4),u16(objects));at(effect)[2]=2;
    if(shapes){
        put32(at(free_head+16),shape);put32(at(free_tail+16),shape);put16(at(free_count+8),1);at(shape)[2]=4;
    }
    CPUState call{};call.gpr[3]=parent;call.gpr[4]=origin;call.gpr[5]=0;
    CallObservation alloc{allocator};
    mod.run_observed(call,ram,helper,alloc);
    const bool made=objects&&descriptors;
    if(alloc.count!=1||alloc.caller!=0x800195D4||alloc.arguments[0]!=2||alloc.arguments[1]!=bucket||
       alloc.arguments[2]||alloc.arguments[3]!=1||call.gpr[3]!=(made?effect:0)||
       half(free_count+4)!=unsigned(objects)-unsigned(made)||half(descriptor_row+18)!=unsigned(descriptors)-unsigned(made)||
       word(active_head+4*bucket)!=(made?effect:0)||bool(at(effect)[0])!=made)
        fail("wall effect did not keep fixed-bucket allocation and refusal separate");
    if(!made){
        std::printf("wall-effect: %u 0 0 0 0 0.00000 0.00000 0.00000\n",scenario);
        return;
    }
    if(at(effect)[0x10]||at(effect)[0x11]||at(effect)[0x18]||word(effect+0x0C)!=0x800196D0||
       word(effect+0x8C)!=parent||word(effect+0x90)!=actor||scalar(effect+0x58)!=0.75f||
       std::memcmp(at(effect+0x20),at(origin),12))
        fail("wall effect constructor changed tag, handler, point or credit");
    put_float(at(parent+0xB4),2.f);
    if(scenario==3)at(parent)[0]=0;
    float initial=0,step_four=0,terminal=0;
    unsigned phase2=0,phase3=0,retirement=0;
    for(unsigned visit=1;visit<=23;++visit){
        CPUState tick{};tick.gpr[3]=bucket;
        mod.run(tick,ram,bucket_visit,1024);
        for(u32 entry:{0x80089554u,0x8008A28Cu,0x80089AECu}){
            CPUState pass{};mod.run(pass,ram,entry,1024);
        }
        const bool returned=visit==23;
        const unsigned phase=visit<21?1:visit==21?2:3;
        const auto scale=scalar(effect+0x58);
        if(bool(at(effect)[0])==returned||at(effect)[0x18]!=phase||half(effect+0x1C)!=(visit<21?visit-1:20)||
           half(effect+0x1E)!=20||word(descriptor+20)!=21||word(active_head+4*bucket)!=(returned?0:effect)||
           half(free_count+4)!=unsigned(returned)||half(descriptor_row+18)!=unsigned(returned)||
           word(free_head+8)!=(returned?effect:0)||word(descriptor_row+12)!=(returned?descriptor:0)||
           half(free_count+8)!=unsigned(shapes)||word(free_head+16)!=(shapes?shape:0)||word(active_head+49*4)||
           half(attack_count)||half(contact::receiver_count)||std::memcmp(at(effect+0x20),at(origin),12)||
           scalar(effect+0x5C)!=scale||scalar(effect+0x60)!=scale||!(scale>=1.f&&scale<=3.f))
            fail("wall effect lifecycle changed its curve, origin, registry or resource return");
        if(visit==1){initial=scale;put_float(at(parent+0xB4),7.f);}
        if(visit==5)step_four=scale;
        if(phase==2){phase2=visit;terminal=scale;}
        if(phase==3&&!phase3)phase3=visit;
        if(returned)retirement=visit;
    }
    std::printf("wall-effect: %u 1 %u %u %u %.5f %.5f %.5f\n",scenario,phase2,phase3,retirement,
                double(initial),double(step_four),double(terminal));
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: wall-effect-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(unsigned i=0;i<6;++i)observe(mod,original,i);
}
