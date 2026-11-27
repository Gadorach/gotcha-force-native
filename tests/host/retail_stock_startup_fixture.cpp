// Real normal installer, size initializer and level-stock writes up to the resource frontier.
#include "retail_recomp_fixture.hpp"
#include <array>
#include <cstdint>

namespace {
using namespace retail_fixture;
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
void inspect(const Module& mod,std::vector<u8>& ram,unsigned model,u32 installer,unsigned extent,bool lifecycle){
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
    if(argc!=3&&!lifecycle)fail("usage: stock-startup-fixture qualified.dol unmodified-recomp.so [--lifecycle] < qualified-plan");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
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
        inspect(mod,ram,model,entry,bytes,lifecycle);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra stock-startup rows");
}
