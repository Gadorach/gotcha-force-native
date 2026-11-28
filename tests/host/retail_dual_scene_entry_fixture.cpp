// Two independent original archive deliveries into two real physical actor seats.
#include "retail_contact_effect_archive.hpp"
#include "retail_scene_entry.hpp"

namespace {
using namespace retail_fixture;
namespace archive=retail_contact_effect_archive;
namespace contact=retail_stationary_contact;
namespace scheduler=retail_scene_entry;
constexpr u32 actors=0x81020000,actor_stride=0x1E00,scene=scheduler::scene,mode=0x8103E000;
constexpr u32 stage=0x81048000,pools=0x81063000,basis=0x81060000;
constexpr u32 shape=0x81070000,shape_stride=0x200,pad=0x803C727C;
constexpr u32 body_pass_end=0x80087CF0,present_body=0x8005A580;
constexpr u32 attack_count=0x8043B5A0-21328,attack_list=0x803C477C;
constexpr u32 bomb=0x8107B000,bomb_shape=0x8107A000,bomb_bucket=0x803C706C+10*4;
constexpr u32 factory=0x800E058C,area_factory=0x80085E00;
constexpr u32 child=0x8107B800,child_shape=0x8107A200,common_hits=0x8107D000;
constexpr u32 next_area=0x8107C000,next_shape=0x8107A400;
constexpr u32 common_view_owner=0x81064040;
constexpr std::array<u16,2> models{0x000A,0x0106},members{495,504},hp{180,220};
constexpr std::array<size_t,2> motion_sizes{368160,435104},view_sizes{241128,171236};
struct Assets {
    contact::ActorSpec spec;
    contact::HitBlock hit;
    std::vector<u8> motion,view;
};
struct AreaAssets {std::array<u8,21504> collision;archive::Source view;};
enum class Run {entry,bomb,area,reaction,cascade,terminal};
constexpr u32 fighter(unsigned seat){return actors+seat*actor_stride;}
constexpr u32 one=0x3F800000,negative_rate=0xBD4CCCCD;
constexpr u32 parent_amount=0x80336418,child_amount=0x802C49D0;
const u8* sampled(const CPUState& cpu,u32 receiver,u32 offset){
    if(receiver!=fighter(0)&&receiver!=fighter(1)&&receiver!=bomb)
        fail("unexpected receiver in the second-placement fork");
    return cpu.ram+(receiver-GC_RAM_BASE)+offset;
}
u32 sample_hp(const CPUState& cpu){
    const auto p=sampled(cpu,cpu.gpr[3],0x1C6);
    return u32(p[0])*256+p[1];
}
u32 sample_tier(const CPUState& cpu){
    const auto receiver=cpu.gpr[5];
    const auto p=sampled(cpu,receiver,0x6CA);
    return receiver==bomb?255:*p;
}

class Replay {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    unsigned byte(u32 a){return at(a)[0];}
    unsigned f(unsigned seat,u32 offset){return byte(fighter(seat)+offset);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void store16(u32 a,u16 x){put16(at(a),x);}
    void call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;
        mod.run(cpu,ram,entry,4096);
    }
    void stage_grid(){
        constexpr u32 grid=stage+0x100,refs=stage+0x200,empty=refs+64*4;
        store(0x8043B5A0-21316,stage);
        for(unsigned i=0;i<3;++i)store(stage+28+4*i,grid);
        constexpr std::array<u32,6> fields{500,500,8,8,u32(-1000),u32(-1000)};
        for(unsigned i=0;i<fields.size();++i)store(grid+4*i,fields[i]);
        store(grid+24,refs);store(empty,0xFFFFFFFF);
        for(unsigned i=0;i<64;++i)store(refs+4*i,empty);
        for(unsigned i=0;i<5;++i)put_float(at(stage+52+4*i),i==2||i==4?-3000.f:3000.f);
    }
    void offer_pools(bool area){
        constexpr u32 bases[]{0x81066000,0x81064000,0x81069000};
        const unsigned bytes[]{44*64,32*(area?3u:2u),16*32};
        archive::Setup sdk(mod,ram);
        sdk.init_heap(archive::arena+0x100000);
        store(0x8043B5A0-21352,basis);store(0x8043B5A0-21024,pools);
        for(unsigned kind=0;kind<3;++kind){
            const auto row=pools+28*kind;
            store(row,bases[kind]-basis);store(row+4,bytes[kind]);
            std::memcpy(at(row+16),at(0x8036F5B8+28*kind+16),12);
            call(0x80197950,kind);
        }
        for(unsigned i=0;i<32;++i){
            const auto cell=shape+shape_stride*i;
            at(cell+2)[0]=4;
            store(cell+4,i==31?0:cell+shape_stride);
        }
        store(contact::free_head+16,shape);store(contact::free_tail+16,shape+31*shape_stride);
        store16(contact::free_count+8,32);
    }
    void deliver(unsigned seat,const Assets& a){
        const auto outputs=0x803B07AC+56*seat,view_owner=0x803BB4C0+3120*seat+0x600;
        const auto data=basis+0x270000+0x800*seat-0x7400;
        const auto hit=basis+0x700000+0x2000*seat+0x3C00;
        const auto motion=basis+0x480000+0x6C000*seat-0x4400;
        const auto view=basis+0x270000+0x58000*seat-0x4400;
        CPUState request{};request.gpr[3]=seat;
        mod.run_to(request,ram,0x800410BC,{0x8000B07C});
        if(request.lr!=0x8004124C||request.gpr[3]!=members[seat]||request.gpr[4]!=outputs||
           word(outputs)!=data||word(outputs+4)!=0xF0000000||word(outputs+8)!=hit||
           word(outputs+12)!=motion||word(outputs+16)!=view||word(outputs+20)!=0xF0000000||
           word(view_owner))
            fail("dual scene initial archive request changed its exact seat or model roles");
        std::memcpy(at(data),a.spec.data(),a.spec.size());
        std::memcpy(at(hit),a.hit.data(),a.hit.size());
        std::memcpy(at(motion),a.motion.data(),a.motion.size());
        std::memcpy(at(view),a.view.data(),a.view.size());
        CPUState ready{};ready.gpr[3]=seat;ready.gpr[4]=0;
        mod.run(ready,ram,0x800412C4,4096);
        if(word(view_owner)!=0x81064000+32*seat||word(0x803C4D94+4*seat)!=data||
           word(scene+128+4*seat)!=hit||word(0x803C4D7C+4*seat)!=motion||word(0x803BB4A0+4*seat)!=view){
            std::fprintf(stderr,"dual delivery seat=%u owner=%08X spec=%08X hit=%08X motion=%08X view=%08X pool_free=%u\n",
                         seat,word(view_owner),word(0x803C4D94+4*seat),word(scene+128+4*seat),
                         word(0x803C4D7C+4*seat),word(0x803BB4A0+4*seat),half(pools+28+18));
            fail("dual scene completion crossed actor resource boundaries");
        }
    }
    void common_area_archive(const AreaAssets& area){
        constexpr u32 globals=archive::source_globals,bank=archive::source_bank,owner=common_view_owner;
        if(area.view.slot!=1||byte(mode+41)||word(globals))fail("world area needs the exact normal common archive");
        store(archive::source_bank_basis,bank-archive::source_bank_delta);
        CPUState open{};
        mod.run_to(open,ram,0x80042878,{0x8000B07C});
        if(open.lr!=0x80042904||open.gpr[3]!=697||open.gpr[4]!=0x803B0760||
           word(0x803B0764)!=bank||word(globals+4)!=bank)
            fail("world area selected the wrong normal common archive");
        store(archive::source_bank_basis,basis); // Preserve the two cold actor owners and their pool.
        std::memcpy(at(bank),area.view.bytes.data(),area.view.bytes.size());
        call(0x80042954);
        if(word(globals)!=owner||byte(owner+8)!=1||byte(owner+9)!=1||byte(owner+10)!=1||
           half(owner+26)!=157||word(owner+20)!=bank||half(pools+28+18))
            fail("world area did not retain a separate relocated effect archive");
        std::memcpy(at(common_hits),area.collision.data(),area.collision.size());
        store(scene+152,common_hits);
    }
    void setup(const std::array<Assets,2>& assets,const AreaAssets* area){
        store(0x8043B5A0-31852,scene);store(scheduler::mode_ptr,mode);
        store(0x8043B5A0-31824,0x81079000);
        at(mode+2)[0]=3;at(mode+12)[0]=1;
        at(0x803D3AE8+852)[0]=1;
        at(scene+0x1828)[0]=3;at(scene+5593)[0]=3;at(scene+5594)[0]=2;
        for(unsigned i=0;i<6;++i)store16(scene+16+2*i,i<2?models[i]:0xFFFF);
        for(unsigned i=0;i<2;++i){
            at(scene+203+i)[0]=i;
            store(scene+60*i+0x114,300);store(scene+60*i+0x118,300);
        }
        call(0x800536B0,actors);
        for(unsigned i=0;i<6;++i){
            if(word(0x803C4E84+4*i)!=fighter(i)||f(i,0)!=unsigned(i<2)||f(i,0x18)||
               f(i,0x3E4)!=i||f(i,0x3E5)!=(1u<<i)||f(i,0x3E7)!=i||f(i,0x743))
                fail("original physical actor initialization changed");
            if(i<2){
                put_float(at(fighter(i)+0x20),i?450.f:150.f);
                put_float(at(fighter(i)+0x24),100.f);
                put_float(at(fighter(i)+0x28),300.f);
            }
        }
        stage_grid();offer_pools(area!=nullptr);
        for(unsigned i=0;i<2;++i)deliver(i,assets[i]);
        if(area)common_area_archive(*area);
        call(0x80254288);
        scheduler::prepare(ram);
    }
    std::array<unsigned,2> rebuild(){
        call(0x8008A28C);call(0x80089AEC);
        std::array<unsigned,2> out{};
        const auto count=half(contact::receiver_count);
        if(count>18)fail("dual scene receiver list extent");
        for(unsigned i=0;i<count;++i){
            const auto ptr=word(contact::receiver_registry+4*i);
            const auto owner=word(ptr+0x20);
            if(owner!=fighter(0)&&owner!=fighter(1))fail("dual scene published a foreign receiver");
            const auto seat=owner==fighter(1)?1:0;
            const auto descriptor=word(ptr+0x28);
            const auto bank=word(owner+0x3FC);
            if(descriptor<bank+32||descriptor>=bank+32+16*80||(descriptor-bank-32)%80)
                fail("dual scene receiving descriptor crossed its actor bank");
            const auto bit=1u<<((descriptor-bank-32)/80);
            if(out[seat]&bit)fail("dual scene duplicate receiving descriptor");
            out[seat]|=bit;
        }
        return out;
    }
    void pulse(unsigned delay,unsigned visit,bool accepted){
        auto fork=ram;
        put32(contact::at(fork,pad+32),0x200);
        CPUState cpu{};
        CallObservation poll{0x80056900};
        const auto frontier=mod.run_to_or_return(cpu,fork,scheduler::dispatch,{0x800E058C,body_pass_end},&poll,65536);
        const auto rd=[&](u32 a){return be32(contact::at(fork,a));};
        const auto paid=contact::half(fork,fighter(1)+0x774);
        if(frontier!=(accepted?0x800E058C:body_pass_end)||poll.count!=(accepted?2u:0u)||
           (accepted&&(poll.caller!=0x80054CD0||poll.first_arguments[0]!=fighter(0)||
              poll.first_arguments[1]!=fighter(1)||cpu.lr!=0x80130584||cpu.gpr[3]!=fighter(1)||cpu.gpr[4]!=2||cpu.gpr[5]!=10))||
           rd(fighter(0)+0x5B4)||rd(fighter(0)+0x5BC)||rd(fighter(0)+0x5D4)||
           contact::half(fork,fighter(0)+0x774)!=10||paid!=(accepted?9:10)||
           rd(fighter(1)+0x5B4)!=(accepted?0x200u:0)||rd(fighter(1)+0x5BC)!=(accepted?0x200u:0)||
           rd(fighter(1)+0x5D4)!=(accepted?0x60u:0))
            fail("dual scene processed pad leaked across seats or reached the wrong factory");
        std::printf("dual-pad: %u %u %u %u %u %08X\n",delay,visit,poll.count,unsigned(accepted),paid,cpu.lr);
    }
    void check_constructor(unsigned delay){
        constexpr std::array<unsigned,2> roots{19,4};
        std::array<u32,2> head{};
        for(unsigned seat=0;seat<2;++seat){
            const auto view=0x803BB4C0+3120*seat;
            const auto owner=word(view+1536);
            const auto node=word(view+1800);
            if(owner!=0x81064000+32*seat||half(owner+26)!=roots[seat]||!node||!word(node+12)||
               half(fighter(seat)+0x272)!=0xEF||half(fighter(seat)+0x20C)||
               half(fighter(seat)+0x784)!=(seat?3u:5u))
                fail("dual normal constructor crossed its rig, stock or collision reset scope");
            head[seat]=word(node+12);
        }
        if(head[0]==head[1]||half(contact::free_count+8)||half(pools+18)!=64-46)
            fail("dual scene did not bind separate SDK view ownership and all the body descriptors");
        std::printf("dual-cold: %u %u %u %u %u\n",delay,roots[0],roots[1],64-half(pools+18),32u);
    }
    void check_payment(unsigned delay,unsigned kind,bool report=true){
        auto fork=ram;
        CPUState cpu{};
        const auto cut=mod.run_to_or_return(cpu,fork,scheduler::dispatch,{factory},nullptr,65536);
        const auto stock=contact::half(fork,fighter(1)+0x774);
        if(cut!=factory||cpu.lr!=0x80130584||cpu.gpr[3]!=fighter(1)||cpu.gpr[4]!=2||cpu.gpr[5]!=10||stock!=9||
           contact::at(fork,bomb)[0]||contact::half(fork,fighter(0)+0x774)!=10)
            fail("world bomb did not pay before the original allocation");
        if(report)std::printf("dual-world-pay: %u %u %u\n",delay,kind,stock);
    }
    void check_bomb_world(unsigned kind,unsigned visit){
        const bool carrier=kind>=2&&visit<(kind==2?3u:183u);
        const bool published=kind==3&&visit<=180;
        const unsigned phase=kind<2?0:kind==2?(visit==1?2:3):published?1:visit==181?2:3;
        const unsigned expected_stock=(kind==2&&visit==1)||(kind==3&&visit<=181)?9:10;
        if(half(fighter(1)+0x774)!=expected_stock||byte(bomb)!=unsigned(carrier)||byte(bomb+0x18)!=phase||
           byte(bomb_shape)!=unsigned(published)||half(attack_count)!=unsigned(published)||
           half(contact::receiver_count)!=18+unsigned(published)||
           word(bomb_bucket)!=(carrier?bomb:0)||half(contact::free_count+6)!=unsigned(kind>=2&&!carrier)||
           half(contact::free_count+8)!=unsigned(kind==3&&!published)||half(pools+18)!=(carrier?17:18))
            fail("world bomb admission or returned resources changed");
        if(carrier){
            const auto visual=word(bomb+0xE0);
            if(byte(bomb+0x10)!=41||byte(bomb+0x11)!=2||byte(bomb+0x13)!=10||word(bomb+0xC)!=0x800E06C4||
               word(bomb+0x8C)!=fighter(1)||word(bomb+0x90)!=fighter(1)||half(bomb+0x1C6)!=1||
               visual<0x81066000||visual>=0x81066000+44*64||word(visual+20)!=3||!word(visual+12))
                fail("world bomb did not keep its exact Time Bomber credit and view ownership");
        }
        if(published){
            const auto descriptor=word(fighter(1)+0x3FC)+0xAA0+8*80;
            unsigned receiving=0;
            for(unsigned j=0;j<19;++j)receiving+=word(contact::receiver_registry+4*j)==bomb_shape;
            if(half(bomb+0x26C)!=1||word(attack_list)!=bomb_shape||receiving!=1||
               word(bomb_shape+0x20)!=bomb||word(bomb_shape+0x24)!=fighter(1)||
               word(bomb_shape+0x28)!=descriptor||(half(descriptor+2)&3)!=3||
               half(bomb+0x152)!=181-visit)
                fail("world bomb did not keep its exact two-role descriptor or natural lifetime");
        }
        for(unsigned i=0;i<2;++i){
            const auto body=fighter(i);
            if(f(i,0x18)!=1||half(body+0x1C6)!=hp[i]||word(body+0x418)||word(body+0x41C)||half(body+0x20C))
                fail("world bomb control changed a body HP, ledger or contact token");
        }
    }
    void world_bomb(unsigned delay){
        const auto saved=ram;
        for(unsigned i=0;i<4;++i){
            std::memcpy(ram.data(),saved.data(),ram.size());
            // The provided empty collision grid is an already initialized
            // stage inlet; original stage state 1 dispatches a no-op.
            at(stage)[0]=1;at(stage+8)[0]=1;
            const bool press=i!=0,offer_carrier=i>=2,offer_shape=i==3;
            if(half(contact::free_count+6)||word(contact::free_head+12)||half(contact::free_count+8)||
               word(contact::free_head+16)||word(bomb_bucket)||half(fighter(1)+0x774)!=10)
                fail("dual world bomb requires an unconsumed active input and empty carrier pools");
            if(offer_carrier){
                at(bomb+2)[0]=3;
                store(contact::free_head+12,bomb);store(contact::free_tail+12,bomb);
                store16(contact::free_count+6,1);
            }
            if(offer_shape){
                at(bomb_shape+2)[0]=4;
                store(contact::free_head+16,bomb_shape);store(contact::free_tail+16,bomb_shape);
                store16(contact::free_count+8,1);
            }
            store(pad+32,press?0x200:0);
            if(press)check_payment(delay,i);
            unsigned last_phase=0;
            for(unsigned visit=1;visit<=(offer_shape?184u:3u);++visit){
                const auto scene_before=word(scene+176);
                const auto turn_before=word(scene+168),combat_before=word(scene+172);
                CPUState cpu{};
                CallObservation seen{visit==1?factory:area_factory};
                const auto cut=mod.run_to_or_return(cpu,ram,scheduler::dispatch,{0x80264290},&seen,1u<<20);
                if(cut==0x80264290)
                    std::fprintf(stderr,"dual world SDK assertion case=%u visit=%u from=%08X line=%u\n",
                                 i,visit,cpu.lr-4,cpu.gpr[4]);
                if(cut||(visit==1&&(seen.count!=unsigned(press)||(seen.count&&
                     (seen.caller!=0x80130580||seen.arguments[0]!=fighter(1)||seen.arguments[1]!=2||seen.arguments[2]!=10))))||
                   word(scene+176)!=scene_before+1||word(scene+168)!=turn_before+1||word(scene+172)!=combat_before+1||
                   half(fighter(0)+0x774)!=10||word(fighter(0)+0x5BC)||
                   word(fighter(1)+0x5BC)!=(press&&visit==1?0x200u:0))
                    fail("dual world did not finish the original actor, bucket and collision sequence");
                const bool terminal=offer_shape&&visit==181;
                if(visit>1&&(seen.count!=unsigned(terminal)||(terminal&&(seen.caller!=0x800E0B4C||
                   seen.arguments[0]!=bomb||seen.arguments[1]!=bomb+0x20||seen.arguments[2]!=19))))
                    fail("world bomb scheduled its natural terminal request at the wrong boundary");
                if(terminal)
                    std::printf("dual-world-terminal: %u %u %u %u %08X %u %u\n",delay,i,visit,seen.count,
                                seen.caller,seen.arguments[2],unsigned(seen.arguments[0]==bomb));
                check_bomb_world(i,visit);
                const unsigned phase=byte(bomb+0x18);
                if(visit<=3||visit>=178||phase!=last_phase||!byte(bomb))
                    std::printf("dual-world-bomb: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %08X %08X %08X\n",
                            delay,i,visit,seen.count,half(fighter(1)+0x774),byte(bomb),byte(bomb+0x18),
                            byte(bomb_shape),half(attack_count),half(contact::receiver_count),
                            half(bomb+0x26C),half(fighter(0)+0x1C6),half(fighter(1)+0x1C6),half(bomb+0x152),
                            word(bomb+0x20),word(bomb+0x24),word(bomb+0x28));
                store(pad+32,0);
                last_phase=phase;
                if(offer_shape&&!byte(bomb))break;
            }
        }
    }
    void check_area_timeout(unsigned delay,unsigned shapes,bool report=true){
        auto fork=ram;
        CPUState terminal{};
        const auto cut=mod.run_to_or_return(terminal,fork,scheduler::dispatch,{area_factory},nullptr,65536);
        if(cut!=area_factory||terminal.lr!=0x800E0B50||terminal.gpr[3]!=bomb||
           terminal.gpr[4]!=bomb+0x20||terminal.gpr[5]!=19||contact::half(fork,bomb+0x152))
            fail("world area did not follow the natural Time Bomber timeout");
        std::memcpy(fork.data(),ram.data(),ram.size());
        CPUState bind{};
        const auto inlet=mod.run_to_or_return(bind,fork,scheduler::dispatch,{0x80086084},nullptr,65536);
        if(inlet!=0x80086084||bind.gpr[3]!=child||!contact::at(fork,bomb_shape)[0]||
           !contact::at(fork,bomb+0x1D8)[0]||contact::half(fork,contact::free_count+8)!=shapes-1||
           be32(contact::at(fork,contact::free_head+16))!=(shapes==2?child_shape:0))
            fail("world area initializer did not see the old marked shape still occupying its cell");
        if(report)std::printf("dual-area-bind: %u %u %u %u %u\n",delay,shapes,181u,shapes-1,1u);
    }
    void check_area_amount(){
        auto fork=ram;
        CPUState cpu{};CallObservation amounts{contact::amount_entry};
        mod.run_observed(cpu,fork,scheduler::dispatch,amounts,65536);
        if(amounts.count!=2||amounts.caller!=0x8002F108||amounts.arguments[2]!=fighter(0))
            fail("world area did not reach both receiving amount inlets in source order");
        for(unsigned j=0;j<2;++j)
            if(amounts.first_arguments[j]!=0x802C49D0||amounts.second_arguments[j]!=child)
                fail("world area HP requests did not use one exact child amount row");
    }
    void check_area_world(unsigned shapes,unsigned visit,const CallObservation& hits,bool published){
        const bool live_parent=visit<=182;
        const bool admitted=shapes==2;
        const bool live_child=visit>=181&&visit<=(admitted?230u:182u);
        const bool landed=admitted&&visit>=185;
        const bool wants_shape=admitted&&visit>=182&&visit<=197;
        const bool bound=admitted&&visit>=181&&visit<=197;
        const unsigned parent_phase=visit<=180?1:visit==181?2:3;
        const unsigned expected_phase=visit<181?0:!admitted?(visit==181?2:3):visit<=228?1:visit==229?2:3;
        if(byte(bomb)!=unsigned(live_parent)||byte(bomb+0x18)!=parent_phase||
           byte(child)!=unsigned(live_child)||byte(child+0x18)!=expected_phase||
           published!=wants_shape||half(fighter(1)+0x774)!=(visit<182?9:10)||half(fighter(0)+0x774)!=10||
           half(attack_count)!=unsigned(visit<181||wants_shape)||half(contact::receiver_count)!=(visit<181?19:18)||
           byte(bomb_shape)!=unsigned(visit<181)||byte(child_shape)!=unsigned(bound)||
           half(contact::free_count+8)!=shapes-unsigned(visit<181)-unsigned(bound)||
           half(fighter(0)+0x1C6)!=(landed?163:180)||half(fighter(1)+0x1C6)!=(landed?216:220)||
           word(fighter(1)+0x418)!=(landed?17:0)||word(fighter(1)+0x42C)||word(fighter(0)+0x418)||
           word(fighter(0)+0x41C)!=(landed?17:0)||word(fighter(1)+0x41C)!=(landed?4:0))
            fail("world area allocation, retirement, HP or self-credit boundary changed");
        for(unsigned i=0;i<2;++i){
            if(half(fighter(i)+0x20C)!=(landed?2:0)||half(fighter(i)+0x20E)||
               f(i,0x26E)!=unsigned(landed)||f(i,0x18)!=1||word(fighter(i)+0x420)||word(fighter(i)+0x424))
                fail("world area changed actor phase or did not preserve a distinct receiving token");
        }
        if(visit<181&&(word(attack_list)!=bomb_shape||half(bomb+0x26C)!=1||half(bomb+0x152)!=181-visit))
            fail("world area changed the original parent's attack or lifetime");
        if(hits.count!=(admitted&&visit==185?2u:0u)||(hits.count&&(hits.caller!=0x8002F1D8||
           hits.first_arguments[0]!=fighter(0)||hits.first_arguments[1]!=fighter(1)||
           hits.second_arguments[0]!=17||hits.second_arguments[1]!=4)))
            fail("world area changed the two HP writer sites or recipient order");
        if(live_child&&(byte(child+0x10)!=5||byte(child+0x11)!=19||word(child+0x8C)!=bomb||
           word(child+0x90)!=fighter(1)||half(child+0x26C)!=(admitted?2:0)||
           (admitted&&visit==181&&word(word(child+0xE0)+20)!=22)))
            fail("world area did not preserve original child identity, credit or opening view");
        if(bound&&(word(child_shape+0x20)!=child||word(child_shape+0x24)!=fighter(1)||
           word(child_shape+0x28)!=common_hits+0x400+246*80||word(child_shape+0x2C)!=0x802C4760||
           byte(child_shape+0x11)!=26||byte(child_shape+0x14)!=179||(published&&word(attack_list)!=child_shape)))
            fail("world area bound or published the wrong common collision descriptor");
        if(visit>=183&&!live_child&&(half(contact::free_count+6)!=2||half(contact::free_count+8)!=shapes||
           half(pools+18)!=18||half(pools+28+18)||word(archive::source_globals)!=common_view_owner))
            fail("world area leaked a carrier, shape, view cell or shared archive");
    }
    void probe_reaction_b(unsigned delay,unsigned shapes,unsigned visit){
        auto fork=ram;
        put32(contact::at(fork,pad+32),0x200);
        CPUState cpu{};CallObservation poll{0x80056900};
        const auto cut=mod.run_to_or_return(cpu,fork,scheduler::dispatch,{factory,body_pass_end,0x80264290},&poll,65536);
        const auto body=fighter(1);
        const auto w=[&](u32 offset){return be32(contact::at(fork,body+offset));};
        const auto at=[&](u32 offset){return contact::at(fork,body+offset)[0];};
        const bool reacted=shapes==2&&visit>185;
        if(cut!=factory||cpu.lr!=0x80130584||cpu.gpr[3]!=body||cpu.gpr[4]!=2||cpu.gpr[5]!=10||
           poll.count!=2||poll.caller!=0x80054CD0||poll.first_arguments[0]!=fighter(0)||poll.first_arguments[1]!=body||
           w(0x5BC)!=0x200||contact::half(fork,body+0x774)!=9||w(0x5D4)!=0x60||
           at(0x71A)!=(reacted?10u:0u)||at(0x71C)||w(0x5A0)||w(0x5F4)!=one||
           w(0x1DC8)!=(reacted?negative_rate:one)||contact::at(fork,bomb)[0])
            fail("original pad ingest or second-B debit after the area changed");
        if(visit==184||visit==185||visit==186||visit==189||visit==198||visit==235)
            std::printf("area-reaction-b: %u %u %u %u %u %u %08X\n",delay,shapes,visit,
                        poll.count,w(0x5BC),contact::half(fork,body+0x774),w(0x1DC8));
        if(visit==185||visit==186){
            std::memcpy(fork.data(),ram.data(),ram.size());
            put32(contact::at(fork,pad+32),0x200);
            CPUState full{};CallObservation seen{contact::hp_write};
            seen.sample=sample_hp;
            const auto stop=mod.run_to_or_return(full,fork,scheduler::dispatch,{0x80264290},&seen,1u<<20);
            const auto read=[&](u32 address){return be32(contact::at(fork,address));};
            const bool area=shapes==2;
            const auto h=[&](u32 address){return contact::half(fork,address);};
            if(stop||seen.count>4||(seen.count&&seen.caller!=0x8002F1D8)||
               read(scene+176)!=word(scene+176)+1||h(body+0x774)!=9||!contact::at(fork,bomb)[0]||
               h(bomb+0x26C)!=(area?3:2)||read(bomb+0x90)!=body||read(bomb+0x8C)!=body||
               read(attack_list)!=bomb_shape||read(bomb_shape+0x20)!=bomb||read(bomb_shape+0x24)!=body||
               h(attack_count)!=(area?2:1)||h(contact::receiver_count)!=19||
               (area&&(read(attack_list+4)!=child_shape||read(child_shape+0x20)!=child||
                       read(child_shape+0x24)!=body||h(child+0x26C)!=2))||
               read(body+0x42C)||read(bomb+0x41C)||read(body+0x420)||read(body+0x424))
                fail("second-B fork did not publish the two exact shapes or preserve separate credits");
            std::printf("area-reaction-placement: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",delay,shapes,visit,
                        seen.count,h(bomb+0x26C),h(bomb+0x1C6),h(attack_count),
                        contact::half(fork,fighter(0)+0x1C6),contact::half(fork,body+0x1C6),
                        h(fighter(0)+0x20C),h(fighter(0)+0x20E),
                        read(body+0x418),read(fighter(0)+0x41C),read(body+0x41C),contact::at(fork,fighter(0)+0x6CA)[0]);
            std::memcpy(fork.data(),ram.data(),ram.size());
            put32(contact::at(fork,pad+32),0x200);
            CPUState amount_cpu{};CallObservation amounts{contact::amount_entry};
            amounts.sample=sample_tier;
            mod.run_observed(amount_cpu,fork,scheduler::dispatch,amounts,65536);
            if(amounts.count!=seen.count||(amounts.count&&amounts.caller!=0x8002F108))
                fail("second-B amount and HP writers diverged");
            for(unsigned i=0;i<seen.count;++i){
                const auto record=amounts.first_arguments[i],attack=amounts.second_arguments[i];
                if(amounts.third_arguments[i]!=seen.first_arguments[i]||
                   !((record==parent_amount&&attack==bomb)||(record==child_amount&&attack==child)))
                    fail("second-B HP writer did not use its exact amount and receiver");
                std::printf("area-reaction-contact: %u %u %u %u %08X %08X %u %u %u\n",delay,shapes,visit,i,
                            record,seen.first_arguments[i],seen.second_arguments[i],seen.samples[i],amounts.samples[i]);
            }
        }
    }
    void report_reaction(unsigned delay,unsigned shapes,unsigned visit){
        for(unsigned i=0;i<2;++i){
            const auto body=fighter(i);
            const bool reacted=shapes==2&&visit>=185;
            const unsigned gate=reacted&&visit<189?189-visit:0;
            if(byte(body+0x19)!=2||byte(body+0x2AE)!=gate||byte(body+0x2AF)!=gate||
               byte(body+0x71A)!=(reacted?10u:0u)||byte(body+0x71C)||word(body+0x5A0)||
               word(body+0x5F4)!=one||word(body+0x1DC8)!=(reacted&&visit>185?negative_rate:one)||
               word(body+0x1DCC)!=(reacted?negative_rate:one)||word(body+0x1DD0)!=one)
                fail("area reaction changed the source input phase, contact gates or rates");
            if(visit<=189||visit==198||visit==231||visit==235)
                std::printf("area-reaction-body: %u %u %u %u %u %u %u %u %u %08X %08X %08X\n",
                            delay,shapes,visit,i,byte(body+0x19),byte(body+0x2AE),byte(body+0x2AF),
                            byte(body+0x71A),byte(body+0x71C),word(body+0x5F4),word(body+0x1DC8),word(body+0x1DCC));
        }
    }
    void check_cascade_amount(unsigned visit,u32 source,u32 receiver){
        auto fork=ram;
        CPUState cpu{};CallObservation amounts{contact::amount_entry};
        amounts.sample=sample_tier;
        mod.run_observed(cpu,fork,scheduler::dispatch,amounts,65536);
        if(amounts.count!=1||amounts.caller!=0x8002F108||amounts.arguments[0]!=child_amount||
           amounts.arguments[1]!=source||amounts.arguments[2]!=receiver||amounts.samples[0]!=(visit==186?255u:0u))
            fail("cascade contact did not use its exact carrier, amount record and receiver");
    }
    void check_cascade_world(unsigned capacity,unsigned visit,unsigned opponent_visit){
        const bool admitted=capacity==2,live_bomb=visit<=188,live_old=visit<=230;
        const bool live_next=visit>=187&&((admitted&&visit<=236)||(capacity==1&&visit<=188));
        const bool bound_next=admitted&&visit>=187&&visit<=203;
        const bool published_next=bound_next&&visit>=188;
        const bool bound_old=visit<=197,bound_bomb=visit==186;
        const unsigned next_phase=!capacity||visit==186?0:!admitted?(visit==187?2:3):visit<=234?1:visit==235?2:3;
        const bool opponent_hit=admitted&&visit>=opponent_visit,self_hit=admitted&&visit>=188;
        const unsigned opponent=opponent_hit?34:17,self=self_hit?8:4;
        if(byte(bomb)!=unsigned(live_bomb)||byte(bomb+0x18)!=(visit==186?1u:visit==187?2u:3u)||
           byte(child)!=unsigned(live_old)||byte(child+0x18)!=(visit<=228?1u:visit==229?2u:3u)||
           byte(next_area)!=unsigned(live_next)||byte(next_area+0x18)!=next_phase||
           byte(bomb_shape)!=unsigned(bound_bomb)||byte(child_shape)!=unsigned(bound_old)||
           byte(next_shape)!=unsigned(bound_next)||half(attack_count)!=bound_bomb+bound_old+published_next||
           half(contact::receiver_count)!=18+unsigned(bound_bomb)||half(fighter(1)+0x774)!=(visit<=187?9:10)||
           half(fighter(0)+0x774)!=10||word(fighter(1)+0x418)!=opponent||word(fighter(0)+0x41C)!=opponent||
           word(fighter(1)+0x41C)!=self||word(fighter(1)+0x42C)||word(fighter(0)+0x418)||
           half(fighter(0)+0x1C6)!=180-opponent||half(fighter(1)+0x1C6)!=220-self||
           half(fighter(0)+0x20C)!=2||half(fighter(1)+0x20C)!=2||
           half(fighter(0)+0x20E)!=(opponent_hit?4:0)||half(fighter(1)+0x20E)!=(self_hit?4:0)||
           half(contact::free_count+6)!=2+unsigned(capacity!=0)-live_bomb-live_old-live_next||
           half(contact::free_count+8)!=2+unsigned(admitted)-bound_bomb-bound_old-bound_next||
           half(pools+18)!=18-2*live_old-2*live_next-live_bomb)
            fail("cascade ownership, shape publication, HP, ledgers or stock changed");
        for(unsigned i=0;i<2;++i)
            if(f(i,0x18)!=1||word(fighter(i)+0x420)||word(fighter(i)+0x424))
                fail("cascade unexpectedly affected a fighter's phase or knockout ledgers");
        if(live_bomb&&(half(bomb+0x1C6)||half(bomb+0x26C)!=3||word(bomb+0x90)!=fighter(1)))
            fail("cascade did not retain the second paid parent with zero HP");
        if(bound_bomb&&(half(bomb+0x20C)!=2||word(bomb_shape+0x20)!=bomb||word(attack_list)!=bomb_shape))
            fail("cascade did not record the old area's token against the second bomb");
        const auto area_identity=[&](u32 carrier,u32 cell,unsigned token,bool live,bool bound){
            if(live&&(byte(carrier+0x10)!=5||byte(carrier+0x11)!=19||word(carrier+0x8C)!=bomb||
               word(carrier+0x90)!=fighter(1)||half(carrier+0x26C)!=token))
                fail("cascade area lost its own token, type, key or actor credit");
            if(bound&&(word(cell+0x20)!=carrier||word(cell+0x24)!=fighter(1)||
               word(cell+0x28)!=common_hits+0x400+246*80||word(cell+0x2C)!=0x802C4760||
               byte(cell+0x11)!=26||byte(cell+0x14)!=179))
                fail("cascade bound the wrong shared area descriptor");
        };
        area_identity(child,child_shape,2,live_old,bound_old);
        area_identity(next_area,next_shape,admitted?4:0,live_next,bound_next);
        if(bound_next&&visit==187&&(word(word(next_area+0xE0)+20)!=22||
           word(next_area+0x20)!=word(bomb+0x20)||word(next_area+0x24)!=word(bomb+0x24)||
           word(next_area+0x28)!=word(bomb+0x28)))
            fail("cascade initializer did not keep its source origin and opening view");
        unsigned old_entries=0,next_entries=0;
        for(unsigned j=0;j<half(attack_count);++j){
            old_entries+=word(attack_list+j*4)==child_shape;
            next_entries+=word(attack_list+j*4)==next_shape;
        }
        if(old_entries!=unsigned(bound_old)||next_entries!=unsigned(published_next))
            fail("cascade did not publish each distinct shape exactly once");
        if(!live_old&&!live_next&&(half(pools+18)!=18||half(pools+28+18)||word(archive::source_globals)!=common_view_owner))
            fail("cascade leaked view cells or retired the common archive");
    }
    void offer_cascade_cells(unsigned capacity){
        if(half(contact::free_count+6)!=1||word(contact::free_head+12)!=bomb||
           word(contact::free_tail+12)!=bomb||word(bomb+4)||byte(bomb)||
           half(contact::free_count+8)!=1||word(contact::free_head+16)!=bomb_shape||
           word(contact::free_tail+16)!=bomb_shape||word(bomb_shape+4)||byte(bomb_shape)||
           !byte(child)||!byte(child_shape)||half(child+0x26C)!=2||half(fighter(1)+0x774)!=10)
            fail("cascade fork must start from the first area's published shape");
        if(capacity>=1){
            at(next_area+2)[0]=3;
            store(bomb+4,next_area);store(contact::free_tail+12,next_area);
            store16(contact::free_count+6,2);
        }
        if(capacity==2){
            at(next_shape+2)[0]=4;
            store(bomb_shape+4,next_shape);store(contact::free_tail+16,next_shape);
            store16(contact::free_count+8,2);
        }
    }
    void probe_cascade(unsigned delay){
        const auto saved=ram;
        for(unsigned capacity=0;capacity<3;++capacity){
            std::memcpy(ram.data(),saved.data(),ram.size());
            offer_cascade_cells(capacity);
            store(pad+32,0x200);
            check_payment(delay,capacity,false);
            const unsigned opponent_visit=190+unsigned(delay!=0);
            for(unsigned visit=186;visit<=(capacity==2?237u:231u);++visit){
                if(visit==187){
                    auto fork=ram;
                    CPUState request{};
                    const auto inlet=mod.run_to_or_return(request,fork,scheduler::dispatch,{area_factory},nullptr,65536);
                    if(inlet!=area_factory||request.lr!=0x800E0B1C||request.gpr[3]!=bomb||
                       request.gpr[4]!=bomb+0x20||request.gpr[5]!=19||contact::half(fork,bomb+0x1C6)||
                       contact::half(fork,bomb+0x152)!=180||contact::half(fork,contact::free_count+6)!=unsigned(capacity!=0))
                        fail("cascade fork did not follow the zero-HP terminal request");
                    std::printf("cascade-request: %u %u %u %08X %u %u %u\n",delay,capacity,visit,request.lr-4,
                                request.gpr[5],contact::half(fork,bomb+0x152),contact::half(fork,contact::free_count+6));
                    if(capacity){
                        std::memcpy(fork.data(),ram.data(),ram.size());
                        CPUState init{};
                        const auto cut=mod.run_to_or_return(init,fork,scheduler::dispatch,{0x80086084},nullptr,65536);
                        if(cut!=0x80086084||init.gpr[3]!=next_area||!contact::at(fork,bomb_shape)[0]||
                           !contact::at(fork,child_shape)[0]||!contact::at(fork,bomb+0x1D8)[0]||
                           contact::half(fork,contact::free_count+8)!=unsigned(capacity==2)||
                           be32(contact::at(fork,contact::free_head+16))!=(capacity==2?next_shape:0))
                            fail("cascade initializer did not run while both earlier shapes were active");
                        std::printf("cascade-init: %u %u %u %u\n",delay,capacity,visit,
                                    contact::half(fork,contact::free_count+8));
                    }
                }
                const auto receiver=visit==186?bomb:capacity==2&&visit==188?fighter(1):
                                    capacity==2&&visit==opponent_visit?fighter(0):0;
                if(receiver)check_cascade_amount(visit,visit==186?child:next_area,receiver);
                const auto before=word(scene+176),turn_before=word(scene+168),combat_before=word(scene+172);
                CPUState cpu{};CallObservation hits{contact::hp_write};
                hits.sample=sample_hp;
                const auto cut=mod.run_to_or_return(cpu,ram,scheduler::dispatch,{0x80264290},&hits,1u<<20);
                if(cut||word(scene+176)!=before+1||word(scene+168)!=turn_before+1||
                   word(scene+172)!=combat_before+1||hits.count!=unsigned(receiver!=0)||
                   (receiver&&(hits.caller!=0x8002F1D8||hits.first_arguments[0]!=receiver||
                              hits.second_arguments[0]!=(receiver==fighter(0)?17:4)||
                              hits.samples[0]!=(receiver==bomb?1:receiver==fighter(0)?163:216))))
                    fail("cascade did not complete its original world call");
                check_cascade_world(capacity,visit,opponent_visit);
                if(visit<=189||visit==198||visit==231||(capacity==2&&(visit==opponent_visit||
                   visit==204||visit>=235))){
                    std::printf("cascade: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                                delay,capacity,visit,half(fighter(1)+0x774),byte(bomb),byte(bomb+0x18),byte(child),
                                byte(child+0x18),byte(next_area),byte(next_area+0x18),byte(next_shape),
                                half(next_area+0x26C),half(attack_count),half(contact::receiver_count),
                                half(fighter(0)+0x1C6),half(fighter(1)+0x1C6),word(fighter(1)+0x418),
                                half(fighter(0)+0x20E),half(fighter(1)+0x20E),half(contact::free_count+6),
                                half(contact::free_count+8),half(pools+18));
                }
                if(receiver)
                    std::printf("cascade-hp: %u %u %u %08X %u %u\n",delay,capacity,visit,
                                hits.first_arguments[0],hits.second_arguments[0],hits.samples[0]);
                store(pad+32,0);
            }
        }
    }
    void probe_terminal(unsigned delay){
        offer_cascade_cells(2);
        store(pad+32,0x200);
        check_payment(delay,2,false);
        const auto step=[&](CallObservation& hits){
            const auto before=word(scene+176),turn_before=word(scene+168),combat_before=word(scene+172);
            CPUState cpu{};
            const auto cut=mod.run_to_or_return(cpu,ram,scheduler::dispatch,{0x80264290},&hits,1u<<20);
            if(cut||word(scene+176)!=before+1||word(scene+168)!=turn_before+1||word(scene+172)!=combat_before+1)
                fail("terminal fork did not complete its original coordinator call");
        };
        CallObservation opening{contact::hp_write};opening.sample=sample_hp;
        step(opening);
        store(pad+32,0);
        const bool landed=delay!=0;
        const unsigned opening_count=landed?4:3,initial_credit=landed?40:17,initial_hp=180-initial_credit;
        if(opening.count!=opening_count||opening.caller!=0x8002F1D8||byte(bomb+0x1D9)!=(landed?16:0)||
           byte(bomb+0x1DA)!=1||half(bomb+0x1C6)||half(bomb+0x152)!=180||half(bomb+0x26C)!=3||
           half(fighter(0)+0x1C6)!=initial_hp||half(fighter(1)+0x1C6)!=216||half(fighter(1)+0x774)!=9||
           word(fighter(1)+0x418)!=initial_credit||word(fighter(0)+0x41C)!=initial_credit||
           word(fighter(1)+0x41C)!=4||word(fighter(1)+0x42C)||half(bomb+0x20C)!=2||
           half(fighter(0)+0x20C)!=(landed?3:2)||half(fighter(0)+0x20E)!=(landed?2:0)||
           half(fighter(1)+0x20C)!=2||half(attack_count)!=2||word(attack_list)!=bomb_shape||
           word(attack_list+4)!=child_shape||!byte(bomb)||byte(bomb+0x18)!=1)
            fail("terminal priority inlet lost its exact same-visit second placement or contacts");
        std::printf("terminal-inlet: %u %u %u %u %u %u %u %u %u %u\n",delay,opening.count,
                    byte(bomb+0x1D9),byte(bomb+0x1DA),half(bomb+0x1C6),half(bomb+0x152),
                    half(bomb+0x26C),half(fighter(0)+0x1C6),half(fighter(1)+0x1C6),word(fighter(1)+0x418));
        for(unsigned i=0;i<opening.count;++i){
            const auto recipient=i<opening_count-2?fighter(0):i==opening_count-2?bomb:fighter(1);
            const unsigned requested=i<opening_count-2?(i?14:landed?26:17):4;
            const unsigned before=i<opening_count-2?(i?154:180):recipient==bomb?1:220;
            if(opening.first_arguments[i]!=recipient||opening.second_arguments[i]!=requested||opening.samples[i]!=before)
                fail("terminal priority inlet changed its ordered HP writers");
            std::printf("terminal-inlet-hp: %u %u %08X %u %u\n",delay,i,opening.first_arguments[i],
                        opening.second_arguments[i],opening.samples[i]);
        }
        const auto saved=ram;
        for(unsigned rule=0;rule<(delay?4u:1u);++rule){
            std::memcpy(ram.data(),saved.data(),ram.size());
            // Declared controls use the exact retained state and change only these two operands.
            if(rule&1)at(bomb+0x1D9)[0]=0;
            if(rule&2)store16(bomb+0x1C6,1);
            auto fork=ram;
            CPUState request{};CallObservation seen{area_factory};
            const auto inlet=mod.run_to_or_return(request,fork,scheduler::dispatch,{area_factory,0x80264290},&seen,65536);
            const bool exits=rule!=3;
            const u32 expected_site=exits?(landed&&!(rule&1)?0x800E0AD4:0x800E0B18):0;
            if(inlet!=(exits?area_factory:0)||seen.count!=unsigned(exits)||seen.caller!=expected_site||
               (exits&&(request.lr!=expected_site+4||request.gpr[3]!=bomb||request.gpr[4]!=bomb+0x20||
                         request.gpr[5]!=19||contact::half(fork,bomb+0x152)!=180||
                         contact::half(fork,contact::free_count+6)!=1||!contact::at(fork,bomb+0x1D8)[0])))
                fail("terminal arbitration changed the original request branch or boundary");
            std::printf("terminal-choice: %u %u %08X %u %08X %u %u %u\n",delay,rule,inlet,seen.count,seen.caller,
                        seen.arguments[2],half(bomb+0x1C6),byte(bomb+0x1D9));
            for(unsigned visit=186;visit<=(exits?190u:186u);++visit){
                const unsigned damage=landed?14:17;
                if(visit==187){
                    std::memcpy(fork.data(),ram.data(),ram.size());
                    CallObservation amount{contact::amount_entry};amount.sample=sample_tier;
                    CPUState cpu{};mod.run_observed(cpu,fork,scheduler::dispatch,amount,65536);
                    if(amount.count!=2||amount.caller!=0x8002F108)
                        fail("terminal follow-up used the wrong amount entry");
                    for(unsigned i=0;i<2;++i)
                        if(amount.first_arguments[i]!=child_amount||amount.second_arguments[i]!=next_area||
                           amount.third_arguments[i]!=fighter(i)||amount.samples[i]!=unsigned(landed&&i==0))
                            fail("terminal follow-up contacts lost their new area, tier or recipient");
                }
                CallObservation hits{contact::hp_write};hits.sample=sample_hp;
                step(hits);
                const bool landed_again=visit>=187;
                const unsigned credit=initial_credit+(landed_again?damage:0);
                const unsigned expected_bomb_phase=!exits?1:visit==186?2:3;
                if(half(fighter(1)+0x774)!=(landed_again?10:9)||half(fighter(0)+0x774)!=10||
                   byte(bomb)!=unsigned(visit<=187)||byte(bomb+0x18)!=expected_bomb_phase||
                   byte(bomb_shape)!=unsigned(!exits)||half(bomb+0x152)!=(exits?180:179)||
                   !byte(child)||!byte(child_shape)||half(child+0x26C)!=2||
                   byte(next_area)!=unsigned(exits)||byte(next_shape)!=unsigned(exits)||
                   half(next_area+0x26C)!=(exits?4:0)||half(attack_count)!=(exits&&visit==186?1:2)||
                   half(contact::receiver_count)!=(exits?18:19)||
                   half(fighter(0)+0x1C6)!=180-credit||half(fighter(1)+0x1C6)!=(landed_again?212:216)||
                   word(fighter(1)+0x418)!=credit||word(fighter(0)+0x41C)!=credit||word(fighter(1)+0x42C)||
                   word(fighter(1)+0x41C)!=(landed_again?8:4)||half(fighter(0)+(landed?0x210:0x20E))!=(landed_again?4:0)||
                   half(fighter(1)+0x20E)!=(landed_again?4:0)||hits.count!=(visit==187?2u:0u))
                    fail("terminal follow-up changed its actual stock, lists, HP or tokens");
                if(exits&&(byte(next_area+0x18)!=1||byte(next_area+0x10)!=5||byte(next_area+0x11)!=19||
                   word(next_area+0x8C)!=bomb||word(next_area+0x90)!=fighter(1)||word(next_shape+0x20)!=next_area||
                   word(next_shape+0x24)!=fighter(1)||word(next_shape+0x28)!=common_hits+0x400+246*80||
                   word(next_shape+0x2C)!=0x802C4760||byte(next_shape+0x11)!=26||byte(next_shape+0x14)!=179||
                   (visit==186&&word(attack_list)!=child_shape)))
                    fail("terminal follow-up changed the new area's descriptor or ownership");
                if(!exits&&(half(bomb+0x1C6)!=1||word(attack_list)!=bomb_shape||word(attack_list+4)!=child_shape))
                    fail("terminal control did not retain both original published shapes");
                if(landed_again){
                    const auto first=word(attack_list),second=word(attack_list+4);
                    if(!((first==child_shape&&second==next_shape)||(first==next_shape&&second==child_shape)))
                        fail("terminal follow-up did not publish both distinct key-19 shapes");
                }
                if(hits.count&&(hits.caller!=0x8002F1D8||hits.first_arguments[0]!=fighter(0)||
                   hits.first_arguments[1]!=fighter(1)||hits.second_arguments[0]!=damage||
                   hits.second_arguments[1]!=4||hits.samples[0]!=initial_hp||hits.samples[1]!=216))
                    fail("terminal follow-up HP writer changed its ordered requests");
                std::printf("terminal-world: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                            delay,rule,visit,half(fighter(1)+0x774),byte(bomb),byte(bomb+0x18),half(bomb+0x152),
                            byte(next_area),byte(next_area+0x18),byte(next_shape),half(next_area+0x26C),
                            half(attack_count),half(fighter(0)+0x1C6),half(fighter(1)+0x1C6),hits.count);
                for(unsigned i=0;i<hits.count;++i)
                    std::printf("terminal-world-hp: %u %u %u %u %08X %u %u\n",delay,rule,visit,i,
                                hits.first_arguments[i],hits.second_arguments[i],hits.samples[i]);
            }
        }
    }
    void world_area(unsigned delay,Run mode){
        const bool reaction=mode==Run::reaction,cascade=mode==Run::cascade,terminal=mode==Run::terminal;
        const bool area_report=!reaction&&!cascade&&!terminal;
        const auto saved=ram;
        for(unsigned shapes:{1u,2u}){
            if((cascade||terminal)&&shapes!=2)continue;
            std::memcpy(ram.data(),saved.data(),ram.size());
            at(stage)[0]=1;at(stage+8)[0]=1;
            if(half(contact::free_count+6)||word(contact::free_head+12)||half(contact::free_count+8)||
               word(contact::free_head+16)||word(bomb_bucket)||half(fighter(1)+0x774)!=10)
                fail("dual world area requires an unconsumed active input and empty carrier pools");
            at(bomb+2)[0]=at(child+2)[0]=3;
            store(bomb+4,child);
            store(contact::free_head+12,bomb);store(contact::free_tail+12,child);
            store16(contact::free_count+6,2);
            at(bomb_shape+2)[0]=at(child_shape+2)[0]=4;
            store(bomb_shape+4,shapes==2?child_shape:0);
            store(contact::free_head+16,bomb_shape);store(contact::free_tail+16,shapes==2?child_shape:bomb_shape);
            store16(contact::free_count+8,u16(shapes));
            store(pad+32,0x200);
            check_payment(delay,shapes,area_report);
            unsigned previous_phase=0,previous_shape=0;
            for(unsigned visit=1;visit<=(reaction?235u:240u);++visit){
                if(cascade&&visit==186){probe_cascade(delay);break;}
                if(terminal&&visit==185){probe_terminal(delay);break;}
                if(visit==181)check_area_timeout(delay,shapes,area_report);
                if(visit==185&&shapes==2)check_area_amount();
                if(reaction&&visit>=184)probe_reaction_b(delay,shapes,visit);
                const auto before=word(scene+176);
                const auto turn_before=word(scene+168),combat_before=word(scene+172);
                CPUState cpu{};CallObservation hits{contact::hp_write};
                const auto cut=mod.run_to_or_return(cpu,ram,scheduler::dispatch,{0x80264290},&hits,1u<<20);
                if(cut)
                    std::fprintf(stderr,"world area SDK assertion shapes=%u visit=%u from=%08X line=%u\n",
                                 shapes,visit,cpu.lr-4,cpu.gpr[4]);
                if(cut||word(scene+176)!=before+1||word(scene+168)!=turn_before+1||
                   word(scene+172)!=combat_before+1||hits.count>2||word(fighter(0)+0x5BC)||
                   word(fighter(1)+0x5BC)!=(visit==1?0x200u:0))
                    fail("world area did not finish its original coordinator call");
                const unsigned phase=byte(child+0x18);
                const auto attack=half(attack_count)?word(attack_list):0;
                const bool child_published=attack&&word(attack+0x20)==child;
                check_area_world(shapes,visit,hits,child_published);
                if(reaction&&visit>=183)report_reaction(delay,shapes,visit);
                if(area_report&&(visit<=2||visit==180||visit==181||visit==182||visit==183||hits.count||
                   phase!=previous_phase||child_published!=bool(previous_shape)||(visit>181&&!byte(child))||visit==240))
                    std::printf("dual-area: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %08X\n",
                                delay,shapes,visit,half(fighter(1)+0x774),byte(bomb),byte(bomb+0x18),byte(child),
                                phase,unsigned(child_published),half(attack_count),half(contact::receiver_count),
                                half(child+0x26C),half(fighter(0)+0x1C6),half(fighter(1)+0x1C6),hits.count,
                                word(fighter(1)+0x418),word(fighter(1)+0x42C),word(fighter(0)+0x41C),
                                word(fighter(1)+0x41C),half(fighter(0)+0x20C),half(fighter(1)+0x20C),
                                child_published?word(attack+0x7C):0);
                for(unsigned j=0;area_report&&j<hits.count;++j)
                    std::printf("dual-area-hp: %u %u %u %u %08X %u\n",
                                delay,shapes,visit,j,hits.first_arguments[j],hits.second_arguments[j]);
                previous_phase=phase;previous_shape=unsigned(child_published);
                store(pad+32,0);
                if(!reaction&&visit>181&&!byte(bomb)&&!byte(child))break;
            }
        }
    }
public:
    Replay(const Module& m,const std::vector<u8>& original,const std::array<Assets,2>& assets,const AreaAssets* area):
        mod(m),ram(original){
        setup(assets,area);
    }
    void observe(Run mode){
        const bool world=mode!=Run::entry;
        const auto start=ram;
        for(unsigned delay:{0u,10u}){
            std::memcpy(ram.data(),start.data(),ram.size());
            unsigned prior_stage=0,prior_ready=0,prior_mask=0;
            const unsigned opens=174+delay;
            for(unsigned visit=1;visit<=opens+1;++visit){
                at(scene+0xD2)[0]=u8(visit>=2&&visit<2+delay?2:0);
                const unsigned before=f(0,0x743)+2*f(1,0x743);
                if(world&&visit==opens+1){
                    if(mode==Run::bomb)world_bomb(delay);else world_area(delay,mode);
                    break;
                }
                if(!world&&visit>=opens)pulse(delay,visit,visit>opens);
                CPUState cpu{};CallObservation actors_called{present_body};
                const auto stop=mod.run_to_or_return(cpu,ram,scheduler::dispatch,{body_pass_end,0x80264290},&actors_called,1u<<20);
                if(stop==0x80264290)
                    std::fprintf(stderr,"dual scene SDK assertion delay=%u visit=%u caller=%08X line=%u\n",
                                 delay,visit,cpu.lr-4,cpu.gpr[4]);
                const auto first=actors_called.first_arguments[0],second=actors_called.first_arguments[1];
                if(stop!=body_pass_end||cpu.lr!=0x80029490||actors_called.count!=2||actors_called.caller!=0x80054140||
                   first!=fighter(0)||second!=fighter(1)||
                   byte(0x8043B5A0-21312)!=2||word(0x803C4E6C)!=first||word(0x803C4E70)!=second){
                    std::fprintf(stderr,"dual actor visit=%u delay=%u cut=%08X lr=%08X calls=%u %08X %08X "
                        "list=%u %08X %08X phase=%u/%u HP=%u/%u\n",visit,delay,stop,cpu.lr,actors_called.count,
                        actors_called.first_arguments[0],actors_called.first_arguments[1],byte(0x8043B5A0-21312),
                        word(0x803C4E6C),word(0x803C4E70),f(0,0x18),f(1,0x18),half(fighter(0)+0x1C6),half(fighter(1)+0x1C6));
                    fail("dual scene did not visit two original actors before its world tail");
                }
                for(unsigned i=0;i<2;++i){
                    if(!f(i,0)||f(i,0x18)!=1||half(fighter(i)+0x3E8)!=models[i]||
                       half(fighter(i)+0x1C6)!=hp[i]||f(i,0x3F0)!=1||
                       word(fighter(i)+0x3FC)!=word(scene+128+4*i)||f(i,0x3E6)||
                       word(fighter(i)+0x5B4)||word(fighter(i)+0x5BC)||half(fighter(i)+0x774)!=10||
                       f(i,0x743)!=unsigned(visit>=81+(i?delay:0))||
                       f(i,0x19)!=(visit<81+(i?delay:0)?0u:visit<opens?1u:2u)||
                       half(fighter(i)+0x272)!=(visit<21+(i?delay:0)?0xEF:visit<opens?0xC3:0)||
                       word(fighter(i)+0x720)||(i&&visit<=11&&delay&&word(fighter(i)+0xD4)))
                        fail("dual scene original actor left its exact normal neutral setup");
                }
                if(visit==1&&!world)check_constructor(delay);
                const auto steps=rebuild();
                const unsigned status=byte(scene+69),stage=byte(scene+1),ready=f(0,0x743)+2*f(1,0x743);
                const unsigned mask=(steps[0]?1u:0u)+(steps[1]?2u:0u);
                const auto expected_stage=visit<82+delay?0u:visit==82+delay?1u:visit<opens-1?2u:3u;
                if(before!=unsigned(visit>81)+2*unsigned(visit>81+delay)||stage!=expected_stage||
                   status!=(visit<opens?3u:4u)||steps[0]!=(visit<opens?0u:0x3EF0)||steps[1]!=steps[0])
                    fail("dual readiness, scene release and receiving publication did not remain separate");
                if(!world&&(visit<=2||visit==11||visit==12||visit==21||(delay&&visit==31)||
                   stage!=prior_stage||ready!=prior_ready||mask!=prior_mask||visit==opens+1)){
                    std::printf("dual-scene: %u %u %u %u %u %u %u %u %u %u %04X %04X %04X %04X\n",
                                delay,visit,before,ready,status,stage,half(scene+8),f(0,0x19),f(1,0x19),
                                unsigned(actors_called.first_arguments[0]==fighter(1)),
                                half(fighter(0)+0x272),half(fighter(1)+0x272),steps[0],steps[1]);
                }
                prior_stage=stage;prior_ready=ready;prior_mask=mask;
            }
        }
    }
};
} // namespace

int main(int argc,char** argv){
    Run mode=Run::entry;
    if(argc==4&&!std::strcmp(argv[3],"--world-bomb"))mode=Run::bomb;
    if(argc==4&&!std::strcmp(argv[3],"--world-area"))mode=Run::area;
    if(argc==4&&!std::strcmp(argv[3],"--world-reaction"))mode=Run::reaction;
    if(argc==4&&!std::strcmp(argv[3],"--world-cascade"))mode=Run::cascade;
    if(argc==4&&!std::strcmp(argv[3],"--world-terminal"))mode=Run::terminal;
    if(argc!=3&&mode==Run::entry)
        fail("usage: dual-scene-entry-fixture qualified.dol unmodified-recomp.so [--world-bomb|--world-area|--world-reaction|--world-cascade|--world-terminal] < qualified-roles");
    const bool area=mode!=Run::entry&&mode!=Run::bomb;
    std::array<Assets,2> assets{};
    const auto read=[](auto& field){
        if(std::fread(field.data(),1,field.size(),stdin)!=field.size())fail("dual scene asset extent");
    };
    for(unsigned i=0;i<2;++i){
        auto& a=assets[i];a.motion.resize(motion_sizes[i]);a.view.resize(view_sizes[i]);
        read(a.spec);read(a.hit);read(a.motion);read(a.view);
    }
    AreaAssets common{};
    if(area){read(common.collision);common.view=archive::read_source();}
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra dual scene assets");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    Replay(mod,original,assets,area?&common:nullptr).observe(mode);
}
