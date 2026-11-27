// Role-0x80 publication from exact installed VM program bytes.
#include "retail_contact_pose_clock.hpp"
#include <cstdint>

namespace {
using namespace retail_contact_pose_clock;
constexpr u32 interpreter=0x8004C050,allocator=0x8008AC80,clip_entry=0x80048648,alternate=0x8002DF08;
constexpr u32 hit_block=0x81035000,shape=0x81039000,descriptor_pool=0x81033A00,body=contact::receiver;
constexpr u32 ordinary_attacks=0x8043B5A0-21328,alternate_attacks=0x8043B5A0-21344;
constexpr u32 alternate_list=0x803C177C,alternate_receivers=0x8043B5A0-21346;
struct Route {unsigned model,group,step,index,joint,first_frame,secondary;u32 program,table;};
constexpr Route routes[]{
    {0x0900,3,5,3,1,22,0,0x802C2B36,0x802D6930},
    {0x0900,4,6,4,7, 9,127,0x802C2BEA,0x802D6930},
    {0x0902,3,5,3,1,16,0,0x803481B2,0x80348910},
};
constexpr contact::Point center{200.f,150.f,325.f};

void observe(const Module& mod,std::vector<u8> ram,const Route& route,unsigned scenario,
             const contact::HitBlock& receive_hit,const contact::ActorSpec& defender,
             const contact::HitBlock& attack_hit,const contact::ActorSpec& spec){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto scalar=[&](u32 a){return std::bit_cast<float>(word(a));};
    const bool slot=scenario!=0,ally=scenario==1||scenario==3;
    const unsigned hp_before=scenario==3?0:90;
    put32(at(0x80436380),descriptor_pool);
    contact::bind_sasuke(mod,ram,center,receive_hit,spec,defender,150);
    at(actor)[0]=1;at(actor+0x18)[0]=1;at(body+0x88)[0]=ally?0:1;
    put16(at(body+0x1C6),u16(hp_before));
    put32(at(0x803C4E84),actor);
    for(unsigned i=2;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    put16(at(actor+0x3E8),u16(route.model));
    put_float(at(actor+0xC0),1.f);put_float(at(actor+0xB8),1.f);
    put_float(at(actor+0x1CDC),float(route.first_frame));put32(at(actor+0x76C),1u<<route.joint);
    put32(at(actor+0x27C),route.table);put32(at(actor+0x3FC),hit_block);
    std::memcpy(at(hit_block),attack_hit.data(),attack_hit.size());
    const auto descriptor=hit_block+0xAA0+80*route.step,record=route.table+24*route.index;
    const auto transform=actor+0x8D4+0x30*route.joint;
    if(at(descriptor)[0]||at(descriptor+1)[0]!=route.joint||half(descriptor+2)!=0x80||
       half(descriptor+4)!=route.index||half(descriptor+6)!=route.first_frame)
        fail("alternate VM shape role or pose-frame source changed");
    for(unsigned i=0;i<3;++i){
        put_float(at(transform+12+16*i),center[i]-scalar(descriptor+12+4*i));
        put_float(at(transform+20*i),1.f);
    }
    if(slot){
        at(shape)[2]=4;
        put32(at(contact::free_head+16),shape);put32(at(contact::free_tail+16),shape);
        put16(at(contact::free_count+8),1);
    }
    CPUState vm{};vm.gpr[3]=actor;vm.gpr[4]=1;vm.gpr[5]=route.program;
    vm.gpr[6]=vm.gpr[7]=0xFFFFFFFF;vm.fpr[1]=-1.;
    CallObservation bind{allocator};
    const auto reached=mod.run_to_or_return(vm,ram,interpreter,{clip_entry},&bind,4096);
    if(reached!=(route.group==3?clip_entry:0)||bind.count!=1||
       bind.caller!=0x8004C6F0||bind.arguments[0]!=actor||bind.arguments[1]!=route.group||
       at(actor+0x744)[0]!=route.group||bool(at(shape)[0])!=slot||
       (slot&&(word(shape+0x20)!=actor||word(shape+0x24)!=actor||word(shape+0x28)!=descriptor||word(shape+0x2C)!=route.table))||
       (route.secondary?(at(actor+0x43D)[0]!=129||at(actor+0x6F8)[0]!=127||at(actor+0x6F9)[0]!=255):
                        (at(actor+0x43D)[0]||at(actor+0x6F8)[0]||at(actor+0x6F9)[0])))
        fail("alternate VM did not retain its exact allocator and secondary writes");
    const auto publish=[&](){
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState step{};mod.run(step,ram,entry,1024);
        }
    };
    if(route.group==4){
        if(scalar(actor+0x1CDC)!=0.f||word(actor+0x1CD4)!=route.program+4||scalar(actor+0x1CE8)!=10.f)
            fail("alternate group-4 VM did not return with its first wait installed");
        // The VM returned at frame zero. Check its waiting shape again
        // immediately below the source frame threshold before opening it.
        for(float frame:{0.f,8.f}){
            put_float(at(actor+0x1CDC),frame);
            publish();
            if(half(alternate_attacks)||half(ordinary_attacks)||half(actor+0x26C)||
               (slot&&(!at(shape)[0]||at(shape+0x10)[0])))
                fail("alternate group-4 shape published before frame 9");
        }
        put_float(at(actor+0x1CDC),9.f);
    }
    publish();
    if(half(ordinary_attacks)||half(alternate_attacks)!=unsigned(slot)||half(alternate_receivers)!=1||
       (slot&&(word(alternate_list)!=shape||word(shape+0x64)!=std::bit_cast<u32>(center[0])||half(actor+0x26C)!=1))){
        std::fprintf(stderr,"VM publish %04X/%u case=%u lists=%u/%u/%u phase=%u frame=%g token=%u active=%u\n",
                     route.model,route.group,scenario,half(ordinary_attacks),half(alternate_attacks),
                     half(alternate_receivers),at(shape+0x10)[0],double(scalar(actor+0x1CDC)),half(actor+0x26C),at(shape)[0]);
        fail("alternate VM body queue published an unexpected role or shape");
    }
    constexpr u32 audio=0x8103E000;
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
    at(0x803D0000+15080+852)[0]=1;
    CPUState ordinary{};CallObservation ordinary_amount{contact::amount_entry};
    mod.run_observed(ordinary,ram,contact::pair_pass,ordinary_amount,4096);
    if(ordinary_amount.count||half(body+0x1C6)!=hp_before||half(body+0x20C))
        fail("alternate VM shape entered the ordinary pair");
    CPUState pair{};CallObservation amount{contact::amount_entry};
    const auto hp_entry=mod.run_to_or_return(pair,ram,alternate,{contact::hp_write},&amount,4096);
    if((amount.count&&(amount.count!=1||amount.caller!=0x8002F108||amount.arguments[0]!=record||
       amount.arguments[1]!=actor||amount.arguments[2]!=body))||half(body+0x1C6)!=hp_before||
       (hp_entry&&(pair.lr!=0x8002F1DC||pair.gpr[3]!=body)))
        fail("alternate VM pair changed the amount or HP writer boundary");
    const auto requested=hp_entry?std::bit_cast<std::int32_t>(pair.gpr[4]):0;
    if(hp_entry)mod.finish_stopped_leaf(pair,contact::hp_write,164);
    std::printf("alternate-vm: %u %u %u %u %u %u %u %u %u %d %u %u %u %u %u %u %u %u\n",
                route.model,route.group,scenario,unsigned(slot),at(actor+0x43D)[0],at(actor+0x6F8)[0],at(actor+0x6F9)[0],
                amount.count,unsigned(hp_entry!=0),requested,half(body+0x1C6),half(body+0x20C),at(actor+timer)[0],
                at(body+timer)[0],word(actor+0x418),word(actor+0x42C),word(body+0x41C),unsigned(route.group==4&&slot));
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: alternate-vm-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    const auto read=[](auto& row){
        if(std::fread(row.data(),1,row.size(),stdin)!=row.size())fail("alternate VM input extent");
    };
    contact::HitBlock hit{};contact::ActorSpec defender{};
    read(hit);read(defender);
    for(const auto& route:routes){
        contact::HitBlock attack{};contact::ActorSpec spec{};
        read(attack);read(spec);
        for(unsigned scenario=0;scenario<4;++scenario)
            observe(mod,original,route,scenario,hit,defender,attack,spec);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("unexpected alternate VM payload");
}
