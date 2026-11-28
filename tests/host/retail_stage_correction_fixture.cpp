// One declared STIH wall and an unedited normal body contact queue.
#include "retail_body_entry_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace entry=retail_body_entry_contact;
constexpr u32 body=contact::receiver,probe_cell=contact::body,capsule=probe_cell+0x200;
constexpr u32 stage=0x81048000,files=stage+0x800,grids=stage+0x1000;
constexpr u32 stage_contacts=0x8002E1C8;
constexpr u32 role08_count=0x8043B5A0-21334,role08_list=0x803C357C;
struct Case {bool wall;int begin,end;int attack_x=0;};
constexpr Case cases[]{
    {false,130,130},{true,151,151},{true,150,150},{true,130,130},
    {true,100,100},{true,170,130},{true,170,90},
    {false,130,130,40},{true,130,130,40},{false,130,130,240},{true,130,130,240},
    {true,90,90},
};

class Probe {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void put(u32 a,float x){put_float(at(a),x);}
    void call(u32 fn,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;mod.run(cpu,ram,fn,65536);
    }
    void rebuild(){call(0x8008A28C);call(0x80089AEC);}
    void grid(unsigned role,bool wall){
        // Independent reference lists for every cell: the original relocator
        // edits each list in place. The single authored wall faces +X.
        const u32 file=grids+role*0x1000,table=file+40,refs=table+4*64;
        const unsigned list_bytes=wall?8:4;
        const u32 triangle=refs+list_bytes*64;
        constexpr std::array<u32,6> fields{500,500,8,8,u32(-1000),u32(-1000)};
        store(file,0x53544948);store(file+4,triangle+(wall?56u:0)-file);
        for(unsigned i=0;i<fields.size();++i)store(file+8+4*i,fields[i]);
        store(file+32,table-file-8);store(file+36,triangle-file-8);
        for(unsigned i=0;i<64;++i){
            const auto ref=refs+list_bytes*i;
            store(table+4*i,ref-file-8);
            store(ref,wall?0:0xFFFFFFFF);
            if(wall)store(ref+4,0xFFFFFFFF);
        }
        if(wall){
            constexpr std::array<float,13> raw{100,-200,-200,100,600,-200,100,-200,600,1,0,0,-100};
            for(unsigned i=0;i<raw.size();++i)put(triangle+4+4*i,raw[i]);
        }
        store(files+4*role,file);
    }
    void stage_setup(bool wall){
        store(0x8043B5A0-21316,stage);store(0x8043B5A0-31820,files);
        for(unsigned i=0;i<3;++i)grid(i,wall&&i==1);
        for(unsigned i=0;i<5;++i)put(stage+52+4*i,i==2||i==4?-1000.f:3000.f);
        call(0x8003D4A0);
        for(unsigned i=0;i<3;++i){
            const auto file=word(files+4*i);
            if(word(stage+28+4*i)!=file+8||word(file+32)!=file+40||
               word(word(file+40))!=(wall&&i==1?word(file+36):0xFFFFFFFF))
                fail("declared stage grid did not relocate through the original reader");
        }
    }
    void pose(float x){
        constexpr u32 root=body+0x1AA4,joint=body+0x8D4+6*0x30;
        put(body+0x20,x);put(root+12,x);
        put(joint+12,x+12.f);
    }
    void walk(unsigned id,bool stale){
        constexpr u32 collision_walk=0x8002BB14,presentation=0x8005A580,kernel=0x80030A3C;
        constexpr u32 receiver_scratch=0x803B0368,attack_scratch=0x803B04D0;
        if(stale){put(body+0x1CC,-64.f);at(body+0x1DD)[0]=1;at(body+0x274)[0]=1;}
        for(unsigned visit=1;visit<=2;++visit){
            if(visit==2){
                // Declare the next rigid pose at the root produced by the
                // previous collision prefix, not another original actor tick.
                pose(std::bit_cast<float>(word(body+0x20)));
                // Publish the retained caster shape without putting the
                // declared caster in the enclosing walk's actor loop.
                constexpr u32 slots=0x803C4E84;
                const auto previous_seat0=word(slots);
                store(slots,actor);
                rebuild();
                store(slots,previous_seat0);
            }
            const auto previous=word(body+0x1CC),bound=word(capsule+0x7C);
            const auto receivers=half(contact::receiver_count),attacks=half(entry::ordinary_count);
            const auto role_mask=half(body+0x272);
            const auto guard=word(body+0x720);
            if(attacks!=1||word(entry::ordinary_attack_list)!=entry::attack_cell||half(actor+0x26C)!=1||
               word(entry::attack_cell+0x64)!=std::bit_cast<u32>(float(cases[id].attack_x))||
               word(entry::attack_cell+0x20)!=actor||word(entry::attack_cell+0x24)!=actor||
               half(role08_count)!=1||word(role08_list)!=probe_cell||
               word(capsule+0x20)!=body||word(capsule+0x28)!=contact::body_block+32+11*80||
               receivers>1||(receivers&&word(contact::receiver_registry)!=capsule))
                fail("stage walk did not retain the exact original attack and body shapes");
            auto fork=ram;
            CPUState inlet{};
            const auto narrow=mod.run_to_or_return(inlet,fork,collision_walk,{kernel,presentation,0x80264290},nullptr,65536);
            unsigned accepted=0;
            u32 scratch_left=0,scratch_right=0;
            if(narrow==kernel){
                if(inlet.lr!=0x800304F0||inlet.gpr[3]!=attack_scratch||inlet.gpr[4]!=receiver_scratch)
                    fail("stage walk changed the original capsule scratch dispatch");
                scratch_left=be32(contact::at(fork,receiver_scratch+0x34));
                scratch_right=be32(contact::at(fork,receiver_scratch+0x40));
                CPUState solve{};solve.gpr[3]=attack_scratch;solve.gpr[4]=receiver_scratch;
                mod.run(solve,fork,kernel,4096);accepted=unsigned(solve.gpr[3]!=0);
            }else if(narrow!=presentation)fail("stage walk did not reach its narrow or presentation frontier");
            CPUState cpu{};CallObservation hp{contact::hp_write};
            const auto cut=mod.run_to_or_return(cpu,ram,collision_walk,{presentation,0x80264290},&hp,65536);
            std::printf("stage-walk: %u %u %u %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %u %u %04X %08X %u %u %u %u %u %u %u %08X %08X\n",
                        id,unsigned(stale),visit,cut,cpu.lr,previous,bound,word(body+0x20),word(body+0x1CC),word(probe_cell+0x64),
                        word(capsule+0x64),word(capsule+0x8C),word(capsule+0x98),at(body+0x1DD)[0],at(body+0x274)[0],
                        role_mask,guard,attacks,receivers,unsigned(narrow==kernel),accepted,hp.count,half(body+0x1C6),half(body+0x20C),
                        scratch_left,scratch_right);
            if(cut!=presentation||cpu.lr!=0x8002C870||cpu.gpr[3]!=body||
               word(body+0x1D0)||word(body+0x1D4)||word(body+0x24)||word(body+0x28)||
               (hp.count&&(hp.count!=1||hp.caller!=0x8002F1D8||hp.arguments[0]!=body||hp.arguments[1]!=17)))
                fail("stage walk did not reach the presentation frontier with its exact HP path");
        }
    }
public:
    Probe(const Module& m,const std::vector<u8>& original):mod(m),ram(original){}
    void run(unsigned id,const entry::Attack& sasuke,const entry::Attack& tau,bool enclosing=false,bool stale=false){
        const auto spec=cases[id];
        stage_setup(spec.wall);
        store(0x8043B5A0-31852,contact::scene);store(0x80436380,0x81033A00);
        for(unsigned i=0;i<6;++i)store(0x803C4E84+4*i,body-0x1E00+i*0x1E00);
        at(body)[0]=at(body+0x18)[0]=1;
        at(body+0x3E4)[0]=at(body+0x3E7)[0]=1;
        put16(at(body+0x3E8),0x000A);put16(at(body+0x1C4),180);put16(at(body+0x1C6),180);
        put(body+0xC0,1.f);store(body+0x90,body);store(body+0x27C,0x802D4160);store(body+0x76C,1u|(1u<<6));
        constexpr u32 root=body+0x1AA4,joint=body+0x8D4+6*0x30;
        for(unsigned axis=0;axis<3;++axis){put(root+20*axis,1.f);put(joint+20*axis,1.f);}
        std::memcpy(at(contact::body_block),sasuke.hit.data(),sasuke.hit.size());
        at(contact::body_block)[0]=1;at(contact::body_block+1)[0]=11;at(contact::body_block+2)[0]=255;
        store(body+0x3FC,contact::body_block);
        at(probe_cell+2)[0]=at(capsule+2)[0]=4;
        store(probe_cell+4,capsule);store(contact::free_head+16,probe_cell);store(contact::free_tail+16,capsule);
        put16(at(contact::free_count+8),2);
        call(0x8008AB30,body);
        pose(float(spec.begin));
        constexpr u32 slots=0x803C4E84;
        const auto original_first=word(slots);
        if(spec.attack_x){
            store(0x8043B5A0-31824,0x81019600);
            store(0x8043B5A0-31856,0x8103E000);at(0x8103E000+12)[0]=1;
            at(0x803D3AE8+852)[0]=1;
            std::memcpy(at(0x81015400),sasuke.spec.data(),sasuke.spec.size());
            store(body+0x4AC,0x81015400);
            if(enclosing){
                store(0x803C4D94+4,0x81015400);
                call(0x8006F4F8,body);
                if(word(body+0x4B0)!=0x802D3EE0||word(body+0x4AC)!=0x81015400||word(body+0x27C)!=0x802D4160)
                    fail("stage walk did not install the exact normal reaction banks");
            }
            put(body+0xB4,1.f);put(body+0xC4,1.f);put16(at(body+0x1C8),0xFFFF);
            put16(at(0x803B069E),180);
            at(body+0x88)[0]=1;
            for(unsigned i=0;i<2;++i){
                store(contact::scene+60*i+0x114,100);store(contact::scene+60*i+0x118,100);
                store(0x803B068C+4*i,100);
            }
            entry::bind_attack_at(mod,ram,tau,entry::routes[0],1,{float(spec.attack_x),0,0},actor);
        }
        rebuild();
        if(spec.begin!=spec.end){pose(float(spec.end));rebuild();}
        if(half(role08_count)!=1||word(role08_list)!=probe_cell||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=capsule||word(probe_cell+0x28)!=contact::body_block+32+80||
           word(probe_cell+0x64)!=std::bit_cast<u32>(float(spec.end))||word(probe_cell+0x68)!=std::bit_cast<u32>(50.f)||
           word(probe_cell+0x88)!=std::bit_cast<u32>(50.f)||word(body+0x1CC)||at(body+0x1DD)[0])
            fail("stage correction did not retain the original source sphere and capsule");
        store(slots,original_first);
        if(enclosing){walk(id,stale);return;}
        call(stage_contacts);
        if(spec.attack_x)store(slots,actor);
        const auto before=word(probe_cell+0x64),capsule_before=word(capsule+0x64);
        const auto endpoints=std::array{word(capsule+0x8C),word(capsule+0x98)};
        call(0x8002CE90,50);
        if(word(body+0x1D0)||word(body+0x1D4)||word(body+0x24)||word(body+0x28)||
           half(role08_count)!=1||word(role08_list)!=probe_cell||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=capsule||half(body+0x20C)||half(body+0x1C6)!=180||
           endpoints!=std::array{word(capsule+0x8C),word(capsule+0x98)})
            fail("stage solver changed an unrelated axis, geometry owner or HP before attack");
        std::printf("stage-correction: %u %u %d %d %08X %08X %08X %08X %u %u %u\n",
                    id,unsigned(spec.wall),spec.begin,spec.end,
                    word(body+0x20),word(body+0x1CC),before,word(probe_cell+0x64),at(body+0x1DD)[0],
                    at(body+0x274)[0],half(body+0x1C6));
        std::printf("stage-capsule: %u %08X %08X %08X %08X %08X %08X\n",id,capsule_before,word(capsule+0x64),
                    endpoints[0],endpoints[1],word(capsule+0x8C),word(capsule+0x98));
        if(spec.attack_x){
            constexpr u32 attack=entry::attack_cell,narrow=0x80030A3C;
            if(half(entry::ordinary_count)!=1||word(entry::ordinary_attack_list)!=attack||half(actor+0x26C)!=1||
               word(attack+0x64)!=std::bit_cast<u32>(float(spec.attack_x)))
                fail("stage-corrected receiver lost its original attack inlet");
            CPUState coarse{};coarse.gpr[3]=attack+0x58;coarse.gpr[4]=capsule+0x58;
            mod.run(coarse,ram,0x80039F6C);
            auto fork=ram;
            CPUState narrow_cpu{};
            const auto cut=mod.run_to_or_return(narrow_cpu,fork,contact::pair_pass,{narrow},nullptr,4096);
            if(cut!=(coarse.gpr[3]?narrow:0)||(cut&&
               (narrow_cpu.lr!=0x800304F0||narrow_cpu.gpr[3]!=0x803B04D0||narrow_cpu.gpr[4]!=0x803B0368||
                be32(contact::at(fork,0x803B0368+0x34))!=std::bit_cast<u32>(
                    std::bit_cast<float>(endpoints[0])+std::bit_cast<float>(word(body+0x1CC))))))
                fail("stage-corrected body did not reach the original corrected capsule test");
            CPUState hit{};CallObservation writes{contact::hp_write};
            mod.run_observed(hit,ram,contact::pair_pass,writes,4096);
            if(writes.count>1||(writes.count&&(writes.caller!=0x8002F1D8||writes.arguments[0]!=body||
               writes.arguments[1]!=17))||half(body+0x1C6)!=180-17*writes.count||half(body+0x20C)!=writes.count){
                std::fprintf(stderr,"stage corrected pair %u coarse=%u writes=%u site=%08X target=%08X request=%u hp=%u token=%u\n",
                             id,coarse.gpr[3],writes.count,writes.caller,writes.arguments[0],writes.arguments[1],
                             half(body+0x1C6),half(body+0x20C));
                fail("stage-corrected capsule changed its original HP writer or ownership");
            }
            std::printf("stage-correction-pair: %u %d %u %u %u %u\n",id,spec.attack_x,coarse.gpr[3],writes.count,
                        half(body+0x1C6),half(body+0x20C));
        }
    }
};
} // namespace

int main(int argc,char** argv){
    const bool enclosing=argc==4&&!std::strcmp(argv[3],"--collision-walk");
    if(argc!=3&&!enclosing)fail("usage: stage-correction-fixture qualified.dol unmodified-recomp.so [--collision-walk] < qualified-roles");
    entry::Attack sasuke{},tau{};
    for(auto* assets:{&sasuke,&tau}){
        if(std::fread(assets->hit.data(),1,assets->hit.size(),stdin)!=assets->hit.size()||
           std::fread(assets->spec.data(),1,assets->spec.size(),stdin)!=assets->spec.size())
            fail("stage correction input extent");
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra stage correction input");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(unsigned i=0;i<std::size(cases);++i){
        if(!enclosing)Probe(mod,original).run(i,sasuke,tau);
        else if(cases[i].attack_x)for(bool stale:{false,true})Probe(mod,original).run(i,sasuke,tau,true,stale);
    }
}
