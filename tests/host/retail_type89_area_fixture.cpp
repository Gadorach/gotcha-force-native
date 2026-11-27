// Original type-89 constructor, initial shape, and supplied exit controls.
#include "retail_recomp_fixture.hpp"
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 carrier=0x81030000,area=0x81030800,shapes=0x81032000,shape_stride=0x200;
constexpr u32 descriptor_row=0x81034000,descriptor=0x81034100,hit_block=0x81036000,common=0x81039000;
constexpr u32 scene=0x81017000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 make_carrier=0x8016CC24,carrier_update=0x8016CD1C,exit_leaf=0x8016D2D0;
constexpr u32 make_area=0x80085E00,area_update=0x80086048,area_key=16;
constexpr u32 list_enroll=0x80089100;

using HitBlock=std::array<u8,7840>;
using CommonBank=std::array<u8,21504>;
struct Pilot {u16 model;u8 key,group,joint;u32 hit_table;};
constexpr Pilot pilots[]={{0x0F03,11,2,2,0x803810F8},{0x062A,16,28,12,0x80365EC0}};
enum class Case {initial_refused, neither, contact, flag, both, no_object, short_descriptors};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){
        if(va<GC_RAM_BASE||va-GC_RAM_BASE>ram.size()-4)fail("type-89 scratch extent");
        return ram.data()+va-GC_RAM_BASE;
    }
    u16 half(u32 va){const auto* p=at(va);return u16(u16(p[0])<<8|p[1]);}
    void pools(unsigned objects,unsigned descriptors,unsigned free_shapes){
        if(objects>2||descriptors>3||free_shapes>2)fail("type-89 pool extent");
        put32(at(0x80436380),descriptor_row);
        put16(at(descriptor_row+18),u16(descriptors));
        put32(at(descriptor_row+12),descriptors?descriptor:0);
        for(unsigned i=0;i<descriptors;++i)
            put32(at(descriptor+0x80*i),i+1<descriptors?descriptor+0x80*(i+1):0);
        put32(at(free_head+12),objects?carrier:0);
        put32(at(free_tail+12),objects==2?area:objects?carrier:0);
        put16(at(free_count+6),u16(objects));
        at(carrier)[2]=3;at(area)[2]=3;
        put32(at(carrier+4),objects==2?area:0);
        put32(at(free_head+16),free_shapes?shapes:0);
        put32(at(free_tail+16),free_shapes?shapes+shape_stride*(free_shapes-1):0);
        put16(at(free_count+8),u16(free_shapes));
        for(unsigned i=0;i<free_shapes;++i){
            const auto cell=shapes+shape_stride*i;
            at(cell)[2]=4;
            put32(at(cell+4),i+1<free_shapes?cell+shape_stride:0);
        }
    }
public:
    Check(const Module& m,const std::vector<u8>& input):mod(m),ram(input){}
    void observe(Pilot p,const HitBlock& hit,const CommonBank& bank,Case which){
        const bool admitted=which!=Case::initial_refused,contact=which==Case::contact||which==Case::both||
                           which==Case::no_object||which==Case::short_descriptors;
        const bool flag=which==Case::flag||which==Case::both;
        const auto objects=which==Case::no_object?1u:2u;
        const auto descriptors=which==Case::short_descriptors?2u:3u;
        pools(objects,descriptors,admitted?2:0);
        auto* owner=at(actor);
        owner[0]=1;owner[0x18]=1;
        put16(owner+0x3E8,p.model);put16(owner+0x94,p.model);
        put32(owner+0x90,actor);
        put32(owner+0x3FC,hit_block);put32(owner+0x27C,p.hit_table);
        put16(owner+0x774,5);
        for(auto offset:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+offset,1.f);
        auto* joint=owner+0x8D4+48*p.joint;
        for(unsigned axis=0;axis<3;++axis)put_float(joint+20*axis,1.f);
        put_float(joint+12,100.f);put_float(joint+28,200.f);put_float(joint+44,300.f);
        std::memcpy(at(hit_block),hit.data(),hit.size());
        std::memcpy(at(common),bank.data(),bank.size());
        put32(at(0x8043B5A0-31852),scene);put32(at(scene+152),common);
        CPUState ctor{};ctor.gpr[3]=actor;ctor.gpr[4]=p.key;
        mod.run(ctor,ram,make_carrier);
        auto* child=at(carrier);
        if(ctor.gpr[3]!=carrier||be32(at(active_head+9*4))!=carrier||
           child[0x10]!=89||child[0x11]!=p.key||child[0x17D]||
           be32(child+12)!=carrier_update||be32(child+0x10C)!=0x8016D4AC||be32(child+0x90)!=actor)
            fail("type-89 constructor identity or owner mismatch");
        CPUState init{};init.gpr[3]=carrier;
        CallObservation bind{0x8008AC80};
        const auto cut=mod.run_to_or_return(init,ram,carrier_update,{list_enroll},&bind);
        if(bind.count!=1||bind.caller!=0x8016CE0C||bind.arguments[0]!=carrier||bind.arguments[1]!=p.group||half(actor+0x774)!=5||
           cut!=(admitted?list_enroll:0)||child[0x18]!=(admitted?1:2)||
           child[0x17D]!=(admitted?area_key:0)||(!admitted&&be32(at(active_head+49*4)))){
            std::fprintf(stderr,"type-89 model=%04X case=%u calls=%u caller=%08X args=%08X/%u cut=%08X phase=%u "
                         "area=%u available=%u head=%08X owner_flags=%08X\n",p.model,unsigned(which),bind.count,bind.caller,
                         bind.arguments[0],bind.arguments[1],cut,child[0x18],child[0x17D],half(free_count+8),
                         be32(at(active_head+49*4)),be32(owner+0x5E0));
            fail("type-89 init did not write its area key only after shape admission");
        }
        if(!admitted){
            std::printf("type89-area: %u %u 0 0 0 0 0 0 %u\n",p.model,unsigned(which),half(actor+0x774));
            return;
        }
        if(init.lr!=0x8016CF8C||init.gpr[3]!=carrier||be32(at(active_head+49*4))!=shapes||
           be32(at(shapes+0x20))!=carrier||be32(at(shapes+0x24))!=actor||half(free_count+8)!=1)
            fail("type-89 opening shape identity mismatch");

        // Same initialized carrier, but declared interior phase and exits.
        // These scratch points make each original r4 branch distinguishable.
        child[0x19]=1;child[0x1D9]=contact?16:0;child[0x17C]=u8(flag);
        constexpr std::array<float,3> root{400,210,620},event{510,220,630};
        for(unsigned i=0;i<3;++i){put_float(child+0x20+4*i,root[i]);put_float(child+0x2A0+4*i,event[i]);}
        CPUState leave{};leave.gpr[3]=carrier;
        CallObservation request{make_area};
        mod.run_observed(leave,ram,exit_leaf,request);
        const bool requested=contact||flag,created=requested&&objects==2&&descriptors==3;
        if(request.count!=unsigned(requested)||child[0x19]!=2||child[0x1D8]!=1||half(actor+0x774)!=5||
           (requested&&(request.caller!=(contact?0x8016D30C:0x8016D32C)||request.arguments[0]!=carrier||
                       request.arguments[1]!=carrier+(contact?0x2A0:0x20)||request.arguments[2]!=area_key))||
           be32(child+8)!=(created?area:0)||half(descriptor_row+18)!=descriptors-1-2u*unsigned(created))
            fail("type-89 exit request key, origin, or area allocation mismatch");
        if(created){
            const auto* placed=at(area);
            const auto& point=contact?event:root;
            if(placed[0x10]!=5||placed[0x11]!=area_key||be32(placed+0x8C)!=carrier||be32(placed+0x90)!=actor||
               !placed[0]||placed[0x18]||be32(placed+0xE0)!=descriptor+0x80||be32(placed+0xE4)!=descriptor+0x100)
                fail("type-5 area did not keep the type-89 parent and owner separate");
            for(unsigned i=0;i<3;++i)
                if(be32(placed+0x20+4*i)!=std::bit_cast<u32>(point[i]))fail("type-89 area request used wrong point");
            // Valid empty renderer records, no retail model or sound data.
            constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400;
            put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);
            for(unsigned id:{18u,22u})put32(at(table+4*id),empty);
            CPUState bind_area{};bind_area.gpr[3]=area;
            CallObservation group{0x8008AE3C};
            mod.run_to(bind_area,ram,area_update,{0x800F036C},&group);
            constexpr u32 shape=shapes+shape_stride;
            if(bind_area.lr!=0x80086240||bind_area.gpr[3]!=area||bind_area.gpr[4]!=12||
               group.count!=1||group.caller!=0x800860E0||
               group.arguments[0]!=actor||group.arguments[1]!=area||group.arguments[2]!=110||
               be32(at(active_head+49*4))!=shape||be32(at(shape+0x20))!=area||be32(at(shape+0x24))!=actor||
               be32(at(shape+0x28))!=common+0x400+150*80||be32(at(shape+0x2C))!=0x802C4760||
               half(free_count+8)||half(actor+0x774)!=5)
                fail("type-89's area did not bind the common group-110 shape");
        }
        std::printf("type89-area: %u %u 1 %u %u %u %u %u %u\n",p.model,unsigned(which),child[0x17D],
                    request.count,requested?(contact?1u:2u):0u,unsigned(created),created?110u:0u,half(actor+0x774));
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type89-area-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    std::array<HitBlock,2> hit{};
    CommonBank common_bank{};
    for(auto& block:hit)
        if(std::fread(block.data(),1,block.size(),stdin)!=block.size())fail("type-89 hit block extent");
    if(std::fread(common_bank.data(),1,common_bank.size(),stdin)!=common_bank.size()||std::getc(stdin)!=EOF)
        fail("type-89 common bank extent");
    for(unsigned i=0;i<hit.size();++i)
        for(auto scenario:{Case::initial_refused,Case::neither,Case::contact,Case::flag,Case::both,
                           Case::no_object,Case::short_descriptors}){
            Check test(mod,original);test.observe(pilots[i],hit[i],common_bank,scenario);
        }
}
