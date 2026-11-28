// Original ordinary sphere versus capsule pairs, with singleton unedited receiver descriptors.
#include "retail_body_entry_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace entry=retail_body_entry_contact;
constexpr u32 broad_phase=0x80039F6C,sphere_capsule=0x80030A3C,relation=0x8002FD7C;
constexpr contact::StationaryBody receivers[]{
    {0x000A,180,11,6,0x802D4160,{-12,0,0}},
    {0x0506,2000,6,12,0x802FEF08,{0,-50,0}},
    {0x0506,2000,2,4,0x802FEF08,{-190,0,0}},
};
struct Case {unsigned body,axis,axis_offset;};
constexpr Case cases[]{
    {0,0,0}, {0,1,76},{0,1,77},{0,1,78},
    {0,0,91},{0,0,92},{0,0,93},{0,0,553},{0,0,554},{0,0,555},
    {1,0,0},{2,0,0},
};

void probe(const Module& mod,std::vector<u8> ram,const entry::Attack& tau,const entry::Attack& receiver_assets,
           unsigned id,bool updated){
    const auto spec=cases[id];
    const auto body=receivers[spec.body];
    constexpr contact::Point center{200,150,325};
    auto attack_center=center;
    attack_center[spec.axis]+=float(spec.axis_offset);
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto call=[&](u32 fn,u32 a=0,u32 b=0,u32 c=0){
        CPUState cpu{};cpu.gpr[3]=a;cpu.gpr[4]=b;cpu.gpr[5]=c;
        mod.run(cpu,ram,fn,4096);return cpu.gpr[3];
    };
    const auto rebuild=[&](){call(0x8008A28C);call(0x80089AEC);};
    put32(at(0x80436380),0x81033A00);
    put32(at(0x8043B5A0-31856),0x8103E000);at(0x8103E000+12)[0]=1;
    at(0x803D3AE8+852)[0]=1;
    contact::bind_stationary(mod,ram,center,receiver_assets.hit,tau.spec,receiver_assets.spec,60,body);
    for(unsigned i=2;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    entry::bind_attack_at(mod,ram,tau,entry::routes[0],1,attack_center,actor);
    rebuild();
    if(updated)rebuild();
    constexpr u32 attack=entry::attack_cell,receive=contact::body,target=contact::receiver;
    if(half(entry::ordinary_count)!=1||word(entry::ordinary_attack_list)!=attack||half(entry::alternate_count)||
       half(contact::receiver_count)!=1||word(contact::receiver_registry)!=receive||
       half(entry::alternate_receive_count)!=1||word(entry::alternate_receive_list)!=receive||
       word(attack+0x58)!=0||word(receive+0x58)!=2||word(attack+0x7C)!=std::bit_cast<u32>(70.f)||
       word(attack+0x88)!=std::bit_cast<u32>(70.f)||half(actor+0x26C)!=1)
        fail("capsule pair did not retain the original sphere and its capsule receiver");
    for(unsigned axis=0;axis<3;++axis)
        if(word(attack+0x64+axis*4)!=std::bit_cast<u32>(attack_center[axis])||
           word(receive+0x64+axis*4)!=std::bit_cast<u32>(center[axis]))
            fail("capsule pair did not retain its declared exact axis separation");
    const auto accepted_relation=call(relation,attack,receive,2);
    const auto coarse=call(broad_phase,attack+0x58,receive+0x58);
    const auto stored_bound=word(receive+0x7C),core_radius=word(receive+0x88);
    auto fork=ram;
    CPUState pair{};CallObservation dispatch{sphere_capsule};
    const auto cut=mod.run_to_or_return(pair,fork,contact::pair_pass,{sphere_capsule},&dispatch,4096);
    if(!accepted_relation||cut!=(coarse?sphere_capsule:0)||dispatch.count!=coarse||
       (coarse&&(dispatch.caller!=0x800304EC||pair.lr!=0x800304F0||
        pair.gpr[3]!=0x803B04D0||pair.gpr[4]!=0x803B0368))){
        std::fprintf(stderr,"capsule pair %u/%u relation=%u coarse=%u cut=%08X calls=%u caller=%08X lr=%08X args=%08X/%08X/%u\n",
                     id,unsigned(updated),accepted_relation,coarse,cut,dispatch.count,dispatch.caller,pair.lr,
                     pair.gpr[3],pair.gpr[4],pair.gpr[5]);
        fail("capsule pair disagreed with the original broad phase or narrow dispatch operands");
    }
    unsigned narrow=0;
    if(coarse){
        CPUState n{};n.gpr[3]=pair.gpr[3];n.gpr[4]=pair.gpr[4];
        mod.run(n,fork,sphere_capsule,4096);narrow=n.gpr[3]!=0;
    }
    std::memcpy(fork.data(),ram.data(),ram.size());
    CPUState request{};CallObservation amount{contact::amount_entry};
    const auto writer=mod.run_to_or_return(request,fork,contact::pair_pass,{contact::hp_write},&amount,4096);
    if(writer!=(narrow?contact::hp_write:0)||amount.count!=narrow||
       (narrow&&(amount.caller!=0x8002F108||amount.arguments[0]!=0x80377AD8||
        amount.arguments[1]!=actor||amount.arguments[2]!=target||request.gpr[3]!=target||request.lr!=0x8002F1DC))||
       contact::half(fork,target+0x1C6)!=body.hp)
        fail("capsule narrow result did not predict the exact original HP path");
    const unsigned requested=writer?request.gpr[4]:0;
    CPUState full{};CallObservation writes{contact::hp_write};
    mod.run_observed(full,ram,contact::pair_pass,writes,4096);
    const auto hp=half(target+0x1C6),token=half(target+0x20C);
    if(writes.count!=narrow||(narrow&&(writes.caller!=0x8002F1D8||writes.first_arguments[0]!=target||
       writes.second_arguments[0]!=requested))||requested>=body.hp||hp+requested!=body.hp||token!=narrow||
       word(actor+0x418)!=requested||word(target+0x41C)!=requested)
        fail("capsule pair did not complete the original hit and request ledgers");
    CPUState next{};CallObservation repeated{contact::hp_write};
    mod.run_observed(next,ram,contact::pair_pass,repeated,4096);
    if(repeated.count||half(target+0x1C6)!=hp||half(target+0x20C)!=token||
       word(receive+0x7C)!=stored_bound||word(receive+0x88)!=core_radius)
        fail("capsule pair repeated an accepted HP write or changed the allocated geometry");
    std::printf("capsule-pair: %u %u %04X %u %u %u %08X %08X %u %u %u %u %u %u\n",
                id,unsigned(updated),body.model,body.step,spec.axis,spec.axis_offset,stored_bound,
                core_radius,coarse,narrow,amount.count,requested,hp,token);
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: capsule-pair-fixture qualified.dol unmodified-recomp.so < qualified-roles");
    entry::Attack tau{},sasuke{},large{};
    for(auto* a:{&tau,&sasuke,&large}){
        if(std::fread(a->hit.data(),1,a->hit.size(),stdin)!=a->hit.size()||
           std::fread(a->spec.data(),1,a->spec.size(),stdin)!=a->spec.size())
            fail("capsule pair input extent");
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra capsule pair inputs");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    for(unsigned i=0;i<std::size(cases);++i)
        for(bool updated:{false,true})probe(mod,original,tau,cases[i].body?large:sasuke,i,updated);
}
