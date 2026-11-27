// Execute the two exact normal type-76 request leaves and optional opening contact.
#include "retail_stationary_contact.hpp"
#include <algorithm>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
struct Route {unsigned model,key,channel,cursor,markers,group,first_step,high_index;u32 pre,entry,debit_site,table;};
constexpr Route routes[]={
    {0x0D01,0,0,0x148,0x146,2,3,2,0x80188A64,0x80188EC4,0x80188F4C,0x803628F0},
    {0x0D04,2,2,0x151,0x152,1,1,1,0x800F77C8,0x800F7DC0,0x800F7EF0,0x8037A830},
};
constexpr u32 first=0x81030000,second=0x81030800,descriptor=0x81032000,descriptor_row=0x81033000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 allocate=0x80088AA0,debit_entry=0x8006DBE0;
constexpr u32 hit_block=0x81034000,shapes=0x81039000,shape_stride=0x200,attack_count=0x8043B5A0-21328;
constexpr u32 attack_registry=0x803C477C,bind=0x8008AC80,opening=0x800FCE50;
struct Opening {
    unsigned ordinal,scenario;
    const contact::HitBlock& attack;
    const contact::HitBlock& body;
    const contact::ActorSpec& attacker;
    const contact::ActorSpec& defender;
};
struct ShapeCase {unsigned slots;float z;int exit_cells=-1;};
constexpr ShapeCase shape_cases[]={{0,25},{1,25},{2,25,1},{2,-40},{2,-80},{2,25,0}};

void opening_contact(const Module& mod,std::vector<u8>& ram,Route r,u32 carrier,const Opening& inlet){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto scalar=[&](u32 address){return std::bit_cast<float>(word(address));};
    const auto point=[&](u32 address){return contact::Point{scalar(address),scalar(address+4),scalar(address+8)};};
    const auto cell=[](unsigned i){return shapes+i*shape_stride;};
    const auto rule=shape_cases[inlet.scenario];
    constexpr unsigned sockets[2][2]={{2,3},{8,10}};
    const unsigned socket=sockets[r.key/2][inlet.ordinal];
    auto* owner=at(actor);
    put32(owner+0x27C,r.table);put32(owner+0x3FC,hit_block);
    std::memcpy(at(hit_block),inlet.attack.data(),inlet.attack.size());
    // Distinct declared socket translations expose the original ordinal lookup.
    const contact::Point translation{float(100*socket),200.f,300.f};
    const auto joint=actor+0x8D4+48*socket;
    for(unsigned axis=0;axis<3;++axis){
        put_float(at(joint+20*axis),1.f);put_float(at(joint+12+16*axis),translation[axis]);
    }
    auto root=translation;
    root[1]+=r.key==0?-42.4f:-30.5f;
    auto target=root;
    target[2]+=rule.z;
    put32(at(0x803C4E84),actor);
    for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    contact::bind_sasuke(mod,ram,target,inlet.body,inlet.attacker,inlet.defender,150);
    put32(at(free_head+16),rule.slots?cell(0):0);
    put32(at(free_tail+16),rule.slots?cell(rule.slots-1):0);
    put16(at(free_count+8),u16(rule.slots));
    for(unsigned i=0;i<rule.slots;++i){
        at(cell(i))[2]=4;put32(at(cell(i)+4),i+1<rule.slots?cell(i+1):0);
    }
    // Empty render records and unavailable audio, not retail model/pose data.
    constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400,audio=0x8103E000;
    put32(at(0x803BB4C0+0x600),visual);put32(at(visual+16),link);put32(at(link),table);put32(at(table+4),empty);
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;at(0x803D0000+15080+852)[0]=1;
    CPUState init{};init.gpr[3]=carrier;
    CallObservation group{bind};
    mod.run_observed(init,ram,opening,group,1024);
    if(group.count!=1||group.caller!=0x800FD03C||group.arguments[0]!=carrier||group.arguments[1]!=r.group||
       at(carrier)[0x18]!=(rule.slots?1:2)||point(carrier+0x20)!=root||half(free_count+8)||
       word(active_head+49*4)!=(rule.slots?cell(rule.slots-1):0))
        fail("type-76 opening did not select the exact socket and scoped carrier group");
    CPUState late{};late.gpr[3]=carrier;
    mod.run(late,ram,0x80047AA4);
    const auto publish=[&]{
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState pass{};mod.run(pass,ram,entry,1024);
        }
    };
    publish();
    const bool has_receiver=rule.slots==2;
    const auto attacks=half(attack_count),receivers=half(contact::receiver_count);
    if(attacks!=unsigned(rule.slots!=0)||receivers!=1+unsigned(has_receiver)||
       word(contact::receiver_registry+4*unsigned(has_receiver))!=contact::body||
       (has_receiver&&word(contact::receiver_registry)!=cell(1))||
       point(contact::body+0x64)!=target||scalar(contact::body+0x7C)!=22.f)
        fail("type-76 opening and Sasuke body did not publish");
    for(unsigned i=0;i<rule.slots;++i){
        auto center=root;if(!i)center[2]+=25.f;
        if((!i&&word(attack_registry)!=cell(i))||word(cell(i)+0x20)!=carrier||
           word(cell(i)+0x24)!=actor||word(cell(i)+0x28)!=hit_block+0xAA0+80*(r.first_step+i)||
           word(cell(i)+0x2C)!=r.table||at(cell(i))[0x11]!=(i?0:r.high_index)||at(cell(i))[0x10]!=1||
           point(cell(i)+0x64)!=center||scalar(cell(i)+0x7C)!=(i?55.f:30.f)||half(carrier+0x26C)!=1)
            fail("type-76 shape admission prefix, registry order or amount index changed");
    }
    CPUState writer{};
    CallObservation amount{contact::amount_entry};
    const auto cut=mod.run_to_or_return(writer,ram,contact::pair_pass,{contact::hp_write},&amount);
    const bool hit=rule.slots&&rule.z==25.f;
    const auto record=r.table+24*r.high_index;
    if(cut!=(hit?contact::hp_write:0)||amount.count!=unsigned(hit)||
       (hit&&(amount.caller!=0x8002F108||amount.arguments[0]!=record||amount.arguments[1]!=carrier||
              amount.arguments[2]!=contact::receiver||writer.lr!=0x8002F1DC||writer.gpr[3]!=contact::receiver))||
       half(contact::receiver+0x1C6)!=180)
        fail("type-76 contact did not use only the attack descriptor's amount record");
    const unsigned loss=hit?writer.gpr[4]:0;
    if(hit)mod.finish_stopped_leaf(writer,contact::hp_write,164);
    const unsigned hp=half(contact::receiver+0x1C6),token=half(contact::receiver+0x20C);
    if(hp!=180-loss||half(actor+0x1C6)!=150||token!=unsigned(hit)||at(carrier)[0x1D9]!=(hit?16:0)||
       half(contact::receiver+0x1C8)!=(hit?180:0xFFFF)||word(actor+0x418)!=loss||word(contact::receiver+0x41C)!=loss||
       at(contact::receiver)[0x26E]!=unsigned(hit)||word(contact::receiver+0x290)!=(hit?carrier:0)||
       word(carrier+0x290)!=(hit?contact::receiver:0))
        fail("type-76 opening HP, credit or contact latch changed");
    contact::pair_hp(mod,ram,carrier,record,false);
    if(half(contact::receiver+0x1C6)!=hp||word(actor+0x418)!=loss||word(contact::receiver+0x41C)!=loss||
       at(contact::receiver)[0x26E]!=unsigned(hit)||half(contact::receiver+0x20C)!=token)
        fail("type-76 same registry applied a token twice");

    unsigned exit_requests=0,child_created=0;
    if(rule.exit_cells>=0){
        if(!hit||at(carrier)[0x1D9]!=16)fail("type-76 exit does not have an original hit latch");
        const bool spare=rule.exit_cells==1;
        constexpr u32 child=0x8103A000;
        put32(at(free_head+12),spare?child:0);put32(at(free_tail+12),spare?child:0);
        put16(at(free_count+6),u16(spare));at(child)[2]=3;
        const auto descriptors=half(descriptor_row+18),stock=half(actor+0x774+8*r.channel);
        CPUState next{};next.gpr[3]=carrier;
        CallObservation child_alloc{allocate};
        mod.run_observed(next,ram,opening,child_alloc,1024);
        if(at(carrier)[0x18]!=2||at(carrier)[0x1D8]!=1||child_alloc.count!=1||
           child_alloc.caller!=0x800FC240||child_alloc.arguments[0]!=actor||child_alloc.arguments[1]!=3||
           child_alloc.arguments[2]!=8||child_alloc.arguments[3]||child_alloc.arguments[4]||
           half(descriptor_row+18)!=descriptors||half(free_count+6)||half(actor+0x774+8*r.channel)!=stock||
           word(active_head+9*4)!=(spare?child:carrier)||
           (spare&&(at(child)[0x10]!=8||at(child)[0x11]||at(child)[0x18]||word(child+8)!=carrier||
                    word(child+0x8C)!=carrier||word(child+0x90)!=actor||point(child+0x20)!=root)))
            fail("type-76 hit-latched stop did not request its separate type-8 child");
        publish();
        if(half(attack_count)||half(contact::receiver_count)!=1||word(contact::receiver_registry)!=contact::body||
           word(active_head+49*4)||half(free_count+8)!=rule.slots||half(contact::receiver+0x1C6)!=hp||
           word(actor+0x418)!=loss||word(contact::receiver+0x41C)!=loss)
            fail("type-76 next publication did not return both carrier shapes independently of child allocation");
        exit_requests=child_alloc.count;child_created=unsigned(at(child)[0]!=0);
    }
    std::printf("type76-opening: %u %u %u %u %u %u %u %u %u %u %u %u %u %08X %.4f %.4f %.4f\n",
                r.model,inlet.ordinal,inlet.scenario,socket,r.group,rule.slots,attacks,receivers,loss,hp,token,
                exit_requests,child_created,amount.arguments[0],double(root[0]),double(root[1]),double(root[2]));
}

void replay(const Module& mod,std::vector<u8> ram,Route r,unsigned scenario,const Opening* inlet=nullptr){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    auto* owner=at(actor);
    owner[0]=1;owner[0x18]=1;
    put16(owner+0x3E8,r.model);put16(owner+0x94,r.model);put32(owner+0x90,actor);
    for(unsigned offset:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+offset,1.f);
    const auto stock=0x774+8*r.channel;
    const auto initial=scenario==3?0u:2u;
    put16(owner+stock,initial);put16(owner+0x78E + 8*r.channel,initial);
    owner[r.cursor]=0x7E;owner[r.markers]=0x7E;owner[r.markers+1]=0x7E;
    CPUState pre{};pre.gpr[3]=actor;
    mod.run(pre,ram,r.pre);
    if(owner[r.cursor]||owner[r.markers]||owner[r.markers+1]||half(actor+stock)!=initial)
        fail("type-76 scoped pre-callback did not clear the ordinal and both markers");

    const auto offer=[&](unsigned count){
        if(half(free_count+6)||word(free_head+12))fail("type-76 carrier inlet was not empty");
        if(!count)return;
        put32(at(free_head+12),first);put32(at(free_tail+12),count==2?second:first);
        put16(at(free_count+6),u16(count));
        at(first)[2]=3;at(second)[2]=3;put32(at(first+4),count==2?second:0);
    };
    offer(scenario==1?0:2);
    const auto desc=scenario==2?0u:2u;
    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),desc?descriptor:0);put16(at(descriptor_row+18),u16(desc));
    put32(at(descriptor),descriptor+0x80);
    for(unsigned visit=0;visit<3;++visit){
        // This selects the exact phase-one leaf again; no controller or inter-action rearm is inferred.
        owner[0x540]=1;
        const auto old=ram;
        const bool paid=visit<initial;
        const auto ordinal=owner[r.cursor];
        const bool made=paid&&(scenario==0||(scenario==1&&visit==1));
        const auto cell=word(free_head+12);
        const auto head=word(active_head+9*4);
        const auto free=half(free_count+6),descriptors=half(descriptor_row+18);
        CPUState payment{};payment.gpr[3]=actor;
        CallObservation charge{debit_entry};
        mod.run_observed(payment,ram,r.entry,charge);
        if(charge.count!=1||charge.caller!=r.debit_site||charge.arguments[0]!=actor||
           charge.arguments[1]!=r.channel||charge.arguments[2]!=1||charge.arguments[3]!=1)
            fail("type-76 dispatch stopped calling its paid leaf");
        std::memcpy(ram.data(),old.data(),ram.size());
        CPUState request{};request.gpr[3]=actor;
        CallObservation allocation{allocate};
        mod.run_observed(request,ram,r.entry,allocation);
        if(owner[0x540]!=2||half(actor+stock)!=initial-std::min(visit+1,initial)||
           owner[r.cursor]!=(paid?ordinal^1:ordinal)||owner[r.markers]!=unsigned(initial!=0)||
           owner[r.markers+1]!=unsigned(initial&&visit>=1)||
           allocation.count!=unsigned(paid)||(paid&&(allocation.caller!=0x800FCD6C||allocation.arguments[0]!=actor||
             allocation.arguments[1]!=3||allocation.arguments[2]!=8||allocation.arguments[3]||allocation.arguments[4]!=1))||
           half(free_count+6)!=free-unsigned(made)||half(descriptor_row+18)!=descriptors-unsigned(made)||
           word(active_head+49*4))
            fail("type-76 paid debit, refusal, cursor or resource boundaries changed");
        if(made&&(!cell||word(active_head+9*4)!=cell||at(cell)[0x10]!=76||at(cell)[0x11]!=r.key||
                  at(cell)[0x13]!=ordinal||at(cell)[0x18]||word(cell+0x8C)!=actor||word(cell+0x90)!=actor))
            fail("type-76 allocated parent had a different row, ordinal or owner");
        if(!made&&word(active_head+9*4)!=head)fail("type-76 allocation refusal changed the live bucket");
        if(!inlet)
            std::printf("type76-paid: %u %u %u %u %u %u %u %u\n",r.model,scenario,visit+1,unsigned(ordinal),
                        allocation.count,unsigned(made),unsigned(owner[r.cursor]),half(actor+stock));
        if(inlet&&made&&ordinal==inlet->ordinal){
            opening_contact(mod,ram,r,cell,*inlet);
            return;
        }
        if(scenario==1&&visit==0)offer(1);
    }
    if(inlet)fail("type-76 did not allocate the requested opening ordinal");
}
}

int main(int argc,char** argv){
    const bool opening_mode=argc==4&&!std::strcmp(argv[3],"--opening");
    if(argc!=3&&!opening_mode)fail("usage: type76-paid-fixture qualified.dol unmodified-recomp.so [--opening]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(opening_mode){
        contact::HitBlock body{};
        contact::ActorSpec defender{};
        const auto read=[](auto& row){
            if(std::fread(row.data(),1,row.size(),stdin)!=row.size())fail("type-76 opening payload extent");
        };
        read(body);read(defender);
        for(const auto& route:routes){
            contact::HitBlock attack{};
            contact::ActorSpec attacker{};
            read(attack);read(attacker);
            for(unsigned ordinal=0;ordinal<2;++ordinal)
                for(unsigned i=0;i<std::size(shape_cases);++i){
                    const Opening inlet{ordinal,i,attack,body,attacker,defender};
                    replay(mod,original,route,ordinal,&inlet);
                }
        }
        if(std::getc(stdin)!=EOF)fail("unexpected type-76 opening payload");
        return 0;
    }
    for(const auto& route:routes)
        for(unsigned i=0;i<4;++i)
            replay(mod,original,route,i);
}
