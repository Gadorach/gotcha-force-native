// Exact type-25/type-80 tether carriers under bounded, declared contact poses.
#include "retail_stationary_contact.hpp"
#include "retail_declared_stage_grid.hpp"
#include "retail_body_entry_contact.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace entry=retail_body_entry_contact;
constexpr u32 carrier=0x81030000,shot=0x81032000,bank=0x81034000;
constexpr u32 view_pool=0x81037000,view=0x81037200,view2=view+32;
constexpr u32 stage=0x81050000,grid=stage+0x100,refs=stage+0x200,empty=refs+64*4;
constexpr u32 callback25=0x800C4838,callback80=0x800FD4CC,request80=0x801367F0;
constexpr u32 attack_count=0x8043B5A0-21328,attack_list=0x803C477C,slots=0x803C4E84;
constexpr u32 wall_sphere=contact::body+0x200,owner_capsule=wall_sphere+0x200;
constexpr u32 count08=0x8043B5A0-21334,list08=0x803C357C;

class Probe {
    const Module& mod;
    std::vector<u8> ram;
    u32 target=contact::receiver;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void put(u32 a,float x){put_float(at(a),x);}
    u32 call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;mod.run(cpu,ram,entry,4096);return cpu.gpr[3];
    }
    void rebuild(){call(0x8008A28C);call(0x80089AEC);}
    void scheduled_visit(){
        call(0x80089A94,8);
        call(0x80089554);
        rebuild();
    }
    void rigid(u32 matrix,const contact::Point& point){
        for(unsigned axis=0;axis<3;++axis){
            put(matrix+20*axis,1.f);put(matrix+12+16*axis,point[axis]);
        }
    }
    contact::Point point(u32 a){
        return {std::bit_cast<float>(word(a)),std::bit_cast<float>(word(a+4)),std::bit_cast<float>(word(a+8))};
    }
    void stage_inlet(){
        store(0x8043B5A0-31852,contact::scene);
        store(0x8043B5A0-21316,stage);
        for(unsigned i=0;i<3;++i)store(stage+28+4*i,grid);
        constexpr std::array<u32,6> fields{500,500,8,8,u32(-1000),u32(-1000)};
        for(unsigned i=0;i<fields.size();++i)store(grid+i*4,fields[i]);
        for(unsigned i=0;i<64;++i)store(refs+i*4,empty);
        store(grid+24,refs);store(empty,0xFFFFFFFF);
        for(unsigned i=0;i<5;++i)put(stage+52+4*i,i==2||i==4?-1000.f:3000.f);
    }
    void visual_and_sound_inlet(){
        constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,audio=0x8103E000;
        store(0x803BB4C0+0x600,visual);store(visual+16,link);store(link,table);
        for(unsigned index:{1u,2u,5u,7u})store(table+4*index,visual+0x400);
        store(0x8043B5A0-31856,audio);at(audio+12)[0]=1;at(0x803D3AE8+852)[0]=1;
        // Supply the same already-requested loop to avoid entering audio playback.
        store(0x803D3AE8+7*16+88,word(0x8031B694+27*4));
    }
    void dump(unsigned scenario,unsigned stage_no){
        std::printf("type25-link: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                    scenario,stage_no,at(carrier+0x18)[0],at(carrier+0x19)[0],half(carrier+0x1C),
                    at(actor+0x144)[0],at(actor+0x71E)[0],at(target+0x71E)[0],
                    at(actor+0x71F)[0],at(target+0x71F)[0],at(target+0x719)[0],
                    unsigned(word(carrier+0x15C)==target),half(attack_count),half(contact::free_count+8),
                    half(target+0x1C6),half(target+0x20C),at(actor+0x731)[0],at(target+0x731)[0],
                    bool(at(carrier)[0]));
    }
    void dump80(unsigned scenario,unsigned stage_no){
        for(u32 field:{actor+0x144,carrier+0x15C})
            if(word(field)&&word(field)!=target)fail("type80 retained a different target");
        if(half(contact::free_count+8)&&word(contact::free_head+16)!=shot)
            fail("type80 shape return changed the original free cell");
        std::printf("type80-link: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                    scenario,stage_no,at(carrier+0x18)[0],at(carrier+0x19)[0],half(carrier+0x1C),
                    at(actor+0x14A)[0],unsigned(word(actor+0x144)==target),at(actor+0x71E)[0],at(target+0x71E)[0],
                    at(actor+0x71F)[0],at(target+0x71F)[0],at(target+0x719)[0],
                    unsigned(word(carrier+0x15C)==target),half(attack_count),half(contact::free_count+8),
                    half(target+0x1C6),half(target+0x20C),at(actor+0x731)[0],at(target+0x731)[0],
                    bool(at(carrier)[0]),half(contact::free_count+6),half(view_pool+18),
                    unsigned(word(contact::active_head+9*4)==carrier));
    }
    void check_type80_busy(){
        auto fork=ram;
        CPUState busy{};busy.gpr[3]=actor;
        const auto gate=mod.run_to_or_return(busy,fork,request80,{0x800FD3C0,0x8006A53C},nullptr,4096);
        if(gate!=0x8006A53C||busy.lr!=0x80136834||busy.gpr[3]!=actor||busy.gpr[4]!=10)
            fail("type80 busy owner did not take its original wait path");
    }
    void wall_join(unsigned scenario,u16 hp,const entry::Attack* tau){
        constexpr u32 propagate=0x8002CB20,writer=0x80035CA4,presentation=0x8005A580;
        if(half(contact::free_count+8)!=1||word(contact::free_head+16)!=shot||half(attack_count)||
           word(carrier+0x15C)!=target||word(actor+0x1CC)||word(target+0x1CC))
            fail("tether wall join did not keep the exact post-contact lifetime");

        // An additional, separately offered cell gets Chainsaw Knight's
        // original ordinary capsule. Leave the returned attack cell free.
        at(bank)[0]=11;at(bank+1)[0]=255;
        store(actor+0x76C,1u<<6);
        rigid(actor+0x1AA4,point(actor+0x20));
        rigid(actor+0x8D4+6*0x30,{220,100,202});
        at(owner_capsule+2)[0]=4;store(owner_capsule+4,shot);
        store(contact::free_head+16,owner_capsule);put16(at(contact::free_count+8),2);
        call(0x8008AB30,actor);
        constexpr u32 caster=actor+2*0x1E00;
        const bool pair=scenario>=4;
        if(pair){
            if(!tau)fail("tether wall missing qualified Tau inputs");
            entry::bind_attack_at(mod,ram,*tau,entry::routes[0],1,{105,100,200},caster,entry::attack_cell,2);
            at(caster+0x88)[0]=1;
        }
        rebuild();
        // Visit the same declared pose again so the wall sphere is stationary
        // at the sampled position, not sweeping from the first contact pose.
        rebuild();
        if(half(count08)!=1||word(list08)!=wall_sphere||half(contact::receiver_count)!=2||
           word(wall_sphere+0x20)!=target||word(wall_sphere+0x28)!=contact::body_block+32+80||
           word(owner_capsule+0x20)!=actor||word(owner_capsule+0x28)!=bank+32+11*80||
           half(contact::free_count+8)!=1||word(contact::free_head+16)!=shot||half(attack_count)!=unsigned(pair)||
           (pair&&(word(attack_list)!=entry::attack_cell||word(entry::attack_cell+0x20)!=caster||
                   half(caster+0x26C)!=2||word(entry::attack_cell+0x64)!=std::bit_cast<u32>(105.f)))){
            std::fprintf(stderr,"tether wall publication: role08=%u/%08X receivers=%u/%08X/%08X free=%u/%08X owners=%08X/%08X "
                         "attacks=%u/%08X owner=%08X token=%u x=%08X\n",
                         half(count08),word(list08),half(contact::receiver_count),word(contact::receiver_registry),
                         word(contact::receiver_registry+4),half(contact::free_count+8),word(contact::free_head+16),
                         word(wall_sphere+0x20),word(owner_capsule+0x20),half(attack_count),word(attack_list),
                         word(entry::attack_cell+0x20),half(caster+0x26C),word(entry::attack_cell+0x64));
            fail("tether wall shape publication changed");
        }

        const auto masks=std::array{at(actor+0x71E)[0],at(target+0x71E)[0],
                                    at(actor+0x71F)[0],at(target+0x71F)[0]};
        const auto count=std::array{at(actor+0x731)[0],at(target+0x731)[0]};
        const auto roots=std::array{word(actor+0x20),word(target+0x20)};
        const auto raw_centers=std::array{word(owner_capsule+0x64),word(wall_sphere+0x64)};
        const auto timer=half(carrier+0x1C);
        const auto phase=at(carrier+0x19)[0];
        const auto inlet=ram;
        call(0x8002E1C8);
        const auto stage_vectors=std::array{word(actor+0x1CC),word(target+0x1CC)};
        CPUState pass{};CallObservation copy{writer};
        mod.run_observed(pass,ram,propagate,copy,4096);
        if(copy.count>1||(copy.count&&(copy.caller!=0x8002CC34||copy.arguments[0]!=actor||
                                       copy.arguments[1]!=target+0x1CC)))
            fail("tether wall did not transfer from Sasuke to the bound owner");
        call(0x8002CE44);
        const auto state=[&]{
            return std::array{word(actor+0x20),word(target+0x20),word(actor+0x1CC),word(target+0x1CC),
                              word(owner_capsule+0x64),word(wall_sphere+0x64),word(owner_capsule+0x8C),
                              word(owner_capsule+0x98)};
        };
        const auto isolated=state();
        ram=inlet;
        u32 scratch_left=0,scratch_right=0;
        if(pair){
            constexpr u32 kernel=0x80030A3C,scratch=0x803B0368;
            auto fork=inlet;
            CPUState narrow{};
            const auto next=mod.run_to_or_return(narrow,fork,0x8002BB14,{kernel,presentation,0x80264290},nullptr,65536);
            if(next!=kernel||narrow.lr!=0x800304F0||narrow.gpr[3]!=0x803B04D0||narrow.gpr[4]!=scratch)
                fail("tether wall pair did not reach original capsule narrow dispatch");
            scratch_left=be32(contact::at(fork,scratch+0x34));
            scratch_right=be32(contact::at(fork,scratch+0x40));
        }
        CPUState walk{};CallObservation amount{contact::amount_entry};
        const auto cut=mod.run_to_or_return(walk,ram,0x8002BB14,{presentation,contact::hp_write,0x80264290},
                                            &amount,65536);
        unsigned damage=0;
        if(cut==contact::hp_write&&pair){
            if(walk.lr!=0x8002F1DC||walk.gpr[3]!=actor||amount.count!=1||
               amount.arguments[0]!=0x80377AD8||amount.arguments[1]!=caster||amount.arguments[2]!=actor||
               half(actor+0x1C6)!=hp)
                fail("tether wall HP write did not belong to the original Tau/Chainsaw pair");
            damage=walk.gpr[4];mod.finish_stopped_leaf(walk,contact::hp_write,164);
        }else if(cut!=presentation||walk.lr!=0x8002C870||walk.gpr[3]!=actor||amount.count){
            fail("tether wall walk did not reach its original frontier");
        }
        if(state()!=isolated||
           word(actor+0x1D0)||word(actor+0x1D4)||word(target+0x1D0)||word(target+0x1D4)||
           !at(carrier)[0]||word(carrier+0x15C)!=target||half(carrier+0x1C)!=timer||at(carrier+0x19)[0]!=phase||
           half(actor+0x1C6)!=hp-damage||half(target+0x1C6)!=180||half(target+0x20C)!=1)
            fail("enclosing original walk did not retain the tether wall result and HP");
        std::printf("tether-wall: %u %u %u",scenario,phase,timer);
        for(auto value:masks)std::printf(" %u",value);
        for(auto value:count)std::printf(" %u",value);
        for(auto value:roots)std::printf(" %08X",value);
        for(auto value:raw_centers)std::printf(" %08X",value);
        for(auto value:stage_vectors)std::printf(" %08X",value);
        std::printf(" %u %08X",copy.count,copy.caller);
        for(auto value:isolated)std::printf(" %08X",value);
        std::printf(" %u %u %u %u %u\n",at(actor+0x1DD)[0],at(target+0x1DD)[0],
                    half(actor+0x1C6),half(target+0x1C6),half(target+0x20C));
        if(pair)std::printf("tether-wall-pair: %u %08X %08X %08X %u %u %u\n",scenario,cut,scratch_left,scratch_right,
                            damage,half(actor+0x20C),half(caster+0x1C6));
    }
public:
    Probe(const Module& module,const std::vector<u8>& original):mod(module),ram(original){}
    void run(unsigned scenario,const contact::HitBlock& attack,const contact::ActorSpec& owner,
             const contact::HitBlock& body,const contact::ActorSpec& defender,bool type80=false,
             const entry::Attack* wall_tau=nullptr){
        const bool join=wall_tau!=nullptr;
        stage_inlet();visual_and_sound_inlet();
        if(join){
            target=actor+0x1E00;
            retail_declared_stage_grid::bind(mod,ram,stage,scenario!=0&&scenario!=4,800.f);
        }
        const bool key1=!type80&&!join&&scenario>=3;
        const unsigned control=join?1:scenario%3;
        const u16 model=type80?0x0D02:key1?0x0809:0x0201;
        const u16 hp=type80?200:key1?50:250;
        const u32 callback=type80?callback80:callback25;
        const unsigned descriptor=type80?2:key1?0:25;
        for(unsigned i=0;i<6;++i)store(slots+4*i,i?target+0x1E00*(i-1):actor);
        at(actor)[0]=at(actor+0x18)[0]=1;
        at(actor+0x3E5)[0]=1;at(target+0x3E5)[0]=2;at(target+0x3E7)[0]=1;
        put16(at(actor+0x94),model);put16(at(actor+0x3E8),model);
        std::memcpy(at(0x81015000),owner.data(),owner.size());
        store(0x803C4D94,0x81015000);call(type80?0x80135EE8:key1?0x8015613C:0x800C8560,actor);
        if(word(actor+0x4B0)!=(type80?0x80338C80:key1?0x80342708:0x80309CA8)||word(actor+0x4AC)!=0x81015000)
            fail("original tether owner installer");
        std::memcpy(at(bank),attack.data(),attack.size());
        store(actor+0x3FC,bank);store(actor+0x90,actor);
        constexpr contact::Point origin{200,100,200};
        rigid(actor+0x8D4,origin);rigid(actor+0x8D4+(type80?1:key1?12:11)*0x30,origin);
        for(unsigned axis=0;axis<3;++axis)put(actor+0x20+4*axis,origin[axis]);
        for(u32 offset:{0xB4u,0xB8u,0xC0u,0xC4u})put(actor+offset,1.f);
        put16(at(actor+0x1C4),hp);put16(at(actor+0x1C6),hp);
        store(actor+0xCC,target);
        const bool offer_carrier=!type80||scenario!=3,offer_view=!type80||scenario!=4;
        if(offer_carrier){
            at(carrier+2)[0]=3;store(contact::free_head+12,carrier);store(contact::free_tail+12,carrier);
            put16(at(contact::free_count+6),1);
        }
        store(0x80436380,view_pool);
        if(offer_view){store(view_pool+12,view);put16(at(view_pool+18),type80?1:2);}
        if(!type80)store(view,view2);
        CPUState create{};create.gpr[3]=actor;
        if(type80){
            CallObservation factory{0x800FD3C0};
            mod.run_observed(create,ram,request80,factory,4096);
            if(factory.count!=1||factory.caller!=0x80136824||factory.arguments[0]!=actor||
               at(actor+0x540)[0]!=1||half(actor+0x548)!=15)
                fail("type80 request did not select its source factory");
            if(!offer_carrier||!offer_view){
                if(at(carrier)[0]||at(actor+0x14A)[0]||word(actor+0x144)||word(contact::active_head+9*4)||
                   half(contact::free_count+6)!=unsigned(offer_carrier)||half(view_pool+18)!=unsigned(offer_view))
                    fail("type80 allocation refusal retained its busy flag or consumed a resource");
                dump80(scenario,0);
                return;
            }
            if(at(actor+0x14A)[0]!=1||word(actor+0x144))fail("type80 allocation did not retain only the busy flag");
            check_type80_busy();
        }else{
            at(actor+0x144)[0]=1; // Declared pending byte from the calling phase.
            create.gpr[4]=unsigned(key1);create.gpr[5]=actor+0x144;
            mod.run(create,ram,0x800C4704,4096);
            if(create.gpr[3]!=1||word(carrier+0x184)!=actor+0x144)
                fail("type25 source factory lost its status byte");
        }
        if(word(contact::active_head+9*4)!=carrier||word(carrier+0xC)!=callback||
           at(carrier+0x10)[0]!=(type80?80:25)||at(carrier+0x11)[0]!=unsigned(key1)||
           word(carrier+0x8C)!=actor||word(carrier+0x90)!=actor)
            fail("tether carrier did not keep its original list or ownership");
        if(!type80||scenario!=5){
            at(shot+2)[0]=4;store(contact::free_head+16,shot);store(contact::free_tail+16,shot);
            put16(at(contact::free_count+8),1);
        }
        CPUState open{};open.gpr[3]=carrier;CallObservation bind{0x8008AC80};
        mod.run_observed(open,ram,callback,bind,4096);
        if(bind.count!=1||bind.caller!=(type80?0x800FD6C8:0x800C4A64)||bind.arguments[0]!=carrier||
           bind.arguments[1]!=(type80?2u:0u))
            fail("tether original group request");
        if(type80&&scenario==5){
            if(at(carrier+0x18)[0]!=2||at(actor+0x14A)[0]||word(actor+0x144)||word(contact::active_head+49*4))
                fail("type80 shape refusal did not clear the busy flag and close");
            dump80(scenario,0);
            for(unsigned i=1;i<=2;++i){scheduled_visit();dump80(scenario,i);}
            if(word(contact::free_head+12)!=carrier||word(contact::free_tail+12)!=carrier||
               word(view_pool+12)!=view||word(view_pool+8))
                fail("type80 opening refusal did not return the exact carrier and view");
            return;
        }
        if(at(carrier+0x18)[0]!=1||(!type80&&at(carrier+0x89)[0]!=(key1?12:11))||
           word(shot+0x20)!=carrier||word(shot+0x24)!=actor||word(shot+0x28)!=bank+0xAA0+descriptor*80)
            fail("tether opening did not bind its original shape");
        call(0x80047AA4,carrier);
        rebuild();
        auto center=point(shot+0x64);
        if(control==0)center[0]+=400;
        contact::bind_stationary(mod,ram,center,body,owner,defender,hp,contact::sasuke,target,join?wall_sphere:0);
        at(target+0x71A)[0]=2;
        rigid(target+0x8D4+2*0x30,{center[0],center[1]+14,center[2]});
        rebuild();
        if(half(attack_count)!=1||word(attack_list)!=shot||half(contact::receiver_count)!=1||
           word(contact::receiver_registry)!=contact::body||half(carrier+0x26C)!=1||
           at(actor+0x71E)[0]||at(target+0x71E)[0]||at(actor+0x71F)[0]||at(target+0x71F)[0])
            fail("tether pair did not publish its exact attack and body");

        CPUState amount{};CallObservation record{contact::amount_entry};
        const auto cut=mod.run_to_or_return(amount,ram,contact::pair_pass,{contact::hp_write},&record,4096);
        unsigned damage=0;
        if(cut==contact::hp_write){
            if(amount.gpr[3]!=target||amount.lr!=0x8002F1DC||record.count!=1||
               record.arguments[1]!=carrier||record.arguments[2]!=target)
                fail("tether HP writer or amount ownership changed");
            damage=amount.gpr[4];mod.finish_stopped_leaf(amount,contact::hp_write,164);
        }
        if(half(actor+0x1C6)!=hp||word(actor+0x418)||word(target+0x41C)||
           word(shot+0x2C)!=word(actor+0x27C)||record.count!=unsigned(cut==contact::hp_write))
            fail("tether pair changed its exact damage bank, payer or ledgers");
        if(join){
            if(cut!=contact::hp_write||record.arguments[0]!=0x80309EB0||damage||at(carrier+0x1D9)[0]!=16||
               word(carrier+0x290)!=target||half(target+0x20C)!=1)
                fail("wall join did not begin with the exact original key-0 contact");
        }else{
            std::printf("%s-pair: %u %u %08X %u %u %u %u %u %u %u\n",type80?"type80":"type25",scenario,
                        unsigned((word(shot+0x28)-bank-0xAA0)/80),record.arguments[0],record.count,damage,
                        at(carrier+0x1D9)[0],half(target+0x1C6),half(target+0x20C),
                        unsigned(word(carrier+0x290)==target),half(carrier+0x26C));
        }
        const auto snapshot=[&](unsigned phase){if(type80)dump80(scenario,phase);else if(!join)dump(scenario,phase);};
        snapshot(0);
        if(control==2)at(target+0x83)[0]=1; // Change eligibility after the original overlap.
        call(callback,carrier);
        rebuild();
        snapshot(1);
        if(control!=1)return;
        call(callback,carrier);
        snapshot(2);
        // Declare a stretched pose after observing one unstretched visit.
        // No link bytes, motion request fields or tether lengths are seeded.
        auto stretched=center;stretched[0]+=600;
        contact::place_receiver(ram,stretched,target);
        put(actor+0x204,100.f);put(target+0x204,100.f);
        if(join){
            rigid(target+0x1AA4,stretched);
            if(scenario!=1)call(callback,carrier);
            if(scenario==3){put16(at(carrier+0x1C),1);call(callback,carrier);}
            wall_join(scenario,hp,wall_tau);
            return;
        }
        call(callback,carrier);
        snapshot(3);
        put16(at(carrier+0x1C),1);
        call(callback,carrier);
        snapshot(4);
        if(type80){
            for(unsigned i=5;i<=8&&at(carrier)[0];++i){scheduled_visit();dump80(scenario,i);}
            check_type80_busy();
        }
    }
};
}

int main(int argc,char** argv){
    const bool type80=argc==4&&!std::strcmp(argv[3],"--type80");
    const bool join=argc==4&&!std::strcmp(argv[3],"--wall-join");
    if(argc!=3&&!type80&&!join)
        fail("usage: type25-link-fixture qualified.dol unmodified-recomp.so [--type80|--wall-join] < qualified-roles");
    std::array<contact::HitBlock,2> attack{};
    std::array<contact::ActorSpec,2> owner{};
    contact::HitBlock body{};
    contact::ActorSpec defender{};
    entry::Attack tau{};
    const auto read=[](auto& x){if(std::fread(x.data(),1,x.size(),stdin)!=x.size())fail("type25 link fixture input extent");};
    for(unsigned i=0;i<(type80||join?1u:2u);++i){read(attack[i]);read(owner[i]);}
    read(body);read(defender);
    if(join){read(tau.hit);read(tau.spec);}
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra type25 link input");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(unsigned i=0;i<6;++i){
        const unsigned input=type80||join?0:i/3;
        Probe(mod,original).run(i,attack[input],owner[input],body,defender,type80,join?&tau:nullptr);
    }
}
