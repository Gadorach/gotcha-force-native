// Original type-9 common shape admission, stationary contact and lifetime.
#include "retail_stationary_contact.hpp"
#include <array>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 declared_parent=0x81028000,effect=0x81030000,shape=0x81032000,descriptor=0x81033000;
constexpr u32 descriptor_row=0x81033800,point=0x81034000,scene=contact::scene,common=0x81048000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 factory=0x800FC6A0,update=0x800FC7C0,init=0x800FC810,alloc=0x80088AA0,bind=0x8008AE3C;
constexpr u32 presentation_frontier=0x800055FC,attack_count=0x8043B5A0-21328,attack_registry=0x803C477C;
constexpr u32 amount_table=0x802C4760;
constexpr u32 amount_record=amount_table+9*24;
constexpr u32 bucket_visit=0x80089A94,unbind_shape=0x8008AFF0,actor_bucket=8,carrier_bucket=9;
using CommonBank=std::array<u8,21504>;
struct ContactInlet {contact::HitBlock hit;contact::ActorSpec owner,defender;};
constexpr contact::Point origin{160,0,400};
constexpr float offsets[]={0,171,172,173};
enum class Mode {bind,contact,lifetime};

void stage_render_inlet(std::vector<u8>& ram){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    // Empty stage and render records, with the original type-9 animation operands.
    constexpr u32 stage=0x81050000,grid=stage+0x100,refs=stage+0x200,empty=refs+64*4;
    put32(at(0x8043B5A0-21316),stage);
    for(unsigned i=0;i<3;++i)put32(at(stage+28+4*i),grid);
    put_float(at(stage+52),1000.f);
    for(unsigned i=0;i<4;++i)put_float(at(stage+56+4*i),i%2?-1000.f:2000.f);
    for(unsigned i=0;i<6;++i)put32(at(grid+4*i),std::array<u32,6>{500,500,8,8,u32(-1000),u32(-1000)}[i]);
    put32(at(grid+24),refs);put32(at(empty),0xFFFFFFFF);
    for(unsigned i=0;i<64;++i)put32(at(refs+4*i),empty);
    constexpr u32 visual=0x81037000,link=visual+0x40,table=visual+0x80;
    put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);
    put32(at(table+4*68),visual+0x400);
    constexpr u32 audio=0x81038000;
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
    at(0x803D0000+15080+852)[0]=1;

    put32(at(0x803C4E84),actor);
    for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
}

void publish(const Module& mod,std::vector<u8>& ram){
    for(u32 entry:{0x80089554u,0x8008A28Cu,0x80089AECu}){
        CPUState pass{};mod.run(pass,ram,entry,1024);
    }
}

void offer_shape(std::vector<u8>& ram){
    *contact::at(ram,shape+2)=4;
    put32(contact::at(ram,free_head+16),shape);put32(contact::at(ram,free_tail+16),shape);
    put16(contact::at(ram,free_count+8),1);
}

void pair(const Module& mod,std::vector<u8>& ram,const ContactInlet& inlet,unsigned key,unsigned scenario){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto real=[&](u32 address){return std::bit_cast<float>(word(address));};
    const auto center=[&](u32 address){return contact::Point{real(address),real(address+4),real(address+8)};};
    stage_render_inlet(ram);
    auto target=origin;
    target[0]+=offsets[scenario];
    contact::bind_sasuke(mod,ram,target,inlet.hit,inlet.owner,inlet.defender,250);
    offer_shape(ram);
    CPUState advance{};advance.gpr[3]=effect;
    mod.run(advance,ram,update,1024);
    if(at(effect)[0x18]!=1||word(effect+0x100)!=0x80047AA4||half(effect+0x1C)||half(effect+0x1E)!=30)
        fail("type-9 full initialization did not enter its active phase");
    publish(mod,ram);
    const auto radius=real(shape+0x7C);
    const auto token=half(effect+0x26C);
    if(center(shape+0x64)!=origin||radius!=150.f||half(attack_count)!=1||word(attack_registry)!=shape||
       half(contact::receiver_count)!=1||word(contact::receiver_registry)!=contact::body||center(contact::body+0x64)!=target||
       real(contact::body+0x7C)!=22.f||at(shape)[0x11]!=9||word(shape+0x2C)!=amount_table||
       real(effect+0x58)!=real(effect+0x60)||word(shape+0x20)!=effect||word(shape+0x24)!=actor||
       token!=1||half(contact::receiver+0x20C)||at(effect)[0x1D9])
        fail("type-9 and Sasuke shapes did not publish at the declared test points");
    CPUState writer{};
    CallObservation amount{contact::amount_entry};
    const auto cut=mod.run_to_or_return(writer,ram,contact::pair_pass,{contact::hp_write},&amount);
    const bool hit=cut==contact::hp_write;
    if(amount.count!=unsigned(hit)||(hit&&(amount.caller!=0x8002F108||amount.arguments[0]!=amount_record||
       amount.arguments[1]!=effect||amount.arguments[2]!=contact::receiver||writer.lr!=0x8002F1DC||
       writer.gpr[3]!=contact::receiver))||half(contact::receiver+0x1C6)!=180)
        fail("type-9 contact did not select its indexed common amount record");
    const auto loss=hit?writer.gpr[4]:0u;
    if(hit)mod.finish_stopped_leaf(writer,contact::hp_write,164);
    const auto hp=half(contact::receiver+0x1C6);
    if(hp!=180-loss||half(actor+0x1C6)!=250||word(actor+0x418)!=loss||word(contact::receiver+0x41C)!=loss||
       half(contact::receiver+0x1C8)!=(hit?180:0xFFFF)||half(contact::receiver+0x20C)!=(hit?token:0)||
       at(contact::receiver)[0x26E]!=unsigned(hit)||at(effect)[0x1D9]!=(hit?16:0)||
       word(effect+0x290)!=(hit?contact::receiver:0)||word(contact::receiver+0x290)!=(hit?effect:0))
        fail("type-9 contact history, HP or credit diverged");
    contact::pair_hp(mod,ram,effect,amount_record,false);
    if(half(contact::receiver+0x1C6)!=hp||half(contact::receiver+0x20C)!=(hit?token:0)||
       at(contact::receiver)[0x26E]!=unsigned(hit)||word(actor+0x418)!=loss||word(contact::receiver+0x41C)!=loss)
        fail("type-9 retained registry applied the same contact twice");
    std::printf("type9-contact: %u %u %u %u %u %u %u %u %u %.5f %.5f\n",key,scenario,unsigned(offsets[scenario]),
                unsigned(radius),unsigned(hit),loss,unsigned(hp),hit?unsigned(token):0,at(shape)[0x11],
                double(real(effect+0x58)),double(real(effect+0x5C)));
}

void lifetime(const Module& mod,std::vector<u8>& ram,unsigned key,unsigned scenario){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    stage_render_inlet(ram);
    put_float(at(actor+0x1DC8),scenario==1?0.5f:1.f);
    const bool refused=scenario==2,owner_exit=scenario==3;
    const unsigned expected_exit=refused?1:owner_exit?2:31,retire=refused||owner_exit?3:33;
    if(at(effect)[1]!=carrier_bucket||word(active_head+4*carrier_bucket)!=effect)
        fail("type-9 carrier was not admitted to the reviewed actor bucket");
    unsigned published=0,shape_return=0,unbind_visit=0,phase2=0,phase3=0,carrier_return=0,descriptor_return=0;
    u32 unbind_site=0;
    for(unsigned visit=1;visit<=retire;++visit){
        if(visit==2&&owner_exit)at(actor)[0x18]=2;
        CPUState tick{};tick.gpr[3]=actor_bucket;
        CallObservation unbind{unbind_shape};
        mod.run_observed(tick,ram,bucket_visit,unbind,4096);
        publish(mod,ram);
        const bool attacking=visit<expected_exit,returned=!refused&&!attacking,retired=visit==retire;
        const unsigned phase=attacking?1:(visit==expected_exit&&!owner_exit)?2:3;
        const unsigned age=refused||owner_exit?0:visit<=expected_exit?visit-1:30;
        const bool unbound=!refused&&visit==expected_exit;
        const u32 expected_site=owner_exit?0x80084090:0x800FC9F4;
        if(half(attack_count)!=unsigned(attacking)||at(effect)[0x18]!=phase||half(effect+0x1C)!=age||
           half(free_count+8)!=unsigned(returned)||bool(at(effect)[0])==retired||
           half(free_count+6)!=unsigned(retired)||half(descriptor_row+18)!=unsigned(retired)||
           word(active_head+4*carrier_bucket)!=(retired?0:effect)||word(active_head+49*4)!=(attacking?shape:0)||
           word(free_head+16)!=(returned?shape:0)||word(free_tail+16)!=(returned?shape:0)||
           word(free_head+12)!=(retired?effect:0)||word(free_tail+12)!=(retired?effect:0)||
           word(descriptor_row+12)!=(retired?descriptor:0)||unbind.count!=unsigned(unbound)||
           half(contact::receiver_count)||(unbound&&(unbind.caller!=expected_site||unbind.arguments[0]!=effect))||
           (attacking&&(word(attack_registry)!=shape||word(shape+0x20)!=effect||word(shape+0x24)!=actor||
                       half(effect+0x26C)!=1||std::bit_cast<float>(word(shape+0x7C))!=150.f))){
            std::fprintf(stderr,"type-9 key=%u case=%u visit=%u phase=%u attack=%u shape=%u object=%u descriptor=%u\n",
                         key,scenario,visit,at(effect)[0x18],half(attack_count),half(free_count+8),half(free_count+6),
                         half(descriptor_row+18));
            fail("type-9 lifetime, registry or free-list transition changed");
        }
        if(attacking)++published;
        if(half(free_count+8)&&!shape_return)shape_return=visit;
        if(at(effect)[0x18]==2&&!phase2)phase2=visit;
        if(at(effect)[0x18]==3&&!phase3)phase3=visit;
        if(half(free_count+6)&&!carrier_return)carrier_return=visit;
        if(half(descriptor_row+18)&&!descriptor_return)descriptor_return=visit;
        if(unbind.count){unbind_visit=visit;unbind_site=unbind.caller;}
    }
    std::printf("type9-lifetime: %u %u %u %u %u %u %u %u %u %08X\n",key,scenario,published,phase2,phase3,
                unbind_visit,shape_return,carrier_return,descriptor_return,unbind_site);
}

void observe(const Module& mod,std::vector<u8> ram,const CommonBank& bank,unsigned key,bool slot,
             Mode mode=Mode::bind,unsigned scenario=0,const ContactInlet* inlet=nullptr){
    const auto at=[&](u32 address){return ram.data()+address-GC_RAM_BASE;};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return unsigned(at(address)[0])*256+at(address)[1];};
    const auto parent=mode==Mode::bind?declared_parent:actor;
    at(actor)[0]=1;at(parent)[0]=1;
    put32(at(actor+0x90),actor);put32(at(parent+0x90),actor);
    put_float(at(parent+0xB4),1.f);put_float(at(parent+0xB8),1.f);
    if(mode!=Mode::bind){
        at(actor)[0x18]=1;
        put16(at(actor+0x3E8),0x0D03);put16(at(actor+0x94),0x0D03);
        put_float(at(actor+0xC0),1.f);put_float(at(actor+0xC4),1.f);
    }
    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),descriptor);put16(at(descriptor_row+18),1);
    put32(at(free_head+12),effect);put32(at(free_tail+12),effect);put16(at(free_count+6),1);
    at(effect)[2]=3;
    for(unsigned i=0;i<origin.size();++i)put_float(at(point+4*i),origin[i]);
    put32(at(0x8043B5A0-31852),scene);put32(at(scene+152),common);
    std::memcpy(at(common),bank.data(),bank.size());
    if(slot&&mode!=Mode::contact)offer_shape(ram);
    CPUState create{};create.gpr[3]=parent;create.gpr[4]=point;create.gpr[5]=key;
    CallObservation allocate{alloc};
    mod.run_observed(create,ram,factory,allocate);
    if(allocate.count!=1||allocate.caller!=0x800FC6D8||allocate.arguments[0]!=actor||
       allocate.arguments[1]!=3||allocate.arguments[2]!=8||allocate.arguments[3]!=1||allocate.arguments[4]!=1||
       create.gpr[3]!=effect||at(effect)[0x10]!=9||at(effect)[0x11]!=key||at(effect)[0x18]||
       word(effect+0x0C)!=update||word(effect+0x8C)!=parent||word(effect+0x90)!=actor||
       std::memcmp(at(effect+0x20),at(point),12)||half(free_count+6)||half(descriptor_row+18)||
       word(active_head+49*4))
        fail("type-9 constructor did not allocate with separate parent and credit");
    if(mode==Mode::contact){
        if(!inlet)fail("type-9 missing contact inlet");
        pair(mod,ram,*inlet,key,scenario);
        return;
    }
    if(mode==Mode::lifetime){
        lifetime(mod,ram,key,scenario);
        return;
    }
    CPUState opening{};opening.gpr[3]=effect;
    CallObservation group{bind};
    const auto frontier=mod.run_to_or_return(opening,ram,init,{presentation_frontier},&group);
    if(group.count!=1||group.caller!=0x800FC870||group.arguments[0]!=actor||group.arguments[1]!=effect||
       group.arguments[2]!=84||frontier!=(slot?presentation_frontier:0)||
       at(effect)[0x18]!=(slot?1:2)||half(free_count+8)||half(attack_count)||
       word(active_head+49*4)!=(slot?shape:0))
        fail("type-9 initializer did not separate common shape admission from refusal");
    if(slot&&(opening.lr!=0x800FC8B8||word(shape+0x20)!=effect||word(shape+0x24)!=actor||
              word(shape+0x28)!=common+0x400+122*80||word(shape+0x2C)!=amount_table||
              at(shape)[0x11]!=9||half(shape+0x48)||half(effect+0x1E)!=30))
        fail("type-9 shape did not retain the common descriptor, amount index or owner");
    std::printf("type9-bind: %u %u %u %u %u %u %u\n",key,unsigned(slot),group.arguments[2],
                unsigned(frontier!=0),unsigned(at(effect)[0x18]),slot?122u:0u,half(attack_count));
}
}

int main(int argc,char** argv){
    const bool contact_mode=argc==4&&!std::strcmp(argv[3],"--contact");
    const bool lifetime_mode=argc==4&&!std::strcmp(argv[3],"--lifetime");
    if(argc!=3&&!contact_mode&&!lifetime_mode)
        fail("usage: type9-bind-fixture qualified.dol unmodified-recomp.so [--contact|--lifetime]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    CommonBank bank{};
    if(std::fread(bank.data(),1,bank.size(),stdin)!=bank.size())fail("type-9 common bank payload extent");
    if(contact_mode){
        ContactInlet inlet{};
        if(std::fread(inlet.hit.data(),1,inlet.hit.size(),stdin)!=inlet.hit.size()||
           std::fread(inlet.owner.data(),1,inlet.owner.size(),stdin)!=inlet.owner.size()||
           std::fread(inlet.defender.data(),1,inlet.defender.size(),stdin)!=inlet.defender.size()||std::getc(stdin)!=EOF)
            fail("type-9 contact payload extent");
        for(unsigned key=0;key<7;++key)
            for(unsigned i=0;i<std::size(offsets);++i)
                observe(mod,original,bank,key,true,Mode::contact,i,&inlet);
        return 0;
    }
    if(std::getc(stdin)!=EOF)fail("unexpected type-9 bind payload");
    if(lifetime_mode){
        for(unsigned key=0;key<7;++key)
            for(unsigned i=0;i<4;++i)
                observe(mod,original,bank,key,i!=2,Mode::lifetime,i);
        return 0;
    }
    for(unsigned key=0;key<7;++key)
        for(bool slot:{false,true})
            observe(mod,original,bank,key,slot);
}
