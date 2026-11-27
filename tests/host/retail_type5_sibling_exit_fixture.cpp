// Original sibling area/type-4 exit requests from declared active parent inlets.
#include "retail_stationary_contact.hpp"

namespace {
using namespace retail_fixture;
namespace scratch=retail_stationary_contact;
struct Pilot {u16 model;u8 type,row,ordinal,area_key;u32 factory,update,contact_site,zero_hp,expiry;};
constexpr Pilot pilots[]={
    {0x0106,33,2,2,19,0x800FE668,0x800FE780,0x800FEB34,0x800FEB78,0x800FEBB8},
    {0x0601,52,0,0,11,0x80128DF8,0x80128F54,0x80129354,0x80129398,0x801293D8},
    {0x0617,52,1,2,11,0x80128DF8,0x80128F54,0x80129354,0x80129398,0x801293D8},
    {0x0D00,74,0,0,6,0x800F9400,0x800F9518,0x800F97F4,0x800F9838,0x800F9878},
    {0x0D01,77,0,0,7,0x800FB220,0x800FB330,0x800FB5DC,0x800FB620,0x800FB648},
    {0x0D05,77,1,0,7,0x800FB220,0x800FB330,0x800FB5DC,0x800FB620,0x800FB648},
};
struct Scenario {bool contact,zero_hp,expired,outside;unsigned descriptors;};
constexpr Scenario cases[]={
    {true,false,false,false,2}, {true,true,true,false,2}, {true,true,true,false,1},
    {false,true,false,false,2}, {false,false,true,false,2}, {false,true,true,true,2},
};
constexpr u32 parent=0x81030000,area_cell=0x81030800,effect_cell=0x81031000,descriptors=0x81032000;
constexpr u32 descriptor_row=0x81033000,stage=0x81040000,audio=0x81041000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 area=0x80085E00,effect=0x8001BDA8;

void observe(const Module& mod,std::vector<u8> ram,Pilot p,unsigned scenario){
    const auto at=[&](u32 address){return scratch::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return scratch::half(ram,address);};
    const auto real=[&](u32 address){return std::bit_cast<float>(word(address));};
    const auto xyz=[&](u32 address){return scratch::Point{real(address),real(address+4),real(address+8)};};
    const auto place=[&](u32 address,const scratch::Point& pos){
        for(unsigned i=0;i<3;++i)put_float(at(address+4*i),pos[i]);
    };
    const auto s=cases[scenario];
    auto* owner=at(actor);
    owner[0]=1;owner[0x18]=1;
    put16(owner+0x3E8,p.model);put16(owner+0x94,p.model);put32(owner+0x90,actor);put16(owner+0x1C6,150);
    for(unsigned offset:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+offset,1.f);
    put32(at(0x8043B5A0-31852),scratch::scene);
    put32(at(0x8043B5A0-21316),stage);
    put_float(at(stage+52),500.f);
    for(unsigned i=0;i<4;++i)put_float(at(stage+56+4*i),i%2?-1000.f:2000.f);
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;at(0x803D0000+15080+852)[0]=1;

    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),descriptors);put16(at(descriptor_row+18),u16(s.descriptors+1));
    for(unsigned i=0;i<s.descriptors;++i)put32(at(descriptors+0x80*i),descriptors+0x80*(i+1));
    put32(at(free_head+12),parent);put32(at(free_tail+12),area_cell);put16(at(free_count+6),2);
    at(parent)[2]=3;at(area_cell)[2]=3;put32(at(parent+4),area_cell);
    put32(at(free_head+8),effect_cell);put32(at(free_tail+8),effect_cell);put16(at(free_count+4),1);at(effect_cell)[2]=2;
    CPUState make{};make.gpr[3]=actor;
    make.gpr[4]=(p.type==52||p.type==74)?p.ordinal:p.row;
    make.gpr[5]=p.type==52?p.row:p.type==74?1:p.ordinal;
    make.gpr[6]=p.type==52?0xFFFFFFFFu:0;
    mod.run(make,ram,p.factory,1024);
    if(!at(parent)[0]||at(parent)[0x10]!=p.type||at(parent)[0x11]!=p.row||at(parent)[0x13]!=p.ordinal||at(parent)[0x18]||
       word(parent+0x0C)!=p.update||word(parent+0x90)!=actor||word(parent+0x8C)!=actor||
       half(free_count+6)!=1||half(free_count+4)!=1||half(descriptor_row+18)!=s.descriptors||
       word(active_head+9*4)!=parent||word(active_head+49*4))
        fail("area sibling constructor lost its row, ordinal or owner");

    // Deliberately declared active inlet. No opening bind or contact is simulated.
    const scratch::Point root{s.outside?8000.f:200.f,100.f,500.f},hit{260.f,120.f,580.f};
    place(parent+0x20,root);place(parent+0x2A0,hit);
    at(parent)[0x18]=1;at(parent)[0x1D9]=s.contact?16:0;
    put16(at(parent+0x1C),s.expired?0:5);put16(at(parent+0x1C6),s.zero_hp?0:10);
    const auto callee=s.outside?0:s.contact?area:effect;
    const unsigned key=s.contact?p.area_key:0;
    const bool use_event=s.contact&&p.type!=77;
    const auto saved=ram;
    unsigned position_offset=0;
    if(callee){
        CPUState request{};request.gpr[3]=parent;
        CallObservation source{callee};
        mod.run_to(request,ram,p.update,{callee},&source);
        const auto expected_pos=parent+(use_event?0x2A0:0x20);
        if(source.count!=1||source.caller!=(s.contact?p.contact_site:s.zero_hp?p.zero_hp:p.expiry)||
           source.arguments[0]!=parent||source.arguments[1]!=expected_pos||source.arguments[2]!=key||
           xyz(expected_pos)!=(use_event?hit:root)||at(parent)[0x18]!=2||at(parent)[0x1D8]!=1)
            fail("area sibling exit did not close before the chosen factory and point");
        position_offset=source.arguments[1]-parent;
    }
    std::memcpy(ram.data(),saved.data(),ram.size());
    CPUState close{};close.gpr[3]=parent;
    CallObservation wrong_factory{s.contact?effect:area};
    mod.run_observed(close,ram,p.update,wrong_factory,1024);
    const bool area_made=s.contact&&s.descriptors>=2,effect_made=!s.contact&&!s.outside;
    const unsigned type=callee==area?5:callee==effect?4:0;
    const unsigned cost=2*unsigned(area_made)+unsigned(effect_made);
    if(wrong_factory.count||at(parent)[0x18]!=2||at(parent)[0x1D8]!=1||half(actor+0x1C6)!=150||
       half(free_count+6)!=1-unsigned(area_made)||half(free_count+4)!=1-unsigned(effect_made)||
       half(descriptor_row+18)!=s.descriptors-cost||word(active_head+49*4)||
       bool(at(area_cell)[0])!=area_made||bool(at(effect_cell)[0])!=effect_made){
        std::fprintf(stderr,"model=%04X case=%u unexpected request count=%u phase=%u/%u stock=%u/%u/%u active=%u/%u\n",
                     p.model,scenario,wrong_factory.count,at(parent)[0x18],at(parent)[0x1D8],
                     half(free_count+6),half(free_count+4),half(descriptor_row+18),at(area_cell)[0],at(effect_cell)[0]);
        fail("area sibling allocation, suppression or pool refusal changed");
    }
    if(area_made||effect_made){
        const auto child=area_made?area_cell:effect_cell;
        if(at(child)[0x10]!=type||at(child)[0x11]!=key||at(child)[0x18]||
           word(child+0x8C)!=parent||word(child+0x90)!=actor||xyz(child+0x20)!=(use_event?hit:root))
            fail("area sibling child did not retain the requested key, origin and separate credit");
    }
    std::printf("area-exit: %u %u %u %u %u %u %u %u %u %u %u\n",p.model,scenario,p.type,p.row,p.ordinal,type,key,
                unsigned(area_made||effect_made),half(descriptor_row+18),at(parent)[0x18],position_offset);
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type5-sibling-exit-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(const auto& p:pilots)
        for(unsigned i=0;i<std::size(cases);++i)
            observe(mod,original,p,i);
}
