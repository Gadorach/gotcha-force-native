// One original set0202 side row through construction, support and the carry prefix.
#include "retail_contact_effect_archive.hpp"
#include "retail_declared_stage_grid.hpp"

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace art=retail_contact_effect_archive;
constexpr u32 actor_body=0x81020000,step=0x1E00,prop=0x81030000,stage=0x81070000;
constexpr u32 banks=0x81054000,resources=0x81240000,collision_bank=0x81268000;
constexpr u32 pools=0x81069000,model_cells=0x81069800,archive_cells=0x81069900;
constexpr u32 shapes=0x81060000,shape_step=0x200;
constexpr u32 common_shapes=0x803C706C+48*4,stage_objects=0x803C706C+40*4;
constexpr u32 plane_list=0x803C237C,plane_count=0x8043B5A0-21340;
using Side=std::array<u8,122880>;
using Common=std::array<u8,21504>;
struct Case {bool far=false,translate=false,masked=false;};
constexpr Case cases[]{{},{.far=true},{.translate=true},{.translate=true,.masked=true}};

class Probe {
    const Module& mod;
    std::vector<u8> ram;
    std::array<u32,8> gqr{};
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    float scalar(u32 a){return std::bit_cast<float>(word(a));}
    void store(u32 a,u32 value){put32(at(a),value);}
    void put(u32 a,float value){put_float(at(a),value);}
    std::array<u32,3> vector(u32 a){return {word(a),word(a+4),word(a+8)};}
    void point(u32 a,const contact::Point& p){for(unsigned i=0;i<3;++i)put(a+4*i,p[i]);}
    void rig(u32 a,const contact::Point& p){
        for(unsigned i=0;i<3;++i){put(a+20*i,1.f);put(a+12+16*i,p[i]);}
    }
    void init_cpu(CPUState& cpu){std::copy(gqr.begin(),gqr.end(),std::begin(cpu.gqr));}
    void call(u32 fn,u32 arg=0,CallObservation* observation=nullptr){
        CPUState cpu{};cpu.gpr[3]=arg;init_cpu(cpu);
        if(mod.run_to_or_return(cpu,ram,fn,{0x80264290},observation,65536)){
            std::fprintf(stderr,"stage prop original assertion: entry=%08X caller=%08X line=%u\n",fn,cpu.lr-4,cpu.gpr[4]);
            fail("stage prop crossed an unsupported assertion");
        }
    }
    void init_render_pool(unsigned type,u32 first,unsigned count,u32 recipe){
        const auto row=pools+28*type;
        store(row,first-word(art::source_bank_basis));
        store(row+4,count*half(recipe+16));
        std::memcpy(at(row+16),at(recipe+16),12);
        call(0x80197950,type);
        if(word(row+12)!=first||half(row+18)!=count||word(row+8))
            fail("stage prop model/archive descriptor pool did not initialize");
    }
    void prepare(const Side& side,const Common& common){
        CPUState boot{};
        const auto cut=mod.run_to_or_return(boot,ram,0x8002A3E4,{0x8002A51C,0x80200318},nullptr,4096);
        if((cut!=0x8002A51C&&cut!=0x80200318)||boot.gqr[5]!=0x00070007)
            fail("stage prop source GQR setup");
        std::copy(std::begin(boot.gqr),std::end(boot.gqr),gqr.begin());
        art::Setup setup(mod,ram);
        setup.init_heap(art::arena+0x100000);
        call(0x80254288);
        store(0x80436380,pools);
        init_render_pool(0,model_cells,2,0x8036F5B8);
        init_render_pool(1,archive_cells,2,0x8036F5D4);

        // An empty stage query inlet, with the original retained side payload.
        retail_declared_stage_grid::bind(mod,ram,stage,false,100.f);
        std::memcpy(at(resources),side.data(),side.size());
        std::memcpy(at(collision_bank),common.data(),common.size());
        store(stage+0x800+24,resources);
        store(0x8043B5A0-31852,contact::scene);store(contact::scene+152,collision_bank);
        for(unsigned i=0;i<6;++i)store(0x803C4E84+4*i,actor_body+step*i);
        CallObservation archive_load{0x80040EB4};
        call(0x8004279C,0,&archive_load);
        if(archive_load.count!=2||archive_load.caller!=0x80042840||
           word(stage+0x800+24)!=resources+2048||half(pools+28+18)||
           word(0x803BB38C)!=archive_cells||word(0x803BB394)!=archive_cells+32||
           word(archive_cells+20)!=resources+4096||word(archive_cells+32+20)!=resources+4096+28*2048||
           half(archive_cells+26)!=9||half(archive_cells+32+26)!=8){
            std::fprintf(stderr,"stage side load count=%u/%08X placement=%08X free=%u rows=%08X/%08X "
                         "bases=%08X/%08X records=%u/%u\n",archive_load.count,archive_load.caller,
                         word(stage+0x800+24),half(pools+28+18),word(0x803BB38C),word(0x803BB394),
                         word(archive_cells+20),word(archive_cells+32+20),half(archive_cells+26),half(archive_cells+32+26));
            fail("stage prop did not bind the exact two original side archives");
        }
    }
    void construct(){
        at(prop+2)[0]=3;store(contact::free_head+12,prop);store(contact::free_tail+12,prop);
        put16(at(contact::free_count+6),1);
        for(unsigned i=0;i<4;++i){
            const auto cell=shapes+shape_step*i;
            at(cell+2)[0]=4;store(cell+4,i<3?cell+shape_step:0);
        }
        store(contact::free_head+16,shapes);store(contact::free_tail+16,shapes+3*shape_step);
        put16(at(contact::free_count+8),4);
        CallObservation bind{0x8008AE10};
        call(0x80081980,resources+2052,&bind);
        const auto joint_root=[](u32 p){return p>=art::payload&&p<art::arena+0x100000;};
        if(word(stage_objects)!=prop||!at(prop)[0]||at(prop+0x83)[0]!=22||at(prop+0x10)[0]!=4||
           at(prop+0x11)[0]!=4||at(prop+0x18)[0]!=0||word(prop+0x0C)!=0x80081D98||
           word(prop+0x100)!=0x80082060||half(prop+0x1C6)!=100||word(prop+0x90)||
           word(prop+0x8C)!=prop||!joint_root(word(model_cells+12))||!joint_root(word(model_cells+44+12))||
           word(model_cells+12)==word(model_cells+44+12)||
           half(pools+18)||half(contact::free_count+6)||half(contact::free_count+8)!=2||
           bind.count!=1||bind.caller!=0x80081CD0||bind.arguments[0]!=prop||bind.arguments[1]!=171||
           word(common_shapes)!=shapes+shape_step)
            fail("stage prop original type-4 constructor or collision selector changed");
        for(unsigned i=0;i<2;++i){
            const auto cell=shapes+shape_step*i;
            if(word(cell+0x20)!=prop||word(cell+0x24)!=prop||word(cell+0x60)!=prop+0x114||
               word(cell+0x28)!=collision_bank+0x400+80*(236+i))
                fail("stage prop collision shape lost its exact original owner or descriptor");
        }
        call(0x80089A94,40);
        call(0x80089554);
        if(at(prop+0x18)[0]!=1||at(prop+0x19)[0]||vector(prop+0x20)!=vector(resources+2056))
            fail("stage prop did not reach its original initial world pose");
    }
    void bind_receiver(const contact::HitBlock& hit,const contact::ActorSpec& spec,const contact::Point& pose){
        std::memcpy(at(banks),hit.data(),hit.size());
        at(banks)[0]=0;at(banks+1)[0]=11;at(banks+2)[0]=255;
        constexpr u32 actor_spec=banks+0x2000;
        std::memcpy(at(actor_spec),spec.data(),spec.size());store(0x803C4D94,actor_spec);
        at(actor_body)[0]=at(actor_body+0x18)[0]=at(actor_body+0x3E5)[0]=1;
        put16(at(actor_body+0x3E8),0x000A);put16(at(actor_body+0x1C4),180);put16(at(actor_body+0x1C6),180);
        store(actor_body+0x3FC,banks);store(actor_body+0x90,actor_body);store(actor_body+0x76C,1|(1u<<6));
        for(u32 field:{0xB4u,0xB8u,0xC0u,0xC4u})put(actor_body+field,1.f);
        point(actor_body+0x20,pose);
        rig(actor_body+0x1AA4,pose);
        auto joint=pose;joint[0]+=12;
        rig(actor_body+0x8D4+6*0x30,joint);
        call(0x8006F4F8,actor_body);
        call(0x8008AB30,actor_body);
    }
    void publish_shapes(){
        call(0x8008A28C);call(0x80089AEC);
    }
    void bucket_visit(CallObservation* marker=nullptr){
        call(0x80089A94,40,marker);
        call(0x80089554);
        publish_shapes();
    }
    void collision(CallObservation* marker=nullptr){
        CPUState walk{};init_cpu(walk);
        const auto cut=mod.run_to_or_return(walk,ram,0x8002BB14,{0x8005A580,0x80264290,contact::hp_write},marker,65536);
        if(cut!=0x8005A580||walk.lr!=0x8002C870||walk.gpr[3]!=actor_body||half(actor_body+0x1C6)!=180)
            fail("original stage prop collision escaped the presentation frontier");
    }
    unsigned cell_index(u32 cell){
        if(!cell)return 255;
        if(cell<shapes||cell>=shapes+4*shape_step||(cell-shapes)%shape_step)
            fail("stage prop recovery escaped the original collision-cell pool");
        return (cell-shapes)/shape_step;
    }
    void record_recovery(unsigned visit){
        const auto p=half(plane_count)?word(plane_list):0;
        if(half(plane_count)>1||(p&&(word(p+0x20)!=prop||word(p+0x28)!=collision_bank+0x400+237*80))||
           word(stage_objects)!=prop||!at(prop)[0]||half(contact::receiver_count)<1)
            fail("stage prop original recovery changed collision or owner identities");
        std::printf("stage-prop-recovery: %u %u %u %u %u %u %u %u %u %u %08X %08X %08X %08X %08X %u %u %u\n",
                    visit,at(prop+0x18)[0],at(prop+0x19)[0],half(prop+0x1C6),half(prop+0x1C),
                    half(contact::free_count+8),half(contact::receiver_count),half(plane_count),cell_index(p),at(prop+0x281)[0],
                    word(prop+0x24),word(prop+0x180),word(prop+0x184),p?word(p+0x68):0,
                    word(0x803B2908+4*4),at(actor_body+0x1DB)[0],at(actor_body+0x1DE)[0],
                    cell_index(word(actor_body+0x1E0)));
    }
    void rising_carry(u32 retired_plane){
        const auto rebinding=ram;
        const auto plane=word(plane_list);
        if(half(plane_count)!=1||plane==retired_plane||word(retired_plane+0x28)!=collision_bank+0x400+236*80||
           word(retired_plane+0x20)!=prop||at(actor_body+0x1DB)[0]||word(actor_body+0x1E0)!=retired_plane)
            fail("the original rebuild did not distinguish plane rebind from stale cell reuse");
        // Fresh declared actor pose on the *rebound* plane. Settle the
        // registered matrices without running another prop visit.
        const contact::Point start{scalar(plane+0x64),scalar(plane+0x68)+144.f,scalar(plane+0x6C)};
        point(actor_body+0x20,start);rig(actor_body+0x1AA4,start);
        auto joint=start;joint[0]+=12;
        rig(actor_body+0x8D4+6*0x30,joint);
        publish_shapes();publish_shapes();
        CallObservation narrow{0x80035698};collision(&narrow);
        if(narrow.count!=1||at(actor_body+0x1DB)[0]!=1||word(actor_body+0x1E0)!=plane||
           word(actor_body+0x1E4)!=prop||word(plane_list)!=plane)
            fail("rising prop fork did not acquire the rebound plane through original contact");
        const auto old_prop=word(prop+0x24),old_plane=word(plane+0x68),old_actor=word(actor_body+0x24);
        const auto old_anchor=word(actor_body+0x1FC);
        bucket_visit();
        const auto new_prop=word(prop+0x24),new_plane=word(plane+0x68);
        if(word(prop+0x30)!=old_prop||new_prop==old_prop||word(plane_list)!=plane)
            fail("original rise did not advance its world and saved owner pose");
        constexpr u32 normal=banks+0x2400;
        CPUState height{};init_cpu(height);
        height.gpr[3]=actor_body;height.gpr[4]=actor_body+0x20;height.gpr[5]=normal;
        mod.run(height,ram,0x8003E0A8,4096);
        const auto queried=std::bit_cast<u32>(float(height.fpr[1]));
        CPUState carry{};init_cpu(carry);carry.gpr[3]=actor_body;
        const auto cut=mod.run_to_or_return(carry,ram,0x80059068,{0x80059E58,0x80264290},nullptr,4096);
        if(cut!=0x80059E58||carry.lr!=0x800590B0||carry.gpr[3]!=actor_body||
           word(actor_body+0x1E0)!=plane||word(actor_body+0x1E4)!=prop||at(actor_body+0x1DB)[0]!=1||
           word(actor_body+0x1FC)!=old_anchor||!(word(actor_body+0x5E0)&0x100000)||
           scalar(actor_body+0x20)!=start[0]||scalar(actor_body+0x28)!=start[2]||half(actor_body+0x1C6)!=180)
            fail("rising prop did not carry its original support receiver");
        std::printf("stage-prop-rise-carry: %u %u %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    cell_index(retired_plane),cell_index(plane),old_prop,new_prop,old_plane,new_plane,
                    old_anchor,old_actor,queried,word(actor_body+0x24));
        ram=rebinding;
    }
    void recovery(){
        const auto original_plane=word(plane_list);
        if(at(actor_body+0x1DB)[0]!=1||word(actor_body+0x1E0)!=original_plane||word(actor_body+0x1E4)!=prop)
            fail("stage prop recovery needs its original support acquisition");
        record_recovery(0);
        // Causal inlet only: the original bucket callback owns all later
        // destruction, collision release, timing, rebinding and motion.
        put16(at(prop+0x1C6),0);
        constexpr u32 muted_audio=0x8106B000;
        store(0x8043B5A0-31856,muted_audio);at(muted_audio+12)[0]=255;
        at(0x803D3AE8+852)[0]=1;
        CallObservation sound{0x800F0468};
        bucket_visit(&sound);
        if(sound.count!=1||sound.caller!=0x80081F48||sound.arguments[0]!=prop||sound.arguments[1]!=115)
            fail("stage prop original destruction did not request its positional cue");
        if(half(plane_count)||half(contact::free_count+8)!=2||at(shapes)[0]||at(shapes+shape_step)[0]||
           !at(shapes+2*shape_step)[0]||!at(shapes+3*shape_step)[0])
            fail("stage prop destruction did not return both original prop cells separately from the actor's cells");
        record_recovery(1);
        collision();
        if(at(actor_body+0x1DB)[0]||at(actor_body+0x1DE)[0]!=1||word(actor_body+0x1E0)!=original_plane)
            fail("stage prop break did not clear active support independently of its retained address");
        record_recovery(2);
        float local_y=0;
        for(unsigned n=1;n<=664;++n){
            bucket_visit();
            if(n<600){
                if(half(prop+0x1C6)||half(prop+0x1C)!=600-n||at(prop+0x19)[0]!=1||half(plane_count)||
                   half(contact::free_count+8)!=2||scalar(prop+0x24)!=10.f)
                    fail("stage prop recovery wait advanced before its original timer edge");
            }else{
                if(half(prop+0x1C6)!=100||half(prop+0x1C)||at(prop+0x19)[0]||half(plane_count)!=1||
                   half(contact::free_count+8))
                    fail("stage prop recovery did not retain the rebound collision and HP");
                if(n==600)local_y=-869.572509765625f;
                else local_y=std::min(0.f,local_y+14.492875099182129f);
                if(scalar(prop+0x180)!=local_y||scalar(prop+0x24)!=10.f+local_y)
                    fail("stage prop original rise changed its per-update offset or world result");
            }
            if(n==1||n==599||n==600||n==601||n==602||n==630||n>=658)record_recovery(n+2);
            if(n==600)rising_carry(original_plane);
        }
    }
public:
    Probe(const Module& m,const std::vector<u8>& pristine):mod(m),ram(pristine){}
    void run(unsigned id,const contact::HitBlock& hit,const contact::ActorSpec& spec,const Side& side,const Common& common,
             bool recover=false){
        const auto test=cases[id];
        prepare(side,common);
        construct();
        const float x=scalar(prop+0x20),z=scalar(prop+0x28);
        bind_receiver(hit,spec,{x+(test.far?600.f:0.f),784,z});
        publish_shapes();publish_shapes();
        constexpr u32 plane=shapes+shape_step,capsule=shapes+3*shape_step;
        if(half(plane_count)!=1||word(plane_list)!=plane||half(contact::free_count+8)||
           word(plane+0x58)!=4||scalar(plane+0x68)!=640.f||scalar(plane+0xA0)!=140.f||
           scalar(plane+0xA4)!=220.f||vector(plane+0x88)!=std::array{0u,0x3F7FFFFFu,0u}){
            std::fprintf(stderr,"stage prop plane n=%u ptr=%08X free=%u kind=%u y=%08X distance=%08X radius=%08X normal=%08X/%08X/%08X\n",
                         half(plane_count),word(plane_list),half(contact::free_count+8),word(plane+0x58),
                         word(plane+0x68),word(plane+0xA0),word(plane+0xA4),word(plane+0x88),word(plane+0x8C),word(plane+0x90));
            fail("original stage prop did not publish the role-20 plane");
        }
        CallObservation narrow{0x80035698};collision(&narrow);
        const bool held=at(actor_body+0x1DB)[0]!=0;
        if(held==test.far||narrow.count!=unsigned(held)||(held&&(word(actor_body+0x1E0)!=plane||
           word(actor_body+0x1E4)!=prop||scalar(actor_body+0x24)!=780.f)))
            fail("original actor did not acquire the exact stage prop support");
        if(recover){recovery();return;}
        const auto local=vector(actor_body+0x1F8),root=vector(actor_body+0x20),center=vector(capsule+0x64);
        const auto old_prop=contact::Point{x,10,z};
        point(prop+0x2C,old_prop);
        if(test.translate){
            // Declare motion as an update to the local placement, then let the
            // same registered source world callback produce its current root.
            point(prop+0x64,{x+32,22,z-20});
            call(0x80089554);
            if(scalar(prop+0x20)!=x+32||scalar(prop+0x24)!=22.f||scalar(prop+0x28)!=z-20)
                fail("stage prop registered world callback did not apply the declared local motion");
        }
        if(test.masked)at(contact::scene+235)[0]=1;
        const auto inlet=ram;

        // The original height/landing branch is observed on its own copy,
        // not appended to the actor-update carry prefix. The far control
        // does not qualify the absent-support stage/floor fallback.
        u32 surface_y=0,landed_y=0,landing_result=0;
        if(held){
            constexpr u32 normal=banks+0x2400;
            CPUState height{};init_cpu(height);
            height.gpr[3]=actor_body;height.gpr[4]=actor_body+0x20;height.gpr[5]=normal;
            mod.run(height,ram,0x8003E0A8,4096);
            surface_y=std::bit_cast<u32>(float(height.fpr[1]));
            if(vector(actor_body+0x20)!=root)fail("prop surface query moved the actor");
            CPUState landing{};init_cpu(landing);landing.gpr[3]=actor_body;
            mod.run(landing,ram,0x800677B0,4096);
            const auto landed=vector(actor_body+0x20);
            landing_result=landing.gpr[3];landed_y=landed[1];
            if(landing_result>1||landed[0]!=root[0]||landed[2]!=root[2]||
               vector(capsule+0x64)!=center||half(actor_body+0x1C6)!=180)
                fail("prop landing control escaped its height-only boundary");
            ram=inlet;
        }

        const auto state=[&]{return std::array{word(actor_body+0x20),word(actor_body+0x24),word(actor_body+0x28),
                                              word(actor_body+0x5E0),u32(half(actor_body+0x72))};};
        call(0x80067A70,actor_body);
        const auto direct=state();
        ram=inlet;
        CPUState update{};init_cpu(update);update.gpr[3]=actor_body;
        CallObservation carry{0x80067A70};
        const auto frontier=mod.run_to_or_return(update,ram,0x80059068,{0x80059E58,0x80264290},&carry,4096);
        if(frontier!=0x80059E58||update.lr!=0x800590B0||update.gpr[3]!=actor_body||carry.count!=1||
           carry.caller!=0x800590A4||state()!=direct||vector(capsule+0x64)!=center||
           vector(actor_body+0x1F8)!=local||half(actor_body+0x1C6)!=180||half(prop+0x1C6)!=100)
            fail("class-22 support carry differed inside the original actor update");
        std::printf("stage-prop-support: %u %u %u %u %u %u %u",id,at(prop+0x83)[0],at(prop+0x18)[0],unsigned(held),
                    narrow.count,half(archive_cells+26),half(archive_cells+32+26));
        const auto emit=[](const auto& v){for(auto a:v)std::printf(" %08X",a);};
        emit(vector(plane+0x64));emit(root);emit(local);emit(direct);emit(center);emit(vector(prop+0x20));
        std::printf(" %08X %08X %u %u %u\n",surface_y,landed_y,landing_result,half(actor_body+0x1C6),half(prop+0x1C6));
    }
};

template<class A> void source(A& data,u64 digest=0){
    if(std::fread(data.data(),1,data.size(),stdin)!=data.size())fail("stage prop input extent");
    if(digest){
        u64 hash=0xCBF29CE484222325ULL;
        for(auto b:data)hash=(hash^b)*0x100000001B3ULL;
        if(hash!=digest)fail("stage prop source archive identity");
    }
}
} // namespace

int main(int argc,char** argv){
    const bool recovery=argc==4&&!std::strcmp(argv[3],"--recovery");
    if(argc!=3&&!recovery)fail("usage: stage-prop-support-fixture qualified.dol unmodified-recomp.so [--recovery] < qualified-inputs");
    Side side{};Common common{};contact::HitBlock hit{};contact::ActorSpec spec{};
    source(side,0x214D5B801F3BD659ULL);source(common,0x4F4B1EB5F77B5BB4ULL);source(hit);source(spec);
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("stage prop input suffix");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    for(unsigned i=0;i<(recovery?1u:std::size(cases));++i)Probe(mod,original).run(i,hit,spec,side,common,recovery);
}
