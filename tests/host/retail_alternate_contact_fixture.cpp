// Original collision reset and the separate role-0x80/role-0x02 contact list.
#include "retail_actor_frame_admission.hpp"

namespace {
using namespace retail_contact_pose_clock;
namespace admission=retail_actor_frame_admission;
constexpr u32 reset=0x8003CC24,bind=0x8008AC80,alt_pass=0x8002DF08;
constexpr u32 attacker_hit_bank=0x81035000,shape=0x81039000,descriptor_pool=0x81033A00;
constexpr u32 alt_attack_count=0x8043B5A0-21344,alt_receiver_count=0x8043B5A0-21346;
constexpr u32 alt_attack_list=0x803C177C,alt_receiver_list=0x803C117C;
constexpr u32 mask=0x272,gate=0x276;
struct Route {unsigned model,hp,seed,loss;u32 table;};
constexpr Route routes[]{
    {0x0600,500,8,26,0x8030A398},
    {0x0401,350,4,7,0x802F5B38},
};

void reset_probe(const Module& mod,std::vector<u8> ram,unsigned slot){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    put32(at(0x8043B5A0-31852),contact::scene);
    put32(at(contact::scene+128),attacker_hit_bank);
    put32(at(contact::scene+128+5*4),contact::body_block);
    at(actor+0x3E7)[0]=u8(slot);
    put32(at(actor+0x3FC),0xDEAD0000);
    put32(at(actor+admission::eligibility_flags),admission::close_bit);
    put16(at(actor+0x1C6),123);
    at(actor+timer)[0]=9;
    at(actor+0x2AF)[0]=3;
    for(auto o:{0x1D9u,0x1DAu,0x1DBu,0x1DCu,0x1DDu,0x1DEu,0x1EBu,0x26Eu,0x274u,0x276u,0x280u,0x703u,0x704u})
        at(actor+o)[0]=0x7F;
    put32(at(actor+0x1E0),0x8103A000);put32(at(actor+0x1E4),0x8103B000);
    put16(at(actor+0x26C),77);put16(at(actor+mask),0xFFFF);
    put16(at(actor+0x20A),0xA55A);put16(at(actor+0x270),0xC123);
    for(unsigned i=0;i<48;++i)put16(at(actor+0x20C+2*i),u16(i+1));
    CPUState init{};init.gpr[3]=actor;
    mod.run(init,ram,reset);
    for(unsigned i=0;i<48;++i)
        if(half(actor+0x20C+2*i))fail("collision reset did not clear its whole token history");
    for(auto o:{0x1D9u,0x1DAu,0x1DBu,0x1DCu,0x1DDu,0x1DEu,0x1EBu,0x26Eu,0x274u,0x276u,0x280u,0x703u,0x704u})
        if(at(actor+o)[0])fail("collision reset left a declared latch behind");
    if(word(actor+0x3FC)!=(slot==0?attacker_hit_bank:contact::body_block)||word(actor+0x1E0)||word(actor+0x1E4)||
       word(actor+admission::eligibility_flags)!=admission::close_bit||half(actor+0x26C)||half(actor+mask)!=0xEF||
       half(actor+0x20A)!=0xA55A||half(actor+0x270)!=0xC123||
       at(actor+timer)[0]!=9||at(actor+0x2AF)[0]!=3||half(actor+0x1C6)!=123)
        fail("collision reset changed clocks, HP, or the slot-specific hit bank");
    std::printf("alternate-reset: %u %u %u %u %u %u %u\n",slot,48u,half(actor+mask),at(actor+gate)[0],
                at(actor+timer)[0],at(actor+0x2AF)[0],half(actor+0x1C6));
}

void route_probe(const Module& mod,std::vector<u8> ram,const Route& route,const contact::HitBlock& receiver_hit,
                 const contact::ActorSpec& defender,const contact::HitBlock& attack_hit,const contact::ActorSpec& attacker){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    constexpr u32 body=contact::receiver,audio=0x8103E000;
    constexpr contact::Point point{200.f,150.f,325.f};
    put32(at(0x80436380),descriptor_pool);
    contact::bind_sasuke(mod,ram,point,receiver_hit,attacker,defender,route.hp);
    put32(at(contact::scene+128),attacker_hit_bank);
    std::memcpy(at(attacker_hit_bank),attack_hit.data(),attack_hit.size());
    CPUState init{};init.gpr[3]=actor;
    mod.run(init,ram,reset);
    if(word(actor+0x3FC)!=attacker_hit_bank||half(actor+mask)!=0xEF||at(actor+gate)[0])
        fail("alternate contact did not begin at the original collision defaults");
    at(actor)[0]=1;at(actor+0x18)[0]=1;
    put32(at(0x803C4E84),actor);
    for(unsigned i=2;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
    put16(at(actor+0x3E8),u16(route.model));
    put32(at(actor+0x27C),route.table);put32(at(actor+0x76C),1);
    put_float(at(actor+0xC0),1.f);
    for(unsigned i=0;i<3;++i){
        put_float(at(actor+0x8D4+12+16*i),point[i]);put_float(at(actor+0x8D4+20*i),1.f);
        put_float(at(actor+0x20+4*i),point[i]);
    }
    const auto record=route.table+4*24;
    const auto descriptor=attacker_hit_bank+0xAA0+44*80;
    if(be32(at(attacker_hit_bank+0xA20+4*26))!=0x2CFFFFFF||
       be32(at(descriptor))!=0x00000080||half(descriptor+4)!=4||at(record+8)[0]!=route.seed)
        fail("alternate contact group, mask or amount seed changed");
    at(shape)[2]=4;
    put32(at(contact::free_head+16),shape);put32(at(contact::free_tail+16),shape);
    put16(at(contact::free_count+8),1);
    CPUState make{};make.gpr[3]=actor;make.gpr[4]=26;
    mod.run(make,ram,bind);
    if(make.gpr[3]!=1||!at(shape)[0]||half(contact::free_count+8)||
       word(shape+0x20)!=actor||word(shape+0x24)!=actor||
       word(shape+0x28)!=descriptor||word(shape+0x2C)!=route.table)
        fail("alternate contact lost its original direct body-bound shape");
    put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
    at(0x803D0000+15080+852)[0]=1;
    for(unsigned i=0;i<5;++i){
        // Both inhibitors are independent. The final visit reopens the
        // same cells; no new shape bind or token is supplied.
        const bool role_excluded=i==0||i==2,late_closed=i==1;
        if(i)put16(at(actor+mask),u16(role_excluded?0xEF:0x6F));
        const bool receiving=i!=3;
        if(i>=3)put16(at(body+mask),u16(receiving?0:2));
        put32(at(actor+admission::eligibility_flags),late_closed?admission::close_bit:0);
        CPUState late{};late.gpr[3]=actor;
        mod.run(late,ram,0x80055EE4);
        if(at(actor+gate)[0]!=unsigned(late_closed))fail("alternate contact late eligibility projection");
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState publish{};mod.run(publish,ram,entry,1024);
        }
        const bool publishing=i>=3,hit=publishing&&receiving;
        if(half(admission::attack_count)||half(contact::receiver_count)!=unsigned(receiving)||
           half(alt_receiver_count)!=unsigned(receiving)||(receiving&&word(alt_receiver_list)!=contact::body)||
           half(alt_attack_count)!=unsigned(publishing)||(publishing&&word(alt_attack_list)!=shape)||
           !at(contact::body)[0]||!at(shape)[0]||at(shape+0x10)[0]!=1||
           half(contact::free_count+8)||half(actor+0x26C)!=1||word(shape+0x28)!=descriptor||
           word(shape+0x64)!=std::bit_cast<u32>(point[0])||word(shape+0x7C)!=std::bit_cast<u32>(10.f))
            fail("alternate contact role publication changed the original shape or wrong list");
        CPUState regular{};CallObservation wrong{contact::amount_entry};
        mod.run_observed(regular,ram,contact::pair_pass,wrong,4096);
        if(wrong.count||half(body+0x1C6)!=180||half(body+0x20C))
            fail("ordinary pair read the separate alternate attack list");
        CPUState pair{};CallObservation amount{contact::amount_entry};
        mod.run_observed(pair,ram,alt_pass,amount,4096);
        const auto lost=hit?route.loss:0;
        if(amount.count!=unsigned(hit)||(hit&&(amount.caller!=0x8002F108||amount.arguments[0]!=record||
           amount.arguments[1]!=actor||amount.arguments[2]!=body))||
           half(body+0x1C6)!=180-lost||word(actor+0x418)!=lost||word(body+0x41C)!=lost||
           half(body+0x20C)!=unsigned(hit)||at(body+timer)[0]!=(hit?route.seed:0)||at(actor+timer)[0]!=(hit?route.seed:0)){
            std::fprintf(stderr,"alternate pair model=%04X case=%u health=%u timer=%u token=%u calls=%u record=%08X\n",
                         route.model,i,half(body+0x1C6),at(body+timer)[0],half(body+0x20C),amount.count,amount.arguments[0]);
            fail("alternate pair eligibility or accepted result changed");
        }
        std::printf("alternate-pair: %u %u %u %u %u %u %u %u %u %u %u %u %u\n",route.model,i,half(actor+mask),
                    at(actor+gate)[0],half(body+mask),half(admission::attack_count),half(alt_attack_count),half(alt_receiver_count),
                    half(actor+0x26C),amount.count,lost,half(body+0x1C6),at(body+timer)[0]);
    }
    CPUState repeat{};CallObservation amount{contact::amount_entry};
    mod.run_observed(repeat,ram,alt_pass,amount,4096);
    if(amount.count||half(body+0x1C6)!=180-route.loss||half(body+0x20C)!=1||word(body+0x41C)!=route.loss)
        fail("alternate pair re-used the retained token");
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: alternate-contact-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    const auto read=[](auto& row){
        if(std::fread(row.data(),1,row.size(),stdin)!=row.size())fail("alternate contact input extent");
    };
    reset_probe(mod,original,0);
    reset_probe(mod,original,5);
    contact::HitBlock hit{};contact::ActorSpec body{};
    read(hit);read(body);
    for(const auto& route:routes){
        contact::HitBlock attack{};contact::ActorSpec owner{};
        read(attack);read(owner);
        route_probe(mod,original,route,hit,body,attack,owner);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("unexpected alternate contact input");
}
