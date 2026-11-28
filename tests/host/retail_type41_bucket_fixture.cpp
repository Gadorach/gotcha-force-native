// Original type-41 placement, bucket order, area request and stock return.
#include "retail_stationary_contact.hpp"
#include "retail_body_phase_cleanup.hpp"
#include <algorithm>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace cleanup_probe=retail_body_phase_cleanup;
using Common=std::array<u8,21504>;
constexpr u32 profile=0x801301F8,owner_hook=0x801304B8,make_bomb=0x800E058C,bomb=0x800E06C4;
constexpr u32 area=0x80085E00,return_stock=0x8006DE10,tail=0x803C706C,head=0x803C6F6C;
constexpr u32 blast=0x81031000,desc=0x81033000,pool=0x81033800,hit_block=0x81035000;
constexpr u32 extra_shape=0x81032400;
constexpr u32 stage=0x81048000,common=0x81049000,side=0x81022000,actor_spec=0x81014000;
constexpr u32 bank=0x803BB4C0,visual=0x81042000,visual_links=visual+0x40,visual_rows=visual+0x80,empty=visual+0x600;
constexpr std::array<u32,2> carriers{0x81030000,0x81030800},shapes{0x81032000,0x81032200};

struct Options {u8 model,shape_cells,carrier_cells,descriptor_cells,exit;bool reverse;};
// exit: 0 placement refusal, 1 binder refusal, 2 time expiry, 3 remote command,
// 4 owner invalidation, 5 remote at cap. A successful area is optional.
constexpr Options cases[]{
    {6,0,0,1,0,false},{7,0,0,1,0,false},
    {6,0,1,0,0,false},{7,0,1,0,0,false},
    {6,0,1,1,1,false},{7,0,1,1,1,false},
    {6,1,1,3,2,false},{6,1,1,2,2,false},
    {7,2,2,4,3,false},{7,2,2,4,3,true},{7,2,2,3,3,false},{7,2,2,4,3,true},
    {7,1,1,1,4,false},{7,0,0,0,5,false},{7,3,2,4,3,false},{6,2,1,3,2,false},
};
inline u32 cell_at(unsigned index,bool reverse){return carriers[reverse?(1-index):index];}
struct AreaCase {float distance;bool shape,ally,masked;};
constexpr AreaCase area_cases[]{
    {22,false,false,false},{22,true,false,false},{70,true,false,false},{409,true,false,false},
    {409.5f,true,false,false},{410,true,false,false},{22,true,true,false},{22,true,false,true},
};
enum class AreaMode {hp,tiers,knockout,body_exit,body_cleanup};
struct AreaProbe {
    unsigned index;
    const contact::HitBlock& hurt;
    const contact::ActorSpec& spec;
    AreaMode mode;
    const cleanup_probe::Delivery* delivery=nullptr;
};
struct TierCase {u8 meter,tier,flags;u16 gauge;bool hp_held;};
constexpr TierCase tier_cases[]{
    {0,0,0,500,false},{49,1,0,500,false},{50,1,0,500,false},{99,2,0,150,false},
    {60,2,0,100,false},{50,1,128,100,false},{99,63,0,500,false},{50,1,0,100,true},
};
struct KnockoutCase {u16 hp,force;bool debit,reserve,eligible,held,ally;int reserves;};
constexpr KnockoutCase knockout_cases[]{
    {18,200,true,false,false,false,false,0}, {17,200,true,false,false,false,false,0},
    {7,140,true,false,false,false,false,0}, {7,200,false,true,true,false,false,2},
    {7,200,false,true,false,false,false,2}, {7,140,true,true,true,true,false,2},
    {3,200,true,false,false,false,true,0}, {7,150,true,true,true,false,false,0},
};
constexpr std::array<unsigned,5> exit_knockout_case{2,2,5,2,2};
struct ParentCase {float offset;bool incoming,ally,command;};
constexpr ParentCase parent_cases[]{
    {30,false,false,false},{42,false,false,false},{43,false,false,false},{30,false,true,false},
    {0,true,false,false},{171,true,false,false},{30,false,false,true},{0,true,false,true},
};
struct ParentProbe {unsigned index;const contact::HitBlock& hurt;const contact::ActorSpec& spec;};
constexpr u32 incoming_cell=0x81031800;

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 pc){return contact::at(ram,pc);}
    u16 half(u32 pc){return contact::half(ram,pc);}
    u32 word(u32 pc){return be32(at(pc));}
    u32 call(u32 entry,u32 arg=actor){
        CPUState cpu{};cpu.gpr[3]=arg;
        mod.run(cpu,ram,entry,4096);
        return cpu.gpr[3];
    }
    CallObservation observe(u32 entry,u32 arg,u32 callee){
        CPUState cpu{};cpu.gpr[3]=arg;
        CallObservation observation{callee};
        mod.run_observed(cpu,ram,entry,observation,4096);
        return observation;
    }
    void set_pool(unsigned kind,const std::array<u32,3>& cells,unsigned count){
        if(count>cells.size()||half(contact::free_count+2*kind))fail("type-41 free pool extent");
        put32(at(contact::free_head+4*kind),count?cells[0]:0);
        put32(at(contact::free_tail+4*kind),count?cells[count-1]:0);
        put16(at(contact::free_count+2*kind),u16(count));
        for(unsigned i=0;i<count;++i){
            at(cells[i])[2]=u8(kind);
            put32(at(cells[i]+4),i+1<count?cells[i+1]:0);
        }
    }
    void rebuild(){for(u32 entry:{0x8008A28C,0x80089AEC})call(entry,0);}
    void place(u32 pc,float x){
        put_float(at(pc),x);put_float(at(pc+4),100.f);put_float(at(pc+8),300.f);
    }
    void empty_stage_grid(){
        constexpr u32 grid=stage+0x100,refs=stage+0x200,no_triangles=refs+64*4;
        for(unsigned i=0;i<3;++i)put32(at(stage+28+4*i),grid);
        constexpr std::array<u32,6> grid_fields{500,500,8,8,u32(-1000),u32(-1000)};
        for(unsigned i=0;i<grid_fields.size();++i)put32(at(grid+4*i),grid_fields[i]);
        put32(at(grid+24),refs);put32(at(no_triangles),0xFFFFFFFF);
        for(unsigned i=0;i<64;++i)put32(at(refs+4*i),no_triangles);
    }
    void placement(unsigned i,Options c){
        put32(at(actor+0x5BC),0x200);
        put_float(at(actor+0x694),0.f); // declared eligible post-hook, not elapsed cooldown
        const auto stock=half(actor+0x774);
        if(c.exit==0){
            const auto seed=ram;
            CPUState cpu{};cpu.gpr[3]=actor;
            CallObservation spend{0x8006DBE0};
            mod.run_to(cpu,ram,owner_hook,{return_stock},&spend);
            if(spend.count!=1||spend.caller!=(c.model==6?0x8013055C:0x801305AC)||spend.arguments[0]!=actor||
               spend.arguments[1]||spend.arguments[2]!=1||spend.arguments[3]!=1||cpu.lr!=0x800E0E80||
               cpu.gpr[3]!=actor||cpu.gpr[4]||half(actor+0x774)!=stock-1||word(head+9*4))
                fail("type-41 refusal did not debit before the table-selected return");
            std::memcpy(ram.data(),seed.data(),seed.size());
        }
        const auto observed=observe(owner_hook,actor,make_bomb);
        const unsigned expected_key=c.model==6?2:3;
        const unsigned expected_pose=c.model==6?10:11+((stock-1)&1);
        if(observed.count!=1||observed.caller!=(c.model==6?0x80130580:0x801305E8)||
           observed.arguments[0]!=actor||observed.arguments[1]!=expected_key||observed.arguments[2]!=expected_pose||
           half(actor+0x774)!=stock-unsigned(c.exit!=0)||word(actor+0x778))
            fail("type-41 paid placement, parity or factory-refusal stock changed");
        if(c.exit==0)return;
        const u32 cell=cell_at(i,c.reverse);
        if(!at(cell)[0]||at(cell+0x10)[0]!=41||at(cell+0x11)[0]!=expected_key||
           at(cell+0x13)[0]!=expected_pose||word(cell+0x8C)!=actor||word(cell+0x90)!=actor||
           word(cell+0xC)!=bomb||at(cell+0x18)[0]||word(cell+0xCC)!=side)
            fail("type-41 carrier did not come from its paid owner and exact key");
        place(cell+0x20,i?420.f:200.f);
    }
    void verify_scope(Options c){
        if(half(actor+0x1C6)!=220||half(side+0x774)!=9||half(actor+0x784)!=(c.model==6?3:0)||
           word(actor+0x778)||word(actor+0x418))
            fail("type-41 tested path changed another actor, clock or HP");
    }
    void isolate_area(unsigned capacity,u32 parent){
        constexpr u32 ordinary_attack_count=0x8043B5A0-21328;
        if(at(blast+0x18)[0]!=1||half(actor+0x774)!=capacity-1)
            fail("area probe requires the real initializer and separate paid parent");
        call(0x80089554,0);rebuild();
        if(half(ordinary_attack_count))fail("area probe counted the unopened area");
        check_return(observe(0x80089A94,8,return_stock));
        call(0x80089554,0);rebuild();
        if(half(ordinary_attack_count)!=1||word(0x803C477C)!=shapes[1]||word(shapes[1]+0x20)!=blast||
           half(blast+0x26C)!=2||half(actor+0x774)!=capacity||!at(parent)[0])
            fail("area probe did not isolate the exact child and receiver");
    }
    void tier_contact(Options c,const AreaProbe& probe,unsigned capacity,u32 parent){
        const auto rule=tier_cases[probe.index];
        constexpr u32 body=contact::receiver,gauge=body+0x6C2,meter=body+0x6C8,tier=body+0x6CA,flags=body+0x6FD;
        constexpr u32 gauge_timer=body+0x684,tier_timer=body+0x68C;
        const auto bits=[](float f){return std::bit_cast<u32>(f);};
        isolate_area(capacity,parent);
        const auto maximum=u16(u16(probe.spec[0])<<8|probe.spec[1]);
        if(maximum!=500)fail("tier probe source receiver capacity");
        put16(at(body+0x6BE),maximum);put16(at(gauge),rule.gauge);put16(at(meter),rule.meter);
        at(tier)[0]=rule.tier;at(flags)[0]=rule.flags;
        at(contact::scene+31)[0]=u8(rule.hp_held);
        auto writer_ram=ram;
        CPUState pair{};CallObservation amount{contact::amount_entry};
        mod.run_to(pair,writer_ram,contact::pair_pass,{contact::hp_write},&amount);
        if(amount.count!=1||amount.arguments[0]!=0x802C4760+24*(c.model==6?26:27)||
           amount.arguments[1]!=blast||amount.arguments[2]!=body||pair.gpr[3]!=body||
           contact::half(writer_ram,meter)!=rule.meter||contact::half(writer_ram,gauge)!=rule.gauge||
           contact::at(writer_ram,tier)[0]!=rule.tier||contact::half(writer_ram,body+0x1C6)!=180)
            fail("tier probe touched meter or gauge before the HP writer");
        const unsigned request=pair.gpr[4];
        call(contact::pair_pass,0);
        const unsigned hp=half(body+0x1C6),post_gauge=half(gauge),post_meter=half(meter),post_tier=at(tier)[0];
        const unsigned post_flags=at(flags)[0];
        if(word(gauge_timer)!=bits(60.f)||word(tier_timer)!=bits(60.f)||half(body+0x20C)!=2||
           hp!=180-(rule.hp_held?0:request)||word(body+0x41C)!=(rule.hp_held?0:request))
            fail("tier probe did not finish the original surviving HP tail");
        const auto post=ram;
        const auto tick=[&](){call(0x8005568C,body);};
        put_float(at(body+0x1DCC),0.5f);put_float(at(body+0x1DD0),0.75f);
        put32(at(body+0x5E0),word(body+0x5E0)|0x01000000u);
        tick();
        if(word(gauge_timer)!=bits(60.f)||word(tier_timer)!=bits(60.f))fail("tier clock ignored its actor phase gate");
        put32(at(body+0x5E0),word(body+0x5E0)&~0x01000000u);
        at(body+0x71C)[0]=4;
        call(0x8005A378,body);
        if(word(body+0x1DC8)!=bits(0.03f))fail("tier clock lost the declared contact hold");
        tick();
        if(word(gauge_timer)!=bits(59.5f)||word(tier_timer)!=bits(59.5f)||word(body+0x1DC8)!=bits(0.03f))
            fail("tier clocks did not use the raw lane factor during the hold");
        for(unsigned n=2;n<=119;++n)tick();
        if(word(gauge_timer)!=bits(0.5f)||word(tier_timer)!=bits(0.5f)||half(gauge)!=post_gauge||
           half(meter)!=post_meter||at(tier)[0]!=post_tier)
            fail("tier clock expired before its half-step threshold");
        tick();
        if(word(gauge_timer)||word(tier_timer)||half(gauge)!=maximum||half(meter)||at(tier)[0]||
           half(body+0x1C6)!=hp||half(actor+0x774)!=capacity||half(body+0x20C)!=2)
            fail("tier expiry changed HP, stock, or accepted history");
        for(unsigned fork=0;fork<2;++fork){
            std::memcpy(ram.data(),post.data(),post.size());
            put32(at(body+0x5E0),word(body+0x5E0)&~0x01000000u);
            put_float(at(body+0x1DCC),0.5f);
            put_float(at(gauge_timer),fork?2.f:0.5f);put_float(at(tier_timer),fork?0.5f:2.f);
            tick();
            if(word(gauge_timer)!=bits(fork?1.5f:0.f)||word(tier_timer)!=bits(fork?0.f:1.5f)||
               half(gauge)!=(fork?post_gauge:maximum)||half(meter)!=(fork?0:post_meter)||
               at(tier)[0]!=(fork?0:post_tier))
                fail("tier probe merged the independent gauge and tier expiry");
        }
        std::printf("tier-contact: %u %u %u %u %u %u %u %u\n",
                    c.model,probe.index,request,hp,post_gauge,post_meter,post_tier,post_flags);
    }
    void follow_body_exit(Options c,unsigned index,unsigned capacity,const std::vector<u8>& before_pair,bool cleanup,
                          const cleanup_probe::Delivery* delivery){
        constexpr u32 victim=contact::receiver,s=contact::scene,seat=victim-0x1E00,actor_array=0x803C4E84;
        constexpr u32 walk=0x8002BB14,body_update=0x80054070,presentation=0x8005A580;
        constexpr u32 begin=0x8005BBC0,reaction=0x8005C290,exit_lane=0x80065E44;
        constexpr unsigned mask=2;
        const bool retained=index==3,held=index==2,excluded=index==1,entered=index<2;
        if(!retained)std::memcpy(ram.data(),before_pair.data(),ram.size());
        for(unsigned i=0;i<6;++i)put32(at(actor_array+4*i),seat+i*0x1E00);
        put32(at(0x803C4D94+4),word(victim+0x4AC));
        at(victim+0x3E7)[0]=1;at(victim+0x3E5)[0]=mask;
        call(0x8006F4F8,victim);
        if(word(victim+0x4AC)!=0x81015400||word(victim+0x4B0)!=0x802D3EE0||word(victim+0x4F0)!=0x802D3FF8||
           word(victim+0x4CC)!=0x8006F954||half(victim+0x1C6)!=(retained?0:7)||at(s+31)[0]!=unsigned(held))
            fail("body exit requires the bound Sasuke class and unchanged HP at the walk inlet");
        if(!retained)put16(at(victim+0x1C6),index==4?180:1);
        constexpr unsigned force_limit=200;
        put32(at(s+60+0x114),force_limit);
        at(s+0xEA)[0]=3;at(s+0xD2)[0]=excluded?mask:0;
        at(s+69)[0]=5; // Declared no-voice resource phase; it is not physical playback.
        at(s+0x1829)[0]=1; // No separate pad accumulation in the later body update.
        put_float(at(victim+0xB8),1.f);
        for(unsigned axis=0;axis<3;++axis)
            put32(at(victim+0x2C+4*axis),word(victim+0x20+4*axis));
        if(!retained){
            auto fork=ram;
            CPUState amount{};amount.gpr[3]=0;
            mod.run_to(amount,fork,walk,{contact::amount_entry});
            const auto entry=amount.gpr[3];
            if(entry!=(c.model==6?0x802C49D0u:0x802C49E8u)||amount.gpr[4]!=blast||
               amount.gpr[5]!=victim||amount.lr!=0x8002F10C)
                fail("body exit reached the wrong original amount route");
            constexpr u32 hp_mirror=0x803B069E,force_mirror=0x803B0690;
            const auto hp=contact::half(fork,hp_mirror);
            const auto force=be32(contact::at(fork,force_mirror));
            if(hp!=(index==4?180:1)||force!=140||force>force_limit)
                fail("body exit collision walk did not refresh the declared receiving mirrors");
            if(!cleanup)std::printf("body-exit-amount: %u %u %u %u",c.model,index,hp,force);
            for(unsigned n=0;n<4;++n){
                put16(contact::at(fork,hp_mirror),u16(n&1?180:hp));
                put32(contact::at(fork,force_mirror),n&2?force_limit:force);
                CPUState isolated{};isolated.gpr[3]=entry;isolated.gpr[4]=blast;isolated.gpr[5]=victim;
                mod.run(isolated,fork,contact::amount_entry);
                if(isolated.gpr[3]>20)fail("body exit calculator exceeded its scoped area amount");
                if(!cleanup)std::printf(" %u",isolated.gpr[3]);
                else if(isolated.gpr[3]!=(n?17:16))fail("cleanup no longer follows the qualified lethal amount");
            }
            if(!cleanup)std::putchar('\n');
        }
        CPUState cpu{};CallObservation init{begin};
        const auto cut=mod.run_to(cpu,ram,walk,{presentation,reaction},&init);
        const auto root=word(victim+0x20);
        if(!cleanup)std::printf("body-exit: %u %u %u %u %u %u %u %u %u %u %u %u %08X %08X %u %u\n",
                    c.model,index,init.count,at(victim+0x18)[0],half(victim+0x1C6),word(victim+0x41C),
                    half(victim+0x20C),at(victim+0x1DA)[0],at(s+0xEA)[0],at(victim+0x5DB)[0],
                    at(victim+0x540)[0],at(victim+0x543)[0],word(victim+0x558),cpu.lr,
                    at(victim+0x435)[0],word(s+60+0x118));
        if(cpu.gpr[3]!=victim||init.count!=unsigned(entered)||(entered&&(init.caller!=0x8002C6DC||
           init.arguments[0]!=victim||cut!=presentation||cpu.lr!=0x8002C870||
           at(victim+0x18)[0]!=2||at(victim+0x5DB)[0]!=63||half(victim+0x272)!=0x443||
           word(s+0xB4)!=actor||word(s+0xB8)!=victim))||
           (!entered&&at(victim+0x18)[0]!=1)||half(actor+0x774)!=capacity)
            fail("original collision walk changed the body exit boundary");
        if(!entered)return;
        if(!cleanup){
            auto fork=ram;
            for(auto entry:{0x8008A28Cu,0x80089AECu}){
                CPUState lists{};
                mod.run(lists,fork,entry,4096);
            }
            std::printf("body-exit-list: %u %u %u %u\n",c.model,index,
                        contact::half(fork,contact::receiver_count),contact::at(fork,contact::body)[0]);
        }
        empty_stage_grid();
        const auto force=word(s+60+0x118),damage=word(victim+0x41C);
        if(excluded){
            for(unsigned visit=1;visit<=2;++visit){
                CPUState masked{};masked.gpr[3]=victim;
                CallObservation lane{exit_lane};
                mod.run_to(masked,ram,body_update,{presentation},&lane);
                if(masked.lr!=0x80054144||masked.gpr[3]!=victim||lane.count||
                   at(victim+0x18)[0]!=2||at(victim+0x540)[0]||at(victim+0x543)[0]||word(victim+0x558)!=0x41F00000||
                   half(victim+0x1C6)||word(victim+0x41C)!=damage||word(s+60+0x118)!=force)
                    fail("declared scene mask advanced the phase-2 lane");
            }
            at(s+0xD2)[0]=0;
        }
        unsigned prior=at(victim+0x540)[0];
        for(unsigned visit=1;visit<=96;++visit){
            CPUState frame{};frame.gpr[3]=victim;
            const bool effect=prior==1;
            CallObservation lane{effect?0x800C1F70u:exit_lane};
            const auto boundary=mod.run_to(frame,ram,body_update,{presentation},&lane);
            const unsigned now=at(victim+0x540)[0],phase=at(victim+0x18)[0];
            if(!cleanup&&(visit<=2||prior!=now||phase==3))
                std::printf("body-exit-visit: %u %u %u %u %u %u %u %08X %u %u %u\n",c.model,index,visit,prior,now,phase,
                            lane.count,word(victim+0x558),at(victim+0x1D8)[0],at(victim)[0],unsigned(effect));
            if(boundary!=presentation||frame.lr!=0x80054144||frame.gpr[3]!=victim||
               lane.count!=1||lane.caller!=(effect?0x8005BF00:0x800555F0)||lane.arguments[0]!=victim||
               (effect&&(lane.arguments[1]||half(contact::free_count+4)))||
               half(victim+0x1C6)||word(victim+0x41C)!=damage||word(s+60+0x118)!=force||
               at(victim+0x435)[0]!=1||half(victim+0x20C)!=2||word(victim+0x20)!=root||half(actor+0x774)!=capacity||
               word(s+0xB4)!=actor||word(s+0xB8)!=victim)
                fail("isolated actor visit crossed knockout accounting or world frontier");
            if(phase==3){
                if(cleanup){
                    if(visit!=47||!at(victim+0x1D8)[0])fail("cleanup changed the qualified phase-3 inlet");
                    for(unsigned kind=0;kind<(delivery?1u:3u);++kind)
                        cleanup_probe::Probe(mod,ram,c.model,kind,delivery);
                }
                return;
            }
            prior=now;
        }
        fail("normal knockout body did not reach phase 3 within the visit budget");
    }
    void knockout_contact(Options c,const AreaProbe& probe,unsigned capacity,u32 parent){
        isolate_area(capacity,parent);
        const auto rule=knockout_cases[probe.mode==AreaMode::body_exit||probe.mode==AreaMode::body_cleanup?
                                      exit_knockout_case[probe.index]:probe.index];
        constexpr u32 victim=contact::receiver,s=contact::scene,observer=actor+0x1E00;
        constexpr u32 requests=0x81050000,request_count=requests+0x2D18,request_slot=requests+0x1518;
        constexpr u32 mode=0x8043B5A0-31856;
        constexpr u32 credit_bonus=0x8043B5A0-21580,opponent_ledger=actor+0x418,ally_ledger=actor+0x42C;
        constexpr unsigned cost=150;
        const u32 slot=s+(rule.ally?0:60),force=slot+0x118,reserves=slot+0x10A;
        const u32 other_force=s+(rule.ally?60:0)+0x118;
        if(at(victim+0x88)[0]!=unsigned(!rule.ally)||at(victim+0x18)[0]!=1||
           at(actor+0x18)[0]!=1||at(victim+0x83)[0]||at(actor+0x83)[0]||word(victim+0x5A4)||
           at(victim+0x3E6)[0]||at(word(mode)+50)[0])
            fail("knockout needs two distinct normal bodies");
        call(0x80055F90,victim);
        if(half(victim+0x4AA)!=cost||half(victim+0x1C4)!=180||half(victim+0x1C6)!=180)
            fail("knockout normal HP or cached force cost changed");
        put16(at(victim+0x3EA),contact::sasuke.model);
        put32(at(0x8043B5A0-21068),requests); // Empty mode-0 model request buffer.
        put16(at(victim+0x1C6),rule.hp); // Leave max HP and the amount mirrors unchanged.
        put16(at(victim+0x6BE),500);put16(at(victim+0x6C2),500);
        put32(at(force),rule.force);
        const auto untouched_force=word(other_force);
        at(slot+0x108)[0]=u8(rule.debit);at(slot+0x109)[0]=u8(rule.reserve);at(reserves)[0]=u8(rule.reserves);
        at(victim+0x1EA)[0]=u8(rule.eligible);at(s+31)[0]=u8(rule.held);
        at(observer)[0]=1;at(observer+0x88)[0]=u8(!rule.ally);at(observer+0x484)[0]=7;
        const auto snapshot=ram;
        const auto expected_request=rule.ally?4u:17u;
        const auto lethal=!rule.held&&rule.hp<=expected_request;
        const auto credited=lethal&&!rule.ally;
        const auto removed=rule.held?0u:std::min(unsigned(rule.hp),expected_request);
        const auto damage=rule.held?0u:expected_request;
        const auto after_force=lethal&&rule.debit?unsigned(std::max(0,int(rule.force)-int(cost))):rule.force;
        const auto used_reserve=lethal&&rule.reserve&&rule.eligible&&rule.reserves>0;
        const auto scene_pair=lethal&&(used_reserve||after_force==0);
        const auto amount_row=c.model==6?0x802C49D0u:0x802C49E8u;
        {
            auto fork=snapshot;
            CPUState writer{};CallObservation amount{contact::amount_entry};
            mod.run_to(writer,fork,contact::pair_pass,{contact::hp_write},&amount);
            if(amount.count!=1||amount.caller!=0x8002F108||amount.arguments[0]!=amount_row||
               amount.arguments[1]!=blast||amount.arguments[2]!=victim||writer.lr!=0x8002F1DC||
               writer.gpr[3]!=victim||writer.gpr[4]!=expected_request||contact::half(fork,victim+0x1C6)!=rule.hp||
               be32(contact::at(fork,force))!=rule.force||be32(contact::at(fork,victim+0x41C))!=damage)
                fail("knockout HP inlet did not separate the original request, ledger and force");
            mod.finish_stopped_leaf(writer,contact::hp_write,164);
            if(writer.gpr[3]!=unsigned(lethal)||contact::half(fork,victim+0x1C6)!=rule.hp-removed||
               contact::half(fork,victim+0x1C8)!=(rule.held?180:rule.hp)||
               be32(contact::at(fork,force))!=rule.force||be32(contact::at(fork,s+0xB8)))
                fail("knockout HP writer did more than the bounded health write");
        }
        call(contact::pair_pass,0);
        const auto check=[&](){
            const auto encoded=~word(request_slot);
            return half(victim+0x1C6)==rule.hp-removed&&word(victim+0x41C)==damage&&
                half(victim+0x1C8)==(rule.held?180:rule.hp)&&half(victim+0x4AA)==cost&&
                word(opponent_ledger)==(rule.ally?0:damage)&&word(ally_ledger)==(rule.ally?damage:0)&&
                word(force)==after_force&&word(other_force)==untouched_force&&
                at(reserves)[0]==unsigned(rule.reserves-used_reserve)&&
                word(s+0xB4)==(scene_pair?actor:0)&&word(s+0xB8)==(scene_pair?victim:0)&&
                at(victim+0x435)[0]==unsigned(lethal)&&word(victim+0x424)==(lethal?cost:0)&&
                at(actor+0x434)[0]==unsigned(credited)&&word(actor+0x420)==(credited?cost:0)&&
                half(credit_bonus)==(credited?150:50)&&word(victim+0x5A4)==(credited?actor:0)&&
                at(observer+0x484)[0]==(lethal?0:7)&&half(observer+0x464+7*4)==(lethal?4:0)&&
                half(observer+0x466+7*4)==unsigned(lethal)&&word(observer+0x430)==(lethal?cost:0)&&
                at(observer+0x437)[0]==unsigned(lethal)&&
                at(actor+0x484)[0]==1+unsigned(lethal)&&half(actor+0x464)==(rule.ally?2:1)&&
                half(actor+0x466)==1&&half(actor+0x468)==(lethal?(rule.ally?4:3):0)&&
                half(actor+0x46A)==unsigned(lethal)&&
                word(actor+0x430)==(lethal&&rule.ally?cost:0)&&at(actor+0x437)[0]==unsigned(lethal&&rule.ally)&&
                word(request_count)==unsigned(credited)&&
                (credited?(encoded>>16)==contact::sasuke.model&&(encoded&0xFFFF)>=1&&(encoded&0xFFFF)<=16:
                    word(request_slot)==0)&&
                at(victim+0x18)[0]==1&&at(victim)[0]&&half(victim+0x20C)==2&&half(actor+0x774)==capacity&&
                half(victim+0x6C8)==(lethal?0:50)&&half(victim+0x6C2)==(lethal?500:400);
        };
        if(!check()){
            std::fprintf(stderr,"ko: model=%u case=%u req=%u hp=%u/%u ledger=%u,%u,%u "
                "force=%u reserve=%u scene=%08X/%08X count=%u/%u costs=%u/%u bonus=%u last=%08X phase=%u "
                "observer=%u/%u/%u/%u actor_history=%u/%u token=%u tier=%u/%u queue=%u/%08X\n",c.model,probe.index,
                expected_request,half(victim+0x1C6),rule.hp-removed,word(opponent_ledger),word(ally_ledger),
                word(victim+0x41C),word(force),
                at(reserves)[0],word(s+0xB4),word(s+0xB8),at(actor+0x434)[0],at(victim+0x435)[0],word(actor+0x420),
                word(victim+0x424),half(credit_bonus),word(victim+0x5A4),at(victim+0x18)[0],at(observer+0x484)[0],
                half(observer+0x464+7*4),half(observer+0x466+7*4),word(observer+0x430),at(actor+0x484)[0],
                half(actor+0x464),half(victim+0x20C),half(victim+0x6C8),half(victim+0x6C2),word(request_count),
                word(request_slot));
            fail("knockout pair did not follow its original force, attribution or survival branch");
        }
        if(observe(contact::pair_pass,0,contact::hp_write).count||!check())
            fail("knockout retained token wrote again");
        if(probe.mode==AreaMode::body_exit||probe.mode==AreaMode::body_cleanup){
            follow_body_exit(c,probe.index,capacity,snapshot,probe.mode==AreaMode::body_cleanup,probe.delivery);
            return;
        }
        std::printf("body-knockout: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                    c.model,probe.index,expected_request,half(victim+0x1C6),word(victim+0x41C),word(force),at(reserves)[0],
                    unsigned(word(s+0xB8)!=0),at(victim+0x435)[0],word(victim+0x424),at(actor+0x434)[0],word(actor+0x420),
                    half(credit_bonus),unsigned(word(victim+0x5A4)!=0),at(victim+0x18)[0],half(victim+0x6C8),
                    half(observer+0x464+7*4),word(request_count),half(actor+0x468));
    }
    void follow_area(Options c,const AreaProbe& probe,unsigned capacity,u32 parent){
        const auto rule=area_cases[probe.index];
        const auto real=[&](u32 a){return std::bit_cast<float>(word(a));};
        constexpr u32 shape=shapes[1],receiver=contact::receiver,attack_count=0x8043B5A0-21328;
        const unsigned group=c.model==6?179:180,descriptor=246+unsigned(c.model==7),amount_index=26+unsigned(c.model==7);
        const u32 amount_row=0x802C4760+24*amount_index;
        unsigned first_publish=0,hit=0,lost=0,token=0,parent_retired=0,shape_retired=0,area_retired=0,requests=0,hit_stock=0;
        unsigned opponent_ledger=0,ally_ledger=0,receiver_ledger=0;
        float peak_radius=0,hit_radius=0,opening_radius=0;
        if(at(blast+0x18)[0]!=(rule.shape?1:2))
            fail("type-41 area initializer did not run during the original terminal bucket");
        for(unsigned n=1;n<=64;++n){
            // Visit 1 is the bucket that already requested and initialized
            // the child; visit 2 consumes the parent's bound return channel.
            if(n>1){
                const auto grant=observe(0x80089A94,8,return_stock);
                if(n==2)check_return(grant);
                else if(grant.count)fail("type-41 area returned stock again after its parent");
            }
            if(half(actor+0x774)!=capacity-unsigned(n==1)||word(actor+0x778)||half(side+0x774)!=9)
                fail("type-41 area changed either actor's stock");
            call(0x80089554,0);
            rebuild();
            const bool has_shape=at(shape)[0]!=0;
            const bool published=half(attack_count)!=0;
            if(half(attack_count)>1||half(contact::receiver_count)!=unsigned(!rule.masked)||
               (published&&(!has_shape||word(0x803C477C)!=shape))||
               (n==1&&has_shape!=rule.shape)){
                std::fprintf(stderr,"type41-area %u/%u n=%u atk=%u/%08X recv=%u active shape=%u child phase=%u parent phase=%u free=%u\n",
                             c.model,probe.index,n,half(attack_count),word(0x803C477C),half(contact::receiver_count),
                             unsigned(has_shape),at(blast+0x18)[0],at(parent+0x18)[0],half(contact::free_count+8));
                fail("type-41 area published the wrong role or shape");
            }
            if(has_shape){
                const float r=real(shape+0x7C);
                if(word(shape+0x20)!=blast||word(shape+0x24)!=actor||word(shape+0x2C)!=0x802C4760||
                   word(shape+0x28)!=common+0x400+descriptor*80||at(shape+0x11)[0]!=amount_index||
                   at(shape+0x14)[0]!=group||real(shape+0x64)!=200.f||real(shape+0x68)!=100.f||
                   real(shape+0x6C)!=300.f||half(blast+0x26C)!=2){
                    std::fprintf(stderr,"type41-area %u/%u n=%u ptrs=%08X/%08X/%08X/%08X group=%u record=%u center=%g/%g/%g token=%u parent=%u\n",
                                 c.model,probe.index,n,word(shape+0x20),word(shape+0x24),word(shape+0x28),word(shape+0x2C),
                                 at(shape+0x14)[0],at(shape+0x11)[0],double(real(shape+0x64)),double(real(shape+0x68)),
                                 double(real(shape+0x6C)),half(blast+0x26C),half(parent+0x26C));
                    std::fprintf(stderr,"type41-area root=%g/%g/%g matrix=%g/%g/%g parent root=%g/%g/%g radius=%g\n",
                                 double(real(blast+0x20)),double(real(blast+0x24)),double(real(blast+0x28)),
                                 double(real(blast+0x118)),double(real(blast+0x128)),double(real(blast+0x138)),
                                 double(real(parent+0x20)),double(real(parent+0x24)),double(real(parent+0x28)),double(r));
                    fail("type-41 area lost its original common row, fresh token, or origin");
                }
                token=half(blast+0x26C);
                if(n==1)opening_radius=r;
                if(published&&!first_publish)first_publish=n;
                peak_radius=std::max(peak_radius,r);
            }else if(token&&!shape_retired)shape_retired=n;

            CPUState pair{};CallObservation amount{contact::amount_entry};
            const auto cut=mod.run_to_or_return(pair,ram,contact::pair_pass,{contact::hp_write},&amount,4096);
            if(amount.count!=unsigned(cut!=0)||(cut&&(amount.caller!=0x8002F108||amount.arguments[0]!=amount_row||
               amount.arguments[1]!=blast||amount.arguments[2]!=receiver)))
                fail("type-41 HP attempt did not originate at the child");
            if(cut){
                if(!published||hit||rule.masked||pair.lr!=0x8002F1DC||pair.gpr[3]!=receiver||
                   pair.gpr[4]>180||!amount.count||half(receiver+0x1C6)!=180)
                    fail("type-41 area repeated a token or used the wrong HP recipient");
                hit=n;requests=pair.gpr[4];hit_radius=real(shape+0x7C);hit_stock=half(actor+0x774);
                mod.finish_stopped_leaf(pair,contact::hp_write,164);
                lost=180-half(receiver+0x1C6);
                opponent_ledger=word(actor+0x418);ally_ledger=word(actor+0x42C);receiver_ledger=word(receiver+0x41C);
                if(lost!=requests||word(receiver+0x290)!=blast||word(blast+0x290)!=receiver||
                   half(receiver+0x20C)!=token||at(blast+0x1D9)[0]!=16)
                    fail("type-41 child HP, contact history or effect gate changed");
            }
            if(half(receiver+0x1C6)!=180-lost||half(actor+0x1C6)!=220||
               word(actor+0x418)!=opponent_ledger||word(actor+0x42C)!=ally_ledger||word(receiver+0x41C)!=receiver_ledger||
               (at(parent)[0]&&word(parent+0x290))||
               (at(blast)[0]&&(word(blast+0x8C)!=parent||word(blast+0x90)!=actor)))
                fail("type-41 retained parent/child did not preserve stock and credited HP separation");
            if(!at(parent)[0]&&!parent_retired)parent_retired=n;
            if(!at(blast)[0]&&!area_retired)area_retired=n;
            if(parent_retired&&area_retired)break;
        }
        if(!parent_retired||!area_retired||word(head+9*4)||half(contact::free_count+6)!=2||
           half(pool+18)!=3||half(contact::free_count+8)!=1+unsigned(rule.shape)||half(attack_count))
            fail("type-41 area lifecycle did not restitute its own resources");
        std::printf("type41-area-hp: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %08X %08X %08X\n",
                    c.model,probe.index,capacity,first_publish,hit,token,requests,lost,hit_stock,opponent_ledger,ally_ledger,
                    receiver_ledger,parent_retired,shape_retired,area_retired,
                    std::bit_cast<u32>(opening_radius),std::bit_cast<u32>(hit_radius),std::bit_cast<u32>(peak_radius));
    }
    void parent_contact(Options c,const ParentProbe& probe,unsigned capacity,u32 parent){
        const auto rule=parent_cases[probe.index];
        constexpr u32 receiver=contact::receiver,attack_list=0x803C477C,attack_count=0x8043B5A0-21328;
        const auto real=[&](u32 a){return std::bit_cast<float>(word(a));};
        const auto contains=[&](u32 list,unsigned count,u32 shape){
            for(unsigned i=0;i<count;++i)if(word(list+4*i)==shape)return true;
            return false;
        };
        place(parent+0x20,200.f);
        call(0x80047AA4,parent);
        if(rule.incoming){
            constexpr u32 point=0x8103D000,make=0x800FC6A0,init=0x800FC7C0;
            empty_stage_grid();
            place(point,200.f+rule.offset);
            put16(at(receiver+0x94),0x000A);
            put32(at(visual_rows+4*68),empty);
            CPUState create{};create.gpr[3]=receiver;create.gpr[4]=point;create.gpr[5]=4;
            mod.run(create,ram,make,4096);
            if(create.gpr[3]!=incoming_cell||at(incoming_cell+0x10)[0]!=9||at(incoming_cell+0x11)[0]!=4||
               word(incoming_cell+0x90)!=receiver||word(incoming_cell+0x8C)!=receiver)
                fail("type-41 incoming test did not construct its separate key-4 projectile");
            call(init,incoming_cell);call(0x80047AA4,incoming_cell);
        }
        rebuild();
        // Hold the declared root for a second publication: the original
        // moving sphere caches the previous endpoint on the first visit.
        rebuild();
        if(half(attack_count)!=1+unsigned(rule.incoming)||half(contact::receiver_count)!=2||
           !contains(attack_list,half(attack_count),shapes[0])||
           !contains(contact::receiver_registry,2,shapes[0])||
           !contains(contact::receiver_registry,2,contact::body)||
           real(shapes[0]+0x64)!=200.f||real(shapes[0]+0x68)!=100.f||real(shapes[0]+0x6C)!=300.f||
           real(shapes[0]+0x7C)!=20.f||half(parent+0x26C)!=1||half(parent+0x1C6)!=1||
           word(parent+0x290)||at(parent+0x1D9)[0]||at(parent+0x1DA)[0]||at(receiver+0x6CA)[0]||
           (rule.incoming&&(!contains(attack_list,2,shapes[1])||word(shapes[1]+0x20)!=incoming_cell||
                            word(shapes[1]+0x24)!=receiver||real(shapes[1]+0x7C)!=150.f||
                            real(shapes[1]+0x64)!=200.f+rule.offset||half(incoming_cell+0x26C)!=2))){
            std::fprintf(stderr,"type41 parent roles: %u/%u atk=%u %08X/%08X recv=%u %08X/%08X pos=%g/%g/%g radius=%g token=%u hp=%u flags=%u/%u\n",
                         c.model,probe.index,half(attack_count),word(attack_list),word(attack_list+4),
                         half(contact::receiver_count),word(contact::receiver_registry),word(contact::receiver_registry+4),
                         double(real(shapes[0]+0x64)),double(real(shapes[0]+0x68)),double(real(shapes[0]+0x6C)),
                         double(real(shapes[0]+0x7C)),half(parent+0x26C),half(parent+0x1C6),at(parent+0x1D9)[0],at(parent+0x1DA)[0]);
            fail("type-41 parent did not publish both original roles with its initial body HP");
        }
        const bool hit=rule.offset<=(rule.incoming?170.f:42.f);
        const u32 attacker=rule.incoming?incoming_cell:parent,recipient=rule.incoming?parent:receiver;
        const u32 record=rule.incoming?0x802C4838:0x80336340+24*(c.model==6?9:10);
        const bool follow_child=hit&&!rule.incoming;
        const auto original_pair=follow_child?ram:std::vector<u8>{};
        CPUState pair{};CallObservation amount{contact::amount_entry};
        const auto cut=mod.run_to_or_return(pair,ram,contact::pair_pass,{contact::hp_write},&amount,4096);
        if(cut!=(hit?contact::hp_write:0)||amount.count!=unsigned(hit)||
           (hit&&(amount.caller!=0x8002F108||amount.arguments[0]!=record||amount.arguments[1]!=attacker||
                 amount.arguments[2]!=recipient||pair.lr!=0x8002F1DC||pair.gpr[3]!=recipient||
                 half(recipient+0x1C6)!=(rule.incoming?1:180))))
            fail("type-41 parent contact reached an unrelated amount or HP path");
        const unsigned requested=hit?pair.gpr[4]:0;
        if(hit)mod.finish_stopped_leaf(pair,contact::hp_write,164);
        const auto actor_hp=half(receiver+0x1C6),carrier_hp=half(parent+0x1C6);
        const auto incoming_latch=at(parent+0x1DA)[0],outgoing_latch=at(parent+0x1D9)[0];
        const auto ledger=word(actor+0x418),allied=word(actor+0x42C),received=word(receiver+0x41C);
        const auto token=half(recipient+0x20C);
        if((hit&&(word(recipient+0x290)!=attacker||word(attacker+0x290)!=recipient||token!=half(attacker+0x26C)))||
           (!hit&&token)||carrier_hp!=unsigned(!hit||!rule.incoming)||
           actor_hp!=180-(rule.incoming?0:requested)||half(actor+0x1C6)!=220||half(actor+0x774)!=capacity-1||
           word(receiver+0x418)||word(parent+0x41C)||
           (rule.incoming?(ledger||allied||received||outgoing_latch):(incoming_latch||received!=requested)))
            fail("type-41 parent contact changed the wrong HP, clock or history");
        if(follow_child){
            std::memcpy(ram.data(),original_pair.data(),original_pair.size());
            // The retained branch completes its original post-HP tail.
            // No fixed-bucket visual cell or playback is offered.
            call(contact::pair_pass,0);
            if(half(receiver+0x1C6)!=actor_hp||half(parent+0x1C6)!=carrier_hp||
               half(receiver+0x20C)!=token||at(parent+0x1D9)[0]!=outgoing_latch||at(receiver+0x6CA)[0]!=1||
               word(actor+0x418)!=ledger||word(actor+0x42C)!=allied||word(receiver+0x41C)!=received)
                fail("type-41 completed parent pair did not preserve the observed HP cut");
        }
        if(hit){
            CPUState again{};
            if(mod.run_to_or_return(again,ram,contact::pair_pass,{contact::hp_write},nullptr,4096))
                fail("type-41 parent accepted the same contact token twice");
        }
        u32 area_site=0;
        unsigned child_token=0,stock_after_exit=capacity-1,final_stock=capacity-1,child_hp=0,final_hp=actor_hp;
        unsigned tier=0,clear_tier_amount=0;
        if(hit){
            if(rule.command){
                put32(at(actor+0x5BC),0x400);call(owner_hook);
                if(at(actor+0x6F7)[0]!=1)fail("type-41 priority test did not arm the exact remote command");
            }
            // Visit only the parent's own bucket. The incoming source is
            // in its credited actor's distinct bucket and is not advanced.
            const auto child=observe(0x80089A94,9,area);
            area_site=rule.command?0x800E0AAC:rule.incoming?0x800E0B18:0x800E0AD4;
            stock_after_exit=half(actor+0x774);
            if(child.count!=1||child.caller!=area_site||child.arguments[0]!=parent||child.arguments[1]!=parent+0x20||
               child.arguments[2]!=(c.model==6?19:20)||stock_after_exit!=capacity-1||
               at(actor+0x6F7)[0]||at(parent+0x18)[0]!=2||!at(blast)[0]||word(blast+0x90)!=actor)
                fail("type-41 parent overlap went to the wrong terminal area or paid early");
            call(0x80089554,0);rebuild();
            child_token=half(blast+0x26C);
            if(!child_token||child_token==1||(rule.incoming&&child_token==2))
                fail("type-41 child did not stage an independent token after its parent");
            check_return(observe(0x80089A94,9,return_stock));
            final_stock=half(actor+0x774);
            if(final_stock!=capacity||half(parent+0x1C6)!=carrier_hp||half(receiver+0x1C6)!=actor_hp)
                fail("type-41 terminal stock return changed the settled contact HP");
            if(follow_child){
                call(0x8002CDA4,receiver); // original per-pass reset, not a history clear
                if(half(receiver+0x20C)!=token||at(receiver+0x26E)[0]!=1||at(receiver+0x1DA)[0])
                    fail("type-41 child contact did not retain the parent's token");
                call(0x80089554,0);rebuild();
                CPUState child_pair{};CallObservation child_amount{contact::amount_entry};
                mod.run_to(child_pair,ram,contact::pair_pass,{contact::hp_write},&child_amount);
                if(child_amount.count!=1||child_amount.caller!=0x8002F108||
                   child_amount.arguments[0]!=0x802C4760+24*(c.model==6?26:27)||
                   child_amount.arguments[1]!=blast||child_amount.arguments[2]!=receiver||
                   child_pair.lr!=0x8002F1DC||child_pair.gpr[3]!=receiver||half(receiver+0x1C6)!=actor_hp)
                    fail("type-41 second hit used the parent's amount or recipient");
                child_hp=child_pair.gpr[4];
                tier=at(receiver+0x6CA)[0];
                auto control=ram;
                const auto calc=[&](){
                    CPUState a{};a.gpr[3]=0x802C4760+24*(c.model==6?26:27);a.gpr[4]=blast;a.gpr[5]=receiver;
                    mod.run(a,control,contact::amount_entry,4096);
                    return a.gpr[3];
                };
                if(tier!=1||calc()!=child_hp)fail("type-41 child damage tier or amount boundary changed");
                contact::at(control,receiver+0x6CA)[0]=0;
                clear_tier_amount=calc();
                mod.finish_stopped_leaf(child_pair,contact::hp_write,164);
                const auto owner_ledger=rule.ally?actor+0x42C:actor+0x418;
                final_hp=half(receiver+0x1C6);
                if(final_hp!=actor_hp-child_hp||half(receiver+0x20C)!=token||half(receiver+0x20E)!=child_token||
                   at(receiver+0x26E)[0]!=2||half(actor+0x774)!=capacity||word(owner_ledger)!=requested+child_hp||
                   word(receiver+0x41C)!=requested+child_hp)
                    fail("type-41 parent and child lost separate history or cumulative ledgers");
                CPUState duplicate{};
                if(mod.run_to_or_return(duplicate,ram,contact::pair_pass,{contact::hp_write},nullptr,4096))
                    fail("type-41 second token repeated its HP write");
            }
        }
        std::printf("type41-parent: %u %u %u %u %u %u %u %u %u %u %u %u %08X %u %u %u %u %u %u %u\n",
                    c.model,probe.index,requested,carrier_hp,actor_hp,outgoing_latch,incoming_latch,token,ledger,
                    allied,received,unsigned(hit),area_site,stock_after_exit,final_stock,child_token,child_hp,final_hp,
                    tier,clear_tier_amount);
    }
public:
    Check(const Module& module,std::vector<u8> data,Options c,const contact::ActorSpec& spec,
          const contact::HitBlock& hit,const Common& bank_data,unsigned index,const AreaProbe* probe=nullptr,
          const ParentProbe* parent_probe=nullptr):mod(module),ram(std::move(data)){
        std::memcpy(at(actor_spec),spec.data(),spec.size());
        put32(at(0x803C4D94),actor_spec);
        put32(at(0x8043B5A0-31852),contact::scene);
        put16(at(actor+0x3E8),0x0100+c.model);
        call(profile);
        if(word(actor+0x4AC)!=actor_spec||word(actor+0x27C)!=0x80336340||word(actor+0x4C0)!=owner_hook)
            fail("type-41 normal installer did not select the reviewed owner");
        CPUState start{};start.gpr[3]=actor;
        mod.run_to(start,ram,0x8005614C,{0x8013BD80,0x80006238});
        const unsigned capacity=c.model==6?10:8;
        if(half(actor+0x774)!=capacity||half(actor+0x78E)!=capacity||half(actor+0x790)!=2||word(actor+0x778))
            fail("type-41 needs exact normal level-1 stock");
        put32(at(actor+0x90),actor);put16(at(actor+0x94),0x0100+c.model);
        at(actor)[0]=1;at(actor+0x18)[0]=1;
        put16(at(actor+0x1C4),220);put16(at(actor+0x1C6),220);
        put16(at(side+0x774),9);put32(at(actor+0xCC),side);
        for(unsigned i=0;i<6;++i)put32(at(0x803C4E84+4*i),i?side:actor);
        for(unsigned off:{0x58u,0x5Cu,0x60u,0xC0u,0xC4u,0x1DC8u})put_float(at(actor+off),1.f);
        constexpr u32 joint=actor+0x8D4+48;
        for(unsigned i=0;i<3;++i)put_float(at(joint+20*i),1.f);
        put_float(at(joint+12),200.f);
        put_float(at(joint+28),100.f);put_float(at(joint+44),300.f);
        put32(at(actor+0x3FC),hit_block);std::memcpy(at(hit_block),hit.data(),hit.size());
        put32(at(contact::scene+152),common);std::memcpy(at(common),bank_data.data(),bank_data.size());
        put32(at(0x8043B5A0-21316),stage);
        for(unsigned i=0;i<5;++i)put_float(at(stage+52+4*i),i==2||i==4?-3000.f:3000.f);
        constexpr u32 audio=0x8103E000;
        put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
        at(0x803D0000+15080+852)[0]=1;
        put32(at(bank+0x600),visual);put32(at(visual_links),visual_rows);
        put32(at(0x803BB374),visual);put32(at(visual+16),visual_links);
        for(unsigned record:{3u,5u,18u,22u,23u})put32(at(visual_rows+4*record),empty);
        put32(at(0x80436380),pool);
        put32(at(pool+12),c.descriptor_cells?desc:0);put16(at(pool+18),c.descriptor_cells);
        for(unsigned i=0;i<c.descriptor_cells;++i)put32(at(desc+i*0x80),i+1<c.descriptor_cells?desc+(i+1)*0x80:0);
        if(probe||parent_probe){
            const auto area_index=probe&&probe->mode!=AreaMode::hp?
                (probe->mode==AreaMode::knockout&&knockout_cases[probe->index].ally?6u:1u):probe?probe->index:0u;
            const auto body_x=probe?200.f+area_cases[area_index].distance:
                parent_cases[parent_probe->index].incoming?800.f:200.f+parent_cases[parent_probe->index].offset;
            const auto is_ally=probe?area_cases[area_index].ally:parent_cases[parent_probe->index].ally;
            const auto& hurt=probe?probe->hurt:parent_probe->hurt;
            const auto& defender=probe?probe->spec:parent_probe->spec;
            contact::bind_sasuke(mod,ram,{body_x,100.f,300.f},hurt,spec,defender,220);
            put32(at(actor+0x4AC),actor_spec);
            at(contact::receiver+0x88)[0]=is_ally?0:1;
            put16(at(contact::receiver+0x272),probe&&area_cases[area_index].masked?2:0);
        }
        // Rows 8/11 have an area cell; row 9 refuses that cell, row 10
        // offers it with only one spare descriptor.
        const bool area_cell=probe||parent_probe||index==6||index==7||index==8||index==10||index==11||index>=14;
        const unsigned bomb_count=c.exit==3&&!probe&&!parent_probe?2u:1u;
        if(parent_probe&&parent_cases[parent_probe->index].incoming)set_pool(3,{cell_at(0,false),incoming_cell,blast},3);
        else set_pool(3,{cell_at(0,c.reverse),c.carrier_cells==1?blast:cell_at(1,c.reverse),blast},
                      c.carrier_cells+unsigned(area_cell));
        set_pool(4,{shapes[0],shapes[1],extra_shape},c.shape_cells);
        call(0x800055E0,0);
        if(c.exit==5){
            put32(at(actor+0x5BC),0x400);call(owner_hook);
            if(at(actor+0x6F7)[0]||half(actor+0x774)!=capacity)fail("type-41 full pool accepted remote command");
            report(index,c,capacity,0,0,0,capacity,capacity,0,0,0,0,0);
            return;
        }
        for(unsigned i=0;i<bomb_count;++i)placement(i,c);
        const unsigned after_pay=half(actor+0x774);
        if(c.exit==0){
            if(word(head+9*4)||word(tail+9*4)||after_pay!=capacity)fail("type-41 refusal registered a carrier");
            verify_scope(c);
            report(index,c,capacity,0,0,0,after_pay,after_pay,0,0,0,0,0);
            return;
        }
        const auto first=cell_at(0,c.reverse);
        const auto second=cell_at(1,c.reverse);
        if(word(tail+9*4)!=first||word(head+9*4)!=(bomb_count==2?second:first)||
           (bomb_count==2&&(word(first+8)!=second||word(second+4)!=first)))
            fail("type-41 source placements did not create the original list order");
        call(0x80089A94,8); // real bucket dispatcher, not individually scheduled callbacks
        for(unsigned i=0;i<bomb_count;++i){
            const auto cell=cell_at(i,c.reverse);
            if(at(cell+0x18)[0]!=(c.exit==1?2:1)||at(cell+0x158)[0]||
               half(cell+0x152)<(c.model==6?180:600)||half(cell+0x152)>=(c.model==6?244:664))
                fail("type-41 original initializer lost its signed channel, lifetime or resource gate");
        }
        const unsigned initial1=half(first+0x152),initial2=bomb_count==2?half(second+0x152):0;
        rebuild();
        if(half(actor+0x774)!=after_pay)fail("type-41 initializer returned stock prematurely");
        if(c.exit==1){
            const auto returned=observe(0x80089A94,8,return_stock);
            check_return(returned);
            if(at(first+0x18)[0]!=3||half(actor+0x774)!=capacity)fail("type-41 no-shape return did not wait for phase 2");
            const auto finish=observe(0x80089A94,8,return_stock);
            if(finish.count||at(first)[0]||half(pool+18)!=c.descriptor_cells)fail("type-41 no-shape cleanup");
            verify_scope(c);
            report(index,c,capacity,initial1,0,0,after_pay,after_pay,0,0,3,1,0);
            return;
        }
        const unsigned group=c.model==6?15:16,step=c.model==6?8:9;
        for(unsigned i=0;i<bomb_count;++i)
            if(word(shapes[i]+0x20)!=cell_at(i,c.reverse)||word(shapes[i]+0x24)!=actor||
               word(shapes[i]+0x28)!=hit_block+0xAA0+80*step||at(shapes[i]+0x14)[0]!=group)
                fail("type-41 original shape ownership or selector changed");
        if(parent_probe){parent_contact(c,*parent_probe,capacity,first);return;}
        unsigned latch=0, area_made=0, area_bound=0, fx_key=0, phase=0, retire_count=0, after_exit=after_pay;
        if(c.exit==4){
            at(actor+0x18)[0]=2;
            check_return(observe(0x80089A94,8,return_stock));
            phase=at(first+0x18)[0];
            if(phase!=3||half(actor+0x774)!=capacity)
                fail("type-41 owner-invalid exit lost its same-visit phase-2 return");
            after_exit=half(actor+0x774);
            retire_count=1;
        }else{
            if(probe)place(first+0x20,200.f);
            if(c.exit==3){
                put32(at(actor+0x5BC),0x400);call(owner_hook);
                if(half(actor+0x774)!=after_pay||at(actor+0x6F7)[0]!=1)
                    fail("type-41 remote command did not arm independently of the debit");
                // Keep the nonselected sibling alive through the bounded
                // visits; declared positions and substate, not live flight.
                if(bomb_count==2)at(second+0x19)[0]=2;
            }else{
                put16(at(first+0x152),1); // explicit near-expiry inlet after original bind
                at(actor+0x6F7)[0]=1; // Time must not consume the remote latch
            }
            const auto request=observe(0x80089A94,8,area);
            after_exit=half(actor+0x774);
            latch=at(actor+0x6F7)[0];
            fx_key=c.exit==2?19:20;
            if(request.count!=1||request.caller!=(c.exit==2?0x800E0B4C:0x800E0AAC)||
               request.arguments[0]!=first||request.arguments[1]!=first+0x20||request.arguments[2]!=fx_key||
               at(first+0x18)[0]!=2||after_exit!=after_pay||latch!=unsigned(c.exit==2)||
               (bomb_count==2&&(at(second+0x18)[0]!=1||half(second+0x152)!=initial2-1)))
                fail("type-41 selected the wrong bomb, point, area key or stock boundary");
            area_made=at(blast)[0]!=0;
            if(bool(area_made)!=(area_cell&&c.descriptor_cells>=bomb_count+2)||
               (area_made&&(at(blast+0x10)[0]!=5||at(blast+0x11)[0]!=fx_key||
                            word(blast+0x8C)!=first||word(blast+0x90)!=actor||word(blast+0x20)!=word(first+0x20))))
                fail("type-41 area allocation changed credit or parent");
            if(probe){
                if(probe->mode==AreaMode::tiers)tier_contact(c,*probe,capacity,first);
                else if(probe->mode==AreaMode::knockout||probe->mode==AreaMode::body_exit||probe->mode==AreaMode::body_cleanup)
                    knockout_contact(c,*probe,capacity,first);
                else follow_area(c,*probe,capacity,first);
                return;
            }
            rebuild();
            check_return(observe(0x80089A94,8,return_stock));
            phase=at(first+0x18)[0];
            if(phase!=3||half(actor+0x774)!=after_pay+1)fail("type-41 delayed return did not follow phase 2");
            area_bound=area_made&&at(blast+0x18)[0]==1;
            if(area_bound!=unsigned(area_made&&c.shape_cells>bomb_count))
                fail("type-41 child shape allocation did not use its separate cell");
            if(area_bound){
                const auto child_shape=bomb_count==2?extra_shape:shapes[1];
                if(word(child_shape+0x20)!=blast||word(child_shape+0x24)!=actor||
                   word(child_shape+0x2C)!=0x802C4760||at(child_shape+0x14)[0]!=(fx_key==19?179:180))
                    fail("type-41 child did not bind the exact common area group and credit");
            }
            retire_count=1;
        }
        rebuild();
        const auto finish=observe(0x80089A94,8,return_stock);
        if(finish.count||at(first)[0]||half(actor+0x774)!=after_pay+1||
           (c.exit==3&&(!at(second)[0]||at(second+0x18)[0]!=1)))
            fail("type-41 repeat cleanup returned a second stock or removed the wrong sibling");
        verify_scope(c);
        report(index,c,capacity,initial1,initial2,area_made,after_pay,after_exit,latch,fx_key,phase,retire_count,area_bound);
    }
    void check_return(const CallObservation& result){
        if(result.count!=1||result.caller!=0x800E0E7C||result.arguments[0]!=actor||result.arguments[1]!=0)
            fail("type-41 original paid return did not use the bound channel and caster");
    }
    void report(unsigned i,Options c,unsigned cap,unsigned life1,unsigned life2,unsigned area_made,unsigned after_pay,
                unsigned after_exit,unsigned latch,unsigned key,unsigned phase,unsigned count,unsigned area_bound){
        std::printf("type41-bucket: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                    i,0x0100+c.model,cap,unsigned(c.reverse),life1,life2,after_pay,after_exit,
                    half(actor+0x774),latch,key,area_made,phase,count,half(pool+18),area_bound);
    }
};
}

int main(int argc,char** argv){
    const bool area_hp=argc==4&&!std::strcmp(argv[3],"--area-hp");
    const bool parent=argc==4&&!std::strcmp(argv[3],"--parent-contact");
    const bool tiers=argc==4&&!std::strcmp(argv[3],"--tiers");
    const bool knockout=argc==4&&!std::strcmp(argv[3],"--knockout");
    const bool body_exit=argc==4&&!std::strcmp(argv[3],"--body-exit");
    const bool replacement_entry=argc==4&&!std::strcmp(argv[3],"--replacement-entry");
    const bool replacement_scene=argc==4&&!std::strcmp(argv[3],"--replacement-scene");
    const bool replacement_contact=argc==4&&!std::strcmp(argv[3],"--replacement-contact");
    const bool heal_tail=argc==4&&!std::strcmp(argv[3],"--replacement-heal-tail");
    const bool heal_effect=argc==4&&!std::strcmp(argv[3],"--replacement-heal-effect");
    const bool heal_exit=argc==4&&!std::strcmp(argv[3],"--replacement-heal-exit");
    const bool replacement=replacement_entry||replacement_scene||replacement_contact||heal_tail||heal_effect||heal_exit||
                           (argc==4&&!std::strcmp(argv[3],"--replacement"));
    const bool cleanup=replacement||(argc==4&&!std::strcmp(argv[3],"--body-cleanup"));
    if(argc!=3&&!area_hp&&!parent&&!tiers&&!knockout&&!body_exit&&!cleanup)
        fail("usage: type41-bucket-fixture qualified.dol unmodified-recomp.so [--area-hp|--parent-contact|--tiers|--knockout|--body-exit|--body-cleanup|--replacement|--replacement-entry|--replacement-scene|--replacement-contact|--replacement-heal-tail|--replacement-heal-effect|--replacement-heal-exit] < qualified-payload");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    const auto read=[](auto& data){
        if(std::fread(data.data(),1,data.size(),stdin)!=data.size())fail("type-41 payload extent");
    };
    Common bank{};contact::HitBlock hit{};std::array<contact::ActorSpec,2> specs{};
    read(bank);read(hit);
    for(auto& spec:specs)read(spec);
    if(area_hp||parent||tiers||knockout||body_exit||cleanup){
        contact::HitBlock hurt{};contact::ActorSpec defender{};read(hurt);read(defender);
        cleanup_probe::Replacement assets{};
        if(replacement){read(assets.motion);read(assets.view);}
        retail_body_entry_contact::Inputs incoming{};
        if(replacement_contact||heal_tail||heal_effect||heal_exit)for(auto& row:incoming){read(row.hit);read(row.spec);}
        cleanup_probe::archive::Source effect{};
        if(heal_effect||heal_exit)effect=cleanup_probe::archive::read_source();
        using CleanupProbe=cleanup_probe::ReplacementProbe;
        const cleanup_probe::Delivery delivery{assets,specs[0],hit,replacement_scene?CleanupProbe::scene:
                                               replacement_entry?CleanupProbe::entry:
                                               replacement_contact?CleanupProbe::contact:
                                               heal_tail?CleanupProbe::heal_tail:
                                               heal_effect?CleanupProbe::heal_effect:
                                               heal_exit?CleanupProbe::heal_exit:CleanupProbe::constructor,
                                               replacement_contact||heal_tail||heal_effect||heal_exit?&incoming:nullptr,
                                               heal_effect||heal_exit?&effect:nullptr};
        for(unsigned m=6;m<=7;++m)
            for(unsigned i=0;i<(parent?(m==6?6u:8u):tiers?std::size(tier_cases):
                                 knockout?std::size(knockout_cases):body_exit?exit_knockout_case.size():
                                 cleanup?1u:std::size(area_cases));++i){
                if(parent){
                    const ParentProbe probe{i,hurt,defender};
                    const bool incoming=parent_cases[i].incoming;
                    const Options opts{u8(m),u8(incoming?3:2),1,u8(incoming?4:3),u8(m==6?2:3),false};
                    Check(mod,original,opts,specs[m-6],hit,bank,0,nullptr,&probe);
                    continue;
                }
                const auto mode=tiers?AreaMode::tiers:knockout?AreaMode::knockout:body_exit?AreaMode::body_exit:
                    cleanup?AreaMode::body_cleanup:AreaMode::hp;
                const AreaProbe probe{i,hurt,defender,mode,replacement?&delivery:nullptr};
                const Options opts{u8(m),u8(1+unsigned(mode!=AreaMode::hp||area_cases[i].shape)),1,3,u8(m==6?2:3),false};
                Check(mod,original,opts,specs[m-6],hit,bank,0,&probe);
            }
    }else{
        for(unsigned i=0;i<std::size(cases);++i)Check(mod,original,cases[i],specs[cases[i].model-6],hit,bank,i);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra type-41 payload");
}
