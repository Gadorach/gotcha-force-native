// Original type-32 borrowed-endpoint collision branches and type-11 handoff.
#include "retail_recomp_fixture.hpp"
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 carrier=0x81030000,segment=0x81030800,shape=0x81032000;
constexpr u32 descriptors=0x81033000,descriptor_row=0x81033800;
constexpr u32 hit_block=0x81034000,scratch_header=0x81036000,scratch=0x81036100;
constexpr u32 stage=0x81050000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 factory=0x801DEB68,update=0x801DEC90,active=0x801DEF80,scratch_alloc=0x80006DC8;
constexpr u32 pose=0x80048288,bind=0x8008AC80,segment_factory=0x80139A6C,segment_update=0x80139B80;
using Block=std::array<u8,7840>;
using Point=std::array<float,3>;
struct Pilot {u16 model;u8 key,index;u32 table,first,second;};
constexpr Pilot pilots[]={{0x0D03,8,1,0x80362C28,actor+0x15C,actor+0x168},
                          {0x0E02,9,6,0x8038DAE0,actor+0x144,actor+0x150}};
enum class Case {scratch_empty,shape_empty,shape_available,segment_empty};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){
        if(va<GC_RAM_BASE||va-GC_RAM_BASE>ram.size()-4)fail("type-32 scratch extent");
        return ram.data()+va-GC_RAM_BASE;
    }
    u32 word(u32 va){return be32(at(va));}
    u16 half(u32 va){const auto* p=at(va);return u16(u16(p[0])<<8|p[1]);}
    Point get(u32 va){return {std::bit_cast<float>(word(va)),std::bit_cast<float>(word(va+4)),std::bit_cast<float>(word(va+8))};}
    void place(u32 va,const Point& point){
        for(unsigned i=0;i<point.size();++i)put_float(at(va+4*i),point[i]);
    }
    void pools(bool spare,bool shape_free){
        put32(at(0x80436380),descriptor_row);
        put16(at(descriptor_row+18),4);
        put32(at(descriptor_row+12),descriptors);
        for(unsigned i=0;i<4;++i)put32(at(descriptors+0x80*i),i<3?descriptors+0x80*(i+1):0);
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
    void shape_identity(Pilot pilot,u32 object){
        if(word(active_head+49*4)!=shape||half(free_count+8)||
           word(shape+0x20)!=object||word(shape+0x24)!=actor||word(shape+0x2C)!=pilot.table||
           word(shape+0x28)!=hit_block+0xAA0+80||at(shape)[0x14]!=1||at(shape)[0x12]!=1||
           at(shape)[0x11]!=pilot.index)
            fail("type-32 group-1 shape used the wrong object, queue or credit");
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
        if(parent_bound)shape_identity(pilot,carrier);
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
                    shape_identity(pilot,segment);
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
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type32-bind-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
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
