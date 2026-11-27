// Original direction-row parents: terrain area before periodic type-11 segment.
#include "retail_stationary_contact.hpp"
#include <array>
#include <cmath>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 carrier=0x81030000,first_child=0x81030800,second_child=0x81031000;
constexpr u32 shape=0x81032000,descriptors=0x81033000,descriptor_row=0x81033800,hit_block=0x81034000;
constexpr u32 second_shape=shape+0x200,common=0x81048000,attack_count=0x8043B5A0-21328,attack_registry=0x803C477C;
constexpr u32 stage=0x81050000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 make_area=0x80085E00,make_segment=0x80139A6C,segment_update=0x80139B80,bind=0x8008AC80;
constexpr u32 terrain_query=0x8003E6E4;
using Point=std::array<float,3>;
using HitBlock=std::array<u8,7840>;
struct Pilot {u16 model;u8 row,socket,group,descriptor,index;u32 ctor,update,area_site,segment_site,table;};
constexpr Pilot pilots[]={
    {0x0E00,0,6,1,5,1,0x801F2BF8,0x801F2D40,0x801F3274,0x801F33A4,0x80363048},
    {0x0E05,0,6,1,5,1,0x801F2BF8,0x801F2D40,0x801F3274,0x801F33A4,0x80363048},
    {0x0E03,2,4,2,2,2,0x801F3784,0x801F38B0,0x801F3D50,0x801F3E80,0x8038DE60},
};
struct Case {bool floor;unsigned objects,extra_descriptors;};
constexpr Case cases[]={{false,2,2},{true,2,2},{true,1,2},{true,2,0}};
using CommonBank=std::array<u8,21504>;
struct Followup {
    unsigned scenario;
    const HitBlock& body;
    const contact::ActorSpec& attacker;
    const contact::ActorSpec& defender;
    const CommonBank& bank;
};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){
        if(va<GC_RAM_BASE||va-GC_RAM_BASE>ram.size()-4)fail("sweep route scratch extent");
        return ram.data()+va-GC_RAM_BASE;
    }
    u32 word(u32 va){return be32(at(va));}
    u16 half(u32 va){const auto* p=at(va);return u16(u16(p[0])<<8|p[1]);}
    float scalar(u32 va){return std::bit_cast<float>(word(va));}
    Point point(u32 va){return {scalar(va),scalar(va+4),scalar(va+8)};}
    void place(u32 va,const Point& xyz){
        for(unsigned i=0;i<3;++i)put_float(at(va+4*i),xyz[i]);
    }
    void terrain(bool floor){
        // One synthetic stage-layer triangle; no other stage geometry.
        constexpr u32 grid=stage+0x100,empty_grid=stage+0x140,refs=stage+0x200,side=48;
        constexpr u32 empty_refs=refs+side*side*4,list=empty_refs+side*side*4,tri=list+8,empty=tri+0x80;
        put32(at(0x8043B5A0-21316),stage);
        put32(at(stage+28),grid);put32(at(stage+32),empty_grid);put32(at(stage+36),empty_grid);
        put_float(at(stage+52),9000.f);put_float(at(stage+56),20000.f);put_float(at(stage+60),-1000.f);
        // The parent clips only its terrain probe to the stage bounds.
        put_float(at(stage+64),6000.f);put_float(at(stage+68),-1000.f);
        for(u32 g:{grid,empty_grid}){
            put32(at(g),500);put32(at(g+4),500);put32(at(g+8),side);put32(at(g+12),side);
            put32(at(g+16),u32(-1000));put32(at(g+20),u32(-1000));
            put32(at(g+24),g==grid?refs:empty_refs);put32(at(g+28),tri);
        }
        put32(at(empty),0xFFFFFFFFu);put32(at(list),tri);put32(at(list+4),0xFFFFFFFFu);
        for(unsigned i=0;i<side*side;++i){
            put32(at(refs+4*i),floor&&i==2*side+3?list:empty);
            put32(at(empty_refs+4*i),empty);
        }
        place(tri+4,{-1000,0,-1000});place(tri+16,{-1000,0,18000});place(tri+28,{9000,0,-1000});
        put_float(at(tri+44),1.f);
    }
    void initial_pools(){
        put32(at(0x80436380),descriptor_row);
        put32(at(descriptor_row+12),descriptors);put16(at(descriptor_row+18),2);
        put32(at(descriptors),descriptors+0x80);
        put32(at(free_head+12),carrier);put32(at(free_tail+12),carrier);put16(at(free_count+6),1);
        at(carrier)[2]=3;
    }
    void offer_shapes(unsigned slots){
        if(slots>2||half(free_count+8)||word(free_head+16))fail("sweep route shape inlet extent");
        put32(at(free_head+16),slots?shape:0);
        put32(at(free_tail+16),slots==2?second_shape:slots?shape:0);
        put16(at(free_count+8),u16(slots));
        if(slots){
            at(shape)[2]=4;put32(at(shape+4),slots==2?second_shape:0);
        }
        if(slots==2)at(second_shape)[2]=4;
    }
    void offer_children(Case rule,unsigned slots){
        if(half(free_count+6)||half(descriptor_row+18)||word(free_head+12))fail("sweep route inlet was not empty");
        put32(at(free_head+12),first_child);
        put32(at(free_tail+12),rule.objects==2?second_child:first_child);
        put16(at(free_count+6),u16(rule.objects));
        at(first_child)[2]=3;at(second_child)[2]=3;
        put32(at(first_child+4),rule.objects==2?second_child:0);
        if(rule.extra_descriptors){
            put32(at(descriptor_row+12),descriptors+0x100);
            put32(at(descriptors+0x100),descriptors+0x180);
            put16(at(descriptor_row+18),u16(rule.extra_descriptors));
        }
        offer_shapes(slots);
    }
    void pose(){
        CPUState late{};late.gpr[3]=carrier;
        mod.run(late,ram,0x80047AA4);
    }
    void publication(){
        for(u32 entry:{0x80089554u,0x8008A28Cu,0x80089AECu}){
            CPUState pass{};
            mod.run(pass,ram,entry,1024);
        }
    }
    void followup(Pilot p,const Followup& inlet,const Point& impact){
        const bool has_segment=inlet.scenario!=3;
        const unsigned slots=inlet.scenario==3?1:inlet.scenario;
        const bool segment_bound=has_segment&&slots;
        const bool area_bound=slots>unsigned(segment_bound);
        const auto area_shape=segment_bound?second_shape:shape;
        const auto area=first_child,segment=second_child;
        const Point body_center{impact[0],impact[1]+22.f,impact[2]};
        put32(at(0x803C4E84),actor);
        for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
        contact::bind_sasuke(mod,ram,body_center,inlet.body,inlet.attacker,inlet.defender,2000);
        std::memcpy(at(common),inlet.bank.data(),inlet.bank.size());
        put32(at(contact::scene+152),common);
        offer_shapes(slots);

        const auto advance=[&]{
            CPUState tick{};tick.gpr[3]=8;
            CallObservation request{make_segment};
            mod.run_observed(tick,ram,0x80089A94,request,4096);
            if(request.count!=1||request.caller!=p.segment_site||request.arguments[0]!=carrier||
               request.arguments[3]!=p.group||half(actor+0x774)!=5)
                fail("sweep follow-up lost the original bucket's segment request");
        };
        advance();
        if(at(area)[0x10]!=5||at(area)[0x11]!=26||at(area)[0x18]!=(area_bound?1:2)||
           word(area+0x8C)!=carrier||word(area+0x90)!=actor||point(area+0x20)!=impact||
           (has_segment?(at(segment)[0x10]!=11||at(segment)[0x11]!=p.group||
                         at(segment)[0x18]!=(segment_bound?1:2)||word(segment+0x8C)!=carrier||
                         word(segment+0x90)!=actor||word(carrier+8)!=segment||word(segment+8)!=area)
                       :(at(segment)[0]||word(carrier+8)!=area))||half(free_count+6)||half(descriptor_row+18)||
           half(free_count+8)||word(active_head+49*4)!=(area_bound?area_shape:segment_bound?shape:0))
            fail("sweep follow-up same-pass bind order, resources or credit changed");
        if(segment_bound&&(word(shape+0x20)!=segment||word(shape+0x24)!=actor||
                           word(shape+0x2C)!=p.table||word(shape+0x28)!=hit_block+0xAA0+p.descriptor*80))
            fail("sweep follow-up segment did not bind the first shape and exact owner queue");
        if(area_bound&&(word(area_shape+0x20)!=area||word(area_shape+0x24)!=actor||
                        word(area_shape+0x28)!=common+0x400+4*80||word(area_shape+0x2C)!=0x802C4760))
            fail("sweep follow-up area did not bind the separate common queue");

        const auto near=point(carrier+0x150);
        publication();
        const auto opening_count=half(attack_count);
        const unsigned area_token=area_bound?half(area+0x26C):0,segment_token=segment_bound?half(segment+0x26C):0;
        const auto initial_radius=area_bound?scalar(area_shape+0x7C):0.f;
        if(opening_count!=unsigned(segment_bound)||(segment_bound&&(word(attack_registry)!=shape||point(shape+0x64)!=near))||
           (area_bound&&(initial_radius<3.f||initial_radius>5.f||point(area_shape+0x64)!=impact))||
           half(contact::receiver_count)!=1||word(contact::receiver_registry)!=contact::body||
           point(contact::body+0x64)!=body_center||half(contact::receiver+0x20C))
            fail("sweep follow-up opening attack radius, position or registry mismatch");
        contact::pair_hp(mod,ram,0,0,false);
        advance();
        publication();
        const auto count=half(attack_count);
        const auto grown_radius=area_bound?scalar(area_shape+0x7C):0.f;
        if(count!=unsigned(segment_bound)+unsigned(area_bound)||
           (area_bound&&(word(attack_registry)!=area_shape||half(area+0x26C)!=area_token||
                         point(area_shape+0x64)!=impact||grown_radius<=initial_radius))||
           (segment_bound&&(word(attack_registry+4*unsigned(area_bound))!=shape||half(segment+0x26C)!=segment_token||
                            point(shape+0x3C)!=near||point(shape+0x30)!=point(carrier+0x15C)))||
           (segment_bound&&area_bound&&segment_token==area_token))
            fail("sweep follow-up sibling register order, endpoint or token changed");
        unsigned area_loss=0,segment_loss=0;
        if(area_bound){
            area_loss=contact::pair_hp(mod,ram,area,0x802C4778,true);
            if(half(contact::receiver+0x1C6)!=180-area_loss||half(contact::receiver+0x1C8)!=180||
               word(contact::receiver+0x290)!=area||word(area+0x290)!=contact::receiver||
               word(actor+0x418)!=area_loss||half(contact::receiver+0x20C)!=area_token||
               (segment_bound&&at(segment)[0x1D9]))
                fail("sweep follow-up area write reached the wrong HP or token");
        }
        if(segment_bound){
            segment_loss=contact::pair_hp(mod,ram,segment,p.table+p.index*24,true);
            if(half(contact::receiver+0x1C8)!=180-area_loss||word(contact::receiver+0x290)!=segment||
               word(segment+0x290)!=contact::receiver||at(segment)[0x1D9]!=16)
                fail("sweep follow-up segment write did not follow the separate area transaction");
        }
        contact::pair_hp(mod,ram,0,0,false);
        const unsigned hp=half(contact::receiver+0x1C6),total=area_loss+segment_loss;
        if(hp!=180-total||word(actor+0x418)!=total||word(contact::receiver+0x41C)!=total||
           at(contact::receiver)[0x26E]!=count||
           half(contact::receiver+0x20C)!=(area_bound?area_token:segment_token)||
           half(contact::receiver+0x20E)!=(area_bound?segment_token:0)||
           word(carrier+0x290)||at(carrier)[0x1D9])
            fail("sweep follow-up confused sibling damage, ownership or per-token history");
        std::printf("sweep-followup: %u %u %u %u %u %u %u %u %u %u %u %.5f %.5f\n",p.model,inlet.scenario,
                    unsigned(segment_bound),unsigned(area_bound),opening_count,count,segment_token,area_token,
                    segment_loss,area_loss,hp,double(initial_radius),double(grown_radius));
    }
public:
    Check(const Module& m,const std::vector<u8>& input):mod(m),ram(input){}
    void observe(Pilot p,const HitBlock& block,unsigned scenario,const Followup* sibling=nullptr){
        const auto rule=cases[scenario];
        terrain(rule.floor);
        initial_pools();
        auto* owner=at(actor);
        owner[0]=1;owner[0x18]=1;
        put16(owner+0x3E8,p.model);put16(owner+0x94,p.model);put16(owner+0x774,5);
        put32(owner+0x90,actor);put32(owner+0x27C,p.table);put32(owner+0x3FC,hit_block);
        for(unsigned off:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+off,1.f);
        std::memcpy(at(hit_block),block.data(),block.size());
        const auto joint=actor+0x8D4+48*p.socket;
        for(unsigned axis=0;axis<3;++axis)put_float(at(joint+20*axis),1.f);
        put_float(at(joint+12),100.f);put_float(at(joint+28),200.f);put_float(at(joint+44),300.f);
        if(p.model==0x0E03){
            // This row uses a declared downward socket so its probe crosses the same floor.
            constexpr float s=0.2588190451f,c=0.9659258263f;
            put_float(at(joint+20),c);put_float(at(joint+24),-s);
            put_float(at(joint+36),s);put_float(at(joint+40),c);
        }

        // Empty visual records for parent row zero, and no sound playback.
        constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400,audio=0x8103E000;
        put32(at(0x803BB4C0+0x600),visual);put32(at(visual+16),link);put32(at(link),table);
        for(unsigned id:{1u,2u,3u,4u,18u,22u})put32(at(table+4*id),empty);
        put32(at(0x803BB374),visual);
        put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;at(0x803D0000+15080+852)[0]=1;
        CPUState ctor{};ctor.gpr[3]=actor;
        if(p.ctor==0x801F2BF8)ctor.gpr[5]=p.row;else ctor.gpr[4]=p.row;
        mod.run(ctor,ram,p.ctor);
        if(ctor.gpr[3]!=carrier||word(carrier+0xC)!=p.update||word(carrier+0x8C)!=actor||
           word(carrier+0x90)!=actor||at(carrier)[0x13]!=p.row||at(carrier)[0x18]||
           word(active_head+9*4)!=carrier||half(free_count+6)||half(descriptor_row+18))
            fail("sweep route constructor or parent row mismatch");
        unsigned visits=0;
        do{
            CPUState tick{};tick.gpr[3]=carrier;
            CallObservation premature{make_area};
            mod.run_observed(tick,ram,p.update,premature,4096);
            if(premature.count||word(carrier+8)||word(active_head+9*4)!=carrier||at(carrier)[0x18]!=1||
               half(actor+0x774)!=5||++visits>64)
                fail("sweep route did not reach the prepared terrain phase");
            pose();
        }while(at(carrier)[0x19]!=2||scalar(carrier+0xD4)>(p.model==0x0E03?1.f:0.f));
        if(at(carrier)[0x89]!=p.socket||!at(carrier)[0x168]||half(descriptor_row+18)||word(active_head+49*4))
            fail("sweep route crossing did not start from original phase and socket");
        offer_children(rule,sibling?0:1);
        const auto before=ram;
        CPUState terrain_call{};terrain_call.gpr[3]=carrier;
        mod.run_to(terrain_call,ram,p.update,{terrain_query});
        const auto probe_far=point(terrain_call.gpr[3]);
        const auto probe_near=point(terrain_call.gpr[4]);
        if(terrain_call.lr!=(p.model==0x0E03?0x801F3D3C:0x801F3260)||
           probe_far[2]!=6000.f||probe_near[1]<=0||probe_far[1]>=0)
            fail("sweep route did not clip its terrain probe to the declared stage extent");
        std::memcpy(ram.data(),before.data(),ram.size());
        CPUState probe{};probe.gpr[3]=carrier;
        CallObservation area_request{make_area};
        const auto cut=mod.run_to_or_return(probe,ram,p.update,{make_area},&area_request);
        if(cut!=(rule.floor?make_area:0)||area_request.count!=unsigned(rule.floor)||
           (rule.floor&&(area_request.caller!=p.area_site||area_request.arguments[0]!=carrier||
                        area_request.arguments[2]!=26)))
            fail("sweep route did not obtain key 26 from the real terrain crossing");
        const auto impact=rule.floor?point(area_request.arguments[1]):Point{};
        if(rule.floor&&std::abs(impact[1])>0.001f)
            fail("sweep route terrain point was not on the declared triangle");
        std::memcpy(ram.data(),before.data(),ram.size());
        if(sibling){
            followup(p,*sibling,impact);
            return;
        }

        CPUState live{};live.gpr[3]=carrier;
        CallObservation segment_request{make_segment};
        mod.run_observed(live,ram,p.update,segment_request,4096);
        const auto near=point(carrier+0x150),far=point(carrier+0x15C);
        const bool area_made=rule.floor&&rule.extra_descriptors>=2,segment_made=rule.objects>unsigned(area_made);
        const auto segment=area_made?second_child:first_child;
        if(near[1]<=0||far[1]>=0||far[2]<=6000.f||segment_request.count!=1||segment_request.caller!=p.segment_site||
           segment_request.arguments[0]!=carrier||segment_request.arguments[1]!=carrier+0x150||
           segment_request.arguments[2]!=carrier+0x15C||segment_request.arguments[3]!=p.group||
           at(carrier)[0x18]!=1||at(carrier)[0x19]!=3||half(actor+0x774)!=5||
           half(descriptor_row+18)!=rule.extra_descriptors-2*unsigned(area_made)||
           half(free_count+6)!=rule.objects-unsigned(area_made)-unsigned(segment_made)||
           word(carrier+8)!=(segment_made?segment:area_made?first_child:0)||
           (area_made&&segment_made&&word(segment+8)!=first_child)){
            std::fprintf(stderr,"sweep model=%04X case=%u prep=%u segment=%u@%08X %08X/%08X/%08X/%u "
                         "phase=%u/%u descriptors=%u cells=%u child=%08X types=%u/%u\n",
                         p.model,scenario,visits,segment_request.count,segment_request.caller,
                         segment_request.arguments[0],segment_request.arguments[1],segment_request.arguments[2],
                         segment_request.arguments[3],at(carrier)[0x18],at(carrier)[0x19],half(descriptor_row+18),
                         half(free_count+6),word(carrier+8),at(first_child)[0x10],at(second_child)[0x10]);
            std::fprintf(stderr,"sweep near=%g/%g/%g far=%g/%g/%g\n",
                         double(near[0]),double(near[1]),double(near[2]),double(far[0]),double(far[1]),double(far[2]));
            fail("sweep route did not keep both requests or exact pool order");
        }
        if(area_made&&(at(first_child)[0x10]!=5||at(first_child)[0x11]!=26||
                       word(first_child+0x8C)!=carrier||word(first_child+0x90)!=actor||
                       point(first_child+0x20)!=impact))
            fail("sweep route area did not retain its original crossing point or credit");
        if(segment_made){
            if(at(segment)[0x10]!=11||at(segment)[0x11]!=p.group||word(segment+0x8C)!=carrier||
               word(segment+0x90)!=actor||word(segment+0x144)!=carrier+0x150||word(segment+0x148)!=carrier+0x15C)
                fail("sweep route segment lost independently borrowed endpoints");
            CPUState opening{};opening.gpr[3]=segment;
            CallObservation group{bind};
            mod.run_observed(opening,ram,segment_update,group);
            if(group.count!=1||group.caller!=0x80139BE0||group.arguments[0]!=segment||group.arguments[1]!=p.group||
               at(segment)[0x18]!=1||point(segment+0x20)!=near||word(active_head+49*4)!=shape||
               word(shape+0x20)!=segment||word(shape+0x24)!=actor||word(shape+0x2C)!=p.table||
               word(shape+0x28)!=hit_block+0xAA0+80*p.descriptor||at(shape)[0x11]!=p.index)
                fail("sweep route segment did not bind the normal credited queue");
        }
        std::printf("sweep-area: %u %u %u %u %u %u %u",p.model,scenario,visits+1,unsigned(rule.floor),unsigned(area_made),
                    unsigned(segment_made),p.group);
        for(const auto& xyz:{impact,probe_near,probe_far,near,far})
            std::printf(" %.5f %.5f %.5f",double(xyz[0]),double(xyz[1]),double(xyz[2]));
        std::printf("\n");
    }
};
}

int main(int argc,char** argv){
    const bool follow=argc==4&&!std::strcmp(argv[3],"--followup");
    if(argc!=3&&!follow)fail("usage: sweep-area-routes-fixture qualified.dol unmodified-recomp.so [--followup]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    std::array<HitBlock,std::size(pilots)> hit{};
    for(auto& block:hit)
        if(std::fread(block.data(),1,block.size(),stdin)!=block.size())fail("sweep route hit block extent");
    if(follow){
        HitBlock body{};
        std::array<contact::ActorSpec,std::size(pilots)> attacker{};
        contact::ActorSpec defender{};
        CommonBank bank{};
        if(std::fread(body.data(),1,body.size(),stdin)!=body.size())fail("sweep follow-up body block extent");
        for(auto& spec:attacker)
            if(std::fread(spec.data(),1,spec.size(),stdin)!=spec.size())fail("sweep follow-up attacker spec extent");
        if(std::fread(defender.data(),1,defender.size(),stdin)!=defender.size()||
           std::fread(bank.data(),1,bank.size(),stdin)!=bank.size()||std::getc(stdin)!=EOF)
            fail("sweep follow-up common bank or defender extent");
        for(unsigned p=0;p<hit.size();++p)
            for(unsigned i=0;i<4;++i){
                Check test(mod,original);
                const Followup inlet{i,body,attacker[p],defender,bank};
                test.observe(pilots[p],hit[p],i==3?2:1,&inlet);
            }
        return 0;
    }
    if(std::getc(stdin)!=EOF)fail("unexpected sweep route payload");
    for(unsigned p=0;p<hit.size();++p)
        for(unsigned i=0;i<std::size(cases);++i){
            Check test(mod,original);
            test.observe(pilots[p],hit[p],i);
        }
}
