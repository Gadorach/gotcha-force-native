// Execute the two exact normal type-76 request leaves without running the carrier.
#include "retail_stationary_contact.hpp"
#include <algorithm>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
struct Route {unsigned model,key,channel,cursor,markers;u32 pre,entry,debit_site;};
constexpr Route routes[]={
    {0x0D01,0,0,0x148,0x146,0x80188A64,0x80188EC4,0x80188F4C},
    {0x0D04,2,2,0x151,0x152,0x800F77C8,0x800F7DC0,0x800F7EF0},
};
constexpr u32 first=0x81030000,second=0x81030800,descriptor=0x81032000,descriptor_row=0x81033000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 allocate=0x80088AA0,debit_entry=0x8006DBE0;

void replay(const Module& mod,std::vector<u8> ram,Route r,unsigned scenario){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    auto* owner=at(actor);
    owner[0]=1;owner[0x18]=1;
    put16(owner+0x3E8,r.model);put16(owner+0x94,r.model);put32(owner+0x90,actor);
    for(unsigned offset:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+offset,1.f);
    const auto stock=0x774+8*r.channel;
    const auto initial=scenario==3?0u:2u;
    put16(owner+stock,initial);put16(owner+0x78E + 8*r.channel,initial);
    owner[r.cursor]=0x7E;owner[r.markers]=0x7E;owner[r.markers+1]=0x7E;
    CPUState pre{};pre.gpr[3]=actor;
    mod.run(pre,ram,r.pre);
    if(owner[r.cursor]||owner[r.markers]||owner[r.markers+1]||half(actor+stock)!=initial)
        fail("type-76 scoped pre-callback did not clear the ordinal and both markers");

    const auto offer=[&](unsigned count){
        if(half(free_count+6)||word(free_head+12))fail("type-76 carrier inlet was not empty");
        if(!count)return;
        put32(at(free_head+12),first);put32(at(free_tail+12),count==2?second:first);
        put16(at(free_count+6),u16(count));
        at(first)[2]=3;at(second)[2]=3;put32(at(first+4),count==2?second:0);
    };
    offer(scenario==1?0:2);
    const auto desc=scenario==2?0u:2u;
    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),desc?descriptor:0);put16(at(descriptor_row+18),u16(desc));
    put32(at(descriptor),descriptor+0x80);
    for(unsigned visit=0;visit<3;++visit){
        // This selects the exact phase-one leaf again; no controller or inter-action rearm is inferred.
        owner[0x540]=1;
        const auto old=ram;
        const bool paid=visit<initial;
        const auto ordinal=owner[r.cursor];
        const bool made=paid&&(scenario==0||(scenario==1&&visit==1));
        const auto cell=word(free_head+12);
        const auto head=word(active_head+9*4);
        const auto free=half(free_count+6),descriptors=half(descriptor_row+18);
        CPUState payment{};payment.gpr[3]=actor;
        CallObservation charge{debit_entry};
        mod.run_observed(payment,ram,r.entry,charge);
        if(charge.count!=1||charge.caller!=r.debit_site||charge.arguments[0]!=actor||
           charge.arguments[1]!=r.channel||charge.arguments[2]!=1||charge.arguments[3]!=1)
            fail("type-76 dispatch stopped calling its paid leaf");
        std::memcpy(ram.data(),old.data(),ram.size());
        CPUState request{};request.gpr[3]=actor;
        CallObservation allocation{allocate};
        mod.run_observed(request,ram,r.entry,allocation);
        if(owner[0x540]!=2||half(actor+stock)!=initial-std::min(visit+1,initial)||
           owner[r.cursor]!=(paid?ordinal^1:ordinal)||owner[r.markers]!=unsigned(initial!=0)||
           owner[r.markers+1]!=unsigned(initial&&visit>=1)||
           allocation.count!=unsigned(paid)||(paid&&(allocation.caller!=0x800FCD6C||allocation.arguments[0]!=actor||
             allocation.arguments[1]!=3||allocation.arguments[2]!=8||allocation.arguments[3]||allocation.arguments[4]!=1))||
           half(free_count+6)!=free-unsigned(made)||half(descriptor_row+18)!=descriptors-unsigned(made)||
           word(active_head+49*4))
            fail("type-76 paid debit, refusal, cursor or resource boundaries changed");
        if(made&&(!cell||word(active_head+9*4)!=cell||at(cell)[0x10]!=76||at(cell)[0x11]!=r.key||
                  at(cell)[0x13]!=ordinal||at(cell)[0x18]||word(cell+0x8C)!=actor||word(cell+0x90)!=actor))
            fail("type-76 allocated parent had a different row, ordinal or owner");
        if(!made&&word(active_head+9*4)!=head)fail("type-76 allocation refusal changed the live bucket");
        std::printf("type76-paid: %u %u %u %u %u %u %u %u\n",r.model,scenario,visit+1,unsigned(ordinal),
                    allocation.count,unsigned(made),unsigned(owner[r.cursor]),half(actor+stock));
        if(scenario==1&&visit==0)offer(1);
    }
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type76-paid-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(const auto& route:routes)
        for(unsigned i=0;i<4;++i)
            replay(mod,original,route,i);
}
