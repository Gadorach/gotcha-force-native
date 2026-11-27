// Original type-41 placement, bucket order, area request and stock return.
#include "retail_stationary_contact.hpp"
#include <algorithm>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
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
struct AreaProbe {unsigned index;const contact::HitBlock& hurt;const contact::ActorSpec& spec;};

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
public:
    Check(const Module& module,std::vector<u8> data,Options c,const contact::ActorSpec& spec,
          const contact::HitBlock& hit,const Common& bank_data,unsigned index,const AreaProbe* probe=nullptr):mod(module),ram(std::move(data)){
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
        if(probe){
            const auto rule=area_cases[probe->index];
            contact::bind_sasuke(mod,ram,{200.f+rule.distance,100.f,300.f},probe->hurt,spec,probe->spec,220);
            put32(at(actor+0x4AC),actor_spec);
            at(contact::receiver+0x88)[0]=rule.ally?0:1;
            put16(at(contact::receiver+0x272),rule.masked?2:0);
        }
        // Rows 8/11 have an area cell; row 9 refuses that cell, row 10
        // offers it with only one spare descriptor.
        const bool area_cell=probe||index==6||index==7||index==8||index==10||index==11||index>=14;
        const unsigned bomb_count=c.exit==3&&!probe?2u:1u;
        set_pool(3,{cell_at(0,c.reverse),c.carrier_cells==1?blast:cell_at(1,c.reverse),blast},
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
            if(probe){follow_area(c,*probe,capacity,first);return;}
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
    if(argc!=3&&!area_hp)fail("usage: type41-bucket-fixture qualified.dol unmodified-recomp.so [--area-hp] < qualified-payload");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    const auto read=[](auto& data){
        if(std::fread(data.data(),1,data.size(),stdin)!=data.size())fail("type-41 payload extent");
    };
    Common bank{};contact::HitBlock hit{};std::array<contact::ActorSpec,2> specs{};
    read(bank);read(hit);
    for(auto& spec:specs)read(spec);
    if(area_hp){
        contact::HitBlock hurt{};contact::ActorSpec defender{};read(hurt);read(defender);
        for(unsigned m=6;m<=7;++m)
            for(unsigned i=0;i<std::size(area_cases);++i){
                const AreaProbe probe{i,hurt,defender};
                const Options opts{u8(m),u8(1+unsigned(area_cases[i].shape)),1,3,u8(m==6?2:3),false};
                Check(mod,original,opts,specs[m-6],hit,bank,0,&probe);
            }
    }else{
        for(unsigned i=0;i<std::size(cases);++i)Check(mod,original,cases[i],specs[cases[i].model-6],hit,bank,i);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra type-41 payload");
}
