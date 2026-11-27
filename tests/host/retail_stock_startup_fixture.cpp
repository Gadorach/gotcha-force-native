// Real normal installer, size initializer and level-stock writes up to the resource frontier.
#include "retail_stationary_contact.hpp"
#include <array>
#include <cstdint>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 role0=0x81014000,scene=0x81018000,resources=0x803C4D94;
constexpr unsigned records=8,record_bytes=432,roster=206,levels=10;
constexpr u32 start=0x8005614C,equipment=0x8013BD80,material=0x80006238;

u16 half(const u8* p){return u16(u16(p[0])<<8|p[1]);}

using ActorImage=std::array<u8,0x1E00>;
struct Channel {
    int current,previous;
    u32 clock;
    bool operator==(const Channel&)const=default;
};
Channel channel_state(const u8* obj,unsigned channel){
    const auto* stock=obj+0x774+8*channel;
    return {std::bit_cast<std::int16_t>(half(stock)),std::bit_cast<std::int16_t>(half(stock+2)),be32(stock+4)};
}
void print_channel(Channel state){
    std::printf(" %d %d %08X",state.current,state.previous,state.clock);
}
void actor_call(const Module& mod,std::vector<u8>& ram,u32 entry){
    CPUState cpu{};cpu.gpr[3]=actor;
    mod.run(cpu,ram,entry);
}
bool stock_debit(const Module& mod,std::vector<u8>& ram,unsigned channel,bool commit){
    CPUState cpu{};cpu.gpr[3]=actor;cpu.gpr[4]=channel;cpu.gpr[5]=1;cpu.gpr[6]=commit;
    mod.run(cpu,ram,0x8006DBE0);
    if(cpu.gpr[3]>1)fail("stock lifecycle invalid debit result");
    return cpu.gpr[3]!=0;
}
void refill_visit(const Module& mod,std::vector<u8>& ram,u8* obj,unsigned mask,bool boosted){
    obj[0x597]=u8(mask);
    obj[0x6FC]=u8(boosted);
    actor_call(mod,ram,0x8005A378);
    if(be32(obj+0x768)!=std::bit_cast<u32>(boosted?1.5f:1.f))
        fail("stock lifecycle expected a produced ordinary base clock");
    CallObservation refill{0x8006DCC0};
    CPUState cpu{};cpu.gpr[3]=actor;
    mod.run_observed(cpu,ram,0x8005568C,refill,1024);
    if(refill.count!=1||refill.caller!=0x80055D08||refill.arguments[0]!=actor||
       obj[0x597]!=mask||obj[0x6FC]!=unsigned(boosted))
        fail("stock lifecycle did not use the original refill caller");
}
void cycle(const Module& mod,std::vector<u8>& ram,u8* obj,unsigned model,unsigned level){
    ActorImage installed{};
    std::memcpy(installed.data(),obj,installed.size());
    for(const unsigned channel:{0u,2u}){
        std::memcpy(obj,installed.data(),installed.size());
        const unsigned other=2-channel;
        const auto sibling=channel_state(obj,other);
        const auto uninitialized=channel_state(obj,1);
        const bool check=stock_debit(mod,ram,channel,false);
        if(std::memcmp(obj,installed.data(),installed.size()))fail("stock check-only changed the actor");
        const bool debit=stock_debit(mod,ram,channel,true);
        const auto paid=channel_state(obj,channel);
        if(check!=debit||channel_state(obj,other)!=sibling||channel_state(obj,1)!=uninitialized||
           (!debit&&std::memcmp(obj,installed.data(),installed.size()))||
           std::memcmp(obj+0x78C,installed.data()+0x78C,24))
            fail("stock debit crossed its availability or channel boundary");
        ActorImage post{};
        std::memcpy(post.data(),obj,post.size());
        refill_visit(mod,ram,obj,2,false);
        const auto ordinary=channel_state(obj,channel);
        const auto ordinary_other=channel_state(obj,other);
        if(channel_state(obj,1)!=uninitialized)fail("inhibited orphan stock changed");
        std::memcpy(obj,post.data(),post.size());
        refill_visit(mod,ram,obj,2u|(1u<<channel),false);
        if(channel_state(obj,channel)!=paid||channel_state(obj,other)!=ordinary_other||channel_state(obj,1)!=uninitialized)
            fail("stock inhibit bit did not isolate its channel");
        std::memcpy(obj,post.data(),post.size());
        refill_visit(mod,ram,obj,2,true);
        if(channel_state(obj,1)!=uninitialized)fail("boosted visit changed inhibited channel 1");
        std::printf("stock-cycle: %04X %u %u %u %u",model,level,channel,unsigned(check),unsigned(debit));
        print_channel(paid);print_channel(ordinary);print_channel(channel_state(obj,channel));std::putchar('\n');
    }
    std::memcpy(obj,installed.data(),installed.size());
}
void timeline(const Module& mod,std::vector<u8>& ram,u8* obj,unsigned model){
    const unsigned channel=model==0x0900||model==0x0103?2:0;
    const unsigned mask=7u^(1u<<channel);
    const auto observe=[&](unsigned visit){
        std::printf("stock-time: %04X %u %u",model,channel,visit);
        print_channel(channel_state(obj,channel));std::putchar('\n');
    };
    if(model==0x0103||model==0x010A){
        // A declared positive clock is a negative control for modes 2/3.
        // The later direct return call does not qualify a gameplay trigger.
        put_float(obj+0x778+8*channel,17.f);
        observe(0);
        refill_visit(mod,ram,obj,mask,false);observe(1);
        if(!stock_debit(mod,ram,channel,true))fail("clear-clock mode timeline debit");
        observe(2);
        refill_visit(mod,ram,obj,mask,false);observe(3);
        CPUState grant{};grant.gpr[3]=actor;grant.gpr[4]=channel;
        mod.run(grant,ram,0x8006DE10);
        observe(4);
    }else if(model==0x0800){
        if(!stock_debit(mod,ram,channel,true))fail("mode-1 first timeline debit");
        refill_visit(mod,ram,obj,mask,false);
        if(!stock_debit(mod,ram,channel,true))fail("mode-1 second timeline debit");
        observe(0);
        for(unsigned visit=1;visit<=41;++visit){
            refill_visit(mod,ram,obj,mask,true);
            if(visit==19||visit==20||visit==40||visit==41)observe(visit);
        }
    }else if(model==0x0900){
        if(!stock_debit(mod,ram,channel,true))fail("mode-0 timeline debit");
        observe(0);
        for(unsigned visit=1;visit<=181;++visit){
            refill_visit(mod,ram,obj,mask,false);
            if(visit==179||visit==180||visit==181)observe(visit);
        }
    }else if(model==0x0705){
        observe(0);
        for(unsigned visit=1;visit<=301;++visit){
            refill_visit(mod,ram,obj,mask,false);
            if(visit==1){
                const auto pending=channel_state(obj,channel);
                if(pending.current!=-1||stock_debit(mod,ram,channel,false)||stock_debit(mod,ram,channel,true)||
                   channel_state(obj,channel)!=pending)
                    fail("negative source stock admitted a debit");
            }
            if(visit==1||visit==299||visit==300||visit==301)observe(visit);
        }
    }
}
constexpr u32 return_stock=0x8006DE10;
bool return_model(unsigned model){return model==0x0200||model==0x020A||model==0x020F||model==0x0403||model==0x0404;}

void receiver_factors(const Module& mod,std::vector<u8>& ram,unsigned model,unsigned record){
    const auto at=[&](u32 a){return ram.data()+a-GC_RAM_BASE;};
    constexpr u32 carrier=0x81030000,credit=0x81020000,mode=0x81021000,settings=0x81022000;
    auto* obj=at(actor);
    auto* spec=at(be32(obj+0x4AC));
    // An inactive credit owner isolates receiving operands without
    // substituting a model-specific matchup. Probe HP/force are declared.
    put32(at(carrier+0x90),credit);put_float(at(carrier+0xC4),1.f);
    obj[0]=obj[0x18]=obj[0x88]=obj[0x3E4]=1;
    put32(obj+0x90,actor);put16(obj+0x1C4,200);
    put32(at(0x8043B5A0-31856),mode);at(mode+50)[0]=2;
    put32(at(0x8043B5A0-31824),settings);at(settings+1)[0]=7;
    put32(at(scene+60+0x114),1000);
    std::printf("receiver-factors: %04X %u %u %u %u %u",model,record,spec[156],spec[157],spec[158],half(obj+0x6BE));
    for(unsigned bank=0;bank<2;++bank){
        obj[0x3E6]=u8(bank);
        for(unsigned low=0;low<2;++low){
            put16(at(0x803B069E),low?120:1000);
            put32(at(0x803B0690),low?217:1000);
            CPUState cpu{};cpu.gpr[3]=0x802C49D0;cpu.gpr[4]=carrier;cpu.gpr[5]=actor;
            mod.run(cpu,ram,0x8003CD5C);
            if(cpu.gpr[3]>200||be32(obj+0x4AC)!=role0+record*record_bytes||half(obj+0x1C6))
                fail("normal receiving amount changed the installed actor or exceeded its inlet");
            std::printf(" %u",cpu.gpr[3]);
        }
    }
    std::putchar('\n');
}

void owner_returns(const Module& mod,std::vector<u8>& ram,u8* obj,unsigned model){
    const bool pre=model==0x0403;
    const u32 entry=pre?0x800DFDA4:0x80073D0C,site=pre?0x800DFE04:0x80073D68;
    const unsigned channel=pre?0:2,other=2-channel;
    if(channel_state(obj,channel)!=Channel{1,1,0}||half(obj+0x78C+channel*8+4)!=3)
        fail("owner stock-return needs the exact initialized mode-3 channel");
    ActorImage initial{};
    std::memcpy(initial.data(),obj,initial.size());
    for(unsigned i=0;i<(pre?4u:3u);++i){
        std::memcpy(obj,initial.data(),initial.size());
        if(i&&!stock_debit(mod,ram,channel,true))fail("owner stock-return debit");
        obj[0x144]=u8(i==1);
        obj[0x145]=u8(i==3);
        put_float(obj+0x778+8*channel,17.f);
        const auto sibling=channel_state(obj,other);
        unsigned calls=0;
        for(unsigned n=0;n<2;++n){
            CPUState cpu{};cpu.gpr[3]=actor;
            CallObservation result{return_stock};
            mod.run_observed(cpu,ram,entry,result);
            const bool expected=i==2&&n==0;
            if(result.count!=unsigned(expected)||(expected&&(result.caller!=site||result.arguments[0]!=actor||
               result.arguments[1]!=channel))||channel_state(obj,other)!=sibling)
                fail("owner stock-return selected the wrong actor or callback gate");
            calls+=result.count;
        }
        const auto after=channel_state(obj,channel);
        if(after!=Channel{i==0||i==2?1:0,1,std::bit_cast<u32>(17.f)})
            fail("owner stock-return changed its count or clock incorrectly");
        std::printf("stock-owner-return: %04X %u %u %u %u",model,i,channel,calls,
                    unsigned(pre?obj[0x58E]&2:obj[0x582]));
        print_channel(after);std::putchar('\n');
    }
}

void wire_returns(const Module& mod,std::vector<u8>& ram,u8* obj,const contact::HitBlock& hit){
    constexpr u32 carrier=0x81030000,shape=0x81032000,descriptors=0x81033000,pool=0x81033800,block=0x81035000;
    constexpr u32 target=0x8101A000,stage=0x81048000,audio=0x8103E000,wire=0x800DA0CC,wire_factory=0x800D9FBC;
    constexpr u32 attack_count=0x8043B5A0-21328;
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto get16=[&](u32 a){return contact::half(ram,a);};
    if(channel_state(obj,2)!=Channel{1,1,0}||half(obj+0x7A0)!=3||word(actor+0x27C)!=0x80310CA8)
        fail("Thunder Wire needs the exact normal installer and level-1 stock");
    const auto other=channel_state(obj,0);
    obj[0]=1;obj[0x18]=1;
    put16(obj+0x94,0x0404);put16(obj+0x1C4,350);put16(obj+0x1C6,350);
    put32(obj+0x90,actor);put32(obj+0xCC,target);put32(obj+0x3FC,block);
    put_float(obj+0xC0,1.f);put_float(obj+0xC4,1.f);
    std::memcpy(at(block),hit.data(),hit.size());
    const u32 joint=actor+0x8D4+13*48;
    for(unsigned i=0;i<3;++i){
        put_float(at(joint+20*i),1.f);
        put_float(at(joint+12+16*i),200.f);
    }
    for(unsigned i=0;i<6;++i)put32(at(0x803C4E84+4*i),i?target:actor);
    put16(at(target+0x784),9);
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
    // Supply an already-held request for the original active-loop sound.
    // 800F0104 then refreshes the request without entering playback.
    put32(at(0x803D0000+15080+32+88),word(0x8031B694+7*4));
    put32(at(0x8043B5A0-21316),stage);
    for(unsigned i=0;i<5;++i)put_float(at(stage+52+4*i),(i==2||i==4)?-3000.f:3000.f);
    put32(at(0x80436380),pool);
    const auto offer=[&](unsigned kind,u32 cell,bool on){
        at(cell)[2]=kind;
        put32(at(contact::free_head+4*kind),on?cell:0);put32(at(contact::free_tail+4*kind),on?cell:0);
        put16(at(contact::free_count+2*kind),u16(on));
    };
    const auto rebuild=[&](){actor_call(mod,ram,0x8008A28C);actor_call(mod,ram,0x80089AEC);};
    const auto witness=[&](u32 entry,u32 owner,u32 observed){
        CPUState cpu{};cpu.gpr[3]=owner;
        CallObservation seen{observed};
        mod.run_observed(cpu,ram,entry,seen,4096);
        return seen;
    };
    const auto write_check=[&](const CallObservation& seen,u32 site){
        if(seen.count!=unsigned(site!=0)||(site&&(seen.caller!=site||seen.arguments[0]!=actor||seen.arguments[1]!=2)))
            fail("Thunder Wire return did not name the exact paying actor and channel");
    };
    const auto seed=ram;
    for(unsigned i=0;i<8;++i){
        std::memcpy(ram.data(),seed.data(),seed.size());
        const bool cell=i!=2,descriptor=i!=3,sh=i>=5,paid=i>=2,made=i>=4;
        if(i==1)put16(at(actor+0x784),0);
        at(actor+0x1CEF)[0]=u8(i!=0);
        offer(3,carrier,cell);offer(4,shape,sh);
        put32(at(pool+12),descriptor?descriptors:0);put16(at(pool+18),u16(descriptor));
        auto branch=ram;
        const auto requested=witness(0x800D8470,actor,wire_factory);
        if(requested.count!=unsigned(paid)||(paid&&(requested.caller!=0x800D84C8||requested.arguments[0]!=actor))||
           at(actor+0x1CEF)[0]||bool(at(carrier)[0])!=made||
           (made&&(at(carrier+0x10)[0]!=43||word(carrier+0xC)!=wire||word(carrier+0x8C)!=actor||
                   word(carrier+0x90)!=actor||word(carrier+0xCC)!=target)))
            fail("Thunder Wire cue or carrier ownership changed");
        std::memcpy(ram.data(),branch.data(),branch.size());
        u32 return_site=i==2||i==3?0x800DA0AC:0;
        CPUState debit_cpu{};debit_cpu.gpr[3]=actor;
        CallObservation debit{0x8006DBE0};
        const auto cut=mod.run_to_or_return(debit_cpu,ram,0x800D8470,{return_stock},&debit,4096);
        if(cut!=(return_site?return_stock:0)||debit.count!=unsigned(i!=0)||
           (debit.count&&(debit.caller!=0x800D84B8||debit.arguments[0]!=actor||debit.arguments[1]!=2||
                         debit.arguments[2]!=1||debit.arguments[3]!=1))||
           (return_site&&(debit_cpu.lr!=return_site+4||get16(actor+0x784))))
            fail("Thunder Wire did not debit before the factory return");
        std::memcpy(ram.data(),branch.data(),branch.size());
        write_check(witness(0x800D8470,actor,return_stock),return_site);
        const auto after_release=get16(actor+0x784);
        unsigned phase=0,shape_published=0,returned_cell=0,shape_return_visits=0;
        if(made){
            return_site=i==4?0x800DA274:0;
            write_check(witness(wire,carrier,return_stock),return_site);
            if(at(carrier+0x18)[0]!=(sh?1:2)||
               (sh&&(word(shape+0x20)!=carrier||word(shape+0x24)!=actor||word(shape+0x28)!=block+0xAA0+24*80||
                    word(shape+0x2C)!=0x80310CA8||at(shape+0x14)[0]!=8)))
                fail("Thunder Wire opening queue or phase changed");
            if(sh){
                CPUState pose{};pose.gpr[3]=carrier;
                mod.run(pose,ram,0x80047AA4);
                rebuild();
                if(get16(attack_count)!=1||word(0x803C477C)!=shape||!get16(carrier+0x26C)||get16(actor+0x784))
                    fail("Thunder Wire admitted shape spent or returned the wrong stock");
                shape_published=1;
                if(i==5){
                    put_float(at(carrier+0x20),9000.f);
                    return_site=0x800DA4CC;
                }else if(i==6)at(actor+0x18)[0]=2;
                else put16(at(actor+0x3E8),0x0403);
                write_check(witness(wire,carrier,return_stock),return_site);
            }
            phase=at(carrier+0x18)[0];
            if(sh){
                do{rebuild();++shape_return_visits;}while(at(shape)[0]&&shape_return_visits<2);
                if(at(shape)[0]||get16(attack_count)||get16(contact::free_count+8)!=1)
                    fail("Thunder Wire did not return its admitted shape cell");
            }
            for(unsigned visits=0;at(carrier)[0]&&visits<2;++visits)
                write_check(witness(wire,carrier,return_stock),0);
            returned_cell=!at(carrier)[0]&&get16(contact::free_count+6)==1&&get16(pool+18)==1;
            if(!returned_cell)fail("Thunder Wire retirement did not release its carrier and descriptor");
        }
        const auto after=channel_state(at(actor),2);
        if(channel_state(at(actor),0)!=other||get16(target+0x784)!=9||get16(actor+0x1C6)!=350||after.clock||
           after.current!=int(i!=1&&i<6))
            fail("Thunder Wire return changed unrelated stock, time or HP");
        std::printf("stock-wire-return: %u %u %u %u %u %08X %u %u %u",i,unsigned(paid),unsigned(made),after_release,
                    shape_published,return_site,phase,returned_cell,shape_return_visits);
        print_channel(after);std::putchar('\n');
    }
}

void inspect(const Module& mod,std::vector<u8>& ram,unsigned model,u32 installer,unsigned extent,bool lifecycle,
             const contact::HitBlock* wire_hit,bool capacity,bool factors){
    const auto at=[&](u32 a){return ram.data()+a-GC_RAM_BASE;};
    auto* obj=at(actor);
    put32(at(resources),role0);
    put32(at(0x8043B5A0-31852),scene);
    put16(obj+0x3E8,u16(model)); // exact normal model, standalone/variant/resource indices are zero
    CPUState install{};install.gpr[3]=actor;
    mod.run(install,ram,installer);
    const auto selected=be32(obj+0x4AC);
    if(selected<role0||selected>=role0+extent||((selected-role0)%record_bytes))
        fail("normal stock startup installer selected an unprovided actor-data row");
    ActorImage pristine{};
    std::memcpy(pristine.data(),obj,pristine.size());
    for(unsigned level=0;level<levels;++level){
        // Before these cuts, the initialization bodies write actor fields
        // only. Restore the original installed actor before each level.
        std::memcpy(obj,pristine.data(),pristine.size());
        obj[0x3EE]=u8(level);
        for(unsigned i=0;i<8;++i){obj[0x77C+i]=u8(0xA0+i);obj[0x794+i]=u8(0xB0+i);}
        CPUState cpu{};cpu.gpr[3]=actor;
        const auto cut=mod.run_to(cpu,ram,start,{equipment,material});
        if((cut==equipment?cpu.lr!=0x80056604:cpu.lr!=0x80056824)||be32(obj+0x4AC)!=selected||
           half(obj+0x1C6)||obj[0x3EE]!=level)
            fail("normal stock startup crossed the declared resource or actor boundary");
        for(unsigned i=0;i<8;++i)
            if(obj[0x77C+i]!=u8(0xA0+i)||obj[0x794+i]!=u8(0xB0+i))
                fail("normal level stock overwrote stock channel 1");
        if(capacity){
            std::printf("tier-capacity: %04X %u %u %u\n",model,(selected-role0)/record_bytes,
                        half(obj+0x6BE),half(obj+0x6C2));
            return;
        }
        if(factors){
            receiver_factors(mod,ram,model,(selected-role0)/record_bytes);
            return;
        }
        if(wire_hit){
            if(model==0x0404)wire_returns(mod,ram,obj,*wire_hit);
            else owner_returns(mod,ram,obj,model);
            return;
        }
        if(lifecycle){
            cycle(mod,ram,obj,model,level+1);
            if(!level)timeline(mod,ram,obj,model);
            continue;
        }
        std::printf("stock-start: %04X %u %u %u %04X %04X %08X %08X %08X %08X",
                    model,level+1,(selected-role0)/record_bytes,unsigned(cut==equipment),
                    half(obj+0x59C),half(obj+0x59E),be32(obj+0x74A),be32(obj+0xB4),be32(obj+0xB8),be32(obj+0x1DD0));
        for(unsigned i:{0u,2u}){
            const auto* value=obj+0x774+8*i;
            const auto* def=obj+0x78C+8*i;
            std::printf(" %u %u %u %u %u %u %08X",half(def),half(def+2),half(def+4),half(def+6),
                        half(value),half(value+2),be32(value+4));
        }
        std::putchar('\n');
    }
}
}

int main(int argc,char** argv){
    const bool lifecycle=argc==4&&!std::strcmp(argv[3],"--lifecycle");
    const bool returns=argc==4&&!std::strcmp(argv[3],"--returns");
    const bool capacity=argc==4&&!std::strcmp(argv[3],"--tier-capacity");
    const bool factors=argc==4&&!std::strcmp(argv[3],"--receiver-factors");
    if(argc!=3&&!lifecycle&&!returns&&!capacity&&!factors)
        fail("usage: stock-startup-fixture qualified.dol unmodified-recomp.so [--lifecycle|--returns|--tier-capacity|--receiver-factors] < qualified-plan");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    contact::HitBlock wire_hit{};
    if(returns&&std::fread(wire_hit.data(),1,wire_hit.size(),stdin)!=wire_hit.size())fail("Thunder Wire hit block extent");
    int previous=-1;
    for(unsigned i=0;i<roster;++i){
        std::array<u8,8> header{};
        if(std::fread(header.data(),1,header.size(),stdin)!=header.size())fail("stock-startup header extent");
        const auto model=half(header.data());
        const auto entry=be32(header.data()+2);
        const auto count=header[6];
        if(int(model)<=previous||model>=0x8000||!count||count>records||header[7]||
           entry<0x80003100||entry>=0x802B0160||(entry&3))fail("stock-startup header");
        previous=model;
        auto ram=original;
        const unsigned bytes=count*record_bytes;
        if(std::fread(ram.data()+role0-GC_RAM_BASE,1,bytes,stdin)!=bytes)fail("stock-startup records extent");
        if(!returns||return_model(model))inspect(mod,ram,model,entry,bytes,lifecycle,returns?&wire_hit:nullptr,capacity,factors);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra stock-startup rows");
}
