// Exact opcode-10 contacts offered to a retained normal replacement body.
#pragma once
#include "retail_stationary_contact.hpp"
#include <cstdint>

namespace retail_body_entry_contact {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
struct Attack {contact::HitBlock hit;contact::ActorSpec spec;};
using Inputs=std::array<Attack,3>;
struct Route {u16 model,hp;u8 group,step,joint,index,frame,role;u32 command,table;};
inline constexpr std::array<Route,3> routes{{
    {0x0807,60,1,2,1,1,10,1,0x80377E40,0x80377AC0},
    {0x0900,150,3,5,1,3,22,128,0x802C2B3A,0x802D6930},
    {0x0902,150,3,5,1,3,16,128,0x803481B2,0x80348910},
}};
inline constexpr u32 body=contact::receiver,attack_bank=0x8105C000,attack_spec=0x8105E000;
inline constexpr u32 vm_cursor=0x8105E200,attack_cell=0x8105E400,old_area=0x81031000;
inline constexpr u32 ordinary_count=0x8043B5A0-21328,alternate_count=0x8043B5A0-21344;
inline constexpr u32 alternate_receive_count=0x8043B5A0-21346,alternate_receive_list=0x803C117C;
inline constexpr u32 ordinary_attack_list=0x803C477C,alternate_attack_list=0x803C177C;

inline unsigned receiving(std::vector<u8>& ram,u32 count,u32 list,u32 first,u32 stride){
    const auto n=contact::half(ram,count);
    unsigned mask=0;
    if(n>16)fail("entry contact receiver extent");
    for(unsigned i=0;i<n;++i){
        const auto cell=be32(contact::at(ram,list+4*i));
        if(cell<first||cell>=first+16*stride||(cell-first)%stride||
           be32(contact::at(ram,cell+0x20))!=body)
            fail("entry contact receiver crossed replacement cells");
        const auto ordinal=(cell-first)/stride;
        if(mask&(1u<<ordinal))fail("entry contact duplicated a receiving cell");
        mask|=1u<<ordinal;
    }
    return mask;
}

inline void observe(const Module& mod,std::vector<u8> ram,const Attack& raw,unsigned route_id,
                    unsigned initial_caster,unsigned visit,unsigned relation,u32 first,u32 stride){
    const auto& r=routes[route_id];
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto scalar=[&](u32 a){return std::bit_cast<float>(word(a));};
    const auto call=[&](u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;mod.run(cpu,ram,entry,4096);
    };
    const auto rebuild=[&](){call(0x8008A28C);call(0x80089AEC);};
    // The earlier KO area is not the attacker in this fork. Mark it by the
    // original close helper, then let the shape walk detach it.
    if(at(old_area)[0])call(0x8008AFF0,old_area);
    rebuild();
    if(half(ordinary_count)||half(alternate_count)||half(body+0x1C6)!=220||
       half(body+0x20C)||half(body+0x3E8)!=0x0106||at(body+0x18)[0]!=1||
       word(body+0x41C)!=16||half(body+0x774)!=10||half(body+0x784)!=3)
        fail("entry contact fork must start from the cleared replacement inlet");
    const auto guard=word(body+0x720);
    const auto role_mask=half(body+0x272);
    const u32 target_shape=first+9*stride;
    if(!at(target_shape)[0]||word(target_shape+0x20)!=body||at(target_shape+0x12)[0]!=9)
        fail("entry contact lost a known replacement body shape");
    // Declared caster and pose; only target HP/mirror (not guard or mask)
    // is set for a nonlethal damage and visible healing observation.
    std::memset(at(actor),0,0x1E00);
    put32(at(0x803C4E84),actor);
    at(actor)[0]=1;at(actor+0x18)[0]=1;at(actor+0x88)[0]=relation?0:at(body+0x88)[0];
    put16(at(actor+0x3E8),r.model);
    put16(at(actor+0x1C4),r.hp);put16(at(actor+0x1C6),r.hp);
    put32(at(actor+0x90),actor);
    put32(at(actor+0x27C),r.table);
    put32(at(actor+0x3FC),attack_bank);
    put32(at(actor+0x4AC),attack_spec);
    std::memcpy(at(attack_bank),raw.hit.data(),raw.hit.size());
    std::memcpy(at(attack_spec),raw.spec.data(),raw.spec.size());
    for(u32 field:{0xB4u,0xB8u,0xC0u,0xC4u})put_float(at(actor+field),1.f);
    put_float(at(actor+0x1CDC),float(r.frame));
    put32(at(actor+0x76C),1u<<r.joint);
    put16(at(body+0x1C6),110);
    put16(at(0x803B069E),110);put16(at(0x803B069C),r.hp);
    const u32 d=attack_bank+0xAA0+80*r.step,matrix=actor+0x8D4+48*r.joint;
    if(at(r.command)[0]!=10||at(r.command+1)[0]!=r.group||at(d)[0]||at(d+1)[0]!=r.joint||
       half(d+2)!=r.role||half(d+4)!=r.index||half(d+6)!=r.frame)
        fail("entry contact lost the indexed queue command or descriptor");
    for(unsigned i=0;i<3;++i){
        put_float(at(matrix+20*i),1.f);
        put_float(at(matrix+12+16*i),scalar(target_shape+0x64+4*i)-scalar(d+12+4*i));
        put_float(at(actor+0x20+4*i),scalar(target_shape+0x64+4*i));
    }
    const auto available=half(contact::free_count+8);
    const auto offered=word(contact::free_head+16);
    at(attack_cell+2)[0]=4;put32(at(attack_cell+4),offered);
    put32(at(contact::free_head+16),attack_cell);
    if(!offered)put32(at(contact::free_tail+16),attack_cell);
    put16(at(contact::free_count+8),u16(available+1));
    put32(at(vm_cursor),r.command);
    CPUState script{};script.gpr[3]=actor;script.gpr[4]=15;script.gpr[5]=vm_cursor;
    CallObservation bind{0x8008AC80};
    mod.run_observed(script,ram,0x8004C6B4,bind,4096);
    if(bind.count!=1||bind.caller!=0x8004C6F0||bind.arguments[0]!=actor||
       bind.arguments[1]!=r.group||word(vm_cursor)!=r.command+4||half(contact::free_count+8)!=available||
       word(attack_cell+0x20)!=actor||word(attack_cell+0x24)!=actor||word(attack_cell+0x28)!=d)
        fail("entry contact VM did not bind the original attack cell");
    rebuild();
    const bool alt=r.role==128;
    const auto ordinary=receiving(ram,contact::receiver_count,contact::receiver_registry,first,stride);
    const auto alternate=receiving(ram,alternate_receive_count,alternate_receive_list,first,stride);
    const auto source_token=half(actor+0x26C);
    if(half(ordinary_count)!=unsigned(!alt)||half(alternate_count)!=unsigned(alt)||
       word(alt?alternate_attack_list:ordinary_attack_list)!=attack_cell||!source_token)
        fail("entry contact reached the wrong attack registry or token");
    const auto pair_entry=alt?0x8002DF08:contact::pair_pass;
    auto fork=ram;
    CPUState pair{};CallObservation amount{contact::amount_entry};
    const auto cut=mod.run_to_or_return(pair,fork,pair_entry,{contact::hp_write},&amount,4096);
    const bool writer=cut==contact::hp_write;
    if((cut&&!writer)||(writer&&(pair.lr!=0x8002F1DC||pair.gpr[3]!=body))||
       contact::half(fork,body+0x1C6)!=110||contact::half(fork,body+0x272)!=role_mask||
       be32(contact::at(fork,body+0x720))!=guard||
       (amount.count&&(amount.count!=1||amount.caller!=0x8002F108||
        amount.arguments[0]!=r.table+24*r.index||amount.arguments[1]!=actor||amount.arguments[2]!=body)))
        fail("entry contact changed the amount or HP target inlet");
    const auto requested=writer?std::bit_cast<std::int32_t>(pair.gpr[4]):0;
    CPUState full{};CallObservation writes{contact::hp_write};
    mod.run_observed(full,ram,pair_entry,writes,4096);
    if(writes.count!=unsigned(writer)||(writer&&(writes.caller!=0x8002F1D8||writes.first_arguments[0]!=body||
       writes.second_arguments[0]!=u32(requested)))||word(body+0x720)!=guard){
        std::fprintf(stderr,"entry contact %u/%u/%04X/%u writer=%u/%u call=%08X target=%08X amount=%d/%d guard=%08X/%08X mask=%04X/%04X\n",
                     initial_caster,visit,r.model,relation,unsigned(writer),writes.count,writes.caller,writes.first_arguments[0],
                     requested,std::bit_cast<std::int32_t>(writes.second_arguments[0]),guard,word(body+0x720),role_mask,half(body+0x272));
        fail("entry contact wrote an unrelated HP target or modified the receiving gates");
    }
    const auto hp_after=half(body+0x1C6),token=half(body+0x20C);
    CPUState repeat{};CallObservation repeated{contact::hp_write};
    mod.run_observed(repeat,ram,pair_entry,repeated,4096);
    if(token!=(writer?source_token:0)||repeated.count||half(body+0x1C6)!=hp_after||half(body+0x20C)!=token||
       half(body+0x774)!=10||half(body+0x784)!=3||word(body+0x720)!=guard)
        fail("entry contact re-used a token or spent receiver stock");
    rebuild();
    for(unsigned i=0;i<16;++i){
        const auto cell=first+i*stride;
        if(!at(cell)[0]||word(cell+0x20)!=body||word(cell+0x28)!=word(body+0x3FC)+32+80*i)
            fail("entry contact's receiving mask retired or rebound an actor cell");
    }
    const auto next_ordinary=receiving(ram,contact::receiver_count,contact::receiver_registry,first,stride);
    const auto next_alternate=receiving(ram,alternate_receive_count,alternate_receive_list,first,stride);
    std::printf("entry-contact: %u %u %04X %u %04X %08X %04X %04X %u %u %d %u %u %u %u %u %04X %04X %04X\n",
                initial_caster,visit,r.model,relation,role_mask,guard,ordinary,alternate,amount.count,
                writes.count,requested,hp_after,token,word(actor+0x418),word(actor+0x42C),word(body+0x41C),
                half(body+0x272),next_ordinary,next_alternate);
}
} // namespace retail_body_entry_contact
