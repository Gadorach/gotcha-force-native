// Two independent original archive deliveries into two real physical actor seats.
#include "retail_contact_effect_archive.hpp"
#include "retail_scene_entry.hpp"

namespace {
using namespace retail_fixture;
namespace archive=retail_contact_effect_archive;
namespace contact=retail_stationary_contact;
namespace scheduler=retail_scene_entry;
constexpr u32 actors=0x81020000,actor_stride=0x1E00,scene=scheduler::scene,mode=0x8103E000;
constexpr u32 stage=0x81048000,pools=0x81063000,basis=0x81060000;
constexpr u32 shape=0x81070000,shape_stride=0x200,pad=0x803C727C;
constexpr u32 body_pass_end=0x80087CF0,present_body=0x8005A580;
constexpr std::array<u16,2> models{0x000A,0x0106},members{495,504},hp{180,220};
constexpr std::array<size_t,2> motion_sizes{368160,435104},view_sizes{241128,171236};
struct Assets {
    contact::ActorSpec spec;
    contact::HitBlock hit;
    std::vector<u8> motion,view;
};
constexpr u32 fighter(unsigned seat){return actors+seat*actor_stride;}

class Replay {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    unsigned byte(u32 a){return at(a)[0];}
    unsigned f(unsigned seat,u32 offset){return byte(fighter(seat)+offset);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void store16(u32 a,u16 x){put16(at(a),x);}
    void call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;
        mod.run(cpu,ram,entry,4096);
    }
    void stage_grid(){
        constexpr u32 grid=stage+0x100,refs=stage+0x200,empty=refs+64*4;
        store(0x8043B5A0-21316,stage);
        for(unsigned i=0;i<3;++i)store(stage+28+4*i,grid);
        constexpr std::array<u32,6> fields{500,500,8,8,u32(-1000),u32(-1000)};
        for(unsigned i=0;i<fields.size();++i)store(grid+4*i,fields[i]);
        store(grid+24,refs);store(empty,0xFFFFFFFF);
        for(unsigned i=0;i<64;++i)store(refs+4*i,empty);
        for(unsigned i=0;i<5;++i)put_float(at(stage+52+4*i),i==2||i==4?-3000.f:3000.f);
    }
    void offer_pools(){
        constexpr u32 bases[]{0x81066000,0x81064000,0x81069000};
        constexpr unsigned bytes[]{44*64,32*2,16*32};
        archive::Setup sdk(mod,ram);
        sdk.init_heap(archive::arena+0x100000);
        store(0x8043B5A0-21352,basis);store(0x8043B5A0-21024,pools);
        for(unsigned kind=0;kind<3;++kind){
            const auto row=pools+28*kind;
            store(row,bases[kind]-basis);store(row+4,bytes[kind]);
            std::memcpy(at(row+16),at(0x8036F5B8+28*kind+16),12);
            call(0x80197950,kind);
        }
        for(unsigned i=0;i<32;++i){
            const auto cell=shape+shape_stride*i;
            at(cell+2)[0]=4;
            store(cell+4,i==31?0:cell+shape_stride);
        }
        store(contact::free_head+16,shape);store(contact::free_tail+16,shape+31*shape_stride);
        store16(contact::free_count+8,32);
    }
    void deliver(unsigned seat,const Assets& a){
        const auto outputs=0x803B07AC+56*seat,view_owner=0x803BB4C0+3120*seat+0x600;
        const auto data=basis+0x270000+0x800*seat-0x7400;
        const auto hit=basis+0x700000+0x2000*seat+0x3C00;
        const auto motion=basis+0x480000+0x6C000*seat-0x4400;
        const auto view=basis+0x270000+0x58000*seat-0x4400;
        CPUState request{};request.gpr[3]=seat;
        mod.run_to(request,ram,0x800410BC,{0x8000B07C});
        if(request.lr!=0x8004124C||request.gpr[3]!=members[seat]||request.gpr[4]!=outputs||
           word(outputs)!=data||word(outputs+4)!=0xF0000000||word(outputs+8)!=hit||
           word(outputs+12)!=motion||word(outputs+16)!=view||word(outputs+20)!=0xF0000000||
           word(view_owner))
            fail("dual scene initial archive request changed its exact seat or model roles");
        std::memcpy(at(data),a.spec.data(),a.spec.size());
        std::memcpy(at(hit),a.hit.data(),a.hit.size());
        std::memcpy(at(motion),a.motion.data(),a.motion.size());
        std::memcpy(at(view),a.view.data(),a.view.size());
        CPUState ready{};ready.gpr[3]=seat;ready.gpr[4]=0;
        mod.run(ready,ram,0x800412C4,4096);
        if(word(view_owner)!=0x81064000+32*seat||word(0x803C4D94+4*seat)!=data||
           word(scene+128+4*seat)!=hit||word(0x803C4D7C+4*seat)!=motion||word(0x803BB4A0+4*seat)!=view){
            std::fprintf(stderr,"dual delivery seat=%u owner=%08X spec=%08X hit=%08X motion=%08X view=%08X pool_free=%u\n",
                         seat,word(view_owner),word(0x803C4D94+4*seat),word(scene+128+4*seat),
                         word(0x803C4D7C+4*seat),word(0x803BB4A0+4*seat),half(pools+28+18));
            fail("dual scene completion crossed actor resource boundaries");
        }
    }
    void setup(const std::array<Assets,2>& assets){
        store(0x8043B5A0-31852,scene);store(scheduler::mode_ptr,mode);
        store(0x8043B5A0-31824,0x81079000);
        at(mode+2)[0]=3;at(mode+12)[0]=1;
        at(0x803D3AE8+852)[0]=1;
        at(scene+0x1828)[0]=3;at(scene+5593)[0]=3;at(scene+5594)[0]=2;
        for(unsigned i=0;i<6;++i)store16(scene+16+2*i,i<2?models[i]:0xFFFF);
        for(unsigned i=0;i<2;++i){
            at(scene+203+i)[0]=i;
            store(scene+60*i+0x114,300);store(scene+60*i+0x118,300);
        }
        call(0x800536B0,actors);
        for(unsigned i=0;i<6;++i){
            if(word(0x803C4E84+4*i)!=fighter(i)||f(i,0)!=unsigned(i<2)||f(i,0x18)||
               f(i,0x3E4)!=i||f(i,0x3E5)!=(1u<<i)||f(i,0x3E7)!=i||f(i,0x743))
                fail("original physical actor initialization changed");
            if(i<2){
                put_float(at(fighter(i)+0x20),i?450.f:150.f);
                put_float(at(fighter(i)+0x24),100.f);
                put_float(at(fighter(i)+0x28),300.f);
            }
        }
        stage_grid();offer_pools();
        for(unsigned i=0;i<2;++i)deliver(i,assets[i]);
        call(0x80254288);
        scheduler::prepare(ram);
    }
    std::array<unsigned,2> rebuild(){
        call(0x8008A28C);call(0x80089AEC);
        std::array<unsigned,2> out{};
        const auto count=half(contact::receiver_count);
        if(count>18)fail("dual scene receiver list extent");
        for(unsigned i=0;i<count;++i){
            const auto ptr=word(contact::receiver_registry+4*i);
            const auto owner=word(ptr+0x20);
            if(owner!=fighter(0)&&owner!=fighter(1))fail("dual scene published a foreign receiver");
            const auto seat=owner==fighter(1)?1:0;
            const auto descriptor=word(ptr+0x28);
            const auto bank=word(owner+0x3FC);
            if(descriptor<bank+32||descriptor>=bank+32+16*80||(descriptor-bank-32)%80)
                fail("dual scene receiving descriptor crossed its actor bank");
            const auto bit=1u<<((descriptor-bank-32)/80);
            if(out[seat]&bit)fail("dual scene duplicate receiving descriptor");
            out[seat]|=bit;
        }
        return out;
    }
    void pulse(unsigned delay,unsigned visit,bool accepted){
        auto fork=ram;
        put32(contact::at(fork,pad+32),0x200);
        CPUState cpu{};
        CallObservation poll{0x80056900};
        const auto frontier=mod.run_to_or_return(cpu,fork,scheduler::dispatch,{0x800E058C,body_pass_end},&poll,65536);
        const auto rd=[&](u32 a){return be32(contact::at(fork,a));};
        const auto paid=contact::half(fork,fighter(1)+0x774);
        if(frontier!=(accepted?0x800E058C:body_pass_end)||poll.count!=(accepted?2u:0u)||
           (accepted&&(poll.caller!=0x80054CD0||poll.first_arguments[0]!=fighter(0)||
              poll.first_arguments[1]!=fighter(1)||cpu.lr!=0x80130584||cpu.gpr[3]!=fighter(1)||cpu.gpr[4]!=2||cpu.gpr[5]!=10))||
           rd(fighter(0)+0x5B4)||rd(fighter(0)+0x5BC)||rd(fighter(0)+0x5D4)||
           contact::half(fork,fighter(0)+0x774)!=10||paid!=(accepted?9:10)||
           rd(fighter(1)+0x5B4)!=(accepted?0x200u:0)||rd(fighter(1)+0x5BC)!=(accepted?0x200u:0)||
           rd(fighter(1)+0x5D4)!=(accepted?0x60u:0))
            fail("dual scene processed pad leaked across seats or reached the wrong factory");
        std::printf("dual-pad: %u %u %u %u %u %08X\n",delay,visit,poll.count,unsigned(accepted),paid,cpu.lr);
    }
    void check_constructor(unsigned delay){
        constexpr std::array<unsigned,2> roots{19,4};
        std::array<u32,2> head{};
        for(unsigned seat=0;seat<2;++seat){
            const auto view=0x803BB4C0+3120*seat;
            const auto owner=word(view+1536);
            const auto node=word(view+1800);
            if(owner!=0x81064000+32*seat||half(owner+26)!=roots[seat]||!node||!word(node+12)||
               half(fighter(seat)+0x272)!=0xEF||half(fighter(seat)+0x20C)||
               half(fighter(seat)+0x784)!=(seat?3u:5u))
                fail("dual normal constructor crossed its rig, stock or collision reset scope");
            head[seat]=word(node+12);
        }
        if(head[0]==head[1]||half(contact::free_count+8)||half(pools+18)!=64-46)
            fail("dual scene did not bind separate SDK view ownership and all the body descriptors");
        std::printf("dual-cold: %u %u %u %u %u\n",delay,roots[0],roots[1],64-half(pools+18),32u);
    }
public:
    Replay(const Module& m,const std::vector<u8>& original,const std::array<Assets,2>& assets):mod(m),ram(original){
        setup(assets);
    }
    void observe(){
        const auto start=ram;
        for(unsigned delay:{0u,10u}){
            std::memcpy(ram.data(),start.data(),ram.size());
            unsigned prior_stage=0,prior_ready=0,prior_mask=0;
            const unsigned opens=174+delay;
            for(unsigned visit=1;visit<=opens+1;++visit){
                at(scene+0xD2)[0]=u8(visit>=2&&visit<2+delay?2:0);
                const unsigned before=f(0,0x743)+2*f(1,0x743);
                if(visit>=opens)pulse(delay,visit,visit>opens);
                CPUState cpu{};CallObservation actors_called{present_body};
                const auto stop=mod.run_to_or_return(cpu,ram,scheduler::dispatch,{body_pass_end,0x80264290},&actors_called,1u<<20);
                if(stop==0x80264290)
                    std::fprintf(stderr,"dual scene SDK assertion delay=%u visit=%u caller=%08X line=%u\n",
                                 delay,visit,cpu.lr-4,cpu.gpr[4]);
                const auto first=actors_called.first_arguments[0],second=actors_called.first_arguments[1];
                if(stop!=body_pass_end||cpu.lr!=0x80029490||actors_called.count!=2||actors_called.caller!=0x80054140||
                   first!=fighter(0)||second!=fighter(1)||
                   byte(0x8043B5A0-21312)!=2||word(0x803C4E6C)!=first||word(0x803C4E70)!=second){
                    std::fprintf(stderr,"dual actor visit=%u delay=%u cut=%08X lr=%08X calls=%u %08X %08X "
                        "list=%u %08X %08X phase=%u/%u HP=%u/%u\n",visit,delay,stop,cpu.lr,actors_called.count,
                        actors_called.first_arguments[0],actors_called.first_arguments[1],byte(0x8043B5A0-21312),
                        word(0x803C4E6C),word(0x803C4E70),f(0,0x18),f(1,0x18),half(fighter(0)+0x1C6),half(fighter(1)+0x1C6));
                    fail("dual scene did not visit two original actors before its world tail");
                }
                for(unsigned i=0;i<2;++i){
                    if(!f(i,0)||f(i,0x18)!=1||half(fighter(i)+0x3E8)!=models[i]||
                       half(fighter(i)+0x1C6)!=hp[i]||f(i,0x3F0)!=1||
                       word(fighter(i)+0x3FC)!=word(scene+128+4*i)||f(i,0x3E6)||
                       word(fighter(i)+0x5B4)||word(fighter(i)+0x5BC)||half(fighter(i)+0x774)!=10||
                       f(i,0x743)!=unsigned(visit>=81+(i?delay:0))||
                       f(i,0x19)!=(visit<81+(i?delay:0)?0u:visit<opens?1u:2u)||
                       half(fighter(i)+0x272)!=(visit<21+(i?delay:0)?0xEF:visit<opens?0xC3:0)||
                       word(fighter(i)+0x720)||(i&&visit<=11&&delay&&word(fighter(i)+0xD4)))
                        fail("dual scene original actor left its exact normal neutral setup");
                }
                if(visit==1)check_constructor(delay);
                const auto steps=rebuild();
                const unsigned status=byte(scene+69),stage=byte(scene+1),ready=f(0,0x743)+2*f(1,0x743);
                const unsigned mask=(steps[0]?1u:0u)+(steps[1]?2u:0u);
                const auto expected_stage=visit<82+delay?0u:visit==82+delay?1u:visit<opens-1?2u:3u;
                if(before!=unsigned(visit>81)+2*unsigned(visit>81+delay)||stage!=expected_stage||
                   status!=(visit<opens?3u:4u)||steps[0]!=(visit<opens?0u:0x3EF0)||steps[1]!=steps[0])
                    fail("dual readiness, scene release and receiving publication did not remain separate");
                if(visit<=2||visit==11||visit==12||visit==21||(delay&&visit==31)||
                   stage!=prior_stage||ready!=prior_ready||mask!=prior_mask||visit==opens+1){
                    std::printf("dual-scene: %u %u %u %u %u %u %u %u %u %u %04X %04X %04X %04X\n",
                                delay,visit,before,ready,status,stage,half(scene+8),f(0,0x19),f(1,0x19),
                                unsigned(actors_called.first_arguments[0]==fighter(1)),
                                half(fighter(0)+0x272),half(fighter(1)+0x272),steps[0],steps[1]);
                }
                prior_stage=stage;prior_ready=ready;prior_mask=mask;
            }
        }
    }
};
} // namespace

int main(int argc,char** argv){
    if(argc!=3)fail("usage: dual-scene-entry-fixture qualified.dol unmodified-recomp.so < qualified-roles");
    std::array<Assets,2> assets{};
    const auto read=[](auto& field){
        if(std::fread(field.data(),1,field.size(),stdin)!=field.size())fail("dual scene asset extent");
    };
    for(unsigned i=0;i<2;++i){
        auto& a=assets[i];a.motion.resize(motion_sizes[i]);a.view.resize(view_sizes[i]);
        read(a.spec);read(a.hit);read(a.motion);read(a.view);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("extra dual scene assets");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    Replay(mod,original,assets).observe();
}
