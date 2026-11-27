// Original sibling area/type-4 exits and the allocated area's follow-up.
#include "retail_stationary_contact.hpp"
#include <algorithm>

namespace {
using namespace retail_fixture;
namespace scratch=retail_stationary_contact;
struct Pilot {u16 model;u8 type,row,ordinal,area_key;u32 factory,update,contact_site,zero_hp,expiry;};
constexpr Pilot pilots[]={
    {0x0106,33,2,2,19,0x800FE668,0x800FE780,0x800FEB34,0x800FEB78,0x800FEBB8},
    {0x0601,52,0,0,11,0x80128DF8,0x80128F54,0x80129354,0x80129398,0x801293D8},
    {0x0617,52,1,2,11,0x80128DF8,0x80128F54,0x80129354,0x80129398,0x801293D8},
    {0x0D00,74,0,0,6,0x800F9400,0x800F9518,0x800F97F4,0x800F9838,0x800F9878},
    {0x0D01,77,0,0,7,0x800FB220,0x800FB330,0x800FB5DC,0x800FB620,0x800FB648},
    {0x0D05,77,1,0,7,0x800FB220,0x800FB330,0x800FB5DC,0x800FB620,0x800FB648},
};
struct Scenario {bool contact,zero_hp,expired,outside;unsigned descriptors;};
constexpr Scenario cases[]={
    {true,false,false,false,2}, {true,true,true,false,2}, {true,true,true,false,1},
    {false,true,false,false,2}, {false,false,true,false,2}, {false,true,true,true,2},
};
constexpr u32 parent=0x81030000,area_cell=0x81030800,effect_cell=0x81031000,descriptors=0x81032000;
constexpr u32 descriptor_row=0x81033000,stage=0x81040000,audio=0x81041000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 area=0x80085E00,effect=0x8001BDA8;
constexpr u32 shape=0x81034000,common_bank=0x81048000,area_bind=0x8008AE3C;
constexpr u32 attack_count=0x8043B5A0-21328,attack_registry=0x803C477C;
using CommonBank=std::array<u8,21504>;
struct Followup {
    unsigned scenario,owner_hp;
    const CommonBank& bank;
    const scratch::HitBlock& body;
    const scratch::ActorSpec& attacker;
    const scratch::ActorSpec& defender;
};
struct Group {unsigned group,descriptor,index;};
constexpr Group groups[]={ {179,246,26},{104,143,18},{104,143,18},{82,120,7},{83,121,8},{83,121,8} };
constexpr unsigned offsets[]={22,22,70,90,420};

void follow_area(const Module& mod,std::vector<u8>& ram,Pilot p,Group group,const Followup& spec,const scratch::Point& origin){
    const auto at=[&](u32 address){return scratch::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return scratch::half(ram,address);};
    const auto real=[&](u32 address){return std::bit_cast<float>(word(address));};
    const auto xyz=[&](u32 address){return scratch::Point{real(address),real(address+4),real(address+8)};};
    const bool admitted=spec.scenario!=0;
    const unsigned distance=offsets[spec.scenario];
    auto target=origin;target[0]+=float(distance);
    put32(at(0x803C4E84),actor);
    for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    scratch::bind_sasuke(mod,ram,target,spec.body,spec.attacker,spec.defender,spec.owner_hp);
    put32(at(scratch::scene+152),common_bank);
    std::memcpy(at(common_bank),spec.bank.data(),spec.bank.size());
    constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400;
    put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);
    for(unsigned id:{18u,22u,23u})put32(at(table+4*id),empty);
    const auto offer=[&]{
        if(half(free_count+8)||word(free_head+16))fail("area follow-up shape inlet is not empty");
        put32(at(free_head+16),shape);put32(at(free_tail+16),shape);put16(at(free_count+8),1);at(shape)[2]=4;
    };
    if(admitted)offer();
    unsigned first=0,hp_loss=0,token=0,first_publish=0,parent_retired=0,unlinked=0,retired=0,bound_visits=0;
    float opening_radius=0,first_radius=0,hit_radius=0,peak_radius=0;
    for(unsigned n=1;n<=64;++n){
        if(n==2&&!admitted)offer();
        CPUState tick{};tick.gpr[3]=8;
        CallObservation bind{area_bind};
        mod.run_observed(tick,ram,0x80089A94,bind,4096);
        if(bind.count!=unsigned(n==1)||(n==1&&(bind.caller!=0x800860E0||bind.arguments[0]!=actor||
           bind.arguments[1]!=area_cell||bind.arguments[2]!=group.group||at(area_cell)[0x18]!=(admitted?1:2))))
            fail("area follow-up did not bind the original common group or retried after refusal");
        for(u32 entry:{0x80089554u,0x8008A28Cu,0x80089AECu}){
            CPUState pass{};mod.run(pass,ram,entry,1024);
        }
        const bool has_shape=word(active_head+49*4)==shape, published=half(attack_count)!=0;
        if(half(scratch::receiver_count)!=1||word(scratch::receiver_registry)!=scratch::body||
           xyz(scratch::body+0x64)!=target||real(scratch::body+0x7C)!=22.f||
           half(free_count+8)!=unsigned(!has_shape&&(admitted||n>1))||half(attack_count)>1||
           (published&&(!has_shape||word(attack_registry)!=shape))||(!admitted&&has_shape)||
           (n==1&&(published||has_shape!=admitted)))
            fail("area follow-up registry, target or shape refusal changed");
        if(has_shape){
            const float r=real(shape+0x7C);
            if(word(shape+0x20)!=area_cell||word(shape+0x24)!=actor||word(shape+0x2C)!=0x802C4760||
               word(shape+0x28)!=common_bank+0x400+80*group.descriptor||at(shape)[0x11]!=group.index||
               xyz(shape+0x64)!=origin||half(area_cell+0x26C)!=1)
                fail("area follow-up did not publish the credited token, descriptor or origin");
            token=half(area_cell+0x26C);
            if(n==1)opening_radius=r;
            if(published&&!first_publish){first_publish=n;first_radius=r;}
            peak_radius=std::max(peak_radius,r);
            ++bound_visits;
        }else if(bound_visits&&!unlinked)unlinked=n;
        const bool hit=published&&!first&&float(distance)<real(shape+0x7C)+22.f;
        const unsigned loss=scratch::pair_hp(mod,ram,area_cell,0x802C4760+24*group.index,hit);
        if(hit){
            first=n;hp_loss=loss;hit_radius=real(shape+0x7C);
            if(word(scratch::receiver+0x290)!=area_cell||word(area_cell+0x290)!=scratch::receiver||
               half(scratch::receiver+0x1C8)!=180||at(area_cell)[0x1D9]!=16)
                fail("area follow-up HP write did not belong to the area");
            scratch::pair_hp(mod,ram,area_cell,0x802C4760+24*group.index,false);
        }
        if(half(scratch::receiver+0x1C6)!=180-hp_loss||half(actor+0x1C6)!=spec.owner_hp||
           half(scratch::receiver+0x20C)!=(first?token:0)||at(scratch::receiver)[0x26E]!=unsigned(first!=0)||
           word(actor+0x418)!=hp_loss||word(scratch::receiver+0x41C)!=hp_loss||
           (at(parent)[0]&&word(parent+0x290))||
           (at(area_cell)[0]&&(word(area_cell+0x90)!=actor||word(area_cell+0x8C)!=parent||xyz(area_cell+0x20)!=origin)))
            fail("area follow-up HP or ledger changed on a miss, duplicate or parent visit");
        if(!parent_retired&&!at(parent)[0]){
            parent_retired=n;
            if(!at(area_cell)[0]||half(free_count+6)!=1||half(descriptor_row+18)!=1||
               word(active_head+9*4)!=area_cell||(admitted&&!has_shape))
                fail("area follow-up did not keep the live area when its parent returned");
        }
        if(!retired&&!at(area_cell)[0])retired=n;
        if(parent_retired&&retired)break;
    }
    if(!parent_retired||!retired||word(active_head+9*4)||half(free_count+6)!=2||half(descriptor_row+18)!=3||
       half(free_count+8)!=1||half(attack_count)||(admitted&&!unlinked))
        fail("area follow-up did not return its shape, parent and descriptors separately");
    std::printf("area-followup: %u %u %u %u %u %u %u %u %u %u %u %u %.5f %.5f %.5f %.5f\n",
                p.model,spec.scenario,p.area_key,distance,first_publish,first,hp_loss,half(scratch::receiver+0x1C6),token,
                parent_retired,unlinked,retired,double(opening_radius),double(first_radius),double(hit_radius),double(peak_radius));
}

void observe(const Module& mod,std::vector<u8> ram,Pilot p,unsigned scenario,const Followup* follow=nullptr,
             const Group* group=nullptr){
    const auto at=[&](u32 address){return scratch::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return scratch::half(ram,address);};
    const auto real=[&](u32 address){return std::bit_cast<float>(word(address));};
    const auto xyz=[&](u32 address){return scratch::Point{real(address),real(address+4),real(address+8)};};
    const auto place=[&](u32 address,const scratch::Point& pos){
        for(unsigned i=0;i<3;++i)put_float(at(address+4*i),pos[i]);
    };
    const auto s=cases[scenario];
    auto* owner=at(actor);
    owner[0]=1;owner[0x18]=1;
    put16(owner+0x3E8,p.model);put16(owner+0x94,p.model);put32(owner+0x90,actor);put16(owner+0x1C6,150);
    for(unsigned offset:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+offset,1.f);
    put32(at(0x8043B5A0-31852),scratch::scene);
    put32(at(0x8043B5A0-21316),stage);
    put_float(at(stage+52),500.f);
    for(unsigned i=0;i<4;++i)put_float(at(stage+56+4*i),i%2?-1000.f:2000.f);
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;at(0x803D0000+15080+852)[0]=1;

    put32(at(0x80436380),descriptor_row);
    put32(at(descriptor_row+12),descriptors);put16(at(descriptor_row+18),u16(s.descriptors+1));
    for(unsigned i=0;i<s.descriptors;++i)put32(at(descriptors+0x80*i),descriptors+0x80*(i+1));
    put32(at(free_head+12),parent);put32(at(free_tail+12),area_cell);put16(at(free_count+6),2);
    at(parent)[2]=3;at(area_cell)[2]=3;put32(at(parent+4),area_cell);
    put32(at(free_head+8),effect_cell);put32(at(free_tail+8),effect_cell);put16(at(free_count+4),1);at(effect_cell)[2]=2;
    CPUState make{};make.gpr[3]=actor;
    make.gpr[4]=(p.type==52||p.type==74)?p.ordinal:p.row;
    make.gpr[5]=p.type==52?p.row:p.type==74?1:p.ordinal;
    make.gpr[6]=p.type==52?0xFFFFFFFFu:0;
    mod.run(make,ram,p.factory,1024);
    if(!at(parent)[0]||at(parent)[0x10]!=p.type||at(parent)[0x11]!=p.row||at(parent)[0x13]!=p.ordinal||at(parent)[0x18]||
       word(parent+0x0C)!=p.update||word(parent+0x90)!=actor||word(parent+0x8C)!=actor||
       half(free_count+6)!=1||half(free_count+4)!=1||half(descriptor_row+18)!=s.descriptors||
       word(active_head+9*4)!=parent||word(active_head+49*4))
        fail("area sibling constructor lost its row, ordinal or owner");

    // Deliberately declared active inlet. No opening bind or contact is simulated.
    const scratch::Point root{s.outside?8000.f:200.f,100.f,500.f},hit{260.f,120.f,580.f};
    place(parent+0x20,root);place(parent+0x2A0,hit);
    at(parent)[0x18]=1;at(parent)[0x1D9]=s.contact?16:0;
    put16(at(parent+0x1C),s.expired?0:5);put16(at(parent+0x1C6),s.zero_hp?0:10);
    const auto callee=s.outside?0:s.contact?area:effect;
    const unsigned key=s.contact?p.area_key:0;
    const bool use_event=s.contact&&p.type!=77;
    const auto saved=ram;
    unsigned position_offset=0;
    if(callee){
        CPUState request{};request.gpr[3]=parent;
        CallObservation source{callee};
        mod.run_to(request,ram,p.update,{callee},&source);
        const auto expected_pos=parent+(use_event?0x2A0:0x20);
        if(source.count!=1||source.caller!=(s.contact?p.contact_site:s.zero_hp?p.zero_hp:p.expiry)||
           source.arguments[0]!=parent||source.arguments[1]!=expected_pos||source.arguments[2]!=key||
           xyz(expected_pos)!=(use_event?hit:root)||at(parent)[0x18]!=2||at(parent)[0x1D8]!=1)
            fail("area sibling exit did not close before the chosen factory and point");
        position_offset=source.arguments[1]-parent;
    }
    std::memcpy(ram.data(),saved.data(),ram.size());
    CPUState close{};close.gpr[3]=parent;
    CallObservation wrong_factory{s.contact?effect:area};
    mod.run_observed(close,ram,p.update,wrong_factory,1024);
    const bool area_made=s.contact&&s.descriptors>=2,effect_made=!s.contact&&!s.outside;
    const unsigned type=callee==area?5:callee==effect?4:0;
    const unsigned cost=2*unsigned(area_made)+unsigned(effect_made);
    if(wrong_factory.count||at(parent)[0x18]!=2||at(parent)[0x1D8]!=1||half(actor+0x1C6)!=150||
       half(free_count+6)!=1-unsigned(area_made)||half(free_count+4)!=1-unsigned(effect_made)||
       half(descriptor_row+18)!=s.descriptors-cost||word(active_head+49*4)||
       bool(at(area_cell)[0])!=area_made||bool(at(effect_cell)[0])!=effect_made){
        std::fprintf(stderr,"model=%04X case=%u unexpected request count=%u phase=%u/%u stock=%u/%u/%u active=%u/%u\n",
                     p.model,scenario,wrong_factory.count,at(parent)[0x18],at(parent)[0x1D8],
                     half(free_count+6),half(free_count+4),half(descriptor_row+18),at(area_cell)[0],at(effect_cell)[0]);
        fail("area sibling allocation, suppression or pool refusal changed");
    }
    if(area_made||effect_made){
        const auto child=area_made?area_cell:effect_cell;
        if(at(child)[0x10]!=type||at(child)[0x11]!=key||at(child)[0x18]||
           word(child+0x8C)!=parent||word(child+0x90)!=actor||xyz(child+0x20)!=(use_event?hit:root))
            fail("area sibling child did not retain the requested key, origin and separate credit");
    }
    if(follow){
        if(!area_made||!group)fail("area follow-up did not reach a successful area allocation");
        follow_area(mod,ram,p,*group,*follow,use_event?hit:root);
        return;
    }
    std::printf("area-exit: %u %u %u %u %u %u %u %u %u %u %u\n",p.model,scenario,p.type,p.row,p.ordinal,type,key,
                unsigned(area_made||effect_made),half(descriptor_row+18),at(parent)[0x18],position_offset);
}
}

int main(int argc,char** argv){
    const bool follow_mode=argc==4&&!std::strcmp(argv[3],"--follow");
    if(argc!=3&&!follow_mode)fail("usage: type5-sibling-exit-fixture qualified.dol unmodified-recomp.so [--follow]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(follow_mode){
        CommonBank bank{};
        scratch::HitBlock body{};
        scratch::ActorSpec defender{};
        const auto read=[](auto& input){
            if(std::fread(input.data(),1,input.size(),stdin)!=input.size())fail("area follow-up payload extent");
        };
        read(bank);read(body);read(defender);
        constexpr unsigned hp[]={220,550,550,150,150,150};
        for(unsigned p=0;p<std::size(pilots);++p){
            scratch::ActorSpec attacker{};
            read(attacker);
            for(unsigned i=0;i<std::size(offsets);++i){
                const Followup inlet{i,hp[p],bank,body,attacker,defender};
                observe(mod,original,pilots[p],0,&inlet,&groups[p]);
            }
        }
        if(std::getc(stdin)!=EOF)fail("unexpected area follow-up payload");
        return 0;
    }
    for(const auto& p:pilots)
        for(unsigned i=0;i<std::size(cases);++i)
            observe(mod,original,p,i);
}
