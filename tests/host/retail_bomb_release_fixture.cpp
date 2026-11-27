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
constexpr u32 effect_cell=0x81034000;
constexpr u32 factory=0x8007DB5C, powered=0x8016711C, ninja=0x800715F8, alpha=0x801A0458;
constexpr u32 audio=0x800F036C, normal_x_helper=0x8016D584, movement_tail=0x800B22F4;

struct Case {
    u16 model;
    u8 state, extra, stock, carriers, descriptors;
};
using HitBlock=std::array<u8,7840>;
using ActorSpec=std::array<u8,432>;
enum class Contact {miss,duplicate,impact,child_cell_empty,child_descriptor_short};
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
        constexpr u32 stage=0x81040000,grid=stage+0x100,refs=stage+0x200,side=128,empty=refs+side*side*4;
        put32(at(0x8043B5A0-21316),stage);
        for(unsigned i=0;i<3;++i)put32(at(stage+28+4*i),grid);
        put_float(at(stage+52),9000.f);put_float(at(stage+56),31000.f);
        put_float(at(stage+60),-1000.f);put_float(at(stage+64),31000.f);put_float(at(stage+68),-1000.f);
        put32(at(grid+12),side);put32(at(grid+16),u32(-1000));put32(at(grid+20),u32(-1000));
        put32(at(grid+24),refs);put32(at(empty),0xFFFFFFFFu);
        for(unsigned i=0;i<side*side;++i)put32(at(refs+4*i),empty);
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
    void receiver_inlet(const HitBlock& block,unsigned distance,const ActorSpec& attacker,
                        const ActorSpec& defender,u16 hp){
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
        put_float(joint+12,fp(shapes+0x64)+float(distance));
        put_float(joint+28,fp(shapes+0x68)+14.f);
        put_float(joint+44,fp(shapes+0x6C));
        if(half(free_count+8))fail("receiver assumes an exhausted class-4 pool");
        at(body)[2]=4;put32(at(free_head+16),body);put32(at(free_tail+16),body);
        put16(at(free_count+8),1);
        CPUState cpu{};cpu.gpr[3]=receiver;
        mod.run(cpu,ram,0x8008AB30);
        if(be32(at(active_head+50*4))!=body||be32(at(body+0x28))!=body_block+32+9*80)
            fail("bomb fixture did not bind original singleton body");
        class4_passes();
        if(half(attack_count)!=1||half(receiver_count)!=1||be32(at(attack_registry))!=shapes||
           be32(at(receiver_registry))!=body||fp(body+0x7C)!=22.f||
           std::abs(fp(body+0x64)-fp(shapes+0x64)-float(distance))>0.001f||
           std::abs(fp(body+0x68)-fp(shapes+0x68))>0.001f||fp(body+0x6C)!=fp(shapes+0x6C))
            fail("bomb fixture body or registry geometry mismatch");

        constexpr u32 receiver_spec=0x81015400,difficulty=0x81019600;
        std::memcpy(at(spec),attacker.data(),attacker.size());
        std::memcpy(at(receiver_spec),defender.data(),defender.size());
        put32(target+0x4AC,receiver_spec);
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
    void first_bind(const Case& c,const HitBlock& block,unsigned capacity,bool report=true){
        run_case(c,false);
        bind_inlets(c,block,capacity);
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
                 const ActorSpec& defender_spec,Contact scenario){
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
        receiver_inlet(defender,miss?256:0,credit_spec,defender_spec,gun?200:180);
        const auto obj=cell(0);
        const auto* carrier=at(obj);const auto* target=at(receiver);
        const auto token=half(obj+0x26C);
        if(token!=1||half(receiver+0x20C)||target[0x26E]||carrier[0x1D9]||fp(shapes+0x7C)!=(gun?20.f:10.f))
            fail("bomb contact token, shape radius or flags baseline mismatch");
        if(duplicate)put16(at(receiver+0x20C),token);
        const u32 amount_record=gun?0x8034EB10:0x802D4178;
        const auto before=ram;
        CPUState input{};
        const auto cut=mod.run_to_or_return(input,ram,0x8002DB58,{0x8003CD5C,0x8003D344});
        if((hit?(cut!=0x8003CD5C||input.lr!=0x8002F10C||input.gpr[3]!=amount_record||
           input.gpr[4]!=obj||input.gpr[5]!=receiver):cut!=0)||half(receiver+0x1C6)!=180)
            fail("bomb pair did not request the exact opening amount record");
        unsigned amount=0;
        if(hit){
            std::memcpy(ram.data(),before.data(),ram.size());
            CPUState writer{};
            mod.run_to(writer,ram,0x8002DB58,{0x8003D344});
            if(writer.lr!=0x8002F1DC||writer.gpr[3]!=receiver||half(receiver+0x1C6)!=180)
                fail("bomb pair reached the wrong HP writer");
            amount=writer.gpr[4];
            mod.finish_stopped_leaf(writer,0x8003D344,164);
        }
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
        constexpr u32 child_callback=0x80086048;
        const auto pos=std::array{be32(at(obj+0x20)),be32(at(obj+0x24)),be32(at(obj+0x28))};
        CPUState update{};update.gpr[3]=9;
        CallObservation request{entry};
        const auto update_cut=mod.run_to_or_return(update,ram,visit,{other,child_callback},&request);
        if(update_cut!=(!gun&&spawn?child_callback:0)||(!gun&&spawn&&update.gpr[3]!=cell(1))||
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
        if(hit){
            for(const bool returning:{false,true}){
                CPUState pass{};pass.gpr[3]=9;
                const auto child_cut=mod.run_to_or_return(pass,ram,visit,{child_callback});
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
    if(argc!=3&&(argc!=4||(!contact&&std::strcmp(argv[3],"--first-bind"))))
        fail("usage: bomb-release-fixture qualified.dol unmodified-recomp.so [--first-bind|--contact]");
    const auto original=load_ram(argv[1]);
    const Module module(argv[2],original);
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
