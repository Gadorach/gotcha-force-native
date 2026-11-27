// Original type-32 carrier paths and type-11 handoff.
#include "retail_stationary_contact.hpp"
#include <array>
#include <cmath>

namespace {
using namespace retail_fixture;
namespace contact_replay=retail_stationary_contact;
constexpr u32 carrier=0x81030000,segment=0x81030800,shape=0x81032000;
constexpr u32 descriptors=0x81033000,descriptor_row=0x81033800;
constexpr u32 hit_block=0x81034000,scratch_header=0x81036000,scratch=0x81036100;
constexpr u32 stage=0x81050000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 factory=0x801DEB68,update=0x801DEC90,active=0x801DEF80,scratch_alloc=0x80006DC8;
constexpr u32 pose=0x80048288,bind=0x8008AC80,segment_factory=0x80139A6C,segment_update=0x80139B80;
constexpr u32 shared_factory=0x800E19A8,shared_update=0x800E1AB4;
constexpr u32 segment_transform=0x80139CC0,attack_count=0x8043B5A0-21328,attack_registry=0x803C477C;
using Block=std::array<u8,7840>;
using contact_replay::ActorSpec;
using contact_replay::receiver;
using contact_replay::body;
using contact_replay::receiver_count;
using contact_replay::receiver_registry;
using Point=std::array<float,3>;
struct Pilot {u16 model;u8 key,index;u32 table,first,second;};
constexpr Pilot pilots[]={{0x0D03,8,1,0x80362C28,actor+0x15C,actor+0x168},
                          {0x0E02,9,6,0x8038DAE0,actor+0x144,actor+0x150}};
enum class Case {scratch_empty,shape_empty,shape_available,segment_empty};
struct SharedPilot {u16 model;u8 key,socket,group,step,index,radius;u32 table;Point offset;};
constexpr SharedPilot shared_pilots[]={
    {0x0104,1,1,10,9,9,30,0x803214E0,{0,25,350}},
    {0x0505,16,3,1,1,1,50,0x803928D8,{0,0,200}},
    {0x0511,19,3,1,1,1,50,0x80392908,{0,0,100}},
    {0x0615,15,1,27,34,31,20,0x80365EC0,{0,40,10}},
};
enum class AimCase {none,moving_copy,outside_gate,half_rate};
enum class ContactCase {clear_hit,clear_miss,wall_hit,wall_miss};
struct Contact {
    ContactCase which;unsigned owner_hp;const Block& hit;const ActorSpec& owner;const ActorSpec& target;
};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){
        if(va<GC_RAM_BASE||va-GC_RAM_BASE>ram.size()-4)fail("type-32 scratch extent");
        return ram.data()+va-GC_RAM_BASE;
    }
    u32 word(u32 va){return be32(at(va));}
    u16 half(u32 va){const auto* p=at(va);return u16(u16(p[0])<<8|p[1]);}
    float scalar(u32 va){return std::bit_cast<float>(word(va));}
    Point get(u32 va){return {std::bit_cast<float>(word(va)),std::bit_cast<float>(word(va+4)),std::bit_cast<float>(word(va+8))};}
    void place(u32 va,const Point& point){
        for(unsigned i=0;i<point.size();++i)put_float(at(va+4*i),point[i]);
    }
    void pools(bool spare,bool shape_free,unsigned count=4){
        if(count>6)fail("type-32 descriptor extent");
        put32(at(0x80436380),descriptor_row);
        put16(at(descriptor_row+18),u16(count));
        put32(at(descriptor_row+12),descriptors);
        for(unsigned i=0;i<count;++i)put32(at(descriptors+0x80*i),i+1<count?descriptors+0x80*(i+1):0);
        put32(at(free_head+12),carrier);
        put32(at(free_tail+12),spare?segment:carrier);
        put16(at(free_count+6),spare?2:1);
        at(carrier)[2]=3;at(segment)[2]=3;
        put32(at(carrier+4),spare?segment:0);
        put16(at(free_count+8),unsigned(shape_free));
        put32(at(free_head+16),shape_free?shape:0);put32(at(free_tail+16),shape_free?shape:0);
        if(shape_free)at(shape)[2]=4;
    }
    void terrain(){
        constexpr u32 grid=stage+0x100,refs=stage+0x200,side=16,empty=refs+4*side*side;
        put32(at(0x8043B5A0-21316),stage);
        for(unsigned i=0;i<3;++i)put32(at(stage+28+4*i),grid);
        put_float(at(stage+52),4000.f);put_float(at(stage+56),3000.f);put_float(at(stage+60),-1000.f);
        put_float(at(stage+64),3000.f);put_float(at(stage+68),-1000.f);
        put32(at(grid),500);put32(at(grid+4),500);put32(at(grid+8),side);put32(at(grid+12),side);
        put32(at(grid+16),u32(-1000));put32(at(grid+20),u32(-1000));put32(at(grid+24),refs);
        put32(at(empty),0xFFFFFFFFu);
        for(unsigned i=0;i<side*side;++i)put32(at(refs+4*i),empty);
    }
    void shape_identity(u32 object,u32 table,unsigned group,unsigned step,unsigned index){
        if(word(active_head+49*4)!=shape||half(free_count+8)||
           word(shape+0x20)!=object||word(shape+0x24)!=actor||word(shape+0x2C)!=table||
           word(shape+0x28)!=hit_block+0xAA0+80*step||at(shape)[0x14]!=group||at(shape)[0x12]!=step||
           at(shape)[0x11]!=index)
            fail("type-32 shape used the wrong object, queue or credit");
    }
    void segment_publication(){
        CPUState late{};late.gpr[3]=segment;
        mod.run(late,ram,segment_transform);
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState pass{};
            mod.run(pass,ram,entry,1024);
        }
    }
    void contact_stage(bool wall){
        // A single rebased triangle in stage layer zero, or an empty list.
        constexpr u32 grid=stage+0x100,empty_grid=stage+0x140,refs=stage+0x200,side=48;
        constexpr u32 empty_refs=refs+side*side*4,list=empty_refs+side*side*4,tri=list+8,empty=tri+0x80;
        put32(at(0x8043B5A0-21316),stage);
        put32(at(stage+28),grid);put32(at(stage+32),empty_grid);put32(at(stage+36),empty_grid);
        put_float(at(stage+52),9000.f);put_float(at(stage+56),20000.f);put_float(at(stage+60),-1000.f);
        put_float(at(stage+64),20000.f);put_float(at(stage+68),-1000.f);
        for(u32 g:{grid,empty_grid}){
            put32(at(g),500);put32(at(g+4),500);put32(at(g+8),side);put32(at(g+12),side);
            put32(at(g+16),u32(-1000));put32(at(g+20),u32(-1000));
            put32(at(g+24),g==grid?refs:empty_refs);put32(at(g+28),tri);
        }
        put32(at(empty),0xFFFFFFFFu);put32(at(list),tri);put32(at(list+4),0xFFFFFFFFu);
        for(unsigned i=0;i<side*side;++i){
            put32(at(refs+4*i),wall?list:empty);
            put32(at(empty_refs+4*i),empty);
        }
        place(tri+4,{-500,-500,1000});place(tri+16,{-500,3500,1000});place(tri+28,{3500,-500,1000});
        put_float(at(tri+48),-1.f);put_float(at(tri+52),1000.f);
    }
    void verify_terrain(const Point& first,const Point& far,bool wall){
        // Independent original terrain query on the same two points.
        constexpr u32 pair=0x8105F000;
        place(pair,first);place(pair+12,far);
        CPUState reader{};reader.gpr[3]=pair;reader.gpr[4]=pair+12;reader.gpr[5]=pair+24;
        mod.run(reader,ram,0x8003E978,4096);
        if(reader.gpr[3]!=unsigned(wall)||(wall&&get(pair+24)!=Point{first[0],first[1],1000}))
            fail("type-32 segment wall control did not span the declared stage triangle");
    }
    void segment_contact(SharedPilot pilot,const Contact& inlet,const Point& first,const Point& far,float radius){
        const bool hit=inlet.which==ContactCase::clear_hit||inlet.which==ContactCase::wall_hit;
        const bool wall=inlet.which==ContactCase::wall_hit||inlet.which==ContactCase::wall_miss;
        const float offset=radius+22.f+(hit?-1.f:1.f);
        const Point center{first[0]+offset,first[1],(first[2]+far[2])*0.5f};
        if(first[0]!=far[0]||first[1]!=far[1]||first[2]>=1000.f||center[2]<=1000.f)
            fail("type-32 segment did not span the fixture's receiver side");
        verify_terrain(first,far,wall);
        contact_replay::bind_sasuke(mod,ram,center,inlet.hit,inlet.owner,inlet.target,inlet.owner_hp);
        segment_publication();
        if(half(attack_count)!=1||word(attack_registry)!=shape||half(receiver_count)!=1||word(receiver_registry)!=body||
           get(shape+0x3C)!=first||get(shape+0x30)!=far||std::abs(scalar(shape+0x88)-radius)>0.001f||
           std::abs(scalar(body+0x64)-center[0])>0.001f||scalar(body+0x68)!=center[1]||
           scalar(body+0x6C)!=center[2]||scalar(body+0x7C)!=22.f||at(segment)[0x1D9]||at(carrier)[0x1D9]||
           !half(segment+0x26C)||half(receiver+0x20C))
            fail("type-32 published segment did not preserve sweep, credit or receiver inlet");
        const auto record=pilot.table+24*pilot.index;
        const auto damage=contact_replay::pair_hp(mod,ram,segment,record,hit);
        const auto hp=half(receiver+0x1C6),token=half(receiver+0x20C);
        const unsigned flags=at(segment)[0x1D9];
        if(hp!=180-damage||half(actor+0x1C6)!=inlet.owner_hp||half(actor+0x774)!=5||
           half(receiver+0x1C8)!=(hit?180:0xFFFF)||word(actor+0x418)!=damage||word(receiver+0x41C)!=damage||
           word(receiver+0x290)!=(hit?segment:0)||word(segment+0x290)!=(hit?receiver:0)||
           word(actor+0x290)!=(hit?receiver:0)||word(carrier+0x290)||at(carrier)[0x1D9]||
           token!=(hit?half(segment+0x26C):0)||at(receiver)[0x26E]!=unsigned(hit)||flags!=(hit?16:0))
            fail("type-32 segment credit, HP, or contact ownership disagreed");
        if(hit&&(contact_replay::pair_hp(mod,ram,segment,record,false)||half(receiver+0x1C6)!=hp||
                 word(actor+0x418)!=damage||word(receiver+0x41C)!=damage||at(receiver)[0x26E]!=1))
            fail("type-32 duplicate segment token wrote HP again");

        CPUState unlink{};unlink.gpr[3]=segment;
        mod.run(unlink,ram,segment_update);
        segment_publication();
        if(at(segment)[0x18]!=3||word(active_head+49*4)||half(free_count+8)!=1||half(attack_count))
            fail("type-32 segment shape did not unlink after its sweep");
        CPUState retire{};retire.gpr[3]=segment;
        mod.run(retire,ram,segment_update);
        if(at(segment)[0]||word(carrier+8)||!at(carrier)[0]||half(free_count+6)!=1||half(descriptor_row+18))
            fail("type-32 segment did not retire separately from the beam carrier");
        std::printf("type32-contact: %u %u %u %u %u %u %u %u %.5f %.5f %.5f %.5f %.5f\n",
                    pilot.model,pilot.key,unsigned(inlet.which),damage,hp,token,flags,unsigned(wall),
                    double(radius),double(offset),double(first[2]),double(far[2]),double(center[2]));
    }
public:
    Check(const Module& m,const std::vector<u8>& input):mod(m),ram(input){}
    void observe(Pilot pilot,const Block& block,Case which){
        const bool direct=pilot.key==8,scratch_free=which!=Case::scratch_empty;
        const bool shape_free=which==Case::shape_available||which==Case::segment_empty;
        const bool spare=which!=Case::segment_empty;
        pools(spare,shape_free);
        auto* owner=at(actor);
        owner[0]=1;owner[0x18]=1;owner[0x6F7]=2;
        put16(owner+0x3E8,pilot.model);put16(owner+0x94,pilot.model);put16(owner+0x774,5);
        put32(owner+0x90,actor);put32(owner+0x27C,pilot.table);put32(owner+0x3FC,hit_block);
        for(unsigned at:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+at,1.f);
        std::memcpy(at(hit_block),block.data(),block.size());
        constexpr Point first{100,200,300},second{220,240,380},moved{250,270,410};
        place(pilot.first,first);place(pilot.second,second);

        CPUState ctor{};ctor.gpr[3]=actor;ctor.gpr[4]=pilot.key;
        ctor.gpr[5]=pilot.first;ctor.gpr[6]=pilot.second;ctor.gpr[7]=actor+0x6F7;
        if(scratch_free){
            mod.run_to(ctor,ram,factory,{scratch_alloc});
            if(ctor.lr!=0x801DEC68||ctor.gpr[3]!=64||word(carrier+0xDC)||at(carrier)[0x18])
                fail("type-32 did not reach the original scratch request boundary");
            // This is an explicit scratch-service inlet, not an observed allocation.
            put32(at(scratch_header+12),scratch);put32(at(carrier+0xDC),scratch_header);
        }else{
            CallObservation request{scratch_alloc};
            mod.run_observed(ctor,ram,factory,request);
            if(request.count!=1||request.caller!=0x801DEC64||request.arguments[0]!=64||
               ctor.gpr[3]||word(carrier+0xDC)||at(carrier)[0x18]!=2)
                fail("type-32 original scratch refusal did not stay a distinct boundary");
        }
        if(word(active_head+9*4)!=carrier||at(carrier)[0x10]!=32||at(carrier)[0x11]!=pilot.key||
           word(carrier+0xC)!=update||word(carrier+0x8C)!=actor||word(carrier+0x90)!=actor||
           word(carrier+0x18C)!=pilot.first||word(carrier+0x190)!=pilot.second||
           word(carrier+0x19C)!=actor+0x6F7||half(descriptor_row+18)||word(active_head+49*4)||half(actor+0x774)!=5)
            fail("type-32 carrier identity or borrowed-address inlet mismatch");
        if(!scratch_free){
            std::printf("type32-branch: %u %u %u 0 0 0 0 0 0 0 0\n",pilot.model,pilot.key,unsigned(which));
            return;
        }

        CPUState init{};init.gpr[3]=carrier;
        CallObservation opening{bind};
        const auto frontier=mod.run_to_or_return(init,ram,update,{pose},&opening);
        const bool parent_bound=direct&&shape_free,admitted=!direct||parent_bound;
        if(opening.count!=unsigned(direct)||(direct&&(opening.caller!=0x801DED2C||
           opening.arguments[0]!=carrier||opening.arguments[1]!=1))||frontier!=(admitted?pose:0)||
           (frontier&&(init.lr!=0x801DEE28||init.gpr[3]!=carrier))||
           at(carrier)[0x18]!=(admitted?1:2)||bool(word(active_head+49*4))!=parent_bound)
            fail("type-32 opening mode did not separate carrier bind and bypass");
        if(parent_bound)shape_identity(carrier,pilot.table,1,1,pilot.index);
        bool child_created=false,child_bound=false,endpoint_refresh=false;
        unsigned requests=0;
        if(admitted){
            // The initialization proof stops before renderer work. Start
            // the original active callback with a declared phase-one inlet.
            at(carrier)[0x19]=1;
            put16(at(carrier+0x1A4),2);
            terrain();
            CPUState live{};live.gpr[3]=carrier;
            CallObservation request{segment_factory};
            mod.run_observed(live,ram,active,request,4096);
            requests=request.count;
            child_created=!direct&&spare;
            if(requests!=unsigned(!direct)||word(carrier+8)!=(child_created?segment:0)||
               half(free_count+6)!=unsigned(spare)-unsigned(child_created)||
               (requests&&(request.caller!=0x801DF2B0||request.arguments[0]!=carrier||
                            request.arguments[1]!=carrier+0x174||request.arguments[2]!=carrier+0x180||
                            request.arguments[3]!=1))||get(carrier+0x174)!=first||get(carrier+0x180)!=second)
                fail("type-32 active path requested the wrong endpoint segment");
            if(direct&&half(carrier+0x272)!=0)fail("type-32 carrier rule did not open its attack mask");
            if(child_created){
                const auto* child=at(segment);
                if(child[0x10]!=11||child[0x11]!=1||word(segment+0xC)!=segment_update||child[0x18]||
                   word(segment+0x8C)!=carrier||word(segment+0x90)!=actor||
                   word(segment+0x144)!=carrier+0x174||word(segment+0x148)!=carrier+0x180)
                    fail("type-11 did not inherit the exact endpoint addresses and owner");
                CPUState segment_init{};segment_init.gpr[3]=segment;
                CallObservation group{bind};
                mod.run_observed(segment_init,ram,segment_update,group);
                child_bound=shape_free;
                if(group.count!=1||group.caller!=0x80139BE0||group.arguments[0]!=segment||group.arguments[1]!=1||
                   child[0x18]!=(child_bound?1:2)||bool(word(active_head+49*4))!=child_bound)
                    fail("type-11 opening bind or refusal mismatch");
                if(child_bound){
                    shape_identity(segment,pilot.table,1,1,pilot.index);
                    if(get(segment+0x20)!=first)fail("type-11 did not sample first endpoint");
                    place(carrier+0x180,moved);
                    CPUState next{};next.gpr[3]=segment;
                    mod.run(next,ram,segment_update);
                    if(child[0x18]!=2||get(segment+0x20)!=moved)
                        fail("type-11 did not sample the updated second endpoint");
                    endpoint_refresh=true;
                }
            }
        }
        if(half(actor+0x774)!=5||half(descriptor_row+18))
            fail("type-32 bounded callbacks changed stock or descriptor ownership");
        std::printf("type32-branch: %u %u %u %u %u %u %u %u %u %u %u\n",pilot.model,pilot.key,unsigned(which),
                    unsigned(scratch_free),opening.count,unsigned(parent_bound),requests,unsigned(child_created),
                    unsigned(child_bound),unsigned(endpoint_refresh),unsigned(admitted));
    }
    void shared(SharedPilot pilot,const Block& block,AimCase which,const Contact* contact=nullptr){
        constexpr u32 original_target=0x8101C000,replacement_target=0x8101E000;
        const bool has_target=which==AimCase::moving_copy||which==AimCase::outside_gate,half_rate=which==AimCase::half_rate;
        const float rate=half_rate?0.5f:1.f;
        pools(true,true,6);
        auto* owner=at(actor);
        owner[0]=1;owner[0x18]=1;owner[0x6F7]=2;
        put16(owner+0x3E8,pilot.model);put16(owner+0x94,pilot.model);put16(owner+0x774,5);
        put32(owner+0x90,actor);put32(owner+0x27C,pilot.table);put32(owner+0x3FC,hit_block);
        put32(owner+0xC8,has_target?original_target:0);
        for(unsigned off:{0xB4,0xB8,0xC0,0xC4})put_float(owner+off,1.f);
        put_float(owner+0x1DC8,rate);
        std::memcpy(at(hit_block),block.data(),block.size());
        if(contact){
            contact_stage(contact->which==ContactCase::wall_hit||contact->which==ContactCase::wall_miss);
            put32(at(0x803C4E84),actor);
            for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
        }
        auto* matrix=owner+0x8D4+48*pilot.socket;
        for(unsigned i=0;i<3;++i)put_float(matrix+20*i,1.f);
        put_float(matrix+12,100.f);put_float(matrix+28,200.f);put_float(matrix+44,300.f);
        const Point origin{100.f+pilot.offset[0],200.f+pilot.offset[1],300.f+pilot.offset[2]};
        place(original_target+100,{origin[0]+300.f,origin[1],origin[2]+(which==AimCase::outside_gate?-2000.f:2000.f)});
        place(replacement_target+100,{origin[0]+300.f,origin[1],origin[2]+2000.f});

        // Empty common model record and unavailable audio backend.
        constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400,audio=0x8103E000;
        put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);
        put32(at(table+4*84),empty);
        put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
        at(0x803D0000+15080+852)[0]=1;

        CPUState ctor{};ctor.gpr[3]=actor;ctor.gpr[4]=pilot.key;ctor.gpr[5]=actor+0x6F7;
        mod.run(ctor,ram,shared_factory);
        if(ctor.gpr[3]!=carrier||word(active_head+9*4)!=carrier||at(carrier)[0x10]!=32||at(carrier)[0x11]!=pilot.key||
           word(carrier+0xC)!=shared_update||word(carrier+0x154)!=actor+0x6F7||
           word(carrier+0xC8)!=(has_target?original_target:0)||half(descriptor_row+18))
            fail("type-32 shared constructor inlet mismatch");
        CPUState init{};init.gpr[3]=carrier;
        CallObservation scratch_request{scratch_alloc};
        mod.run_observed(init,ram,shared_update,scratch_request,1024);
        const bool needs_scratch=pilot.key!=1;
        if(at(carrier)[0x18]!=1||at(carrier)[0x89]!=pilot.socket||half(carrier+0x14C)!=pilot.group||
           get(carrier+0x64)!=pilot.offset||scratch_request.count!=unsigned(needs_scratch)||
           (needs_scratch&&(scratch_request.caller!=0x800E1C40||scratch_request.arguments[0]!=400))||
           word(carrier+0xDC)||word(active_head+49*4)||half(free_count+8)!=1){
            const auto p=get(carrier+0x64);
            std::fprintf(stderr,"type-32 init model=%04X phase=%u socket=%u group=%u scratch=%u/%08X/%u ptr=%08X shape=%08X free=%u offset=%g/%g/%g\n",
                         pilot.model,at(carrier)[0x18],at(carrier)[0x89],half(carrier+0x14C),scratch_request.count,
                         scratch_request.caller,scratch_request.arguments[0],word(carrier+0xDC),word(active_head+49*4),
                         half(free_count+8),double(p[0]),double(p[1]),double(p[2]));
            fail("type-32 model-relative initializer or empty scratch handling mismatch");
        }
        const auto emission_visits=half_rate?3u:2u;
        unsigned requests=0;
        Point first_endpoint{},first_far{},next_near{},next_far{};
        float first_radius=0;
        for(unsigned n=1;n<=emission_visits;++n){
            if(n==2){
                put32(owner+0xC8,replacement_target);
                if(which==AimCase::moving_copy)place(original_target+100,{origin[0]-300.f,origin[1],origin[2]+2000.f});
            }
            CPUState tick{};tick.gpr[3]=carrier;
            CallObservation emit{segment_factory};
            mod.run_observed(tick,ram,shared_update,emit,4096);
            const bool due=n==1||n==emission_visits;
            if(emit.count!=unsigned(due)||(due&&(emit.caller!=0x800E20C0||emit.arguments[0]!=carrier||
               emit.arguments[1]!=carrier+0x38||emit.arguments[2]!=carrier+0x9C||emit.arguments[3]!=pilot.group))||
               word(carrier+8)!=segment||half(free_count+6)||at(carrier)[0x18]!=1||
               word(carrier+0xC8)!=(has_target?original_target:0))
                fail("type-32 segment clock, pointer or request disagreement");
            requests+=emit.count;
            if(n==1){
                first_endpoint=get(carrier+0x38);first_far=get(carrier+0x9C);
                const auto* child=at(segment);
                if(child[0x10]!=11||child[0x11]!=pilot.group||child[0x18]||word(segment+0xC)!=segment_update||
                   word(segment+0x8C)!=carrier||word(segment+0x90)!=actor||
                   word(segment+0x144)!=carrier+0x38||word(segment+0x148)!=carrier+0x9C)
                    fail("type-32 shared child did not borrow exact endpoints");
                CPUState bind_child{};bind_child.gpr[3]=segment;
                mod.run(bind_child,ram,segment_update);
                shape_identity(segment,pilot.table,pilot.group,pilot.step,pilot.index);
                if(child[0x18]!=1||get(segment+0x20)!=first_endpoint)
                    fail("type-32 shared child did not sample first endpoint");
                if(contact){
                    segment_publication();
                    first_radius=scalar(shape+0x7C);
                    if(half(attack_count)!=1||word(attack_registry)!=shape||get(shape+0x64)!=first_endpoint||
                       std::abs(first_radius-pilot.radius*scalar(segment+0xC0))>0.001f||first_radius<=0)
                        fail("type-32 first segment publication did not produce its near sphere");
                }
            }
        }
        next_near=get(carrier+0x38);next_far=get(carrier+0x9C);
        CPUState next{};next.gpr[3]=segment;
        mod.run(next,ram,segment_update);
        if(at(segment)[0x18]!=2||get(segment+0x20)!=next_far||next_far==first_far)
            fail("type-32 shared child did not sample refreshed second endpoint");
        if(contact){
            segment_contact(pilot,*contact,first_endpoint,next_far,first_radius);
            return;
        }

        owner[0x6F7]=0;
        CPUState close{};close.gpr[3]=carrier;
        CallObservation refused{segment_factory};
        mod.run_observed(close,ram,shared_update,refused,4096);
        if(refused.count||at(carrier)[0x18]!=2||half(actor+0x774)!=5)
            fail("type-32 shared gate did not terminate emission");
        std::printf("type32-shared: %u %u %u %u %u %u",pilot.model,pilot.key,unsigned(which),pilot.group,requests,scratch_request.count);
        for(const auto& point:{first_endpoint,first_far,next_near,next_far})
            std::printf(" %.5f %.5f %.5f",double(point[0]),double(point[1]),double(point[2]));
        std::printf("\n");
    }
};
}

int main(int argc,char** argv){
    const bool shared=argc==4&&!std::strcmp(argv[3],"--shared");
    const bool contact=argc==4&&!std::strcmp(argv[3],"--contact");
    if(argc!=3&&!shared&&!contact)fail("usage: type32-bind-fixture qualified.dol unmodified-recomp.so [--shared|--contact]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(shared||contact){
        std::array<Block,std::size(shared_pilots)> hits{};
        for(auto& hit:hits)
            if(std::fread(hit.data(),1,hit.size(),stdin)!=hit.size())fail("type-32 shared hit block extent");
        if(contact){
            Block body_hit{};
            std::array<ActorSpec,std::size(shared_pilots)> owners{};
            ActorSpec target{};
            if(std::fread(body_hit.data(),1,body_hit.size(),stdin)!=body_hit.size())fail("type-32 body block extent");
            for(auto& spec:owners)
                if(std::fread(spec.data(),1,spec.size(),stdin)!=spec.size())fail("type-32 attacker spec extent");
            if(std::fread(target.data(),1,target.size(),stdin)!=target.size()||std::getc(stdin)!=EOF)
                fail("type-32 receiver spec extent");
            constexpr unsigned hp[]={300,500,250,200};
            for(unsigned i=0;i<hits.size();++i)
                for(auto which:{ContactCase::clear_hit,ContactCase::clear_miss,ContactCase::wall_hit,ContactCase::wall_miss}){
                    Check test(mod,original);
                    const Contact inlet{which,hp[i],body_hit,owners[i],target};
                    test.shared(shared_pilots[i],hits[i],AimCase::none,&inlet);
                }
            return 0;
        }
        if(std::getc(stdin)!=EOF)fail("unexpected type-32 shared payload");
        for(unsigned i=0;i<hits.size();++i)
            for(auto which:{AimCase::none,AimCase::moving_copy,AimCase::outside_gate,AimCase::half_rate}){
                Check test(mod,original);
                test.shared(shared_pilots[i],hits[i],which);
            }
        return 0;
    }
    std::array<Block,std::size(pilots)> blocks{};
    for(auto& hit:blocks)
        if(std::fread(hit.data(),1,hit.size(),stdin)!=hit.size())fail("type-32 hit block extent");
    if(std::getc(stdin)!=EOF)fail("unexpected type-32 hit payload");
    for(unsigned i=0;i<blocks.size();++i)
        for(auto scenario:{Case::scratch_empty,Case::shape_empty,Case::shape_available,Case::segment_empty}){
            if(i==0&&scenario==Case::segment_empty)continue;
            Check test(mod,original);
            test.observe(pilots[i],blocks[i],scenario);
        }
}
