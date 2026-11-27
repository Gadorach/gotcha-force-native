// Original opcode-10 melee publication and body-local contact timer ownership.
#include "retail_contact_pose_clock.hpp"

namespace {
using namespace retail_contact_pose_clock;
struct Route {
    unsigned model,group,index,seed,hp,damage,frame,count;
    u32 command,table;
    std::array<unsigned,2> steps,joints;
};
constexpr Route routes[]{
    {0x0807,1,1,4,60,20,10,1,0x80377E40,0x80377AC0,{2,0},{1,0}},
    {0x0001,4,5,8,200,41,9,2,0x8031AAD4,0x8031AF90,{6,5},{0,7}},
};
constexpr u32 commands=0x81034000,hit_block=0x81035000,cells=0x81039000,stride=0x200;
constexpr u32 attack_count=0x8043B5A0-21328,attack_registry=0x803C477C,body=contact::receiver;
constexpr u32 amount=contact::amount_entry,script=0x8004C6B4,allocator=0x8008AC80;
constexpr contact::Point center{200.f,150.f,325.f};

void observe(const Module& mod,std::vector<u8> ram,const Route& r,const contact::HitBlock& receiver_hit,
             const contact::ActorSpec& defender,const contact::HitBlock& attacker_hit,
             const contact::ActorSpec& attacker,unsigned scenario){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto point=[&](u32 address){
        return contact::Point{std::bit_cast<float>(word(address)),std::bit_cast<float>(word(address+4)),
                              std::bit_cast<float>(word(address+8))};
    };
    const bool enabled=scenario!=0,hit=scenario>=2;
    const unsigned prior_a=scenario==3?r.seed+2:scenario==4?2:0;
    const unsigned prior_b=scenario==4?r.seed+2:scenario==3?2:0;
    const auto displaced=[&](unsigned i){return (scenario==5&&i==0)||(scenario==6&&i==1);};
    auto target=center;
    if(scenario==1)target[0]+=4000.f;
    put32(at(0x80436380),0x81033A00);
    contact::bind_sasuke(mod,ram,target,receiver_hit,attacker,defender,r.hp);
    at(actor)[0]=1;
    put32(at(0x803C4E84),actor);
    for(unsigned i=2;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    for(const u32 field:{0xB8u,0xC0u})put_float(at(actor+field),1.f);
    put16(at(actor+0x3E8),u16(r.model));
    put32(at(actor+0x27C),r.table);put32(at(actor+0x3FC),hit_block);
    put_float(at(actor+0x1CDC),float(r.frame));
    std::memcpy(at(hit_block),attacker_hit.data(),attacker_hit.size());
    const auto record=r.table+24*r.index;
    if(at(r.command)[0]!=10||at(r.command)[1]!=r.group||at(r.command)[2]||
       at(record+8)[0]!=r.seed||half(record+18)&0x0FC0)
        fail("melee timer command or amount inlet changed");
    u32 pose_joints=0;
    for(unsigned i=0;i<r.count;++i){
        const auto raw=hit_block+0xAA0+0x50*r.steps[i];
        const auto matrix=actor+0x8D4+0x30*r.joints[i];
        const auto local=point(raw+12);
        const float delta=displaced(i)?-4000.f:0.f;
        if(at(raw)[1]!=r.joints[i]||half(raw+4)!=r.index||half(raw+6)!=r.frame)
            fail("melee timer shape index, joint or local frame changed");
        for(unsigned axis=0;axis<3;++axis){
            put_float(at(matrix+20*axis),1.f);
            put_float(at(matrix+12+16*axis),center[axis]-local[axis]+(axis==0?delta:0.f));
        }
        const auto cell=cells+i*stride;
        at(cell)[2]=4;put32(at(cell+4),i+1<r.count?cell+stride:0);
        pose_joints|=1u<<r.joints[i];
    }
    put32(at(actor+0x76C),pose_joints);
    put32(at(contact::free_head+16),cells);put32(at(contact::free_tail+16),cells+(r.count-1)*stride);
    put16(at(contact::free_count+8),u16(r.count));
    put32(at(commands),r.command);
    CPUState command{};command.gpr[3]=actor;command.gpr[4]=enabled?15:0;command.gpr[5]=commands;
    CallObservation request{allocator};
    mod.run_observed(command,ram,script,request,1024);
    if(word(commands)!=r.command+4||request.count!=unsigned(enabled)||
       (enabled&&(request.caller!=0x8004C6F0||request.arguments[0]!=actor||request.arguments[1]!=r.group))||
       half(contact::free_count+8)!=(enabled?0:r.count)||at(actor+0x744)[0]!=(enabled?r.group:0))
        fail("melee opcode did not preserve the original lane/shape boundary");
    for(u32 entry:{0x8008A28Cu,0x80089AECu}){
        CPUState publish{};mod.run(publish,ram,entry,1024);
    }
    const auto shape_count=enabled?r.count:0;
    if(half(attack_count)!=shape_count||half(contact::receiver_count)!=1||
       word(contact::receiver_registry)!=contact::body||point(contact::body+0x64)!=target){
        std::fprintf(stderr,"publish model=%04X case=%u attack=%u/%u receive=%u head=%08X phase=%u flags=%u/%u\n",
                     r.model,scenario,half(attack_count),shape_count,half(contact::receiver_count),
                     word(contact::active_head+48*4),at(cells+0x10)[0],at(actor+0x86)[0],at(actor+0x1CA)[0]);
        fail("melee or stationary receiver publication disagreed");
    }
    unsigned seen=0;
    for(unsigned i=0;i<shape_count;++i){
        const auto cell=word(attack_registry+4*i);
        const unsigned n=(cell-cells)/stride;
        if(cell<cells||(cell-cells)%stride||n>=r.count||(seen&(1u<<n))||
           word(cell+0x20)!=actor||word(cell+0x24)!=actor||word(cell+0x2C)!=r.table||
           word(cell+0x28)!=hit_block+0xAA0+0x50*r.steps[n]||at(cell+0x11)[0]!=r.index||
           half(actor+0x26C)!=1||at(actor+0x83)[0]||at(body+0x83)[0])
            fail("melee queue did not publish the executing body as the attack object");
        auto expected=center;
        if(displaced(n))expected[0]-=4000.f;
        if(point(cell+0x64)!=expected)fail("melee shape did not follow the declared matrix");
        seen|=1u<<n;
    }
    constexpr u32 audio=0x8103E000;
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
    at(0x803D0000+15080+852)[0]=1;
    for(const auto object:{actor,body}){
        factors(ram,object);tick(mod,ram,object);
        check_clocks(ram,object,0,unit,unit,neutral_secondary);
    }
    at(actor+timer)[0]=u8(prior_a);at(body+timer)[0]=u8(prior_b);
    const unsigned armed_a=hit?std::max(prior_a,r.seed):prior_a;
    const unsigned armed_b=hit?std::max(prior_b,r.seed):prior_b;
    const unsigned loss=hit?r.damage:0;
    unsigned selected=0;
    {
        auto fork=ram;
        CPUState probe{};CallObservation reached{amount};
        const auto cut=mod.run_to_or_return(probe,fork,contact::pair_pass,{amount},&reached);
        if(cut!=(hit?amount:0)||reached.count!=unsigned(hit)||
           (hit&&(reached.caller!=0x8002F108||reached.arguments[0]!=record||
                  reached.arguments[1]!=actor||reached.arguments[2]!=body))||
           contact::half(fork,body+0x1C6)!=180||contact::at(fork,body+0x2AF)[0])
            fail("melee timer amount cut or carrier ownership changed");
        if(hit){
            const auto offset=probe.gpr[20]-cells;
            if(offset%stride||offset/stride>=r.count||displaced(offset/stride))
                fail("melee amount cut selected an ineligible shape");
            selected=r.steps[offset/stride];
        }
        check_clocks(fork,actor,armed_a,unit,unit,neutral_secondary);
        check_clocks(fork,body,armed_b,unit,unit,neutral_secondary);
    }
    CPUState full{};CallObservation amounts{amount};
    mod.run_observed(full,ram,contact::pair_pass,amounts,4096);
    const auto hp_stable=[&](){
        return half(body+0x1C6)==180-loss&&word(actor+0x418)==loss&&word(body+0x41C)==loss&&
               half(body+0x20C)==unsigned(hit)&&at(body+0x26E)[0]==unsigned(hit)&&half(actor+0x1C6)==r.hp&&
               at(body+0x2AF)[0]==(hit?4u:0u);
    };
    if(!hp_stable()||amounts.count!=unsigned(hit)){
        std::fprintf(stderr,"melee model=%04X case=%u hp=%u/%u ledgers=%u/%u amounts=%u token=%u cursor=%u\n",
                     r.model,scenario,half(body+0x1C6),half(actor+0x1C6),word(actor+0x418),word(body+0x41C),
                     amounts.count,half(body+0x20C),at(body+0x26E)[0]);
        fail("melee duplicate shape or whiff applied a different HP result");
    }
    check_clocks(ram,actor,armed_a,unit,unit,neutral_secondary);
    check_clocks(ram,body,armed_b,unit,unit,neutral_secondary);
    unsigned slow_a=0,slow_b=0;
    for(unsigned visit=1;visit<=std::max(armed_a,armed_b)+2;++visit){
        const auto clock=[&](u32 object,unsigned armed,unsigned& slowed){
            tick(mod,ram,object);
            const unsigned left=visit<armed?armed-visit:0;
            check_clocks(ram,object,left,unit,left?slow:unit,left?slow:neutral_secondary);
            slowed+=unsigned(left!=0);
        };
        clock(actor,armed_a,slow_a);clock(body,armed_b,slow_b);
        if(!hp_stable())fail("melee body clock changed accepted HP or token history");
    }
    // Do not reopen the presentation gate. Check the retained attack
    // tokens after both timers expire; no timer or amount may be replayed.
    CPUState repeat{};CallObservation replayed{amount};
    mod.run_observed(repeat,ram,contact::pair_pass,replayed,4096);
    if(replayed.count||!hp_stable()||at(actor+timer)[0]||at(body+timer)[0])
        fail("melee retained token could re-arm the contact clocks");
    std::printf("melee-clock: %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",r.model,scenario,shape_count,selected,
                r.seed,prior_a,prior_b,armed_a,armed_b,slow_a,slow_b,loss,half(body+0x1C6),half(body+0x20C));
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: melee-pose-clock-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    const auto read=[](auto& value){
        if(std::fread(value.data(),1,value.size(),stdin)!=value.size())fail("melee clock source payload extent");
    };
    contact::HitBlock body_hit{};contact::ActorSpec defender{};
    read(body_hit);read(defender);
    for(const auto& route:routes){
        contact::HitBlock hit{};contact::ActorSpec attacker{};
        read(hit);read(attacker);
        for(unsigned scenario=0;scenario<(route.count==2?7u:5u);++scenario)
            observe(mod,original,route,body_hit,defender,hit,attacker,scenario);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("unexpected melee clock payload");
}
