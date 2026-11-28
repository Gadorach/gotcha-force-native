// Original ordinary sphere versus capsule pairs, with singleton unedited receiver descriptors.
#include "retail_body_entry_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace entry=retail_body_entry_contact;
constexpr u32 broad_phase=0x80039F6C,sphere_sphere=0x80030500,sphere_capsule=0x80030A3C;
constexpr u32 capsule_capsule=0x80031E50,relation=0x8002FD7C;
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
struct Outcome {unsigned coarse,narrow,amount,requested,hp,token;};

Outcome resolve(const Module& mod,std::vector<u8>& ram,unsigned initial_hp,u32 kernel,bool swap=false,
                bool verify_current_receiver=false){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto call=[&](u32 fn,u32 a=0,u32 b=0,u32 c=0){
        CPUState cpu{};cpu.gpr[3]=a;cpu.gpr[4]=b;cpu.gpr[5]=c;
        mod.run(cpu,ram,fn,4096);return cpu.gpr[3];
    };
    constexpr u32 attack=entry::attack_cell,receive=contact::body,target=contact::receiver;
    if(half(entry::ordinary_count)!=1||word(entry::ordinary_attack_list)!=attack||half(entry::alternate_count)||
       half(contact::receiver_count)!=1||word(contact::receiver_registry)!=receive||
       half(entry::alternate_receive_count)!=1||word(entry::alternate_receive_list)!=receive||
       word(attack+0x88)!=std::bit_cast<u32>(70.f)||half(actor+0x26C)!=1||
       half(target+0x20C)||half(target+0x1C6)!=initial_hp)
        fail("capsule pair did not retain its original attack and receiving roles");
    const auto accepted_relation=call(relation,attack,receive,2);
    const auto coarse=call(broad_phase,attack+0x58,receive+0x58);
    const auto stored_bound=word(receive+0x7C),core_radius=word(receive+0x88);
    auto fork=ram;
    CPUState pair{};CallObservation dispatch{kernel};
    const auto cut=mod.run_to_or_return(pair,fork,contact::pair_pass,{kernel},&dispatch,4096);
    const u32 expected_lr=swap?0x800304E4:0x800304F0;
    if(!accepted_relation||cut!=(coarse?kernel:0)||dispatch.count!=coarse||
       (coarse&&(dispatch.caller!=expected_lr-4||pair.lr!=expected_lr||
        pair.gpr[3]!=(swap?0x803B0368u:0x803B04D0u)||pair.gpr[4]!=(swap?0x803B04D0u:0x803B0368u)))){
        std::fprintf(stderr,"pair kernel=%08X relation=%u coarse=%u cut=%08X calls=%u caller=%08X lr=%08X args=%08X/%08X/%u\n",
                     kernel,accepted_relation,coarse,cut,dispatch.count,dispatch.caller,pair.lr,
                     pair.gpr[3],pair.gpr[4],pair.gpr[5]);
        fail("capsule pair disagreed with the original broad phase or narrow dispatch operands");
    }
    unsigned narrow=0;
    if(coarse){
        if(verify_current_receiver){
            const auto scratch=swap?pair.gpr[3]:pair.gpr[4];
            for(unsigned axis=0;axis<3;++axis){
                if(word(target+0x1CC+4*axis))
                    fail("receiver motion needs the unshifted original narrow-phase inlet");
                for(u32 p:{0x64u,0x8Cu,0x98u}){
                    if(p!=0x64&&word(receive+0x58)!=2)continue;
                    if(be32(contact::at(fork,scratch+p-0x58+4*axis))!=word(receive+p+4*axis))
                        fail("receiver motion narrow kernel did not receive the current body geometry");
                }
            }
        }
        CPUState n{};n.gpr[3]=pair.gpr[3];n.gpr[4]=pair.gpr[4];
        mod.run(n,fork,kernel,4096);narrow=n.gpr[3]!=0;
    }
    std::memcpy(fork.data(),ram.data(),ram.size());
    CPUState request{};CallObservation amount{contact::amount_entry};
    const auto writer=mod.run_to_or_return(request,fork,contact::pair_pass,{contact::hp_write},&amount,4096);
    if(writer!=(narrow?contact::hp_write:0)||amount.count!=narrow||
       (narrow&&(amount.caller!=0x8002F108||amount.arguments[0]!=0x80377AD8||
        amount.arguments[1]!=actor||amount.arguments[2]!=target||request.gpr[3]!=target||request.lr!=0x8002F1DC))||
       contact::half(fork,target+0x1C6)!=initial_hp)
        fail("capsule narrow result did not predict the exact original HP path");
    const unsigned requested=writer?request.gpr[4]:0;
    CPUState full{};CallObservation writes{contact::hp_write};
    mod.run_observed(full,ram,contact::pair_pass,writes,4096);
    const auto hp=half(target+0x1C6),token=half(target+0x20C);
    if(writes.count!=narrow||(narrow&&(writes.caller!=0x8002F1D8||writes.first_arguments[0]!=target||
       writes.second_arguments[0]!=requested))||requested>=initial_hp||hp+requested!=initial_hp||token!=narrow||
       word(actor+0x418)!=requested||word(target+0x41C)!=requested)
        fail("capsule pair did not complete the original hit and request ledgers");
    CPUState next{};CallObservation repeated{contact::hp_write};
    mod.run_observed(next,ram,contact::pair_pass,repeated,4096);
    if(repeated.count||half(target+0x1C6)!=hp||half(target+0x20C)!=token||
       word(receive+0x7C)!=stored_bound||word(receive+0x88)!=core_radius)
        fail("capsule pair repeated an accepted HP write or changed the allocated geometry");
    return {coarse,narrow,amount.count,requested,hp,token};
}

void bind(const Module& mod,std::vector<u8>& ram,const entry::Attack& tau,const entry::Attack& hurt,
          const contact::StationaryBody& body,const contact::Point& center,const contact::Point& attack_center){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    put32(at(0x80436380),0x81033A00);
    put32(at(0x8043B5A0-31856),0x8103E000);at(0x8103E000+12)[0]=1;
    at(0x803D3AE8+852)[0]=1;
    contact::bind_stationary(mod,ram,center,hurt.hit,tau.spec,hurt.spec,60,body);
    for(unsigned i=2;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    entry::bind_attack_at(mod,ram,tau,entry::routes[0],1,attack_center,actor);
}

void rebuild(const Module& mod,std::vector<u8>& ram){
    for(u32 fn:{0x8008A28C,0x80089AEC}){
        CPUState cpu{};mod.run(cpu,ram,fn,4096);
    }
}

void probe(const Module& mod,std::vector<u8> ram,const entry::Attack& tau,const entry::Attack& receiver_assets,
           unsigned id,bool updated){
    const auto spec=cases[id];
    const auto body=receivers[spec.body];
    constexpr contact::Point center{200,150,325};
    auto attack_center=center;
    attack_center[spec.axis]+=float(spec.axis_offset);
    bind(mod,ram,tau,receiver_assets,body,center,attack_center);
    rebuild(mod,ram);
    if(updated)rebuild(mod,ram);
    const auto word=[&](u32 a){return be32(contact::at(ram,a));};
    constexpr u32 attack=entry::attack_cell,receive=contact::body;
    if(word(attack+0x58)!=0||word(receive+0x58)!=2||word(attack+0x7C)!=std::bit_cast<u32>(70.f))
        fail("capsule pair did not retain the original sphere and its capsule receiver");
    for(unsigned axis=0;axis<3;++axis)
        if(word(attack+0x64+axis*4)!=std::bit_cast<u32>(attack_center[axis])||
           word(receive+0x64+axis*4)!=std::bit_cast<u32>(center[axis]))
            fail("capsule pair did not retain its declared exact axis separation");
    const auto out=resolve(mod,ram,body.hp,sphere_capsule);
    std::printf("capsule-pair: %u %u %04X %u %u %u %08X %08X %u %u %u %u %u %u\n",
                id,unsigned(updated),body.model,body.step,spec.axis,spec.axis_offset,word(receive+0x7C),
                word(receive+0x88),out.coarse,out.narrow,out.amount,out.requested,out.hp,out.token);
}

struct Motion {bool capsule;unsigned axis;float first,last,side;unsigned closed=0;};
constexpr Motion motions[]{
    {false,0,0,.25f,0}, {false,0,0,.5f,0}, {false,0,0,.75f,0},
    {false,0,-120,120,0}, {false,0,-120,120,91}, {false,0,-120,120,92}, {false,0,-120,120,93},
    {true,0,-120,120,0}, {true,2,-120,120,0}, {true,2,-120,120,76},
    {true,2,-120,120,77}, {true,2,-120,120,78},
    {false,0,-120,120,0,1}, {false,0,-120,120,0,2},
};

void motion(const Module& mod,std::vector<u8> ram,const entry::Attack& tau,const entry::Attack& sasuke,unsigned id){
    const auto m=motions[id];
    const auto body=m.capsule?receivers[0]:contact::sasuke;
    constexpr contact::Point center{200,150,325};
    auto point=center;
    point[m.axis]+=m.first;
    point[1]+=m.side;
    bind(mod,ram,tau,sasuke,body,center,point);
    constexpr u32 attack=entry::attack_cell,rcv=contact::body;
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    for(unsigned visit=0;visit<3;++visit){
        if(visit==1){
            point[m.axis]+=m.last-m.first;
            const u32 mtx=actor+0x8D4+entry::routes[0].joint*0x30;
            const u32 loc=mtx+12+16*m.axis;
            put_float(at(loc),std::bit_cast<float>(word(loc))+m.last-m.first);
            put_float(at(actor+0x20+4*m.axis),point[m.axis]);
        }
        const auto closed=visit==1?m.closed:0;
        put16(at(actor+0x272),u16(closed==1));
        at(actor+0x276)[0]=u8(closed==2);
        rebuild(mod,ram);
        const auto kind=word(attack+0x58);
        if((kind!=0&&kind!=2)||word(rcv+0x58)!=(m.capsule?2u:0u)||
           word(attack+0x28)!=entry::attack_bank+0xAA0+80*2||word(attack+0x20)!=actor||word(attack+0x24)!=actor)
            fail("sphere motion changed owner, descriptor, or receiver kind");
        const auto kernel=kind==2?(m.capsule?capsule_capsule:sphere_capsule):(m.capsule?sphere_capsule:sphere_sphere);
        auto fork=ram;
        Outcome out{};
        if(closed){
            CPUState refused{};CallObservation hits{contact::hp_write};
            mod.run_observed(refused,fork,contact::pair_pass,hits,4096);
            if(contact::half(fork,entry::ordinary_count)||contact::half(fork,contact::receiver_count)!=1||
               hits.count||contact::half(fork,contact::receiver+0x1C6)!=body.hp||
               contact::half(fork,contact::receiver+0x20C)||contact::half(fork,actor+0x26C)!=1)
                fail("sphere sweep escaped its independent attack publication gate");
            out.hp=body.hp;
        }else out=resolve(mod,fork,body.hp,kernel,kind==2&&!m.capsule);
        std::printf("sphere-motion: %u %u %u %u %08X %08X",id,visit,kind,at(attack+0x13)[0],
                    word(attack+0x7C),word(attack+0x88));
        for(u32 field:{0x64u,0x70u,0x8Cu,0x98u})
            for(unsigned axis=0;axis<3;++axis)std::printf(" %08X",word(attack+field+4*axis));
        std::printf(" %u %08X %u %u %u %u %u %u\n",unsigned(!closed),out.coarse?kernel:0,
                    out.coarse,out.narrow,out.amount,out.requested,out.hp,out.token);
    }
}

struct Paths {float attack_start,attack_end,receiver_start,receiver_end;};
constexpr Paths paths[]{
    {0,0,-120,120}, {-120,120,120,-120}, {-120,120,120,-360},
    {-120,120,-240,0}, {-120,120,120,360},
};

void moving_receiver(const Module& mod,std::vector<u8> ram,const entry::Attack& tau,const entry::Attack& sasuke,
                     unsigned id,bool capsule){
    const auto p=paths[id];
    const auto body=capsule?receivers[0]:contact::sasuke;
    const contact::Point from_attack{200+p.attack_start,150,325},from_receiver{200+p.receiver_start,150,325};
    bind(mod,ram,tau,sasuke,body,from_receiver,from_attack);
    constexpr u32 attack=entry::attack_cell,receive=contact::body;
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto move_x=[&](u32 owner,unsigned joint,float from,float to){
        const u32 loc=owner+0x8D4+0x30*joint+12;
        put_float(at(loc),std::bit_cast<float>(word(loc))+to-from);
        put_float(at(owner+0x20),200+to);
    };
    for(unsigned visit=0;visit<3;++visit){
        if(visit==1){
            move_x(actor,entry::routes[0].joint,p.attack_start,p.attack_end);
            move_x(contact::receiver,body.joint,p.receiver_start,p.receiver_end);
        }
        rebuild(mod,ram);
        const auto kind=word(attack+0x58);
        if((kind!=0&&kind!=2)||word(receive+0x58)!=(capsule?2u:0u)||at(receive+0x13)[0]||
           word(attack+0x28)!=entry::attack_bank+0xAA0+80*2||word(attack+0x20)!=actor||
           word(receive+0x28)!=contact::body_block+32+80*body.step||word(receive+0x20)!=contact::receiver)
            fail("moving receiver changed its distinct body kind, descriptor or ownership");
        const auto kernel=kind==2?(capsule?capsule_capsule:sphere_capsule):(capsule?sphere_capsule:sphere_sphere);
        auto fork=ram;
        const auto out=resolve(mod,fork,body.hp,kernel,kind==2&&!capsule,true);
        std::printf("moving-receiver: %u %u %u %u %u %u %u",unsigned(capsule),id,visit,kind,word(receive+0x58),
                    at(attack+0x13)[0],at(receive+0x13)[0]);
        for(u32 value:{word(attack+0x64),word(attack+0x70),word(attack+0x7C),word(receive+0x64),word(receive+0x70),
                       word(receive+0x7C),word(receive+0x88),capsule?word(receive+0x8C):0,
                       capsule?word(receive+0x98):0,capsule?word(receive+0xD4):0,capsule?word(receive+0xE0):0})
            std::printf(" %08X",value);
        std::printf(" %08X %u %u %u %u %u %u\n",out.coarse?kernel:0,out.coarse,out.narrow,out.amount,
                    out.requested,out.hp,out.token);
    }
}
}

int main(int argc,char** argv){
    const bool moving=argc==4&&!std::strcmp(argv[3],"--moving-sphere");
    const bool receiver=argc==4&&!std::strcmp(argv[3],"--moving-receiver");
    if(argc!=3&&!moving&&!receiver)
        fail("usage: capsule-pair-fixture qualified.dol unmodified-recomp.so [--moving-sphere|--moving-receiver] < qualified-roles");
    entry::Attack tau{},sasuke{},large{};
    for(auto* a:{&tau,&sasuke,&large}){
        if(std::fread(a->hit.data(),1,a->hit.size(),stdin)!=a->hit.size()||
           std::fread(a->spec.data(),1,a->spec.size(),stdin)!=a->spec.size())
            fail("capsule pair input extent");
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra capsule pair inputs");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    if(moving)
        for(unsigned i=0;i<std::size(motions);++i)motion(mod,original,tau,sasuke,i);
    else if(receiver)
        for(bool capsule:{false,true})
            for(unsigned i=0;i<std::size(paths);++i)moving_receiver(mod,original,tau,sasuke,i,capsule);
    else
        for(unsigned i=0;i<std::size(cases);++i)
            for(bool updated:{false,true})probe(mod,original,tau,cases[i].body?large:sasuke,i,updated);
}
