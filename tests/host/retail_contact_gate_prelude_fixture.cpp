// Original gate prelude in an idle collision walk, cut before its later callback.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 prelude=0x8002BB14,frontier=0x8005A580,active_head=0x803C706C;
constexpr u32 actor_stride=7680,actors=0x803C4E84,first_view=0x803C73B0,view_stride=996;
constexpr u32 root=0x81040000,registered_child=0x81040800,unregistered_child=0x81041000;
constexpr u32 inactive_seat_child=0x81041800,end_bucket_child=0x81042000,scene=0x81043000;
constexpr unsigned gate=0x2AF;

void observe(const Module& mod,std::vector<u8> ram,float step){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    for(unsigned n=0;n<6;++n){
        const auto cell=actor+n*actor_stride;
        put32(at(actors+4*n),cell);
        at(cell)[0]=n<2?1:0;
        at(cell+gate)[0]=n==1?0:4;
        put_float(at(cell+0x1DC8),step);
    }
    put16(at(actor+0x1C6),180);put16(at(actor+0x20C),1);
    put32(at(0x8043B5A0-31852),scene);
    for(u32 obj:{root,registered_child,unregistered_child,inactive_seat_child,end_bucket_child}){
        at(obj)[0]=1;at(obj+gate)[0]=4;
        at(obj+0x86)[0]=obj==unregistered_child?0:1;
    }
    put32(at(active_head),root);
    put32(at(active_head+4),registered_child);
    put32(at(registered_child+8),unregistered_child);
    put32(at(active_head+3*4),inactive_seat_child);
    put32(at(active_head+7*4),end_bucket_child);
    for(unsigned i=0;i<12;++i)at(first_view+i*view_stride+gate)[0]=4;

    for(unsigned visit=1;visit<=5;++visit){
        CPUState cpu{};
        mod.run_to(cpu,ram,prelude,{frontier});
        const unsigned remaining=visit>=4?0:4-visit;
        const auto get=[&](u32 object){return unsigned(at(object+gate)[0]);};
        const auto active=get(actor),empty=get(actor+actor_stride),idle=get(actor+2*actor_stride);
        const auto base=get(root),child=get(registered_child),unbound=get(unregistered_child);
        const auto inactive=get(inactive_seat_child),end=get(end_bucket_child),view=get(first_view);
        const auto last_view=get(first_view+11*view_stride);
        for(unsigned i=0;i<12;++i)
            if(get(first_view+i*view_stride)!=remaining)fail("contact gate prelude did not tick every view record");
        if(cpu.lr!=0x8002C870||active!=remaining||empty||idle!=4||base!=remaining||child!=remaining||
           unbound!=4||inactive!=4||end!=remaining||view!=remaining||last_view!=remaining||
           half(actor+0x1C6)!=180||half(actor+0x20C)!=1||be32(at(actor+0x1DC8))!=std::bit_cast<u32>(step))
            fail("contact gate prelude did not respect object scope or changed its visit count");
        std::printf("impact-prelude: %u %u %u %u %u %u %u %u %u %u %u %u\n",step==0.5f?1u:2u,visit,
                    active,empty,idle,base,child,unbound,inactive,end,view,last_view);
    }
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: contact-gate-prelude-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(float step:{1.f,0.5f})observe(mod,original,step);
}
