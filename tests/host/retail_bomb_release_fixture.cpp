// Bounded original bomb release leaves, with declared stock, state and free lists.
#include "retail_recomp_fixture.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
using namespace retail_fixture;
constexpr u32 spec=0x81014000,scene=0x81017000,mode=0x81019000;
constexpr u32 descriptor_row=0x81030000,descriptors=0x81030100,cells=0x81032000;
constexpr u32 descriptor_stride=0x80,cell_stride=0x800;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC;
constexpr u32 active_head=0x803C706C,active_tail=0x803C6F6C;
constexpr u32 shapes=0x81034800,hit_block=0x81037000;
constexpr u32 shape_stride=0x200;
constexpr u32 bind=0x8008AC80,visit=0x80089A94;
constexpr u32 attack_count=0x8043B5A0-21328,attack_registry=0x803C477C;
constexpr u32 receiver_count=0x8043B5A0-21330,receiver_registry=0x803C417C;
constexpr u32 receiver=0x8101C000,body=0x81035200,body_block=0x8103B000;
constexpr u32 effect_cell=0x81034000,common_collision=0x81068000;
constexpr u32 stage=0x81040000;
constexpr u32 factory=0x8007DB5C, powered=0x8016711C, ninja=0x800715F8, alpha=0x801A0458;
constexpr u32 audio=0x800F036C, normal_x_helper=0x8016D584, movement_tail=0x800B22F4;
constexpr u32 area_callback=0x80086048, area_amount=0x802C4778;

struct Case {
    u16 model;
    u8 state, extra, stock, carriers, descriptors;
};
using HitBlock=std::array<u8,7840>;
using ActorSpec=std::array<u8,432>;
using CommonCollision=std::array<u8,21504>;
using Point=std::array<float,3>;
struct AreaReceiver {
    const HitBlock& block;
    const ActorSpec& attacker;
    const ActorSpec& defender;
    unsigned offset_x;
    u16 attacker_hp;
};
enum class Contact {miss,duplicate,impact,child_cell_empty,child_descriptor_short};
enum class Terrain {empty,floor,offset_floor,child_cell_empty,bounds_exit,area_no_spare,area_spare};
constexpr std::array cases{
    Case{0x0101,3,0,1,4,4}, Case{0x0101,3,0,0,4,4},
    Case{0x0101,3,0,1,2,4}, Case{0x0101,3,0,1,4,2},
    Case{0x0101,3,0,1,0,4}, Case{0x0101,3,0,1,4,0},
    Case{0x0101,2,0,1,4,4}, Case{0x0101,2,0,0,4,4},
    Case{0x0000,2,0,1,1,1}, Case{0x0000,2,0,0,1,1},
    Case{0x0003,2,0,1,2,2}, Case{0x0003,2,1,1,2,1}, Case{0x0003,2,1,1,2,2},
    Case{0x000A,2,0,1,0,1}, Case{0x000A,2,0,2,1,1},
    Case{0x000C,2,0,2,1,1},
};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){
        if(va<GC_RAM_BASE||va-GC_RAM_BASE>ram.size()-4)fail("bomb fixture address extent");
        return ram.data()+va-GC_RAM_BASE;
    }
    u16 half(u32 va){const auto* b=at(va);return u16(u16(b[0])<<8|b[1]);}
    float fp(u32 va){return std::bit_cast<float>(be32(at(va)));}
    static u32 cell(unsigned i){return cells+i*cell_stride;}
    static u32 descriptor(unsigned i){return descriptors+i*descriptor_stride;}
    void free_pools(unsigned objects,unsigned capacity){
        if(objects>4||capacity>4)fail("bomb pool capacity extent");
        put32(at(0x80436380),descriptor_row);
        put16(at(descriptor_row+18),u16(capacity));
        put32(at(descriptor_row+12),capacity?descriptor(0):0);
        for(unsigned i=0;i<capacity;++i)
            put32(at(descriptor(i)),i+1<capacity?descriptor(i+1):0);
        put16(at(free_count+6),u16(objects));
        put32(at(free_head+12),objects?cell(0):0);
        put32(at(free_tail+12),objects?cell(objects-1):0);
        for(unsigned i=0;i<objects;++i){
            at(cell(i))[2]=3;
            put32(at(cell(i)+4),i+1<objects?cell(i+1):0);
        }
    }
    void owner(const Case& c){
        auto* a=at(actor);
        a[0]=1;a[0x18]=1;
        put16(a+0x3E8,c.model);put16(a+0x94,c.model);
        a[0x580]=c.state;a[0x7C8]=c.extra;
        put32(a+0x90,actor);put32(a+0x4AC,spec);
        put16(a+0x784,c.stock);put16(a+0x786,c.stock);
        put16(a+0x79E,c.model==0x0101?1:5);put16(a+0x7A2,c.model==0x0101?120:300);
        put_float(a+0xB4,1.f);put_float(a+0xB8,1.f);put_float(a+0xC0,1.f);put_float(a+0xC4,1.f);
        // Keep the Powered Gunman nonrelease path at the movement-call
        // frontier too; no synthetic result or guest epilogue is supplied.
        put_float(a+0x38,300.f);
        put32(at(0x8043B5A0-31852),scene);put32(at(0x8043B5A0-31856),mode);
        // Two distinct rigid sockets. The launcher uses the original
        // matrix math on these inlets; no scene or pose producer runs.
        for(const unsigned joint:{7u,11u}){
            auto* m=a+0x8D4+joint*48;
            for(unsigned axis=0;axis<3;++axis)put_float(m+20*axis,1.f);
            put_float(m+12,float(10*joint));put_float(m+28,200.f);put_float(m+44,300.f);
        }
        put_float(a+0x7B0,23.f); // distinct declared add-parent-offset operand for key 2
    }
    void check_pools(const Case& c,const std::array<u8,4>& keys,unsigned made){
        unsigned seen=0;
        u32 prior=0;
        for(u32 obj=be32(at(active_head+36));obj;obj=be32(at(obj+8))){
            if(seen>=made)fail("bomb active-list length");
            const unsigned index=made-1-seen; // Original factory prepends each accepted cell.
            if(obj!=cell(index)||be32(at(obj+4))!=prior||
               !at(obj)[0]||at(obj)[1]!=9||at(obj)[2]!=3||at(obj)[0x10]!=15||
               at(obj)[0x11]!=keys[index]||at(obj)[0x83]!=0x10||
               be32(at(obj+0x8C))!=actor||be32(at(obj+0x90))!=actor||
               be32(at(obj+0xE0))!=descriptor(index)){
                std::fprintf(stderr,"bomb list model=%04X index=%u obj=%08X prev=%08X next=%08X "
                             "on=%u bucket=%u kind=%u type=%u key=%u tag=%u owner=%08X credit=%08X descriptor=%08X\n",
                             c.model,index,obj,be32(at(obj+4)),be32(at(obj+8)),at(obj)[0],at(obj)[1],at(obj)[2],
                             at(obj)[0x10],at(obj)[0x11],at(obj)[0x83],be32(at(obj+0x8C)),be32(at(obj+0x90)),
                             be32(at(obj+0xE0)));
                fail("bomb active-list order, header or descriptor association");
            }
            prior=obj;
            ++seen;
        }
        if(seen!=made||be32(at(active_tail+36))!=prior||half(free_count+6)!=c.carriers-made||
           half(descriptor_row+18)!=c.descriptors-made||
           be32(at(free_head+12))!=(c.carriers>made?cell(made):0)||
           be32(at(descriptor_row+12))!=(c.descriptors>made?descriptor(made):0))
            fail("bomb allocator did not preserve the unused pools");
    }
    void shape_pool(unsigned capacity){
        if(capacity>4)fail("bomb shape pool capacity extent");
        put32(at(free_head+16),capacity?shapes:0);
        put32(at(free_tail+16),capacity?shapes+(capacity-1)*shape_stride:0);
        put16(at(free_count+8),u16(capacity));
        for(unsigned i=0;i<capacity;++i){
            const u32 shape=shapes+i*shape_stride;
            at(shape)[2]=4;
            put32(at(shape+4),i+1<capacity?shape+shape_stride:0);
        }
    }
    void bind_inlets(const Case& c,const HitBlock& block,unsigned capacity){
        std::memcpy(at(hit_block),block.data(),block.size());
        put32(at(actor+0x3FC),hit_block);
        put32(at(actor+0x27C),c.model==0x0101?0x8034EAF8:0x802D4160);
        // Scheduler seat zero visits bucket 9. Other seats have inert headers.
        put32(at(0x803C4E84),actor);
        for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
        shape_pool(capacity);
        // Render inlets point to a declared missing model record; effect
        // cells are unavailable, independently of the shape pool.
        constexpr u32 resource=0x8103A000,link=resource+0x40,table=resource+0x80,row=resource+0x880;
        put32(at(0x803BB4C0+0x600),resource);
        put32(at(0x803BB32C+72),resource);
        put32(at(resource+16),link);put32(at(link),table);
        put32(at(table+4*14),row);put32(at(table+4*125),row);
        // Supply an empty bounded stage, not a patched collision result.
        constexpr u32 grid=stage+0x100,refs=stage+0x200,side=128,empty=refs+side*side*4;
        put32(at(0x8043B5A0-21316),stage);
        for(unsigned i=0;i<3;++i)put32(at(stage+28+4*i),grid);
        put_float(at(stage+52),9000.f);put_float(at(stage+56),31000.f);
        put_float(at(stage+60),-1000.f);put_float(at(stage+64),31000.f);put_float(at(stage+68),-1000.f);
        put32(at(grid+12),side);put32(at(grid+16),u32(-1000));put32(at(grid+20),u32(-1000));
        put32(at(grid+24),refs);put32(at(empty),0xFFFFFFFFu);
        for(unsigned i=0;i<side*side;++i)put32(at(refs+4*i),empty);
    }
    void terrain_inlet(Terrain terrain){
        if(terrain==Terrain::empty)return;
        if(terrain==Terrain::bounds_exit){
            put_float(at(stage+64),400.f);
            return;
        }
        // One rebased STIH triangle in layer zero, no other geometry.
        // List addresses and triangle references are guest pointers, as
        // after the original loader's separate rebase.
        constexpr u32 grid=stage+0x140,refs=0x81053000,side=16,list=refs+4*side*side,tri=list+8;
        const float x=terrain==Terrain::offset_floor?2000.f:-500.f;
        constexpr float y=25.f;
        const std::array vertices{x,y,-500.f,x,y,6000.f,x+3500.f,y,-500.f};
        put32(at(stage+28),grid);
        put32(at(grid),500);put32(at(grid+4),500);put32(at(grid+8),side);put32(at(grid+12),side);
        put32(at(grid+16),u32(-1000));put32(at(grid+20),u32(-1000));
        put32(at(grid+24),refs);put32(at(grid+28),tri);
        for(unsigned i=0;i<side*side;++i)put32(at(refs+4*i),list);
        put32(at(list),tri);put32(at(list+4),0xFFFFFFFFu);
        for(unsigned i=0;i<vertices.size();++i)put_float(at(tri+4+4*i),vertices[i]);
        put_float(at(tri+44),1.f);put_float(at(tri+52),-y);
    }
    void retire_unbound(const Case& c,unsigned count,unsigned stock){
        // A shape becomes free before the next visit. The refused carriers
        // must take disposal phases instead of making a fresh bind attempt.
        shape_pool(1);
        for(unsigned phase:{3u,0u}){
            CPUState cpu{};cpu.gpr[3]=8;
            CallObservation observation{bind};
            mod.run_observed(cpu,ram,visit,observation,1024);
            if(observation.count||half(free_count+8)!=1||be32(at(active_head+49*4))||half(actor+0x784)!=stock)
                fail("bomb refused shape was retried or stock changed");
            for(unsigned i=0;i<count;++i)
                if(phase?(!at(cell(i))[0]||at(cell(i))[0x18]!=phase):at(cell(i))[0])
                    fail("bomb unbound carrier did not take its disposal phases");
            if(half(free_count+6)!=(phase?c.carriers-count:c.carriers)||
               half(descriptor_row+18)!=(phase?c.descriptors-count:c.descriptors)||
               (phase?be32(at(active_head+9*4))!=cell(count-1):be32(at(active_head+9*4))!=0))
                fail("bomb disposal did not return the carrier and descriptor pools");
        }
    }
    void class4_passes(){
        for(u32 entry:{0x80089554u,0x8008A28Cu,0x80089AECu}){
            CPUState pass{};
            mod.run(pass,ram,entry,1024);
        }
    }
    void bind_receiver(const HitBlock& block,const Point& center){
        std::memcpy(at(body_block),block.data(),block.size());
        // Narrow the runtime list, not the exact body descriptor.
        at(body_block)[0]=9;at(body_block)[1]=255;
        auto* target=at(receiver);
        target[0]=1;target[0x18]=1;target[0x88]=1;target[0x3E4]=1;
        put32(at(0x803C4E84+4),receiver);
        put16(target+0x3E8,0x000A);put32(target+0x3FC,body_block);
        put32(target+0x27C,0x802D4160);put32(target+0x76C,1u<<2);
        put_float(target+0xC0,1.f);
        auto* joint=target+0x8D4+2*0x30;
        for(unsigned axis=0;axis<3;++axis)put_float(joint+20*axis,1.f);
        put_float(joint+12,center[0]);
        put_float(joint+28,center[1]+14.f);
        put_float(joint+44,center[2]);
        // Supply a distinct body cell without taking any declared spare
        // area cell away. The original bind must pop only this cell.
        const auto available=half(free_count+8);
        const auto head=be32(at(free_head+16)),tail=be32(at(free_tail+16));
        if(available>1||bool(head)!=bool(available)||head!=tail||head==body)
            fail("unsupported receiver class-4 inlet");
        at(body)[2]=4;put32(at(body+4),head);put32(at(free_head+16),body);
        if(!available)put32(at(free_tail+16),body);
        put16(at(free_count+8),u16(available+1));
        CPUState cpu{};cpu.gpr[3]=receiver;
        mod.run(cpu,ram,0x8008AB30);
        if(be32(at(active_head+50*4))!=body||be32(at(body+0x28))!=body_block+32+9*80||
           half(free_count+8)!=available||be32(at(free_head+16))!=head||be32(at(free_tail+16))!=tail)
            fail("bomb fixture did not bind original singleton body");
        class4_passes();
        if(half(receiver_count)!=1||be32(at(receiver_registry))!=body||fp(body+0x7C)!=22.f||
           std::abs(fp(body+0x64)-center[0])>0.001f||
           std::abs(fp(body+0x68)-center[1])>0.001f||fp(body+0x6C)!=center[2])
            fail("bomb fixture body or registry geometry mismatch");
    }
    void damage_inlet(const ActorSpec& attacker,const ActorSpec& defender,u16 hp){
        constexpr u32 receiver_spec=0x81015400,difficulty=0x81019600;
        std::memcpy(at(spec),attacker.data(),attacker.size());
        std::memcpy(at(receiver_spec),defender.data(),defender.size());
        put32(at(receiver+0x4AC),receiver_spec);
        put32(at(0x8043B5A0-31824),difficulty);
        for(unsigned team=0;team<2;++team){
            const auto max_hp=team?u16(180):hp;
            auto* fighter=at(team?receiver:actor);
            put32(fighter+0x90,team?receiver:actor);
            put_float(fighter+0xB4,1.f);put_float(fighter+0xC4,1.f);
            put16(fighter+0x1C4,max_hp);put16(fighter+0x1C6,max_hp);put16(fighter+0x1C8,0xFFFF);
            put16(at(0x803B069C+2*team),max_hp);
            put32(at(0x803B068C+4*team),100);
            put32(at(scene+team*60+0x114),100);put32(at(scene+team*60+0x118),100);
        }
    }
    void receiver_inlet(const HitBlock& block,unsigned distance,const ActorSpec& attacker,
                        const ActorSpec& defender,u16 hp){
        bind_receiver(block,{fp(shapes+0x64)+float(distance),fp(shapes+0x68),fp(shapes+0x6C)});
        if(half(attack_count)!=1||be32(at(attack_registry))!=shapes)
            fail("bomb fixture opening attack registry mismatch");
        damage_inlet(attacker,defender,hp);
    }
    unsigned contact_write(u32 obj,u32 record,u16 hp,bool should_hit){
        const auto before=ram;
        CPUState input{};
        const auto cut=mod.run_to_or_return(input,ram,0x8002DB58,{0x8003CD5C,0x8003D344});
        if((should_hit?(cut!=0x8003CD5C||input.lr!=0x8002F10C||input.gpr[3]!=record||
           input.gpr[4]!=obj||input.gpr[5]!=receiver):cut!=0)||half(receiver+0x1C6)!=hp){
            std::fprintf(stderr,"bomb pair object=%08X expected=%08X hp=%u cut=%08X lr=%08X record=%08X "
                         "damage-actor=%08X receiver=%08X\n",
                         obj,should_hit?record:0,hp,cut,input.lr,input.gpr[3],input.gpr[4],input.gpr[5]);
            fail("bomb pair amount record mismatch");
        }
        if(!should_hit)return 0;
        std::memcpy(ram.data(),before.data(),ram.size());
        CPUState writer{};
        mod.run_to(writer,ram,0x8002DB58,{0x8003D344});
        if(writer.lr!=0x8002F1DC||writer.gpr[3]!=receiver||half(receiver+0x1C6)!=hp)
            fail("bomb pair reached the wrong HP writer");
        const unsigned amount=writer.gpr[4];
        mod.finish_stopped_leaf(writer,0x8003D344,164);
        return amount;
    }
    void visit_areas(){
        CPUState pass{};pass.gpr[3]=9;
        mod.run(pass,ram,visit,1024);
        class4_passes();
    }
    void area_inlets(const CommonCollision& common){
        std::memcpy(at(common_collision),common.data(),common.size());
        put32(at(scene+152),common_collision); // Exact scene-bank inlet normally written by 80040FC0.
        constexpr u32 resource_table=0x8103A080,empty_visual=0x8103A880;
        put32(at(resource_table+4*22),empty_visual);
        put32(at(resource_table+4*18),empty_visual);
        put32(at(resource_table+4*23),empty_visual);
    }
    bool run_same_pass_area_init(u32 child,bool spare){
        CPUState pass{};pass.gpr[3]=9;
        CallObservation bind_area{0x8008AE3C};
        const auto cut=mod.run_to_or_return(pass,ram,visit,{audio},&bind_area);
        if(bind_area.count!=1||bind_area.caller!=0x800860E0||bind_area.arguments[0]!=actor||
           bind_area.arguments[1]!=child||bind_area.arguments[2]!=3||
           cut!=(spare?audio:0)||(cut&&(pass.lr!=0x80086240||pass.gpr[3]!=child||pass.gpr[4]!=12)))
            fail("same-pass area used the wrong bind or sound request");
        return cut!=0;
    }
    void floor_area_contact(u16 model,unsigned impact,bool spare,unsigned offset){
        const u32 obj=cell(0),child=cell(1),shape=shapes+shape_stride;
        // The first growth visit is already complete. Keep the same
        // receiver pose and tokens while advancing to the second.
        unsigned hits=0,first_amount_visit=0,duplicates=0,last_token=0;
        std::array<unsigned,2> hp{};
        for(unsigned i=0;i<hp.size();++i){
            if(i)visit_areas();
            const bool hit=spare&&!hits&&(i==1||offset==56);
            if(half(attack_count)!=unsigned(spare)||half(receiver_count)!=1||
               (spare&&(be32(at(attack_registry))!=shape||half(child+0x26C)!=2||
                        std::abs(fp(body+0x64)-fp(shape+0x64)-float(offset))>0.001f||
                        fp(body+0x68)!=fp(shape+0x68)||fp(body+0x6C)!=fp(shape+0x6C)||
                        std::abs(fp(shape+0x7C)-(i?65.1f:34.1f))>0.001f))||
               (i&&(at(obj)[0]||bool(at(child)[0])!=spare)))
                fail("bomb floor-contact next-visit publication or retirement mismatch");
            const auto before_hp=half(receiver+0x1C6);
            const auto amount=contact_write(child,area_amount,before_hp,hit);
            if(hit){
                ++hits;
                first_amount_visit=impact+i+1;
                if(amount!=17||half(receiver+0x1C8)!=before_hp)
                    fail("floor area used the wrong damage or previous HP");
                contact_write(child,area_amount,u16(before_hp-amount),false);
                ++duplicates;
            }else if(hits){
                ++duplicates;
            }
            last_token=half(receiver+0x20C);
            hp[i]=half(receiver+0x1C6);
            if(last_token!=2*hits||half(receiver+0x20E)||at(receiver)[0x26E]!=hits||
               hp[i]!=180-17*hits||half(actor+0x784)!=1||be32(at(actor+0x418))!=17*hits||
               be32(at(receiver+0x41C))!=17*hits||be32(at(receiver+0x290))!=(hits?child:0))
                fail("floor area changed the wrong HP, recipient, credit or stock");
        }
        std::printf("bomb-floor-contact: %u %u %u %u %u %u %u %u %u %u\n",
                    model,unsigned(spare),offset,impact,first_amount_visit,hp[0],hp[1],
                    last_token,duplicates,half(actor+0x784));
    }
    void floor_area_visit(u16 model,unsigned n,bool spare,float crossing_z,const std::vector<u8>& before,
                          const AreaReceiver* target){
        // Repeat the original owner-list pass from before impact, this
        // time letting the appended child initialize at its real position.
        std::memcpy(ram.data(),before.data(),ram.size());
        const u32 obj=cell(0),child=cell(1),child_shape=shapes+shape_stride;
        const auto cue=run_same_pass_area_init(child,spare);
        if(at(obj)[0x18]!=2||at(child)[0x18]!=(spare?1:2)||fp(child+0x24)!=25.f||fp(child+0x28)!=crossing_z)
            fail("same-pass floor area initializer disagreed");
        class4_passes();
        if(half(attack_count)!=1||be32(at(attack_registry))!=shapes||be32(at(active_tail+49*4))!=shapes||
           be32(at(active_head+49*4))!=(spare?child_shape:shapes)||half(free_count+8)||
           at(child)[0x86]!=unsigned(spare)||half(child+0x26C)!=(spare?2:0))
            fail("floor area shape result did not preserve the parent's first-visit publication");
        if(target){
            // No actor is present during flight. Place one at a fixed
            // offset from the committed point for every later pair pass.
            bind_receiver(target->block,{165.f+float(target->offset_x),25.f,crossing_z});
            damage_inlet(target->attacker,target->defender,target->attacker_hp);
            if(half(attack_count)!=1||be32(at(attack_registry))!=shapes)
                fail("floor impact pair did not begin with only the opening shape");
            contact_write(obj,0x802D4178,180,false);
            if(at(obj)[0x1D9]||be32(at(obj+0x290))||half(receiver+0x20C)||at(receiver)[0x26E]||
               be32(at(actor+0x418))||be32(at(receiver+0x41C)))
                fail("opening bomb made contact in the floor area test");
        }
        CPUState next{};next.gpr[3]=9;
        CallObservation retry{0x8008AE3C};
        mod.run_observed(next,ram,visit,retry);
        class4_passes();
        if(retry.count||at(obj)[0x18]!=3||at(child)[0x18]!=(spare?1:3)||
           be32(at(active_head+49*4))!=(spare?child_shape:0)||be32(at(free_head+16))!=shapes||
           half(free_count+8)!=1||half(actor+0x784)!=1||half(attack_count)!=unsigned(spare)||
           (spare&&(be32(at(attack_registry))!=child_shape||be32(at(child_shape+0x20))!=child||
                   be32(at(child_shape+0x24))!=actor||fp(child_shape+0x68)!=25.f||
                   fp(child_shape+0x6C)!=crossing_z||std::abs(fp(child_shape+0x7C)-34.1f)>0.001f)))
            fail("next floor visit rebound a refusal or lost the grown area");
        if(target)floor_area_contact(model,n,spare,target->offset_x);
        else std::printf("bomb-floor-area: %u %u %u %u %u %u %u %u %u\n",
                         model,unsigned(spare),n,unsigned(cue),unsigned(spare),at(child)[0x18],
                         half(attack_count),half(free_count+8),half(actor+0x784));
    }
public:
    Check(const Module& module,const std::vector<u8>& original):mod(module),ram(original){}
    void run_case(const Case& c,bool report=true){
        owner(c);free_pools(c.carriers,c.descriptors);
        const bool gun=c.model==0x0101,charged=gun&&c.state==3;
        const bool ordinary_paid=!charged&&c.stock;
        std::array<u8,4> keys{};
        unsigned count=0;
        if(charged){keys={8,9,6,7};count=4;}
        else if(ordinary_paid&&!gun){
            keys[0]=c.model==3?1:c.model==0xA?3:c.model==0xC?5:0;
            count=1;
            if(c.model==3&&c.extra){keys[1]=2;count=2;}
        }
        // The Gunman slice starts after the separate cue and movement
        // owners. Observe the actual state split and commit below it.
        CPUState cpu{};cpu.gpr[3]=actor;cpu.gpr[31]=actor;
        CallObservation observation{factory};
        const auto boundary=mod.run_to_or_return(cpu,ram,gun?powered:c.model==0xC?alpha:ninja,
                                                  {audio,normal_x_helper,movement_tail},&observation);
        const u32 expected_boundary=charged?audio:gun&&ordinary_paid?normal_x_helper:gun?movement_tail:0;
        if(boundary!=expected_boundary||observation.count!=count||
           (boundary==audio&&(cpu.lr!=0x801671C4||cpu.gpr[3]!=actor||cpu.gpr[4]!=6))||
           (boundary==normal_x_helper&&(cpu.lr!=0x80167154||cpu.gpr[3]!=actor||cpu.gpr[4]!=0))||
           (boundary==movement_tail&&(cpu.lr!=0x801671E0||cpu.gpr[3]!=actor))){
            std::fprintf(stderr,"bomb model=%04X state=%u boundary=%08X lr=%08X calls=%u\n",
                         c.model,c.state,boundary,cpu.lr,observation.count);
            fail("bomb release left its reviewed continuation");
        }
        unsigned attempted=0,published=0;
        const auto made=std::min({count,unsigned(c.carriers),unsigned(c.descriptors)});
        for(unsigned i=0;i<count;++i){
            if(observation.second_arguments[i]!=keys[i])fail("bomb requested the wrong factory key");
            attempted=10*attempted+keys[i];
            if(i>=made)continue;
            published=10*published+keys[i];
            const bool lower=keys[i]==8||keys[i]==9,left=keys[i]==8||keys[i]==6;
            const float x=gun?(left?70.f-118.f:110.f+118.f):165.f+(keys[i]==2?23.f:0.f);
            const float y=gun?(lower?178.f:236.f):200.f,z=gun?330.f:300.f;
            if(fp(cell(i)+0x20)!=x||fp(cell(i)+0x24)!=y||fp(cell(i)+0x28)!=z)
                fail("bomb did not sample its exact constructor socket");
        }
        check_pools(c,keys,made);
        const auto remaining=half(actor+0x784);
        const auto clock=fp(actor+0x788);
        if(remaining!=(ordinary_paid?c.stock-1:c.stock)||
           clock!=(ordinary_paid&&!remaining?(gun?120.f:300.f):0.f))
            fail("bomb stock debit or refill-clock mismatch");
        if(report)std::printf("bomb: %u %u %u %u %u %u %u %u %u %u %u %.0f %u\n",
                    c.model,c.state,c.extra,c.stock,c.carriers,c.descriptors,count,attempted,
                    made,published,remaining,double(clock),boundary==audio?1:boundary==normal_x_helper?2:boundary==movement_tail?3:0);
    }
    void first_bind(const Case& c,const HitBlock& block,unsigned capacity,bool report=true,Terrain terrain=Terrain::empty){
        run_case(c,false);
        bind_inlets(c,block,capacity);
        terrain_inlet(terrain);
        const bool gun=c.model==0x0101;
        const unsigned count=gun?std::min(c.carriers,c.descriptors):1;
        const unsigned group=gun?15:1,step=gun?8:1,accepted=std::min(count,capacity);
        CPUState cpu{};cpu.gpr[3]=8;
        CallObservation observation{bind};
        mod.run_observed(cpu,ram,visit,observation,1024);
        if(observation.count!=count||observation.caller!=0x8007DE04)
            fail("bomb scheduler did not bind every published carrier");
        unsigned order=0,bound=0;
        u32 last=0;
        for(unsigned i=0;i<count;++i){
            const auto obj=cell(count-1-i);
            const auto* carrier=at(obj);
            order=10*order+carrier[0x11];
            if(observation.first_arguments[i]!=obj||observation.second_arguments[i]!=group||
               carrier[0x18]!=(i<accepted?1:2)||carrier[0x86]!=unsigned(i<accepted)||
               half(obj+0x1C)!=(i<accepted?599:0))
                fail("bomb bind order, phase, or first-flight timer disagrees");
            if(i>=accepted)continue;
            bound=10*bound+carrier[0x11];
            const u32 shape=shapes+shape_stride*i;
            const u32 previous=i+1<accepted?shape+shape_stride:0;
            if(at(shape)[0]!=1||at(shape)[1]!=49||at(shape)[2]!=4||at(shape)[0x12]!=step||
               at(shape)[0x11]!=1||at(shape)[0x14]!=group||be32(at(shape+4))!=previous||
               be32(at(shape+8))!=last||be32(at(shape+0x20))!=obj||be32(at(shape+0x24))!=actor||
               be32(at(shape+0x28))!=hit_block+0xAA0+80*step||
               be32(at(shape+0x2C))!=be32(at(actor+0x27C))||be32(at(shape+0x60))!=obj+0x114)
                fail("bomb shape list, descriptor, or credit mismatch");
            last=shape;
        }
        if(half(free_count+8)!=capacity-accepted||
           be32(at(active_head+49*4))!=last||be32(at(active_tail+49*4))!=(accepted?shapes:0)||
           half(actor+0x784)!=(gun?c.stock:c.stock-1))
            fail("bomb post-bind pools or stock disagree");
        class4_passes();
        const auto registered=half(attack_count);
        if(registered!=accepted)fail("bomb attack registry count disagrees");
        unsigned registry_order=0;
        for(unsigned i=0;i<registered;++i){
            const auto shape=be32(at(attack_registry+4*i));
            if(shape!=shapes+shape_stride*(accepted-1-i))fail("bomb attack registry order disagrees");
            registry_order=10*registry_order+at(be32(at(shape+0x20)))[0x11];
        }
        const auto stock=half(actor+0x784);
        if(!capacity)retire_unbound(c,count,stock);
        if(report)std::printf("bomb-bind: %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                    c.model,c.carriers,c.descriptors,capacity,count,order,accepted,bound,group,step,stock,
                    registered,registry_order,capacity?0:3);
    }
    void contact(bool gun,const HitBlock& block,const HitBlock& defender,const ActorSpec& credit_spec,
                 const ActorSpec& defender_spec,Contact scenario,bool leave_area=false){
        const bool miss=scenario==Contact::miss,duplicate=scenario==Contact::duplicate;
        const bool hit=!miss&&!duplicate,spawn=scenario==Contact::impact;
        const bool no_cell=scenario==Contact::child_cell_empty,short_desc=scenario==Contact::child_descriptor_short;
        const u8 carriers=gun||no_cell?1:2,descriptors=gun?(short_desc?1:2):(short_desc?2:3);
        const Case c{u16(gun?0x0101:0x000A),u8(gun?3:2),0,u8(gun?1:2),carriers,descriptors};
        first_bind(c,block,1,false);
        if(gun&&!no_cell){
            at(effect_cell)[2]=2;
            put32(at(free_head+8),effect_cell);put32(at(free_tail+8),effect_cell);
            put16(at(free_count+4),1);
        }
        if(leave_area&&(gun||!spawn))fail("unsupported bomb area fixture seed");
        receiver_inlet(defender,leave_area?30:miss?256:0,credit_spec,defender_spec,gun?200:180);
        const auto obj=cell(0);
        const auto* carrier=at(obj);const auto* target=at(receiver);
        const auto token=half(obj+0x26C);
        if(token!=1||half(receiver+0x20C)||target[0x26E]||carrier[0x1D9]||fp(shapes+0x7C)!=(gun?20.f:10.f))
            fail("bomb contact token, shape radius or flags baseline mismatch");
        if(duplicate)put16(at(receiver+0x20C),token);
        const u32 amount_record=gun?0x8034EB10:0x802D4178;
        const auto amount=contact_write(obj,amount_record,180,hit);
        const unsigned key=carrier[0x11],flags=carrier[0x1D9],receiver_token=half(receiver+0x20C),history=target[0x26E];
        const auto hp=half(receiver+0x1C6);
        if(flags!=(hit?16:0)||receiver_token!=unsigned(hit||duplicate)||history!=unsigned(hit)||
           amount!=(hit?(gun?17:8):0)||hp!=180-amount||half(receiver+0x1C8)!=(hit?180:65535)||
           be32(at(actor+0x418))!=amount||be32(at(receiver+0x41C))!=amount||
           be32(at(obj+0x290))!=(hit?receiver:0)||be32(at(actor+0x290))!=(hit?receiver:0)||
           be32(at(receiver+0x290))!=(hit?obj:0))
            fail("bomb contact history, recipient, HP or ledger mismatch");

        // Run only bucket 9 and stop at the Sasuke child's own callback
        // if appended there. Gunman's child is in bucket 25, not visited.
        const auto entry=gun?0x8001BDA8u:0x80085E00u;
        const auto other=gun?0x80085E00u:0x8001BDA8u;
        const auto pos=std::array{be32(at(obj+0x20)),be32(at(obj+0x24)),be32(at(obj+0x28))};
        CPUState update{};update.gpr[3]=9;
        CallObservation request{entry};
        const auto update_cut=mod.run_to_or_return(update,ram,visit,{other,area_callback},&request);
        if(update_cut!=(!gun&&spawn?area_callback:0)||(!gun&&spawn&&update.gpr[3]!=cell(1))||
           request.count!=unsigned(hit)||(hit&&(request.caller!=(gun?0x8007E298:0x8007E284)||
              request.arguments[0]!=obj||request.arguments[1]!=obj+0x20||request.arguments[2]!=0)))
            fail("bomb first follow-up requested the wrong child");
        class4_passes();
        const u32 bucket_offset=4*(gun?25u:9u);
        const u32 child=gun?be32(at(active_head+bucket_offset)):be32(at(obj+8));
        const unsigned phase=carrier[0x18],timer=half(obj+0x1C),attacks=half(attack_count);
        const auto stock=half(actor+0x784);
        const auto expected_child=spawn?(gun?effect_cell:cell(1)):0;
        if(child!=expected_child||be32(at(active_tail+bucket_offset))!=(gun||child?child:obj)||phase!=(hit?2:1)||
           attacks!=unsigned(!hit)||timer!=598||stock!=1||half(receiver+0x1C6)!=hp||
           be32(at(active_head+49*4))!=(hit?0:shapes)||be32(at(active_head+9*4))!=obj||
           half(free_count+8)!=unsigned(hit)||half(receiver_count)!=1){
            std::fprintf(stderr,"model=%04X case=%u child=%08X head9=%08X head25=%08X phase=%u timer=%u "
                         "attacks=%u stock=%u hp=%u shape=%08X free=%u body=%u\n",
                         c.model,unsigned(scenario),child,be32(at(active_head+9*4)),be32(at(active_head+25*4)),
                         phase,timer,attacks,stock,half(receiver+0x1C6),be32(at(active_head+49*4)),
                         half(free_count+8),half(receiver_count));
            fail("bomb first follow-up phase, contact shape or child list mismatch");
        }
        unsigned child_kind=0,child_key=0;
        if(child){
            const auto* a=at(child);
            child_kind=a[0x10];child_key=a[0x11];
            if(!a[0]||a[2]!=(gun?2:3)||child_kind!=(gun?4:5)||child_key||
               be32(a+0x8C)!=obj||be32(a+0x90)!=actor||a[0x18]||a[0x86]||
               be32(a+0x20)!=pos[0]||be32(a+0x24)!=pos[1]||be32(a+0x28)!=pos[2]||
               be32(a+0xE0)!=descriptor(1)||be32(a+0xE4)!=(gun?0:descriptor(2)))
                fail("bomb child did not retain its tested owner, credit, origin or descriptors");
        }
        const unsigned spent_child=child?(gun?1:2):0;
        if(half(descriptor_row+18)!=descriptors-1-spent_child||
           half(free_count+6)!=carriers-1-unsigned(child&&!gun)||
           half(free_count+4)!=unsigned(gun&&!no_cell&&!child))
            fail("bomb impact child did not consume exactly its own pool entries");
        if(leave_area)return;
        if(hit){
            for(const bool returning:{false,true}){
                CPUState pass{};pass.gpr[3]=9;
                const auto child_cut=mod.run_to_or_return(pass,ram,visit,{area_callback});
                if((child_cut&&(gun||!child||pass.gpr[3]!=child))||(!returning&&!gun&&child&&!child_cut)||
                   (returning?bool(carrier[0]):(!carrier[0]||carrier[0x18]!=3))||
                   be32(at(active_head+9*4))!=(returning?(gun?0:child):obj)||
                   half(descriptor_row+18)!=descriptors-unsigned(!returning)-spent_child||
                   half(free_count+6)!=carriers-unsigned(!returning)-unsigned(child&&!gun)||
                   half(actor+0x784)!=stock||half(receiver+0x1C6)!=hp||
                   (child&&(!at(child)[0]||at(child)[0x18]||at(child)[0x86])))
                    fail("contact bomb did not retire independently of its untouched child");
            }
        }
        std::printf("bomb-contact: %u %u %u %u %u %08X %u %u %u %u %u %u %u %u %u %u %u\n",
                    c.model,unsigned(scenario),key,flags,receiver_token,hit?amount_record:0,history,amount,hp,
                    phase,timer,attacks,hit?3:0,unsigned(request.count),child_kind,child_key,stock);
    }
    void sasuke_area(const HitBlock& block,const ActorSpec& actor_spec,const CommonCollision& common,bool free_shape){
        contact(false,block,block,actor_spec,actor_spec,Contact::impact,true);
        const auto child=cell(1);
        area_inlets(common);
        // The released parent shape is the only free cell. A reservation
        // control withholds it for the area's bind then makes it free again.
        if(half(free_count+8)!=1||be32(at(free_head+16))!=shapes||be32(at(free_tail+16))!=shapes)
            fail("area expects a released opening shape");
        if(!free_shape)shape_pool(0);
        CPUState init{};init.gpr[3]=child;
        CallObservation select{0x8008AE3C};
        const auto frontier=mod.run_to_or_return(init,ram,area_callback,{audio},&select);
        if(select.count!=1||select.caller!=0x800860E0||select.arguments[0]!=actor||
           select.arguments[1]!=child||select.arguments[2]!=3||
           frontier!=(free_shape?audio:0)||(frontier&&(init.lr!=0x80086240||init.gpr[3]!=child||init.gpr[4]!=12))||
           at(child)[0x18]!=(free_shape?1:2)||at(child)[0x86]!=unsigned(free_shape)||
           bool(be32(at(active_head+49*4)))!=free_shape||half(receiver+0x1C6)!=172||half(actor+0x784)!=1){
            std::fprintf(stderr,"area shape=%u bind=%u caller=%08X args=%08X/%08X/%u "
                         "frontier=%08X lr=%08X phase=%u active=%u head=%08X\n",
                         unsigned(free_shape),select.count,select.caller,select.arguments[0],select.arguments[1],
                         select.arguments[2],frontier,init.lr,at(child)[0x18],at(child)[0x86],
                         be32(at(active_head+49*4)));
            fail("area shape bind, cue frontier, or direct HP mismatch");
        }
        class4_passes();
        const auto initial_radius=fp(shapes+0x7C);
        unsigned token=0,after_growth=0,duplicate_passes=0,unlink_visit=0,retired=0;
        float grown_radius=0,peak_radius=0;
        if(free_shape){
            const auto* shape=at(shapes);
            token=half(child+0x26C);
            CPUState scale_gate{};scale_gate.gpr[3]=shapes;
            mod.run(scale_gate,ram,0x8008AA40);
            if(be32(at(active_head+49*4))!=shapes||half(attack_count)||scale_gate.gpr[3]!=1||
               shape[0x12]!=4||shape[0x11]!=1||shape[0x14]!=3||be32(shape+0x20)!=child||
               be32(shape+0x24)!=actor||be32(shape+0x28)!=common_collision+0x400+4*80||
               be32(shape+0x2C)!=0x802C4760||be32(shape+0x60)!=child+0x114||
               token!=2||half(receiver+0x20C)!=1||at(receiver)[0x26E]!=1||
               fp(body+0x68)!=fp(shapes+0x68)||fp(body+0x6C)!=fp(shapes+0x6C)||
               std::abs(fp(body+0x64)-fp(shapes+0x64)-30.f)>0.001f||std::abs(initial_radius-3.1f)>0.001f){
                std::fprintf(stderr,"area attack=%u head=%08X group/step/index=%u/%u/%u shape=%08X/%08X/%08X/%08X/%08X "
                             "token=%u receiver=%u/%u delta=%.5f/%.5f/%.5f radius=%.5f\n",
                             half(attack_count),be32(at(active_head+49*4)),shape[0x14],shape[0x12],shape[0x11],
                             be32(shape+0x20),be32(shape+0x24),be32(shape+0x28),be32(shape+0x2C),be32(shape+0x60),
                             token,half(receiver+0x20C),at(receiver)[0x26E],double(fp(body+0x64)-fp(shapes+0x64)),
                             double(fp(body+0x68)-fp(shapes+0x68)),double(fp(body+0x6C)-fp(shapes+0x6C)),double(initial_radius));
                fail("area did not register exact common group, origin or fresh token");
            }
            contact_write(child,area_amount,172,false);
            visit_areas();
            grown_radius=fp(shapes+0x7C);
            scale_gate={};scale_gate.gpr[3]=shapes;
            mod.run(scale_gate,ram,0x8008AA40);
            if(scale_gate.gpr[3]||half(attack_count)!=1||be32(at(attack_registry))!=shapes)
                fail("area failed to publish after growing past the scale gate");
            after_growth=contact_write(child,area_amount,172,true);
            const auto hp=half(receiver+0x1C6);
            contact_write(child,area_amount,hp,false);
            ++duplicate_passes;
            if(half(receiver+0x20C)!=1||half(receiver+0x20E)!=2||at(receiver)[0x26E]!=2||
               half(receiver+0x1C8)!=172||hp!=172-after_growth||be32(at(actor+0x418))!=8+after_growth||
               be32(at(receiver+0x41C))!=8+after_growth||be32(at(child+0x290))!=receiver||
               be32(at(receiver+0x290))!=child||std::abs(grown_radius-34.1f)>0.001f)
                fail("area follow-up HP, history or growth mismatch");
            peak_radius=grown_radius;
        }else{
            if(half(attack_count)||half(child+0x26C))fail("refused area published a shape or token");
            shape_pool(1);
        }
        for(unsigned n=free_shape?2:1;n<=96;++n){
            visit_areas();
            const auto remaining=half(attack_count);
            if(n==2&&free_shape){
                // Still overlapping after the parent retires and the
                // registry is rebuilt. The area's own token must survive.
                if(at(cell(0))[0]||!remaining||half(child+0x26C)!=token)
                    fail("area depended on its retired parent or replaced its token");
                contact_write(child,area_amount,u16(172-after_growth),false);
                ++duplicate_passes;
            }
            if(remaining>unsigned(free_shape)||(remaining&&be32(at(attack_registry))!=shapes)||
               be32(at(active_head+49*4))!=(remaining?shapes:0)||
               be32(at(active_tail+49*4))!=(remaining?shapes:0)||
               be32(at(free_head+16))!=(remaining?0:shapes)||half(free_count+8)!=unsigned(!remaining)||
               half(receiver_count)!=1||half(actor+0x784)!=1||half(receiver+0x1C6)!=172-after_growth||
               (!free_shape&&half(child+0x26C)))
                fail("area lifetime changed unrelated pools or HP");
            if(remaining)peak_radius=std::max(peak_radius,fp(shapes+0x7C));
            else if(!unlink_visit&&free_shape)unlink_visit=n;
            if(!at(child)[0]){retired=n;break;}
        }
        if(!retired||be32(at(active_head+9*4))||half(free_count+6)!=2||
           half(descriptor_row+18)!=3||half(free_count+8)!=1||be32(at(free_head+16))!=shapes)
            fail("area did not return both descriptors and carrier");
        std::printf("bomb-area: %u %u %u %.4f %.4f %.4f %u %u %u %u %u %u\n",
                    unsigned(free_shape),unsigned(frontier!=0),token,double(free_shape?initial_radius:0.f),
                    double(grown_radius),double(peak_radius),after_growth,duplicate_passes,half(receiver+0x1C6),
                    unlink_visit,retired,half(actor+0x784));
    }
    void actor_area(u16 model,const HitBlock& block,const CommonCollision& common,const AreaReceiver& target,bool spare){
        const Case c{model,2,0,2,2,3};
        first_bind(c,block,spare?2:1,false);
        receiver_inlet(target.block,target.offset_x,target.attacker,target.defender,target.attacker_hp);
        area_inlets(common);
        const auto obj=cell(0),child=cell(1),area_shape=shapes+shape_stride;
        if(contact_write(obj,0x802D4178,180,true)!=8||at(obj)[0x1D9]!=16||at(obj)[0x1D8]||
           half(receiver+0x1C6)!=172||half(receiver+0x20C)!=1||at(receiver)[0x26E]!=1)
            fail("same-pass area did not begin with the original direct bomb hit");

        const auto before=ram;
        CPUState init{};init.gpr[3]=9;
        CallObservation factory_call{0x80085E00};
        mod.run_to(init,ram,visit,{0x8008AE3C},&factory_call);
        const auto pending=at(obj)[0x1D8];
        const auto free_at_bind=half(free_count+8);
        if(factory_call.count!=1||factory_call.caller!=0x8007E284||factory_call.arguments[0]!=obj||
           factory_call.arguments[1]!=obj+0x20||factory_call.arguments[2]||init.lr!=0x800860E4||
           init.gpr[3]!=actor||init.gpr[4]!=child||init.gpr[5]!=3||at(obj)[0x18]!=2||pending!=1||
           free_at_bind!=unsigned(spare)||be32(at(free_head+16))!=(spare?area_shape:0)||
           be32(at(active_head+49*4))!=shapes||be32(at(shapes+0x20))!=obj||!at(shapes)[0]||
           be32(at(child+0x8C))!=obj||be32(at(child+0x90))!=actor||at(child)[0x10]!=5||at(child)[0x11])
            fail("original actor unlink request freed or replaced its shape before the area bind");

        // Rerun the same bucket from its entry without synthesizing a
        // function result or moving the shape pass ahead of the allocator.
        std::memcpy(ram.data(),before.data(),ram.size());
        const bool cue=run_same_pass_area_init(child,spare);
        if(at(obj)[0x1D8]!=pending||half(free_count+8)||be32(at(active_tail+49*4))!=shapes||
           be32(at(active_head+49*4))!=(spare?area_shape:shapes)||
           at(child)[0x18]!=(spare?1:2)||at(child)[0x86]!=unsigned(spare)||
           half(child+0x26C)||half(receiver+0x1C6)!=172)
            fail("actor area same-pass shape result mismatch before the class-4 pass");
        class4_passes();
        const auto free_after_shape_pass=half(free_count+8);
        if(half(attack_count)||half(receiver_count)!=1||free_after_shape_pass!=1||
           be32(at(free_head+16))!=shapes||half(child+0x26C)!=(spare?2:0)||
           be32(at(active_head+49*4))!=(spare?area_shape:0)||
           (spare&&(be32(at(area_shape+0x20))!=child||be32(at(area_shape+0x24))!=actor||
                    std::abs(fp(area_shape+0x7C)-3.1f)>0.001f)))
            fail("actor shape was not returned by the first separate class-4 pass");
        contact_write(child,area_amount,172,false);

        const unsigned final_hp=spare?155:172;
        unsigned retry_count=0;
        for(unsigned i=0;i<2;++i){
            CPUState later{};later.gpr[3]=9;
            CallObservation retry{0x8008AE3C};
            mod.run_observed(later,ram,visit,retry);
            retry_count+=retry.count;
            class4_passes();
            if(retry.count||half(attack_count)!=unsigned(spare)||half(receiver_count)!=1||
               half(free_count+8)!=1||half(actor+0x784)!=1||
               (i?(at(obj)[0]||bool(at(child)[0])!=spare):(at(obj)[0x18]!=3||at(child)[0x18]!=(spare?1:3)))||
               (spare&&(be32(at(attack_registry))!=area_shape||
                        std::abs(fp(area_shape+0x7C)-(i?65.1f:34.1f))>0.001f)))
                fail("actor area growth or class-4 refusal retried or disagreed");
            const auto amount=contact_write(child,area_amount,u16(i?final_hp:172),spare&&!i);
            if(amount!=(spare&&!i?17:0)||half(receiver+0x1C6)!=final_hp||half(receiver+0x20C)!=1||
               half(receiver+0x20E)!=(spare?2:0)||at(receiver)[0x26E]!=1+unsigned(spare)||
               be32(at(receiver+0x41C))!=180-final_hp||be32(at(actor+0x418))!=180-final_hp)
                fail("actor area HP, token duplication or ledger mismatch");
        }
        if(half(descriptor_row+18)!=(spare?1:3)||half(free_count+6)!=(spare?1:2))
            fail("actor area did not preserve its own descriptor/cell retirement");
        std::printf("bomb-actor-area: %u %u %u %u %u %u %u %u %u %u %u %u\n",
                    model,unsigned(spare),pending,free_at_bind,unsigned(cue),unsigned(spare),free_after_shape_pass,retry_count,
                    final_hp,half(receiver+0x20C),half(receiver+0x20E),half(actor+0x784));
    }
    void floor_impact(u16 model,const HitBlock& block,const CommonCollision& common,Terrain terrain,
                      const AreaReceiver* target=nullptr){
        const bool replay_area=terrain==Terrain::area_no_spare||terrain==Terrain::area_spare;
        const bool spawn=terrain==Terrain::floor||replay_area,bounds=terrain==Terrain::bounds_exit;
        const bool floor=spawn||terrain==Terrain::child_cell_empty;
        const unsigned shape_capacity=terrain==Terrain::area_spare?2:1;
        const unsigned cap=terrain==Terrain::child_cell_empty?1:2;
        const Case c{model,2,0,2,u8(cap),3};
        first_bind(c,block,shape_capacity,false,terrain);
        if(replay_area)area_inlets(common);
        const auto obj=cell(0);
        const auto* carrier=at(obj);
        const auto pos=[&](u32 a){return Point{fp(a+0x20),fp(a+0x24),fp(a+0x28)};};
        auto previous=pos(obj);
        const auto launch=previous;
        if(std::abs(launch[1]-199.85f)>0.0001f||launch[0]!=165.f||launch[2]!=340.f||
           fp(obj+56)!=0.f||std::abs(fp(obj+64)-39.9988785f)>0.0001f||std::abs(fp(obj+60)+0.3f)>0.0001f){
            std::fprintf(stderr,"floor model=%04X launch=%.7f/%.7f/%.7f delta=%.7f/%.7f/%.7f\n",
                         model,double(launch[0]),double(launch[1]),double(launch[2]),
                         double(fp(obj+56)),double(fp(obj+60)),double(fp(obj+64)));
            fail("floor fixture did not start from actual no-target launch motion");
        }
        constexpr unsigned max_visits=64;
        unsigned event=0,after_clock=0,requests=0,returned=0,segment_grid=0,triangle_tests=0,unlink=0;
        Point segment_start{},proposal{},final{};
        for(unsigned n=2;n<=max_visits;++n){
            const auto predicted=Point{previous[0]+fp(obj+56),previous[1]+std::fma(-fp(obj+80),0.5f,fp(obj+60)),
                                       previous[2]+fp(obj+64)};
            const bool crosses=previous[1]>25.f&&predicted[1]<=25.f;
            const auto before=replay_area&&crosses?ram:std::vector<u8>{};
            CPUState pass{};pass.gpr[3]=9;
            CallObservation observe{0x80085E00};
            const auto cut=mod.run_to_or_return(pass,ram,visit,{area_callback,0x8001BDA8},&observe);
            class4_passes();
            if(carrier[0x1D9]||be32(at(obj+0x290))||half(receiver_count)||be32(at(actor+0x418))||
               half(actor+0x784)!=1||observe.count>1)
                fail("terrain replay saw an actor hit, changed stock, or repeated an allocation");
            const bool terminal=carrier[0x18]==2;
            const auto current=pos(obj);
            if((crosses&&!bounds)||terminal){
                // Independently verify the reader/kernel on the segment the
                // carrier just presented, without feeding its result back.
                constexpr u32 pair=0x81057000;
                for(unsigned axis=0;axis<3;++axis){
                    put_float(at(pair+4*axis),previous[axis]);
                    put_float(at(pair+12+4*axis),predicted[axis]);
                }
                CPUState query{};query.gpr[3]=pair;query.gpr[4]=pair+12;query.gpr[5]=pair+24;
                CallObservation triangles{0x80039DA4};
                mod.run_observed(query,ram,0x8003E978,triangles,1024);
                segment_grid=query.gpr[3];triangle_tests=triangles.count;
                if((floor?(segment_grid!=1||!triangle_tests||fp(pair+28)!=25.f):(segment_grid!=0))||
                   ((terrain==Terrain::offset_floor)!=bool(!floor&&triangle_tests)))
                    fail("bomb terrain segment kernel returned the wrong proof");
                event=n;segment_start=previous;proposal=predicted;final=current;after_clock=half(obj+0x1C);
            }
            if(observe.count){
                if(!floor||!crosses||!terminal||observe.caller!=0x8007E5FC||observe.arguments[0]!=obj||
                   observe.arguments[1]!=obj+0x20||observe.arguments[2]||current[1]!=25.f||
                   std::abs(current[2]-fp(0x81057000+32))>0.0001f)
                    fail("bomb used the wrong floor-impact call or corrected crossing point");
                requests=observe.count;
            }
            const auto child=be32(at(obj+8));
            const bool still_bound=!terminal||floor;
            if(cut!=(spawn&&terminal?area_callback:0)||(cut&&pass.gpr[3]!=child)||
               child!=(spawn&&terminal?cell(1):0)||bool(observe.count)!=(floor&&terminal)||
               half(attack_count)!=unsigned(still_bound)||half(free_count+8)!=shape_capacity-unsigned(still_bound)||
               be32(at(active_head+49*4))!=(still_bound?shapes:0)){
                std::fprintf(stderr,"floor model=%04X case=%u n=%u phase=%u cut=%08X/%08X child=%08X "
                             "requests=%u attacks=%u free-shapes=%u\n",model,unsigned(terrain),n,carrier[0x18],
                             cut,pass.gpr[3],child,observe.count,half(attack_count),half(free_count+8));
                fail("floor impact changed the wrong shape or child list");
            }
            if(child){
                if(at(child)[0x10]!=5||at(child)[0x11]||at(child)[0x18]||
                   be32(at(child+0x8C))!=obj||be32(at(child+0x90))!=actor||pos(child)!=current||
                   half(descriptor_row+18)!=0||half(free_count+6)!=0)
                    fail("floor child did not inherit the corrected position and original credit");
            }
            if(terminal){
                if(!floor&&!bounds)fail("clear terrain unexpectedly terminated the bomb");
                if(replay_area){
                    if(before.empty()||!child)fail("missing floor area replay boundary");
                    floor_area_visit(model,n,shape_capacity==2,current[2],before,target);
                    return;
                }
                for(const bool retire:{false,true}){
                    CPUState tail{};tail.gpr[3]=9;
                    CallObservation again{0x80085E00};
                    const auto stopped=mod.run_to_or_return(tail,ram,visit,{area_callback},&again);
                    class4_passes();
                    if(again.count||(stopped&&(!child||tail.gpr[3]!=child))||
                       (retire?bool(carrier[0]):(!carrier[0]||carrier[0x18]!=3))||
                       be32(at(active_head+9*4))!=(retire?child:obj)||
                       half(descriptor_row+18)!=3-unsigned(!retire)-2u*unsigned(bool(child))||
                       half(free_count+6)!=cap-unsigned(!retire)-unsigned(bool(child))||
                       half(attack_count)||half(free_count+8)!=1||be32(at(free_head+16))!=shapes||
                       be32(at(active_head+49*4)))
                        fail("floor-impact parent did not complete its separate retirement");
                }
                unlink=still_bound?n+1:n;
                returned=n+2;
                break;
            }
            if(carrier[0x18]!=1||std::abs(current[1]-predicted[1])>0.0001f||current[0]!=predicted[0]||current[2]!=predicted[2])
                fail("clear original terrain pass did not preserve integrated motion");
            previous=current;
        }
        if(!event||bool(returned)!=(floor||bounds)||bool(requests)!=floor||
           (!returned&&half(obj+0x1C)!=600-max_visits)||half(actor+0x784)!=1)
            fail("floor fixture did not reach its expected bounded outcome");
        std::printf("bomb-floor: %u %u %u %u %u %u %u %u %u %u %u %.5f %.5f %.5f %.5f %.5f\n",
                    model,unsigned(terrain),event,after_clock,requests,unsigned(spawn),segment_grid,triangle_tests,unlink,returned,
                    half(actor+0x784),double(segment_start[1]),double(proposal[1]),double(proposal[2]),
                    double(final[1]),double(final[2]));
    }
    void wrong_parent(){
        // A reviewed charged key, offered to a different normal actor.
        const Case c{0x000A,2,0,1,1,1};
        owner(c);free_pools(1,1);
        CPUState cpu{};cpu.gpr[3]=actor;cpu.gpr[4]=8;
        mod.run(cpu,ram,factory);
        check_pools(c,{},0);
        if(half(actor+0x784)!=1)fail("wrong-parent bomb changed stock");
        std::puts("bomb-wrong-parent: 10 8 0 1 1");
    }
};
}

int main(int argc,char** argv){
    const bool contact=argc==4&&!std::strcmp(argv[3],"--contact");
    const bool area=argc==4&&!std::strcmp(argv[3],"--sasuke-area");
    const bool floor=argc==4&&!std::strcmp(argv[3],"--floor");
    const bool floor_contact=argc==4&&!std::strcmp(argv[3],"--floor-contact");
    const bool actor_area=argc==4&&!std::strcmp(argv[3],"--actor-area");
    if(argc!=3&&(argc!=4||(!contact&&!area&&!floor&&!floor_contact&&!actor_area&&std::strcmp(argv[3],"--first-bind"))))
        fail("usage: bomb-release-fixture qualified.dol unmodified-recomp.so "
             "[--first-bind|--contact|--sasuke-area|--floor|--floor-contact|--actor-area]");
    const auto original=load_ram(argv[1]);
    const Module module(argv[2],original);
    if(floor||floor_contact||actor_area){
        constexpr std::array<u16,2> models{0,0x000A};
        std::array<HitBlock,models.size()> blocks{};
        for(auto& block:blocks)
            if(std::fread(block.data(),1,block.size(),stdin)!=block.size())fail("bomb floor hit block extent");
        CommonCollision common{};
        if(std::fread(common.data(),1,common.size(),stdin)!=common.size())
            fail("bomb floor common collision extent");
        if(floor_contact||actor_area){
            std::array<ActorSpec,models.size()> specs{};
            for(auto& spec:specs)
                if(std::fread(spec.data(),1,spec.size(),stdin)!=spec.size())fail("bomb area actor spec extent");
            if(std::getc(stdin)!=EOF)fail("bomb area actor spec trailing input");
            for(unsigned i=0;i<models.size();++i){
                if(actor_area){
                    const AreaReceiver target{blocks[1],specs[i],specs[1],30,u16(i?180:150)};
                    for(bool spare:{false,true}){
                        Check test(module,original);
                        test.actor_area(models[i],blocks[i],common,target,spare);
                    }
                    continue;
                }
                for(unsigned scenario:{0u,1u,2u}){
                    const AreaReceiver target{blocks[1],specs[i],specs[1],scenario==2?57u:56u,
                                               u16(i?180:150)};
                    Check test(module,original);
                    test.floor_impact(models[i],blocks[i],common,scenario?Terrain::area_spare:Terrain::area_no_spare,
                                      &target);
                }
            }
            return 0;
        }
        if(std::getc(stdin)!=EOF)fail("bomb floor trailing input");
        for(unsigned i=0;i<models.size();++i){
            for(auto terrain:{Terrain::empty,Terrain::floor,Terrain::offset_floor,Terrain::child_cell_empty,
                              Terrain::bounds_exit,Terrain::area_no_spare,Terrain::area_spare}){
                Check test(module,original);
                test.floor_impact(models[i],blocks[i],common,terrain);
            }
        }
        return 0;
    }
    if(area){
        HitBlock sasuke{};
        ActorSpec sasuke_spec{};
        CommonCollision common{};
        if(std::fread(sasuke.data(),1,sasuke.size(),stdin)!=sasuke.size()||
           std::fread(sasuke_spec.data(),1,sasuke_spec.size(),stdin)!=sasuke_spec.size()||
           std::fread(common.data(),1,common.size(),stdin)!=common.size()||std::getc(stdin)!=EOF)
            fail("bomb area inlet extent");
        for(bool free_shape:{false,true}){
            Check test(module,original);
            test.sasuke_area(sasuke,sasuke_spec,common,free_shape);
        }
        return 0;
    }
    if(argc==4){
        HitBlock gun{},sasuke{};
        if(std::fread(gun.data(),1,gun.size(),stdin)!=gun.size()||
           std::fread(sasuke.data(),1,sasuke.size(),stdin)!=sasuke.size())
            fail("bomb hit block extent");
        if(contact){
            ActorSpec powered_spec{},sasuke_spec{};
            if(std::fread(powered_spec.data(),1,powered_spec.size(),stdin)!=powered_spec.size()||
               std::fread(sasuke_spec.data(),1,sasuke_spec.size(),stdin)!=sasuke_spec.size()||std::getc(stdin)!=EOF)
                fail("bomb contact actor spec extent");
            for(bool powered:{true,false}){
                for(auto scenario:{Contact::miss,Contact::duplicate,Contact::impact,Contact::child_cell_empty,
                                   Contact::child_descriptor_short}){
                    Check test(module,original);
                    test.contact(powered,powered?gun:sasuke,sasuke,powered?powered_spec:sasuke_spec,sasuke_spec,scenario);
                }
            }
            return 0;
        }
        if(std::getc(stdin)!=EOF)fail("bomb hit block trailing input");
        for(unsigned capacity:{0u,1u,2u,4u}){
            Check test(module,original);
            test.first_bind(cases[0],gun,capacity);
        }
        for(unsigned i:{2u,3u}){
            Check test(module,original);
            test.first_bind(cases[i],gun,1);
        }
        for(unsigned capacity:{0u,1u}){
            Check test(module,original);
            test.first_bind(cases[14],sasuke,capacity);
        }
        return 0;
    }
    for(const auto& c:cases){Check test(module,original);test.run_case(c);}
    Check wrong(module,original);wrong.wrong_parent();
}
