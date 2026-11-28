// Original sphere/plane support, reference refresh and secondary wall admission.
#include "retail_declared_stage_grid.hpp"
#include <cmath>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 actors=0x803C4E84,fighter=0x81020000,stride=0x1E00,stage=0x81070000;
constexpr u32 bank=0x81050000,shape_base=0x81060000,shape_stride=0x200;
constexpr u32 sphere=shape_base,wall=sphere+shape_stride,capsule=wall+shape_stride;
constexpr u32 reference=0x803B0708,sphere_plane=0x8002CF80,wall_pass=0x8002E1C8;
constexpr u32 broad_entry=0x80039F6C,narrow_entry=0x80035698,bounds=0x8003FB00;
constexpr u32 plane_count=0x8043B5A0-21340,plane_list=0x803C237C;
constexpr u32 solid_count=0x8043B5A0-21338,solid_list=0x803C297C;
constexpr u32 wall_count=0x8043B5A0-21334,wall_list=0x803C357C;
constexpr u32 broad_scratch=0x803B04D0,plane_scratch=0x803B0368;
enum class Plane {absent,near,far};
struct Case {Plane first,second;int wall_jy=160;bool self=false,below=false;};
constexpr Case cases[]{
    {Plane::absent,Plane::absent},{Plane::near,Plane::absent},{Plane::far,Plane::absent},
    {Plane::near,Plane::far},{Plane::far,Plane::near},{Plane::near,Plane::far,159},
    {Plane::absent,Plane::absent,160,true},{Plane::near,Plane::far,159,false,true},
};
constexpr unsigned anchor_cases=8;
struct BroadOperands {std::array<u8,180> sphere,plane;};
struct BroadCapture {std::array<BroadOperands,2> calls{};unsigned count=0;};
thread_local BroadCapture* capture=nullptr;

struct State {
    std::array<u32,3> root,offset;
    u32 lower,plane;
    u8 correction_flag,wall_flag,support_flag;
    bool operator==(const State&)const=default;
};

class Probe {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void put(u32 a,float x){put_float(at(a),x);}
    void call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;mod.run(cpu,ram,entry,4096);
    }
    std::array<u32,3> vector(u32 a){return {word(a),word(a+4),word(a+8)};}
    State state(){
        return {vector(fighter+0x20),vector(fighter+0x1CC),word(reference),word(fighter+0x1E0),
                at(fighter+0x1DD)[0],at(fighter+0x274)[0],at(fighter+0x1DB)[0]};
    }
    void rigid(u32 m,const contact::Point& center){
        for(unsigned k=0;k<3;++k){put(m+20*k,1.f);put(m+12+16*k,center[k]);}
    }
    void body_setup(unsigned seat,const contact::HitBlock& hit,std::initializer_list<u8> descriptors,
                    const contact::Point& root){
        const auto owner=fighter+stride*seat,table=bank+0x2000*seat;
        std::memcpy(at(table),hit.data(),hit.size());
        unsigned index=0;for(auto d:descriptors)at(table+index++)[0]=d;
        at(table+index)[0]=255;
        at(owner)[0]=at(owner+0x18)[0]=1;
        at(owner+0x3E4)[0]=at(owner+0x3E7)[0]=u8(seat);at(owner+0x3E5)[0]=u8(1u<<seat);
        put16(at(owner+0x3E8),0x000A);put16(at(owner+0x1C4),180);put16(at(owner+0x1C6),180);
        put(owner+0xC0,1.f);store(owner+0x90,owner);store(owner+0x27C,0x802D4160);
        store(owner+0x3FC,table);store(owner+0x76C,1u|(1u<<3)|(1u<<6)|(1u<<12));
        for(unsigned k=0;k<3;++k)put(owner+0x20+4*k,root[k]);
        rigid(owner+0x1AA4,root);
        call(0x8008AB30,owner);
    }
    void plane_pose(u32 owner,float x,const contact::HitBlock& hit){
        const auto dy=std::bit_cast<float>(be32(hit.data()+32+3*80+16));
        rigid(owner+0x8D4+12*0x30,{x,119.f-dy,0});
    }
    void require_plane(u32 cell,unsigned seat,float x){
        if(cell<shape_base||cell>shape_base+4*shape_stride||(cell-shape_base)%shape_stride||
           word(cell+0x20)!=fighter+seat*stride||word(cell+0x28)!=bank+seat*0x2000+32+3*80||
           word(cell+0x58)!=4||vector(cell+0x64)!=std::array{std::bit_cast<u32>(x),std::bit_cast<u32>(119.f),0u}||
           vector(cell+0x88)!=std::array{0u,std::bit_cast<u32>(1.f),0u}||
           word(cell+0xA0)!=std::bit_cast<u32>(5.f)||word(cell+0xA4)!=std::bit_cast<u32>(20.f))
            fail("original role-20 plane did not publish its normal descriptor and declared pose");
    }
    static unsigned cell_id(u32 p){
        if(!p)return 255;
        if(p<shape_base||p>shape_base+4*shape_stride||(p-shape_base)%shape_stride)fail("support cell escaped original pool");
        return (p-shape_base)/shape_stride;
    }
    void support_anchor(unsigned id,const contact::ActorSpec& spec){
        constexpr u32 cfg=0x81054000,position=0x81054300,normal_out=position+0x10;
        constexpr u32 plane_cell=capsule+shape_stride,owner=fighter+stride;
        const auto saved=vector(fighter+0x1F8),saved_normal=vector(fighter+0x1EC);
        const auto joint=word(plane_cell+0x60);
        if(!at(fighter+0x1DB)[0]||word(fighter+0x1E0)!=plane_cell||joint!=owner+0x8D4+12*0x30||
           word(fighter+0x24)!=std::bit_cast<u32>(124.f)||word(plane_cell+0x20)!=owner)
            fail("support anchor did not come from the original foreign plane contact");
        std::memcpy(at(cfg),spec.data(),spec.size());store(0x803C4D94,cfg);
        call(0x8006F4F8,fighter);
        if(word(fighter+0x4B0)!=0x802D3EE0||word(fighter+0x4AC)!=cfg||saved!=vector(fighter+0x1F8))
            fail("support anchor lost the exact Sasuke landing configuration");
        const auto jt=[&](unsigned axis){return std::bit_cast<float>(word(joint+12+16*axis));};
        if(id==1||id==5)put(joint+28,jt(1)+12.f);
        if(id==2){put(joint+12,jt(0)+32.f);put(joint+44,jt(2)-20.f);}
        if(id==3)put(joint+28,jt(1)-12.f);
        if(id==4||id==7){
            // A declared 90-degree rotation about Z and a raised translation.
            put(joint,0.f);put(joint+4,-1.f);put(joint+16,1.f);put(joint+20,0.f);
            put(joint+28,150.f);
        }
        if(id==6)put(joint+28,150.f); // Same translation as the tilted controls, no rotation.
        if(id==7){call(0x8008A28C);call(0x80089AEC);}
        if(id==5)call(0x8002CDA4,fighter);
        const auto allocated_center=vector(plane_cell+0x64),allocated_normal=vector(plane_cell+0x88);
        const auto capsule_center=vector(capsule+0x64);
        if(word(plane_cell+0x60)!=joint||word(plane_cell+0x20)!=owner||word(fighter+0x1E0)!=plane_cell||
           saved!=vector(fighter+0x1F8)||saved_normal!=vector(fighter+0x1EC))
            fail("support pose inlet lost its original saved anchor or plane lease");
        // Declared empty-grid fallback height; no floor was constructed.
        put(stage+104,-9000.f);
        const auto root_before=vector(fighter+0x20);
        for(unsigned k=0;k<3;++k)store(position+4*k,root_before[k]);
        CPUState ground{},surface{};
        ground.gpr[3]=surface.gpr[3]=fighter;ground.gpr[4]=surface.gpr[4]=position;
        surface.gpr[5]=normal_out;
        mod.run(ground,ram,0x8003D6E4,4096);
        mod.run(surface,ram,0x8003E0A8,4096);
        const auto ground_y=std::bit_cast<u32>(float(ground.fpr[1]));
        const auto surface_y=std::bit_cast<u32>(float(surface.fpr[1]));
        const auto normal=vector(normal_out);
        if(vector(fighter+0x20)!=root_before||vector(fighter+0x1F8)!=saved||
           vector(fighter+0x1EC)!=saved_normal||word(fighter+0x1E0)!=plane_cell)
            fail("original support query changed the anchor or actor root");
        const auto land=state();
        CPUState step{};step.gpr[3]=fighter;CallObservation query{0x8003E0A8};
        mod.run_observed(step,ram,0x800677B0,query,4096);
        const auto after=state();
        if(query.count!=1||query.caller!=0x800677F0||query.arguments[0]!=fighter||
           query.arguments[1]!=fighter+0x20||step.gpr[3]>1||after.root[0]!=root_before[0]||
           after.root[2]!=root_before[2]||after.offset!=land.offset||vector(capsule+0x64)!=capsule_center||
           vector(plane_cell+0x64)!=allocated_center||vector(plane_cell+0x88)!=allocated_normal||
           vector(fighter+0x1F8)!=saved||vector(fighter+0x1EC)!=saved_normal||
           word(fighter+0x1E0)!=plane_cell||half(fighter+0x1C6)!=180)
            fail("support height path changed an unrelated axis, shape, anchor, or HP");
        std::array<u32,3> rebuilt_normal=allocated_normal;
        if(id==4){
            call(0x8008A28C);call(0x80089AEC);
            rebuilt_normal=vector(plane_cell+0x88);
            if(word(plane_cell+0x60)!=joint||word(plane_cell+0x20)!=owner)
                fail("tilted support control rebound a different plane");
        }
        std::printf("support-anchor: %u %u %u",id,land.support_flag,at(fighter+0x1DE)[0]);
        const auto emit=[](const auto& xyz){for(auto word:xyz)std::printf(" %08X",word);};
        emit(saved);emit(saved_normal);
        std::printf(" %08X %08X",ground_y,surface_y);
        emit(normal);emit(root_before);emit(after.root);emit(capsule_center);emit(allocated_normal);emit(rebuilt_normal);
        std::printf(" %u %u\n",step.gpr[3],half(fighter+0x1C6));
    }

public:
    Probe(const Module& m,const std::vector<u8>& original):mod(m),ram(original){}
    void run(unsigned id,const contact::HitBlock& hit,const contact::ActorSpec* anchor=nullptr){
        const auto spec=cases[anchor?1:id];
        const auto foreign=unsigned(spec.first!=Plane::absent)+unsigned(spec.second!=Plane::absent);
        const auto planes=foreign+unsigned(spec.self),capacity=3+planes;
        retail_declared_stage_grid::bind(mod,ram,stage,true,100.f);
        store(0x8043B5A0-31852,contact::scene);store(0x80436380,0x81068000);
        for(unsigned i=0;i<6;++i)store(actors+4*i,fighter+stride*i);
        for(unsigned i=0;i<capacity;++i){
            const auto cell=shape_base+i*shape_stride;
            at(cell+2)[0]=4;
            store(cell+4,i+1<capacity?cell+shape_stride:0);
        }
        store(contact::free_head+16,shape_base);store(contact::free_tail+16,shape_base+(capacity-1)*shape_stride);
        put16(at(contact::free_count+8),u16(capacity));
        if(spec.self){body_setup(0,hit,{0,2,11,3},{130,128,0});plane_pose(fighter,130,hit);}
        else body_setup(0,hit,{0,2,11},{130,128,0});
        const float wall_jy=spec.below?std::nextafter(float(spec.wall_jy),0.f):float(spec.wall_jy);
        rigid(fighter+0x8D4+3*0x30,{130,wall_jy,0});
        rigid(fighter+0x8D4+6*0x30,{142,128,0});
        const auto bind_plane=[&](unsigned seat,Plane where){
            if(where==Plane::absent)return;
            const float x=where==Plane::near?130.f:600.f;
            body_setup(seat,hit,{3},{x,119,0});
            plane_pose(fighter+stride*seat,x,hit);
        };
        bind_plane(1,spec.first);bind_plane(2,spec.second);
        call(0x8008A28C);call(0x80089AEC);
        call(0x8008A28C);call(0x80089AEC);
        if(half(contact::free_count+8)||half(plane_count)!=planes||half(solid_count)!=1||word(solid_list)!=sphere||
           half(wall_count)!=1||word(wall_list)!=wall||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=capsule||word(sphere+0x28)!=bank+32||
           word(wall+0x28)!=bank+32+2*80||word(capsule+0x28)!=bank+32+11*80)
            fail("original plane-reference body roles did not publish");
        if(spec.self)require_plane(word(plane_list),0,130.f);
        if(spec.first!=Plane::absent)require_plane(word(plane_list),1,spec.first==Plane::near?130.f:600.f);
        if(spec.second!=Plane::absent)require_plane(word(plane_list+4),2,spec.second==Plane::near?130.f:600.f);
        const auto wall_bottom=std::bit_cast<float>(word(wall+0x68))-std::bit_cast<float>(word(wall+0x88));
        const auto ends=std::array{vector(capsule+0x8C),vector(capsule+0x98)};
        put(reference,5000.f);put(reference+4,5001.f);put(reference+8,5002.f);
        const auto inlet=ram;

        put(reference,-10000.f); // Used only in this isolated producer fork.
        auto dispatch_fork=ram;
        CPUState p{};CallObservation broad{broad_entry};
        broad.sample=[](const CPUState& s)->u32{
            if(s.lr!=0x8002D0D0||s.gpr[3]!=broad_scratch||s.gpr[4]!=plane_scratch)
                fail("sphere/plane broad site lost its original scratch operands");
            if(!capture||capture->count>=capture->calls.size())fail("sphere/plane scratch capture budget");
            auto& out=capture->calls[capture->count++];
            std::memcpy(out.sphere.data(),s.ram+(broad_scratch-GC_RAM_BASE),out.sphere.size());
            std::memcpy(out.plane.data(),s.ram+(plane_scratch-GC_RAM_BASE),out.plane.size());
            return be32(s.ram+(reference-GC_RAM_BASE));
        };
        BroadCapture captured{};
        capture=&captured;
        mod.run_observed(p,ram,sphere_plane,broad,4096);
        capture=nullptr;
        const auto support=state();
        CPUState n{};CallObservation narrow{narrow_entry};
        narrow.sample=[](const CPUState& s)->u32{
            if(s.lr!=0x80030410||s.gpr[3]!=broad_scratch||s.gpr[4]!=plane_scratch)
                fail("sphere/plane narrow site changed");
            const auto a=be32(s.ram+(plane_scratch+4-GC_RAM_BASE));
            return cell_id(a);
        };
        mod.run_observed(n,dispatch_fork,sphere_plane,narrow,4096);
        if(broad.count!=foreign||broad.count!=captured.count||narrow.count>1||
           be32(contact::at(dispatch_fork,fighter+0x1CC+4))!=support.offset[1]||
           be32(contact::at(dispatch_fork,reference))!=support.lower)
            fail("isolated foreign support observations disagree");
        unsigned broad_hits=0;
        std::array<unsigned,2> tested{255,255};
        for(unsigned i=0;i<captured.count;++i){
            const auto& args=captured.calls[i];
            tested[i]=cell_id(be32(args.plane.data()+4));
            if(tested[i]!=3+i||be32(args.sphere.data()+4)!=sphere)
                fail("sphere/plane tested an unexpected source cell or publication order");
            std::memcpy(contact::at(dispatch_fork,broad_scratch),args.sphere.data(),args.sphere.size());
            std::memcpy(contact::at(dispatch_fork,plane_scratch),args.plane.data(),args.plane.size());
            CPUState pair{};pair.gpr[3]=broad_scratch;pair.gpr[4]=plane_scratch;
            mod.run(pair,dispatch_fork,broad_entry,4096);
            if(pair.gpr[3])broad_hits|=1u<<i;
        }
        if(std::popcount(broad_hits)!=int(narrow.count)||(narrow.count&&
           narrow.samples[0]!=tested[std::countr_zero(broad_hits)]))
            fail("sphere/plane original broad replay disagrees with narrow admission");
        CPUState s{};CallObservation solver{bounds};
        solver.sample=[](const CPUState& c)->u32{return c.lr==0x80035ECC?1u:0u;};
        mod.run_observed(s,ram,wall_pass,solver,4096);
        unsigned solver_visits=0;
        if(solver.count>std::size(solver.samples))fail("wall bounds report budget");
        for(unsigned i=0;i<solver.count;++i)solver_visits+=solver.samples[i];
        const auto isolated=state();
        call(0x8002CE44);
        const auto isolated_center=vector(capsule+0x64);
        ram=inlet;
        CPUState full{};CallObservation used{bounds};used.sample=solver.sample;
        const auto cut=mod.run_to_or_return(full,ram,0x8002BB14,{0x8005A580,0x80264290,contact::hp_write},&used,65536);
        unsigned full_visits=0;
        if(used.count>std::size(used.samples))fail("full bounds report budget");
        for(unsigned i=0;i<used.count;++i)full_visits+=used.samples[i];
        if(cut!=0x8005A580||full.lr!=0x8002C870||full.gpr[3]!=fighter||half(fighter+0x1C6)!=180||
           state()!=isolated||vector(capsule+0x64)!=isolated_center||full_visits!=solver_visits||
           ends!=std::array{vector(capsule+0x8C),vector(capsule+0x98)})
            fail("enclosing foreign support walk escaped its original frontier");
        for(unsigned seat=1;seat<=2;++seat){
            const float ref=at(fighter+seat*stride)[0]?-10000.f:5000.f+float(seat);
            if(word(reference+4*seat)!=std::bit_cast<u32>(ref)||vector(fighter+seat*stride+0x1CC)!=std::array{0u,0u,0u})
                fail("sphere/plane support changed a foreign owner's reference or vector");
        }
        if(anchor){support_anchor(id,*anchor);return;}

        const auto final=state();
        std::printf("plane-reference: %u %u %u %08X %08X %08X %u %u %u %u %u %u %08X %08X %08X %08X %u %u\n",
                    id,planes,broad.count,broad.samples[0],broad.samples[1],support.lower,tested[0],tested[1],broad_hits,narrow.count,
                    narrow.count?narrow.samples[0]:255,cell_id(support.plane),support.root[1],support.offset[1],
                    std::bit_cast<u32>(wall_bottom),isolated.offset[0],solver_visits,support.support_flag);
        std::printf("plane-walk: %u %08X %08X %08X %08X %08X %08X %08X %08X %u %u %u %u %u\n",
                    id,final.root[0],final.root[1],final.offset[0],final.offset[1],word(capsule+0x64),
                    word(capsule+0x68),isolated_center[0],isolated_center[1],final.correction_flag,
                    final.wall_flag,final.support_flag,full_visits,half(fighter+0x1C6));
    }
};
}

int main(int argc,char** argv){
    const bool anchor=argc==4&&!std::strcmp(argv[3],"--support-anchor");
    if(argc!=3&&!anchor)
        fail("usage: plane-reference-fixture qualified.dol unmodified-recomp.so [--support-anchor] < qualified-inputs");
    contact::HitBlock hit{};
    contact::ActorSpec spec{};
    if(std::fread(hit.data(),1,hit.size(),stdin)!=hit.size()||
       (anchor&&std::fread(spec.data(),1,spec.size(),stdin)!=spec.size())||std::getc(stdin)!=EOF||std::ferror(stdin))
        fail("plane reference input extent");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    for(unsigned i=0;i<(anchor?anchor_cases:std::size(cases));++i)Probe(mod,original).run(i,hit,anchor?&spec:nullptr);
}
