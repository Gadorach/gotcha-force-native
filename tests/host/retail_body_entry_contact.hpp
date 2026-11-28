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
inline constexpr u32 heal_effect_cell=0x8105E800,heal_effect_bucket=26,view_pool=0x81063000;
inline constexpr u32 heal_storage=0x81069000,common_owner=0x81064020;
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

inline void require_heal_returned(std::vector<u8>& ram,u32 payload,u32 view){
    const auto word=[&](u32 a){return be32(contact::at(ram,a));};
    constexpr u32 arena=0x81600000,limit=0x81700000,storage_pool=view_pool+3*28;
    auto cur=word(arena+8);
    for(unsigned i=0;cur;++i){
        if(i>=(limit-arena)/32||cur<arena+32||cur>=limit||(cur&31))
            fail("heal effect allocation list escaped its declared heap");
        if(cur==payload-32)fail("heal effect heap block remained live after retirement");
        cur=word(cur+4);
    }
    bool returned=false;
    cur=word(arena+4);
    for(unsigned i=0;cur;++i){
        if(i>=(limit-arena)/32||cur<arena+32||cur>=limit||(cur&31))
            fail("heal effect free list escaped its declared heap");
        const auto size=word(cur+8);
        if(size>limit-cur)fail("heal effect free block escaped its declared heap");
        if(cur<=payload-32&&cur+size>=payload+32*92){returned=true;break;}
        cur=word(cur+4);
    }
    if(word(heal_storage+12)||word(storage_pool+12)!=heal_storage||word(view_pool+12)!=view||
       word(contact::free_head+8)!=heal_effect_cell||word(contact::free_tail+8)!=heal_effect_cell||!returned)
        fail("heal effect did not return its exact original storage, view and carrier cells");
}

inline void bind_attack(const Module& mod,std::vector<u8>& ram,const Attack& raw,const Route& r,
                        unsigned relation,u32 target_shape,u32 caster,u32 cell=attack_cell){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto scalar=[&](u32 a){return std::bit_cast<float>(word(a));};
    std::memset(at(caster),0,0x1E00);
    put32(at(0x803C4E84),caster);
    at(caster)[0]=1;at(caster+0x18)[0]=1;at(caster+0x88)[0]=relation?0:at(body+0x88)[0];
    put16(at(caster+0x3E8),r.model);
    put16(at(caster+0x1C4),r.hp);put16(at(caster+0x1C6),r.hp);
    put32(at(caster+0x90),caster);
    put32(at(caster+0x27C),r.table);
    put32(at(caster+0x3FC),attack_bank);
    put32(at(caster+0x4AC),attack_spec);
    std::memcpy(at(attack_bank),raw.hit.data(),raw.hit.size());
    std::memcpy(at(attack_spec),raw.spec.data(),raw.spec.size());
    for(u32 field:{0xB4u,0xB8u,0xC0u,0xC4u})put_float(at(caster+field),1.f);
    put_float(at(caster+0x1CDC),float(r.frame));
    put32(at(caster+0x76C),1u<<r.joint);
    put16(at(0x803B069C),r.hp);
    const u32 d=attack_bank+0xAA0+80*r.step,matrix=caster+0x8D4+48*r.joint;
    if(at(r.command)[0]!=10||at(r.command+1)[0]!=r.group||at(d)[0]||at(d+1)[0]!=r.joint||
       half(d+2)!=r.role||half(d+4)!=r.index||half(d+6)!=r.frame)
        fail("entry contact lost the indexed queue command or descriptor");
    for(unsigned i=0;i<3;++i){
        put_float(at(matrix+20*i),1.f);
        put_float(at(matrix+12+16*i),scalar(target_shape+0x64+4*i)-scalar(d+12+4*i));
        put_float(at(caster+0x20+4*i),scalar(target_shape+0x64+4*i));
    }
    auto available=half(contact::free_count+8);
    const auto offered=word(contact::free_head+16);
    if(offered!=cell){
        at(cell+2)[0]=4;put32(at(cell+4),offered);
        put32(at(contact::free_head+16),cell);
        if(!offered)put32(at(contact::free_tail+16),cell);
        put16(at(contact::free_count+8),++available);
    }
    put32(at(vm_cursor),r.command);
    CPUState script{};script.gpr[3]=caster;script.gpr[4]=15;script.gpr[5]=vm_cursor;
    CallObservation bind{0x8008AC80};
    mod.run_observed(script,ram,0x8004C6B4,bind,4096);
    if(!available||bind.count!=1||bind.caller!=0x8004C6F0||bind.arguments[0]!=caster||
       bind.arguments[1]!=r.group||word(vm_cursor)!=r.command+4||half(contact::free_count+8)!=available-1||
       word(cell+0x20)!=caster||word(cell+0x24)!=caster||word(cell+0x28)!=d)
        fail("entry contact VM did not bind the original attack cell");
}

inline void observe(const Module& mod,std::vector<u8>& ram,const Attack& raw,unsigned route_id,
                    unsigned initial_caster,unsigned visit,unsigned relation,u32 first,u32 stride,bool report=true,
                    bool effect_admitted=false){
    const auto& r=routes[route_id];
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
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
    put16(at(body+0x1C6),110);
    put16(at(0x803B069E),110);
    bind_attack(mod,ram,raw,r,relation,target_shape,actor);
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
    if(!report){
        std::memcpy(fork.data(),ram.data(),ram.size());
        CPUState effect{};
        const auto inlet=mod.run_to_or_return(effect,fork,pair_entry,{0x8010E158},nullptr,4096);
        if(inlet!=0x8010E158||effect.lr!=0x8002F6E0||effect.gpr[3]!=body||effect.gpr[4]!=1||
           contact::half(fork,body+0x1C6)!=160||contact::half(fork,body+0x272)!=role_mask||
           contact::half(fork,contact::free_count+4)!=unsigned(effect_admitted))
            fail("replacement heal tail changed its post-HP effect request or capacity inlet");
    }
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
    if(report)std::printf("entry-contact: %u %u %04X %u %04X %08X %04X %04X %u %u %d %u %u %u %u %u %04X %04X %04X\n",
                initial_caster,visit,r.model,relation,role_mask,guard,ordinary,alternate,amount.count,
                writes.count,requested,hp_after,token,word(actor+0x418),word(actor+0x42C),word(body+0x41C),
                half(body+0x272),next_ordinary,next_alternate);
}

inline void defeat_effect(const Module& mod,std::vector<u8> ram,const Attack& tau,unsigned initial_caster,u32 first,u32 stride){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto call=[&](u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;mod.run(cpu,ram,entry,4096);
    };
    const auto rebuild=[&](){call(0x8008A28C);call(0x80089AEC);};
    constexpr u32 caster=0x81052000,cell=0x8105E600;
    constexpr u32 walk=0x8002BB14,begin=0x8005BBC0,presentation=0x8005A580,knock_reaction=0x8005C290;
    constexpr u32 wrap=0x80054070;
    if(half(body+0x1C6)!=160||half(body+0x20C)!=3||word(body+0x8C4)!=2||word(body+0x720)||half(body+0x272)||
       at(heal_effect_cell+0x18)[0]!=1||word(heal_effect_cell+0xDC)!=heal_storage||
       half(contact::receiver_count)!=9||half(alternate_receive_count)!=9)
        fail("heal exit needs the admitted effect and expired original guard");
    const auto previous_actor=word(0x803C4E84);
    const auto payload=word(heal_storage+12),view=word(heal_effect_cell+0xE0);
    const auto saved=ram;
    for(unsigned lethal:{0u,1u}){
        std::memcpy(ram.data(),saved.data(),ram.size());
        const unsigned initial_hp=lethal?7:160;
        put16(at(body+0x1C6),u16(initial_hp));
        bind_attack(mod,ram,tau,routes[0],1,first+9*stride,caster,cell);
        rebuild();
        if(half(ordinary_count)!=1||word(ordinary_attack_list)!=cell||half(alternate_count)||
           half(caster+0x26C)!=4)
            fail("heal exit did not publish one fresh ordinary attack token");
        put32(at(0x803C4E84),previous_actor); // Only the loaded receiver runs its actor callbacks.
        auto fork=ram;
        CPUState writer{};CallObservation amount{contact::amount_entry};
        const auto inlet=mod.run_to_or_return(writer,fork,walk,{contact::hp_write},&amount,65536);
        if(inlet!=contact::hp_write||writer.lr!=0x8002F1DC||writer.gpr[3]!=body||
           amount.count!=1||amount.arguments[0]!=0x80377AD8||amount.arguments[1]!=caster||amount.arguments[2]!=body||
           contact::half(fork,body+0x1C6)!=initial_hp||be32(contact::at(fork,body+0x8C4))!=2)
            fail("heal exit reached the wrong original amount or HP inlet");
        const auto requested=writer.gpr[4];
        const unsigned expected_request=lethal?15:16,remaining_hp=lethal?0:144;
        if(requested!=expected_request||amount.caller!=0x8002F108)
            fail("heal exit did not reach its exact low or normal HP amount route");
        std::memcpy(fork.data(),ram.data(),ram.size());
        CPUState phase{};CallObservation starts{begin};
        const auto frontier=mod.run_to_or_return(phase,fork,walk,{begin,presentation,knock_reaction,0x80264290},&starts,65536);
        if(frontier!=(lethal?begin:knock_reaction)||phase.lr!=(lethal?0x8002C6E0u:0x8002C6C4u)||
           phase.gpr[3]!=body||starts.count!=lethal||contact::half(fork,body+0x1C6)!=remaining_hp||
           be32(contact::at(fork,body+0x8C4))!=2||contact::at(fork,body+0x18)[0]!=1)
            fail("heal exit did not retain the effect bit until the post-HP defeat helper");
        std::printf("heal-exit-inlet: %u %u %u %u %08X %08X %u %u %u\n",initial_caster,lethal,initial_hp,
                    requested,frontier,phase.lr,starts.count,contact::half(fork,body+0x1C6),
                    be32(contact::at(fork,body+0x8C4)));
        CPUState state{};CallObservation began{begin};
        const auto cut=mod.run_to_or_return(state,ram,walk,{presentation,0x80264290},&began,65536);
        const unsigned phase_after=lethal?2:1,owner_after=lethal?0:2,role_after=lethal?0x443:0;
        if(cut!=presentation||state.lr!=0x8002C870||state.gpr[3]!=body||began.count!=lethal||
           (lethal&&(began.caller!=0x8002C6DC||began.arguments[0]!=body))||
           half(body+0x1C6)!=remaining_hp||at(body+0x18)[0]!=phase_after||word(body+0x8C4)!=owner_after||
           half(body+0x272)!=role_after||word(body+0x41C)!=16+requested||word(caster+0x418)!=requested||
           half(body+0x20C)!=3||half(body+0x20E)!=4||at(heal_effect_cell)[0]!=1||at(heal_effect_cell+0x18)[0]!=1||
           word(heal_effect_cell+0xDC)!=heal_storage||word(heal_storage+12)!=payload)
            fail("heal exit defeat helper changed the wrong role, credit, or resource boundary");
        std::printf("heal-exit-contact: %u %u %08X %08X %u %u %u %u %u %04X %u %u %u\n",
                    initial_caster,lethal,cut,state.lr,began.count,half(body+0x1C6),at(body+0x18)[0],word(body+0x8C4),
                    at(heal_effect_cell+0x18)[0],half(body+0x272),word(body+0x41C),half(body+0x20E),word(caster+0x418));
        call(0x8008AFF0,caster);
        rebuild();
        const auto receive=[&](){
            const auto ordinary=receiving(ram,contact::receiver_count,contact::receiver_registry,first,stride);
            const auto alternate=receiving(ram,alternate_receive_count,alternate_receive_list,first,stride);
            if(ordinary!=(lethal?0u:0x3EF0u)||alternate!=ordinary||half(ordinary_count)||half(alternate_count))
                fail("heal exit mixed body registry closure with effect lifetime");
        };
        receive();
        for(unsigned n=1;n<=(lethal?20u:4u);++n){
            CPUState actor_update{};actor_update.gpr[3]=body;
            const auto actor_cut=mod.run_to_or_return(actor_update,ram,wrap,{presentation,0x80264290},nullptr,65536);
            if(actor_cut!=presentation||actor_update.lr!=0x80054144)
                fail("heal exit next visit did not reach its original actor prefix");
            call(0x80089A94,heal_effect_bucket);
            rebuild();
            receive();
            const bool retired=lethal&&n==20;
            const unsigned effect_phase=!lethal?1:n<19?2:3;
            if(at(body+0x18)[0]!=phase_after||half(body+0x1C6)!=remaining_hp||word(body+0x8C4)!=owner_after||
               word(body+0x41C)!=16+requested||half(body+0x20C)!=3||half(body+0x20E)!=4||
               at(heal_effect_cell)[0]!=unsigned(!retired)||at(heal_effect_cell+0x18)[0]!=effect_phase||
               word(heal_effect_cell+0xDC)!=(retired?0:heal_storage)||half(contact::free_count+4)!=unsigned(retired)||
               half(view_pool+18)!=unsigned(retired)||half(view_pool+3*28+18)!=unsigned(retired)||
               word(0x803BB374)!=common_owner||word(0x803BB4C0+3120+0x600)!=0x81064000)
                fail("heal exit lost its original effect retirement or body accounting");
            if(retired)require_heal_returned(ram,payload,view);
            if(n<=2||n==4||n>=18)
                std::printf("heal-exit-tail: %u %u %u %u %u %u %u %u %u %u %u\n",initial_caster,lethal,n,
                            at(body+0x18)[0],word(body+0x8C4),at(heal_effect_cell)[0],at(heal_effect_cell+0x18)[0],
                            half(contact::free_count+4),half(view_pool+18),half(contact::receiver_count),half(alternate_receive_count));
        }
    }
}

inline void follow_heal(const Module& mod,std::vector<u8> ram,const Attack& raw,unsigned initial_caster,
                         unsigned initial_visit,u32 first,u32 stride,bool effect_admitted=false,const Attack* exit_toward=nullptr){
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto call=[&](u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;mod.run(cpu,ram,entry,4096);
    };
    const auto rebuild=[&](){call(0x8008A28C);call(0x80089AEC);};
    const auto previous_actor=word(0x803C4E84);
    if(effect_admitted){
        if(half(contact::free_count+4)||word(contact::free_head+8)||half(view_pool+18)!=1)
            fail("heal effect needs one spare original view and an isolated carrier pool");
        at(heal_effect_cell+2)[0]=2;
        put32(at(contact::free_head+8),heal_effect_cell);
        put32(at(contact::free_tail+8),heal_effect_cell);
        put16(at(contact::free_count+4),1);
    }
    observe(mod,ram,raw,1,initial_caster,initial_visit,1,first,stride,false,effect_admitted);
    if(half(body+0x272)!=0x43||half(body+0x1C6)!=160||half(body+0x20C)!=3||
       half(contact::receiver_count)||half(alternate_receive_count)||half(contact::free_count+4)||
       word(body+0x8C4)!=(effect_admitted?2u:0u))
        fail("replacement heal tail did not retain its accepted HP and closed role mask");
    if(effect_admitted&&(word(contact::active_head+4*heal_effect_bucket)!=heal_effect_cell||
       at(heal_effect_cell)[0]!=1||at(heal_effect_cell+0x10)[0]!=21||at(heal_effect_cell+0x11)[0]!=1||
       at(heal_effect_cell+0x18)[0]||word(heal_effect_cell+0x8C)!=body||word(heal_effect_cell+0x90)!=word(body+0x90)||
       word(heal_effect_cell+0xC)!=0x8010E2A8||word(heal_effect_cell+0x54)!=0x80325BD0||half(view_pool+18))){
        std::fprintf(stderr,"heal effect alloc head=%08X kind=%u bucket=%u type=%u/%u phase=%u owner=%08X/%08X data=%08X view=%u\n",
                     word(contact::active_head+4*heal_effect_bucket),at(heal_effect_cell+2)[0],at(heal_effect_cell+1)[0],
                     at(heal_effect_cell+0x10)[0],at(heal_effect_cell+0x11)[0],at(heal_effect_cell+0x18)[0],
                     word(heal_effect_cell+0x8C),word(heal_effect_cell+0x90),word(heal_effect_cell+0x54),half(view_pool+18));
        fail("heal effect factory did not allocate its exact type, selector and target owner");
    }
    call(0x8008AFF0,actor);
    rebuild();
    put32(at(0x803C4E84),previous_actor);
    if(at(attack_cell)[0]||half(ordinary_count)||half(alternate_count))
        fail("replacement heal tail did not close its disposable attacking shape");
    const auto saved=ram;
    constexpr u32 wrap=0x80054070,presentation=0x8005A580,pad=0x803C727C+32,bomb=0x800E058C;
    for(unsigned rule=0;rule<2;++rule){
        if(!effect_admitted&&rule&&initial_visit==138)break;
        std::memcpy(ram.data(),saved.data(),ram.size());
        const unsigned initial_guard=rule&&!effect_admitted?unsigned(initial_visit==139):139-initial_visit;
        if(rule&&!effect_admitted)put_float(at(body+0x720),float(initial_guard));
        if(word(body+0x720)!=std::bit_cast<u32>(float(initial_guard)))
            fail("replacement heal tail did not begin at the retained original guard");
        // A discarded fresh input fork measures the exact debit boundary,
        // not an added bomb in the retained neutral follow-up.
        auto input=ram;
        put32(contact::at(input,pad),0x200);
        CPUState b{};b.gpr[3]=body;
        CallObservation poll_b{0x80056900};
        const auto cut=mod.run_to_or_return(b,input,wrap,{bomb,presentation,0x80264290},&poll_b,65536);
        const auto roles_at_factory=contact::half(input,body+0x272);
        if(cut!=bomb||b.lr!=0x80130584||b.gpr[3]!=body||b.gpr[4]!=2||b.gpr[5]!=10||
           poll_b.count!=1||poll_b.caller!=0x80054CD0||poll_b.arguments[0]!=body||
           contact::half(input,body+0x774)!=9||roles_at_factory!=(initial_guard==1?0:0x43)||
           be32(contact::at(input,body+0x5BC))!=0x200)
            fail("replacement heal tail changed fresh B admission or debit");
        if(!effect_admitted)std::printf("heal-tail-b: %u %u %u %u %04X %u\n",initial_caster,initial_visit,rule,poll_b.count,
                    roles_at_factory,contact::half(input,body+0x774));
        const auto view=effect_admitted?word(heal_effect_cell+0xE0):0;
        u32 payload=0,joint=0;
        for(unsigned n=0;n<=64;++n){
            unsigned polled=0;
            if(n){
                CPUState frame{};frame.gpr[3]=body;
                CallObservation poll{0x80056900};
                const auto cut=mod.run_to_or_return(frame,ram,wrap,{presentation,0x80264290},&poll,65536);
                if(cut!=presentation||frame.lr!=0x80054144||frame.gpr[3]!=body||poll.count!=1||
                   poll.caller!=0x80054CD0||poll.arguments[0]!=body)
                    fail("replacement heal tail did not finish the original active actor prefix");
                polled=poll.count;
                if(effect_admitted){
                    if(n==17&&rule)put32(at(body+0x8C4),word(body+0x8C4)&~2u);
                    if(n==1){
                        auto inlet_ram=ram;
                        CPUState bind{};bind.gpr[3]=heal_effect_bucket;
                        const auto model=mod.run_to_or_return(bind,inlet_ram,0x80089A94,{0x80007198},nullptr,65536);
                        if(model!=0x80007198||bind.lr!=0x8010E410||bind.gpr[3]!=common_owner||
                           bind.gpr[4]!=view||bind.gpr[5]!=77||bind.gpr[6]!=32||
                           be32(contact::at(inlet_ram,heal_effect_cell+0xDC))!=heal_storage)
                            fail("heal effect initializer did not request its exact common model");
                    }
                    CPUState update{};update.gpr[3]=heal_effect_bucket;
                    if(mod.run_to_or_return(update,ram,0x80089A94,{0x80264290},nullptr,65536)){
                        std::fprintf(stderr,"heal effect assertion visit=%u from=%08X line=%u\n",n,update.lr-4,update.gpr[4]);
                        fail("heal effect did not complete its original bucket update");
                    }
                }
                rebuild();
            }
            if(half(body+0x1C6)!=160||half(body+0x20C)!=3||word(body+0x41C)!=16||half(body+0x774)!=10||
               half(body+0x784)!=3||at(body+0x18)[0]!=1||at(body+0x19)[0]!=2||word(pad)||
               (!effect_admitted&&word(body+0x8C4)))
                fail("replacement heal tail changed HP, took stock or left active input");
            const auto roles=half(body+0x272);
            const auto guard=n<initial_guard?initial_guard-n:0,pose=n<15?15-n:0;
            const auto open=initial_guard&&n>=initial_guard;
            const auto ordinary=receiving(ram,contact::receiver_count,contact::receiver_registry,first,stride);
            const auto alternate=receiving(ram,alternate_receive_count,alternate_receive_list,first,stride);
            if(roles!=(open?0:0x43)||word(body+0x720)!=std::bit_cast<u32>(float(guard))||
               at(body+0x71C)[0]!=pose||word(body+0x1DCC)!=0x3F800000||
               word(body+0x1DC8)!=(n&&pose?0x3CF5C28F:0x3F800000)||
               ordinary!=(open?0x3EF0u:0)||alternate!=ordinary)
                fail("replacement heal tail changed its guard expiry, role closure or separate pose clock");
            for(unsigned i=0;i<16;++i){
                const auto cell=first+i*stride;
                if(!at(cell)[0]||word(cell+0x20)!=body||word(cell+0x28)!=word(body+0x3FC)+32+80*i)
                    fail("replacement heal tail lost or rebound an original body cell");
            }
            if(effect_admitted){
                const bool released=rule&&n>=17,retired=rule&&n>=33;
                const unsigned phase=!n?0:!released?1:n<32?2:3;
                const bool owns_payload=n&&!retired,enabled=n&&phase<3;
                constexpr u32 head=contact::active_head+4*heal_effect_bucket,storage_pool=view_pool+3*28;
                if(n==1){payload=word(heal_storage+12);joint=word(view+12);}
                if(at(heal_effect_cell+0x18)[0]!=phase||word(body+0x8C4)!=(released?0u:2u)||
                   at(heal_effect_cell)[0]!=unsigned(!retired)||at(heal_effect_cell+0x82)[0]!=unsigned(enabled)||
                   word(head)!=(retired?0:heal_effect_cell)||half(contact::free_count+4)!=unsigned(retired)||
                   half(view_pool+18)!=unsigned(retired)||half(storage_pool+18)!=unsigned(!owns_payload)||
                   word(heal_effect_cell+0xDC)!=(owns_payload?heal_storage:0)||
                   word(0x803BB374)!=common_owner||word(0x803BB4C0+3120+0x600)!=0x81064000||
                   half(ordinary_count)||half(alternate_count))
                    fail("heal effect lifetime lost its source owner, resources, or phase");
                if(owns_payload&&(word(heal_storage+12)!=payload||word(view+12)!=joint||word(view+20)!=77||
                   payload<0x81600040||payload>=0x81700000||((payload-32)&31)||
                   joint<0x81600040||joint>=0x81700000||word(payload-24)!=32*92+32))
                    fail("heal effect did not retain its exact heap block and original loaded model");
                if(retired)require_heal_returned(ram,payload,view);
                if(exit_toward&&n==1){
                    defeat_effect(mod,ram,*exit_toward,initial_caster,first,stride);
                    return;
                }
                if(!exit_toward&&(n<=1||n==15||n==16||n==17||n==31||n==32||n==33||n==57||n==58||n==64))
                    std::printf("heal-effect: %u %u %u %u %04X %08X %u %u %u %u %u %u %u %04X %04X %u\n",
                                initial_caster,initial_visit,rule,n,roles,word(body+0x720),at(body+0x71C)[0],at(heal_effect_cell)[0],phase,
                                word(body+0x8C4),half(contact::free_count+4),half(view_pool+18),word(heal_effect_cell+0xDC)!=0,
                                ordinary,alternate,polled);
                continue;
            }
            if(n<=1||n==14||n==15||n==57||n==58||n==64)
                std::printf("heal-tail: %u %u %u %u %04X %08X %u %08X %08X %04X %04X %u\n",
                            initial_caster,initial_visit,rule,n,roles,word(body+0x720),at(body+0x71C)[0],
                            word(body+0x1DCC),word(body+0x1DC8),
                            ordinary,alternate,polled);
        }
    }
}
} // namespace retail_body_entry_contact
