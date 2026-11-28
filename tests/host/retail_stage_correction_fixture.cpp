// One declared STIH wall and an unedited normal body contact queue.
#include "retail_body_entry_contact.hpp"
#include "retail_declared_stage_grid.hpp"
#include "retail_stage_grid_replay.hpp"
#include <cmath>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace entry=retail_body_entry_contact;
constexpr u32 body=contact::receiver,probe_cell=contact::body,capsule=probe_cell+0x200;
constexpr u32 stage=0x81048000;
constexpr u32 stage_contacts=0x8002E1C8;
constexpr u32 role08_count=0x8043B5A0-21334,role08_list=0x803C357C;
constexpr u32 slots=0x803C4E84,seat_size=0x1E00;
struct Case {bool wall;int begin,end;int attack_x=0;};
constexpr Case cases[]{
    {false,130,130},{true,151,151},{true,150,150},{true,130,130},
    {true,100,100},{true,170,130},{true,170,90},
    {false,130,130,40},{true,130,130,40},{false,130,130,240},{true,130,130,240},
    {true,90,90},
};
struct LinkCase {bool wall;unsigned masks;u32 threshold;bool reverse=false,both=false,attack=false;};
constexpr LinkCase linked_cases[]{
    {false,15,0},{true,0,0},{true,14,0},{true,13,0},{true,11,0},{true,7,0},
    {true,15,0},{true,15,0x47434FFF},{true,15,0x47435000},{true,15,0x47435001},
    {true,15,0,true},{true,15,0,false,true},
    {true,0,0,false,false,true},{true,15,0,false,false,true},
};
struct Site {unsigned triangle,axis;int sign;contact::Point root;};
constexpr Site sites[]{
    {1549,0,1,{0,202,1649}}, {1505,0,-1,{0,202,1557}}, {1546,2,1,{824,202,0}},
};
struct StageSample {bool empty;int initial,final;bool vertex=false;int step=0;};
constexpr StageSample stage_samples[]{
    {true,30,30},{false,51,51},{false,50,50},{false,30,30},
    {false,80,-10},{false,-10,-10},
    {false,50,50,true},{false,50,50,true,1},{false,50,50,true,-1},
};

class Probe {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void put(u32 a,float x){put_float(at(a),x);}
    void call(u32 fn,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;mod.run(cpu,ram,fn,65536);
    }
    void rebuild(){call(0x8008A28C);call(0x80089AEC);}
    void stage_setup(bool wall){
        retail_declared_stage_grid::bind(mod,ram,stage,wall,100.f);
    }
    void pose(float x,u32 owner=body){
        const u32 root=owner+0x1AA4,joint=owner+0x8D4+6*0x30;
        put(owner+0x20,x);put(root+12,x);
        put(joint+12,x+12.f);
    }
    void pose_xyz(const contact::Point& point){
        for(unsigned axis=0;axis<3;++axis){
            put(body+0x20+4*axis,point[axis]);
            put(body+0x1AA4+12+16*axis,point[axis]);
            put(body+0x8D4+6*0x30+12+16*axis,point[axis]+(axis==0?12.f:0.f));
        }
    }
    void bind_body(u32 owner,u32 block,u32 first,u32 damage_cell,u8 seat,const contact::HitBlock& hit){
        at(owner)[0]=at(owner+0x18)[0]=1;
        at(owner+0x3E4)[0]=at(owner+0x3E7)[0]=seat;
        put16(at(owner+0x3E8),0x000A);put16(at(owner+0x1C4),180);put16(at(owner+0x1C6),180);
        put(owner+0xC0,1.f);store(owner+0x90,owner);store(owner+0x27C,0x802D4160);store(owner+0x76C,1u|(1u<<6));
        const u32 root=owner+0x1AA4,joint=owner+0x8D4+6*0x30;
        for(unsigned axis=0;axis<3;++axis){put(root+20*axis,1.f);put(joint+20*axis,1.f);}
        std::memcpy(at(block),hit.data(),hit.size());
        const bool sphere=first!=damage_cell;
        if(sphere){
            at(block)[0]=1;at(block+1)[0]=11;at(block+2)[0]=255;
            at(first+2)[0]=4;store(first+4,damage_cell);
        }else{at(block)[0]=11;at(block+1)[0]=255;}
        store(owner+0x3FC,block);
        at(damage_cell+2)[0]=4;
        store(contact::free_head+16,first);store(contact::free_tail+16,damage_cell);
        put16(at(contact::free_count+8),sphere?2:1);
        call(0x8008AB30,owner);
        if(half(contact::free_count+8)||word(damage_cell+0x20)!=owner||
           word(damage_cell+0x28)!=block+32+11*80)
            fail("stage fixture did not bind the original capsule descriptor");
    }
    void walk(unsigned id,bool stale){
        constexpr u32 collision_walk=0x8002BB14,presentation=0x8005A580,kernel=0x80030A3C;
        constexpr u32 receiver_scratch=0x803B0368,attack_scratch=0x803B04D0;
        if(stale){put(body+0x1CC,-64.f);at(body+0x1DD)[0]=1;at(body+0x274)[0]=1;}
        for(unsigned visit=1;visit<=2;++visit){
            if(visit==2){
                // Declare the next rigid pose at the root produced by the
                // previous collision prefix, not another original actor tick.
                pose(std::bit_cast<float>(word(body+0x20)));
                // Publish the retained caster shape without putting the
                // declared caster in the enclosing walk's actor loop.
                const auto previous_seat0=word(slots);
                store(slots,actor);
                rebuild();
                store(slots,previous_seat0);
            }
            const auto previous=word(body+0x1CC),bound=word(capsule+0x7C);
            const auto receivers=half(contact::receiver_count),attacks=half(entry::ordinary_count);
            const auto role_mask=half(body+0x272);
            const auto guard=word(body+0x720);
            if(attacks!=1||word(entry::ordinary_attack_list)!=entry::attack_cell||half(actor+0x26C)!=1||
               word(entry::attack_cell+0x64)!=std::bit_cast<u32>(float(cases[id].attack_x))||
               word(entry::attack_cell+0x20)!=actor||word(entry::attack_cell+0x24)!=actor||
               half(role08_count)!=1||word(role08_list)!=probe_cell||
               word(capsule+0x20)!=body||word(capsule+0x28)!=contact::body_block+32+11*80||
               receivers>1||(receivers&&word(contact::receiver_registry)!=capsule))
                fail("stage walk did not retain the exact original attack and body shapes");
            auto fork=ram;
            CPUState inlet{};
            const auto narrow=mod.run_to_or_return(inlet,fork,collision_walk,{kernel,presentation,0x80264290},nullptr,65536);
            unsigned accepted=0;
            u32 scratch_left=0,scratch_right=0;
            if(narrow==kernel){
                if(inlet.lr!=0x800304F0||inlet.gpr[3]!=attack_scratch||inlet.gpr[4]!=receiver_scratch)
                    fail("stage walk changed the original capsule scratch dispatch");
                scratch_left=be32(contact::at(fork,receiver_scratch+0x34));
                scratch_right=be32(contact::at(fork,receiver_scratch+0x40));
                CPUState solve{};solve.gpr[3]=attack_scratch;solve.gpr[4]=receiver_scratch;
                mod.run(solve,fork,kernel,4096);accepted=unsigned(solve.gpr[3]!=0);
            }else if(narrow!=presentation)fail("stage walk did not reach its narrow or presentation frontier");
            CPUState cpu{};CallObservation hp{contact::hp_write};
            const auto cut=mod.run_to_or_return(cpu,ram,collision_walk,{presentation,0x80264290},&hp,65536);
            std::printf("stage-walk: %u %u %u %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %u %u %04X %08X %u %u %u %u %u %u %u %08X %08X\n",
                        id,unsigned(stale),visit,cut,cpu.lr,previous,bound,word(body+0x20),word(body+0x1CC),word(probe_cell+0x64),
                        word(capsule+0x64),word(capsule+0x8C),word(capsule+0x98),at(body+0x1DD)[0],at(body+0x274)[0],
                        role_mask,guard,attacks,receivers,unsigned(narrow==kernel),accepted,hp.count,half(body+0x1C6),half(body+0x20C),
                        scratch_left,scratch_right);
            if(cut!=presentation||cpu.lr!=0x8002C870||cpu.gpr[3]!=body||
               word(body+0x1D0)||word(body+0x1D4)||word(body+0x24)||word(body+0x28)||
               (hp.count&&(hp.count!=1||hp.caller!=0x8002F1D8||hp.arguments[0]!=body||hp.arguments[1]!=17)))
                fail("stage walk did not reach the presentation frontier with its exact HP path");
        }
    }
public:
    Probe(const Module& m,const std::vector<u8>& original):mod(m),ram(original){}
    void run_retail_stage(unsigned site_id,unsigned id,const entry::Attack& sasuke,
                          const retail_stage_grid_replay::Grids& grids){
        const auto site=sites[site_id];const auto spec=stage_samples[id];
        const auto& secondary=grids[1];
        const auto triangle=secondary.data()+8+be32(secondary.data()+36)+site.triangle*56;
        const float anchor=spec.vertex?std::bit_cast<float>(be32(triangle+4+4*site.axis)):
                                       -std::bit_cast<float>(be32(triangle+52))*float(site.sign);
        const auto root=[&](int distance){
            auto p=site.root;p[site.axis]=anchor+float(site.sign*distance);
            if(spec.step)p[site.axis]=std::nextafter(p[site.axis],anchor+float(500*site.sign*spec.step));
            return p;
        };
        store(0x8043B5A0-31852,contact::scene);store(0x80436380,0x81033A00);
        for(unsigned i=0;i<6;++i)store(slots+4*i,body-seat_size+i*seat_size);
        retail_stage_grid_replay::bind(mod,ram,stage,grids,spec.empty);
        bind_body(body,contact::body_block,probe_cell,capsule,1,sasuke.hit);
        pose_xyz(root(spec.initial));rebuild();
        if(spec.initial!=spec.final){pose_xyz(root(spec.final));rebuild();}
        const auto p=root(spec.final);
        if(half(role08_count)!=1||word(role08_list)!=probe_cell||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=capsule||word(probe_cell+0x28)!=contact::body_block+32+80||
           word(probe_cell+0x88)!=std::bit_cast<u32>(50.f)||word(body+0x1CC)||at(body+0x1DD)[0])
            fail("st42 original stage sphere/capsule was not published");
        const auto vector_bits=[&](u32 a){return std::array{word(a),word(a+4),word(a+8)};};
        const auto endpoints=std::array{vector_bits(capsule+0x8C),vector_bits(capsule+0x98)};
        const auto inlet=ram;
        CPUState local{};CallObservation projection{0x80039AEC};
        projection.sample=[](const CPUState& c)->u32{return c.lr==0x80036190?c.gpr[4]:0;};
        mod.run_observed(local,ram,stage_contacts,projection,65536);
        const auto only_root=vector_bits(body+0x20),only_vector=vector_bits(body+0x1CC);
        const auto stage_flags=std::array{at(body+0x1DD)[0],at(body+0x274)[0]};
        call(0x8002CE44);
        const auto only_centers=std::array{vector_bits(probe_cell+0x64),vector_bits(capsule+0x64)};
        ram=inlet;
        constexpr u32 presentation=0x8005A580;
        CPUState walk{};CallObservation hit{contact::hp_write};
        const auto stop_at=mod.run_to_or_return(walk,ram,0x8002BB14,{presentation,0x80264290},&hit,65536);
        if(stop_at!=presentation||walk.lr!=0x8002C870||walk.gpr[3]!=body||hit.count||half(body+0x1C6)!=180||
           endpoints!=std::array{vector_bits(capsule+0x8C),vector_bits(capsule+0x98)})
            fail("st42 enclosing walk missed its original frontier or changed HP/endpoints");
        std::printf("st42-wall: %u %u %u %d %d %u %d",site_id,id,unsigned(spec.empty),spec.initial,spec.final,
                    unsigned(spec.vertex),spec.step);
        const auto emit=[](const auto& values){for(auto v:values)std::printf(" %08X",v);};
        for(auto x:p)std::printf(" %08X",std::bit_cast<u32>(x));
        emit(only_root);emit(only_vector);emit(only_centers[0]);emit(only_centers[1]);
        std::printf(" %u %u",stage_flags[0],stage_flags[1]);
        emit(vector_bits(body+0x20));emit(vector_bits(body+0x1CC));
        emit(vector_bits(probe_cell+0x64));emit(vector_bits(capsule+0x64));
        std::printf(" %u %u %u\n",at(body+0x1DD)[0],at(body+0x274)[0],half(body+0x1C6));
        std::printf("st42-projection: %u %u %u",site_id,id,projection.count);
        const auto first_vertex=word(word(stage+32)+28)+4;
        const auto triangle_count=(secondary.size()-8-be32(secondary.data()+36))/56;
        for(unsigned i=0;i<std::min(projection.count,8u);++i){
            const auto vertex=projection.samples[i];
            if(!vertex){std::printf(" -1");continue;}
            if(vertex<first_vertex||(vertex-first_vertex)%56||(vertex-first_vertex)/56>=triangle_count)
                fail("st42 projection referenced outside the secondary grid");
            std::printf(" %u",(vertex-first_vertex)/56);
        }
        std::putchar('\n');
    }
    void run_linked(unsigned id,const entry::Attack& sasuke,const entry::Attack& tau){
        constexpr u32 other=body+seat_size,other_block=0x81050000;
        constexpr u32 other_sphere=capsule+0x200,other_capsule=other_sphere+0x200;
        constexpr u32 propagate=0x8002CB20,writer=0x80035CA4,presentation=0x8005A580;
        const auto spec=linked_cases[id];
        stage_setup(spec.wall);
        store(0x8043B5A0-31852,contact::scene);store(0x80436380,0x81033A00);
        for(unsigned i=0;i<6;++i)store(slots+4*i,body-seat_size+i*seat_size);
        const u32 source=spec.reverse?other:body,target=spec.reverse?body:other;
        const u32 source_cell=capsule,target_cell=other_capsule;
        const u8 source_seat=spec.reverse?2:1,target_seat=spec.reverse?1:2;
        const u8 source_bit=u8(1u<<target_seat),target_bit=u8(1u<<source_seat);
        bind_body(source,contact::body_block,probe_cell,source_cell,source_seat,sasuke.hit);
        bind_body(target,other_block,spec.both?other_sphere:target_cell,target_cell,target_seat,sasuke.hit);
        pose(130.f,source);pose(spec.both?140.f:250.f,target);
        at(source+0x71E)[0]=(spec.masks&1)?source_bit:0;
        at(target+0x71E)[0]=(spec.masks&2)?target_bit:0;
        at(source+0x71F)[0]=(spec.masks&4)?source_bit:0;
        at(target+0x71F)[0]=(spec.masks&8)?target_bit:0;
        store(target+0x204,spec.threshold);
        const auto dormant=word(slots);
        if(spec.attack){
            constexpr u32 sasuke_spec=0x81015400;
            store(0x8043B5A0-31824,0x81019600);
            store(0x8043B5A0-31856,0x8103E000);at(0x8103E000+12)[0]=1;
            at(0x803D3AE8+852)[0]=1;
            std::memcpy(at(sasuke_spec),sasuke.spec.data(),sasuke.spec.size());
            store(0x803C4D94+4*target_seat,sasuke_spec);
            call(0x8006F4F8,target);
            if(word(target+0x4B0)!=0x802D3EE0||word(target+0x4AC)!=sasuke_spec||word(target+0x27C)!=0x802D4160)
                fail("linked correction did not install the original receiver factors");
            put(target+0xB4,1.f);put(target+0xC4,1.f);put16(at(target+0x1C8),0xFFFF);
            put16(at(0x803B069C+2*target_seat),180);
            at(target+0x88)[0]=1;
            for(unsigned i=0;i<3;++i){
                store(contact::scene+60*i+0x114,100);store(contact::scene+60*i+0x118,100);
                store(0x803B068C+4*i,100);
            }
            entry::bind_attack_at(mod,ram,tau,entry::routes[0],1,{360.f,0,0},actor);
        }
        rebuild();
        store(slots,dormant);
        if(half(role08_count)!=(spec.both?2:1)||half(contact::receiver_count)!=2||
           word(source+0x1CC)||word(target+0x1CC)||at(source+0x1DD)[0]||at(target+0x1DD)[0]||
           half(entry::ordinary_count)!=unsigned(spec.attack)||(spec.attack&&
           (word(entry::ordinary_attack_list)!=entry::attack_cell||half(actor+0x26C)!=1)))
            fail("linked correction did not start with exactly the declared original shape roles");

        auto fork=ram;
        call(stage_contacts);
        const auto source_stage=word(source+0x1CC),target_stage=word(target+0x1CC);
        CPUState copy{};CallObservation moves{writer};
        mod.run_observed(copy,ram,propagate,moves,4096);
        if(moves.count>1||(moves.count&&(moves.caller!=(spec.reverse?0x8002CC34:0x8002CC08)||
           moves.arguments[0]!=target||moves.arguments[1]!=source+0x1CC)))
            fail("linked correction called the writer with the wrong operands");
        const auto source_raw=word(source_cell+0x64),target_raw=word(target_cell+0x64);
        call(0x8002CE44);
        const auto state=[&](){
            return std::array{word(source+0x20),word(target+0x20),word(source+0x1CC),word(target+0x1CC),
                              word(source_cell+0x64),word(target_cell+0x64),word(target_cell+0x8C),word(target_cell+0x98)};
        };
        const auto isolated=state();
        ram.swap(fork);
        if(spec.attack)fork=ram;
        CPUState walk_cpu{};CallObservation hp{contact::hp_write};
        const auto cut=mod.run_to_or_return(walk_cpu,ram,0x8002BB14,{presentation,0x80264290},&hp,65536);
        if(cut!=presentation||walk_cpu.lr!=0x8002C870||walk_cpu.gpr[3]!=body||
           state()!=isolated||half(source+0x1C6)!=180||half(source+0x20C)||
           hp.count>unsigned(spec.attack)||(hp.count&&(hp.caller!=0x8002F1D8||hp.arguments[0]!=target||hp.arguments[1]!=17))||
           half(target+0x1C6)!=180-17*hp.count||half(target+0x20C)!=hp.count)
            fail("enclosing walk did not reproduce the isolated linked correction");
        std::printf("linked-correction: %u %08X %08X %u %08X %08X %08X",id,source_stage,target_stage,
                    moves.count,moves.caller,source_raw,target_raw);
        for(auto value:isolated)std::printf(" %08X",value);
        std::printf(" %u %u\n",at(source+0x1DD)[0],at(target+0x1DD)[0]);
        if(spec.attack){
            constexpr u32 kernel=0x80030A3C,scratch=0x803B0368;
            CPUState narrow{};
            const auto next=mod.run_to_or_return(narrow,fork,0x8002BB14,{kernel,presentation,0x80264290},nullptr,65536);
            if(next!=(hp.count?kernel:presentation)||(next==kernel&&
               (narrow.lr!=0x800304F0||narrow.gpr[3]!=0x803B04D0||narrow.gpr[4]!=scratch)))
                fail("linked correction did not reach the expected original damage kernel");
            const auto endpoint=[&](u32 offset){return next==kernel?be32(contact::at(fork,scratch+offset)):0;};
            std::printf("linked-pair: %u %u %08X %08X %u %u %u\n",id,unsigned(next==kernel),
                        endpoint(0x34),endpoint(0x40),hp.count,half(target+0x1C6),half(target+0x20C));
        }
    }
    void run(unsigned id,const entry::Attack& sasuke,const entry::Attack& tau,bool enclosing=false,bool stale=false){
        const auto spec=cases[id];
        stage_setup(spec.wall);
        store(0x8043B5A0-31852,contact::scene);store(0x80436380,0x81033A00);
        for(unsigned i=0;i<6;++i)store(slots+4*i,body-seat_size+i*seat_size);
        bind_body(body,contact::body_block,probe_cell,capsule,1,sasuke.hit);
        pose(float(spec.begin));
        const auto original_first=word(slots);
        if(spec.attack_x){
            store(0x8043B5A0-31824,0x81019600);
            store(0x8043B5A0-31856,0x8103E000);at(0x8103E000+12)[0]=1;
            at(0x803D3AE8+852)[0]=1;
            std::memcpy(at(0x81015400),sasuke.spec.data(),sasuke.spec.size());
            store(body+0x4AC,0x81015400);
            if(enclosing){
                store(0x803C4D94+4,0x81015400);
                call(0x8006F4F8,body);
                if(word(body+0x4B0)!=0x802D3EE0||word(body+0x4AC)!=0x81015400||word(body+0x27C)!=0x802D4160)
                    fail("stage walk did not install the exact normal reaction banks");
            }
            put(body+0xB4,1.f);put(body+0xC4,1.f);put16(at(body+0x1C8),0xFFFF);
            put16(at(0x803B069E),180);
            at(body+0x88)[0]=1;
            for(unsigned i=0;i<2;++i){
                store(contact::scene+60*i+0x114,100);store(contact::scene+60*i+0x118,100);
                store(0x803B068C+4*i,100);
            }
            entry::bind_attack_at(mod,ram,tau,entry::routes[0],1,{float(spec.attack_x),0,0},actor);
        }
        rebuild();
        if(spec.begin!=spec.end){pose(float(spec.end));rebuild();}
        if(half(role08_count)!=1||word(role08_list)!=probe_cell||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=capsule||word(probe_cell+0x28)!=contact::body_block+32+80||
           word(probe_cell+0x64)!=std::bit_cast<u32>(float(spec.end))||word(probe_cell+0x68)!=std::bit_cast<u32>(50.f)||
           word(probe_cell+0x88)!=std::bit_cast<u32>(50.f)||word(body+0x1CC)||at(body+0x1DD)[0])
            fail("stage correction did not retain the original source sphere and capsule");
        store(slots,original_first);
        if(enclosing){walk(id,stale);return;}
        call(stage_contacts);
        if(spec.attack_x)store(slots,actor);
        const auto before=word(probe_cell+0x64),capsule_before=word(capsule+0x64);
        const auto endpoints=std::array{word(capsule+0x8C),word(capsule+0x98)};
        call(0x8002CE90,50);
        if(word(body+0x1D0)||word(body+0x1D4)||word(body+0x24)||word(body+0x28)||
           half(role08_count)!=1||word(role08_list)!=probe_cell||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=capsule||half(body+0x20C)||half(body+0x1C6)!=180||
           endpoints!=std::array{word(capsule+0x8C),word(capsule+0x98)})
            fail("stage solver changed an unrelated axis, geometry owner or HP before attack");
        std::printf("stage-correction: %u %u %d %d %08X %08X %08X %08X %u %u %u\n",
                    id,unsigned(spec.wall),spec.begin,spec.end,
                    word(body+0x20),word(body+0x1CC),before,word(probe_cell+0x64),at(body+0x1DD)[0],
                    at(body+0x274)[0],half(body+0x1C6));
        std::printf("stage-capsule: %u %08X %08X %08X %08X %08X %08X\n",id,capsule_before,word(capsule+0x64),
                    endpoints[0],endpoints[1],word(capsule+0x8C),word(capsule+0x98));
        if(spec.attack_x){
            constexpr u32 attack=entry::attack_cell,narrow=0x80030A3C;
            if(half(entry::ordinary_count)!=1||word(entry::ordinary_attack_list)!=attack||half(actor+0x26C)!=1||
               word(attack+0x64)!=std::bit_cast<u32>(float(spec.attack_x)))
                fail("stage-corrected receiver lost its original attack inlet");
            CPUState coarse{};coarse.gpr[3]=attack+0x58;coarse.gpr[4]=capsule+0x58;
            mod.run(coarse,ram,0x80039F6C);
            auto fork=ram;
            CPUState narrow_cpu{};
            const auto cut=mod.run_to_or_return(narrow_cpu,fork,contact::pair_pass,{narrow},nullptr,4096);
            if(cut!=(coarse.gpr[3]?narrow:0)||(cut&&
               (narrow_cpu.lr!=0x800304F0||narrow_cpu.gpr[3]!=0x803B04D0||narrow_cpu.gpr[4]!=0x803B0368||
                be32(contact::at(fork,0x803B0368+0x34))!=std::bit_cast<u32>(
                    std::bit_cast<float>(endpoints[0])+std::bit_cast<float>(word(body+0x1CC))))))
                fail("stage-corrected body did not reach the original corrected capsule test");
            CPUState hit{};CallObservation writes{contact::hp_write};
            mod.run_observed(hit,ram,contact::pair_pass,writes,4096);
            if(writes.count>1||(writes.count&&(writes.caller!=0x8002F1D8||writes.arguments[0]!=body||
               writes.arguments[1]!=17))||half(body+0x1C6)!=180-17*writes.count||half(body+0x20C)!=writes.count){
                std::fprintf(stderr,"stage corrected pair %u coarse=%u writes=%u site=%08X target=%08X request=%u hp=%u token=%u\n",
                             id,coarse.gpr[3],writes.count,writes.caller,writes.arguments[0],writes.arguments[1],
                             half(body+0x1C6),half(body+0x20C));
                fail("stage-corrected capsule changed its original HP writer or ownership");
            }
            std::printf("stage-correction-pair: %u %d %u %u %u %u\n",id,spec.attack_x,coarse.gpr[3],writes.count,
                        half(body+0x1C6),half(body+0x20C));
        }
    }
};
} // namespace

int main(int argc,char** argv){
    const bool enclosing=argc==4&&!std::strcmp(argv[3],"--collision-walk");
    const bool linked=argc==4&&!std::strcmp(argv[3],"--linked-correction");
    const bool retail_stage=argc==4&&!std::strcmp(argv[3],"--retail-stage");
    if(argc!=3&&!enclosing&&!linked&&!retail_stage)
        fail("usage: stage-correction-fixture qualified.dol unmodified-recomp.so [--collision-walk|--linked-correction|--retail-stage] < inputs");
    entry::Attack sasuke{},tau{};
    for(auto* assets:{&sasuke,&tau}){
        if(retail_stage&&assets==&tau)break;
        if(std::fread(assets->hit.data(),1,assets->hit.size(),stdin)!=assets->hit.size()||
           std::fread(assets->spec.data(),1,assets->spec.size(),stdin)!=assets->spec.size())
            fail("stage correction input extent");
    }
    retail_stage_grid_replay::Grids grids;
    if(retail_stage)grids=retail_stage_grid_replay::input();
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra stage correction input");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(retail_stage){
        for(unsigned site=0;site<std::size(sites);++site)
            for(unsigned i=0;i<std::size(stage_samples);++i)Probe(mod,original).run_retail_stage(site,i,sasuke,grids);
        return 0;
    }
    if(linked){
        for(unsigned i=0;i<std::size(linked_cases);++i)Probe(mod,original).run_linked(i,sasuke,tau);
        return 0;
    }
    for(unsigned i=0;i<std::size(cases);++i){
        if(!enclosing)Probe(mod,original).run(i,sasuke,tau);
        else if(cases[i].attack_x)for(bool stale:{false,true})Probe(mod,original).run(i,sasuke,tau,true,stale);
    }
}
