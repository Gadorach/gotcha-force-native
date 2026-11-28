// All normal body queues through the original binder and list rebuild, not a pose or hit replay.
#include "retail_stationary_contact.hpp"
#include "retail_declared_stage_grid.hpp"
#include <algorithm>
#include <cmath>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr unsigned models=206,seats=6,slots=32,views=6;
constexpr u32 fighters=0x81020000,stride=0x1E00,bank=0x81050000,cells=0x81060000,cell_stride=0x200;
constexpr u32 actors=0x803C4E84,count_base=0x8043B5A0-21328;
constexpr u32 wall_stage=0x81070000;
constexpr std::array<u32,10> lists{0x803C477C,0x803C417C,0x803C3B7C,0x803C357C,0x803C2F7C,
                                  0x803C297C,0x803C237C,0x803C1D7C,0x803C177C,0x803C117C};
constexpr unsigned descriptor_size=80;
using Registry=std::array<u32,lists.size()>;
struct Report {
    unsigned bound,special;
    std::array<Registry,views> visits;
    bool operator==(const Report&)const=default;
};
struct GeometryView {
    std::array<float,3> columns;
    float actor_scale;
    // 0 applies to every bound matrix, 1 only to the root, 2 only to joints.
    unsigned selected;
};
constexpr std::array<GeometryView,11> geometry_views{{
    {{1,1,1},1,0}, {{1,1,1},2,0}, {{2,.5f,4},1,0}, {{-2,.5f,4},1,0},
    {{0,1,1},1,0}, {{1,.005f,1},1,0}, {{1,1,.01f},1,0}, {{1,1,.02f},1,0},
    {{1,1,0},1,1}, {{1,1,0},1,2},
    {{1,1,std::bit_cast<float>(0x3C23D70Bu)},1,0},
}};
constexpr std::array<float,3> translation{256,128,-64};
enum class Mode {registry,spheres,capsules,wall};

class Probe {
    const Module& mod;
    const std::vector<u8>& initial;
    std::vector<u8> ram;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void store16(u32 a,u16 x){put16(at(a),x);}
    void call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;
        mod.run(cpu,ram,entry,4096);
    }
    void rebuild(){call(0x8008A28C);call(0x80089AEC);}
    void basis(u32 at_matrix){
        for(unsigned axis=0;axis<3;++axis)put_float(at(at_matrix+20*axis),1.f);
    }
    Registry registry(u32 fighter,unsigned bound){
        Registry results{};
        for(unsigned n=0;n<lists.size();++n){
            const unsigned count=half(count_base-2*n);
            if(count>bound)fail("normal body registry extent");
            unsigned previous=bound;
            for(unsigned i=0;i<count;++i){
                const auto shape=word(lists[n]+4*i);
                if(shape<cells||shape>=cells+bound*cell_stride||(shape-cells)%cell_stride||
                   word(shape+0x20)!=fighter||word(shape+0x24)!=fighter)
                    fail("normal body registry used a foreign or unbound shape cell");
                const unsigned ordinal=(shape-cells)/cell_stride;
                if(ordinal>=previous)fail("normal body registry repeated a cell or changed reverse queue order");
                results[n]|=1u<<ordinal;
                previous=ordinal;
            }
        }
        return results;
    }
    void geometry(u16 model,u32 fighter,unsigned bound,Mode mode){
        const bool capsules=mode==Mode::capsules;
        store16(fighter+0x272,0);
        for(unsigned view=0;view<(capsules?4u:geometry_views.size());++view){
            const auto& pose=geometry_views[view];
            const auto matrix=[&](u32 pointer,bool root){
                const bool changed=!pose.selected||(pose.selected==1?root:!root);
                for(unsigned axis=0;axis<3;++axis){
                    put_float(at(pointer+20*axis),changed?pose.columns[axis]:1.f);
                    put_float(at(pointer+12+16*axis),translation[axis]);
                }
            };
            matrix(fighter+0x1AA4,true);
            for(unsigned i=0;i<32;++i)matrix(fighter+0x8D4+0x30*i,false);
            put_float(at(fighter+0xC0),pose.actor_scale);
            rebuild();
            const auto lists=registry(fighter,bound);
            if(lists[1]!=lists[9]||lists[0]||lists[8])
                fail("normal body geometry altered the separate receiver/attack roles");
            std::printf("geometry-registry: %04X %u %08X %08X\n",model,view,lists[1],lists[9]);
            for(unsigned pass=0;pass<(view<4?2u:1u);++pass){
                if(pass){
                    rebuild();
                    if(registry(fighter,bound)!=lists)fail("settled normal shape changed its registry membership");
                }
                for(unsigned ordinal=0;ordinal<bound;++ordinal){
                    const auto cell=cells+cell_stride*ordinal,d=word(cell+0x28);
                    if(!at(cell)[0]||d<bank+32||d>=bank+32+33*descriptor_size||word(cell+0x20)!=fighter)
                        fail("normal body geometry lost a bound cell");
                    if(view>=4||at(d)[0]!=(capsules?2u:0u)||!(half(d+2)&2))continue;
                    if(!(lists[1]&(1u<<ordinal)))fail("normal body refused an admitted matrix");
                    if(capsules){
                        std::printf("geometry-capsule: %04X %u %u %u %u",model,view,pass,ordinal,word(cell+0x58));
                        for(u32 loc:{0x64u,0x70u,0x8Cu,0x98u,0xD4u,0xE0u})
                            for(unsigned axis=0;axis<3;++axis)std::printf(" %08X",word(cell+loc+4*axis));
                        std::printf(" %08X %08X\n",word(cell+0x88),word(cell+0x7C));
                    }else{
                        std::printf("geometry-sphere: %04X %u %u %u %08X %08X %08X %08X %08X %08X %08X %08X\n",
                                    model,view,pass,ordinal,word(cell+0x64),word(cell+0x68),word(cell+0x6C),
                                    word(cell+0x70),word(cell+0x74),word(cell+0x78),word(cell+0x88),word(cell+0x7C));
                    }
                }
            }
        }
    }
    void wall(u16 model,u32 fighter,unsigned bound){
        constexpr float z=-64.f;
        constexpr u32 stage_pass=0x8002E1C8,bounds=0x8003FB00,centers=0x8002CE44;
        const auto scalar=[&](u32 addr){return std::bit_cast<float>(word(addr));};
        float tangent=0;
        u32 stage_bits=0,sphere_bits=0,receive_bits=0;
        for(unsigned i=0;i<bound;++i){
            const auto d=word(cells+i*cell_stride+0x28);
            const auto roles=half(d+2);
            if(roles&2)receive_bits|=1u<<i;
            if(!(roles&8))continue;
            stage_bits|=1u<<i;
            if(at(d)[0]==1)continue;
            if(at(d)[0]!=0)fail("roster wall stage kind outside declared census");
            const float radius=scalar(d+24),local_x=scalar(d+12);
            if(!(radius>=40.f&&radius<=980.f&&(local_x==0.f||local_x==-10.f)))
                fail("roster wall sphere outside reviewed radius/offset domain");
            sphere_bits|=1u<<i;
            tangent=std::max(tangent,100.f+radius-local_x);
        }
        if(!stage_bits||std::popcount(sphere_bits)>2)fail("roster wall shape-count budget");
        const auto bound_state=ram;
        for(unsigned variant=0;variant<7;++variant){
            std::memcpy(ram.data(),bound_state.data(),ram.size());
            const float y=variant>=5?128.f:1280.f;
            const float lower_reference=variant==6?-10000.f:0.f;
            const auto x=variant==3&&tangent?tangent:variant==4&&tangent?std::nextafter(tangent,3000.f):130.f;
            const auto matrix=[&](u32 pointer){
                put_float(at(pointer+12),x);put_float(at(pointer+28),y);put_float(at(pointer+44),z);
            };
            matrix(fighter+0x1AA4);
            for(unsigned i=0;i<32;++i)matrix(fighter+0x8D4+0x30*i);
            put_float(at(fighter+0x20),x);put_float(at(fighter+0x24),y);put_float(at(fighter+0x28),z);
            put_float(at(fighter+0xC0),1.f);store16(fighter+0x1C6,180);
            store16(fighter+0x272,variant==1?8:0);
            put_float(at(0x803B0708),lower_reference);
            if(variant==2){
                // Same relocated stage, but omit the first reference of each
                // secondary list in this guest-only empty-grid fork.
                const auto table=word(word(wall_stage+32)+24);
                for(unsigned i=0;i<64;++i)store(word(table+4*i),0xFFFFFFFF);
            }
            rebuild();
            const auto lists=registry(fighter,bound);
            if(lists[3]!=(variant==1?0u:stage_bits)||lists[1]!=receive_bits||lists[9]!=receive_bits||
               lists[0]||lists[8]||word(fighter+0x1CC)||word(fighter+0x1D0)||word(fighter+0x1D4))
                fail("roster wall did not publish its exact open or masked queue");
            std::array<std::array<u32,3>,slots> before{};
            std::array<std::array<u32,6>,slots> ends{};
            u32 admitted=0;
            for(unsigned i=0;i<bound;++i){
                const auto cell=cells+i*cell_stride,d=word(cell+0x28);
                for(unsigned k=0;k<3;++k)before[i][k]=word(cell+0x64+4*k);
                for(unsigned k=0;k<6;++k)ends[i][k]=word(cell+0x8C+4*k);
                if((sphere_bits&(1u<<i))&&variant!=1){
                    if(word(cell+0x58)||scalar(cell+0x88)!=scalar(d+24)||
                       scalar(cell+0x64)!=x+scalar(d+12)||scalar(cell+0x68)!=y+scalar(d+16)||
                       scalar(cell+0x6C)!=z+scalar(d+20))
                        fail("roster wall source sphere did not use its exact rigid center and radius");
                    if(scalar(cell+0x68)-scalar(cell+0x88)>=lower_reference)admitted|=1u<<i;
                }
            }
            CPUState cpu{};CallObservation trace{bounds};
            trace.sample=[](const CPUState& c)->u32{
                if(c.lr!=0x80035ECC)return 0xFFFFFFFF;
                const auto shape=be32(c.ram+(0x803B04D4-GC_RAM_BASE));
                const auto prior=std::bit_cast<float>(be32(c.ram+(fighters+0x1CC-GC_RAM_BASE)));
                if(shape<cells||shape>=cells+slots*cell_stride||(shape-cells)%cell_stride||
                   be32(c.ram+(shape+0x20-GC_RAM_BASE))!=fighters||
                   !(prior>=0.f&&prior<=980.f&&float(u32(prior))==prior))
                    fail("roster wall bounds sample lost its shape or integer correction");
                return ((shape-cells)/cell_stride+1)*0x10000+u32(prior);
            };
            mod.run_observed(cpu,ram,stage_pass,trace,4096);
            // Capture only the first bounds call in each solver visit. The
            // solver can also query bounds later on another path.
            std::array<u32,2> visits{};
            unsigned entries=0;
            if(trace.count>std::size(trace.samples))fail("roster wall exhausted its bounds capture");
            for(unsigned i=0;i<trace.count;++i){
                if(trace.samples[i]==0xFFFFFFFF)continue;
                if(entries>=visits.size())fail("roster wall exceeded its source sphere count");
                visits[entries++]=trace.samples[i];
            }
            if(entries!=unsigned(std::popcount(admitted))){
                std::fprintf(stderr,"roster wall %04X variant=%u bounds=%u first=%08X entries=%u spheres=%08X\n",
                             model,variant,trace.count,trace.caller,entries,sphere_bits);
                fail("roster wall solver count did not match the original kind-0 filter");
            }
            call(centers);
            const auto shift=scalar(fighter+0x1CC);
            if(word(fighter+0x1D0)||word(fighter+0x1D4)||scalar(fighter+0x24)!=y||scalar(fighter+0x28)!=z||
               scalar(fighter+0x20)!=x+shift||half(fighter+0x1C6)!=180||registry(fighter,bound)!=lists)
                fail("roster wall changed an unrelated axis, HP or publication");
            unsigned moved=0;
            for(unsigned i=0;i<bound;++i){
                const auto cell=cells+cell_stride*i;
                if(scalar(cell+0x64)!=std::bit_cast<float>(before[i][0])+shift||
                   word(cell+0x68)!=before[i][1]||word(cell+0x6C)!=before[i][2])
                    fail("roster wall broad centers did not consume the exact actor vector");
                for(unsigned k=0;k<6;++k)if(word(cell+0x8C+4*k)!=ends[i][k])
                    fail("roster wall changed allocated geometry beyond the broad center");
                moved+=unsigned(word(cell+0x64)!=before[i][0]);
            }
            std::printf("roster-wall: %04X %u %u %08X %08X %08X %08X %u %08X %08X %08X %08X %08X %u %u %u\n",
                        model,variant,bound,lists[3],sphere_bits,admitted,lists[1],entries,visits[0],visits[1],
                        std::bit_cast<u32>(x),word(fighter+0x20),word(fighter+0x1CC),
                        at(fighter+0x1DD)[0],at(fighter+0x274)[0],moved);
        }
    }
public:
    Probe(const Module& mod,const std::vector<u8>& initial):mod(mod),initial(initial),ram(initial){}
    Report run(u16 model,const contact::HitBlock& hit,unsigned seat,unsigned capacity,Mode mode=Mode::registry){
        std::memcpy(ram.data(),initial.data(),ram.size());
        store(0x8043B5A0-31852,contact::scene);
        // Kind 4 asks the allocator for zero view cells; its precheck still reads the pool header.
        store(0x8043B5A0-21024,0x81068000);
        for(unsigned i=0;i<seats;++i)store(actors+4*i,fighters+stride*i);
        const u32 fighter=fighters+stride*seat,bucket=49+seat;
        at(fighter)[0]=1;at(fighter+0x18)[0]=1;
        at(fighter+0x3E4)[0]=at(fighter+0x3E7)[0]=u8(seat);
        store16(fighter+0x3E8,model);
        std::memcpy(at(bank),hit.data(),hit.size());
        store(contact::scene+0x80+4*seat,bank);
        call(0x8003CC24,fighter);
        if(word(fighter+0x3FC)!=bank||half(fighter+0x272)!=0xEF||at(fighter+0x276)[0])
            fail("normal body registry did not start with the original reset defaults");
        basis(fighter+0x1AA4);
        for(unsigned i=0;i<32;++i)basis(fighter+0x8D4+0x30*i);
        store(fighter+0x76C,0xFFFFFFFF);
        for(unsigned i=0;i<capacity;++i){
            const auto cell=cells+cell_stride*i;
            at(cell+2)[0]=4;
            store(cell+4,i+1<capacity?cell+cell_stride:0);
        }
        if(capacity){
            store(contact::free_head+16,cells);
            store(contact::free_tail+16,cells+cell_stride*(capacity-1));
            store16(contact::free_count+8,u16(capacity));
        }
        call(0x8008AB30,fighter);
        Report out{};
        const auto head=contact::active_head+4*bucket;
        unsigned queued=0;
        while(queued<slots&&hit[queued]<0x80)++queued;
        const unsigned allocated=std::min(queued,capacity);
        for(auto cur=word(head);cur;cur=word(cur+8)){
            if(out.bound>=allocated||cur!=cells+cell_stride*(allocated-1-out.bound)){
                std::fprintf(stderr,"body bind %04X seat=%u capacity=%u queue=%u ordinal=%u cell=%08X head=%08X free=%u\n",
                             model,seat,capacity,queued,out.bound,cur,word(head),half(contact::free_count+8));
                fail("normal body binder skipped, reordered or overran its finite list");
            }
            const unsigned ordinal=allocated-1-out.bound;
            const u32 step=hit[ordinal],d=bank+32+descriptor_size*step;
            const unsigned selector=at(d+1)[0];
            if(step>=33||(selector>=32&&selector!=0x80)||word(cur+0xC)!=0x8008A65C||
               !at(cur)[0]||at(cur+1)[0]!=bucket||at(cur+2)[0]!=4||
               word(cur+0x20)!=fighter||word(cur+0x24)!=fighter||word(cur+0x28)!=d||
               word(cur+0x60)!=fighter+(selector==0x80?0x1AA4:0x8D4+0x30*selector)||
               at(cur+0x12)[0]!=step||at(cur+0x11)[0]!=(half(d+4)&255)||
               half(cur+0x48)!=half(d+6)||half(cur+0x4A)!=half(d+8))
                fail("normal body shape lost its exact descriptor, ownership, or source matrix slot");
            ++out.bound;
        }
        for(unsigned ordinal=0;ordinal<out.bound&&out.special<4;++ordinal)
            if(half(bank+32+descriptor_size*hit[ordinal]+2)&0x200){
                if(word(fighter+0x530+4*out.special)!=cells+cell_stride*ordinal)
                    fail("normal body secondary shape index lost the bound cell");
                ++out.special;
            }
        if(out.bound!=std::min(queued,capacity)||half(contact::free_count+8)!=capacity-out.bound||
           at(fighter+0x501)[0]!=out.special)
            fail("normal body bind and secondary index disagreed with available capacity");
        if(mode==Mode::wall)wall(model,fighter,out.bound);
        else if(mode!=Mode::registry)geometry(model,fighter,out.bound,mode);
        for(unsigned view=0;mode==Mode::registry&&view<views;++view){
            if(view==1)store16(fighter+0x272,0xC3);
            if(view==2)store16(fighter+0x272,0);
            if(view==3)put_float(at(fighter+0x720),1.f);
            if(view==4){store(fighter+0x720,0);store(fighter+0x76C,1);}
            if(view==5)store(fighter+0x76C,0);
            rebuild();
            out.visits[view]=registry(fighter,out.bound);
            if(half(contact::free_count+8)!=capacity-out.bound)
                fail("normal body publication modified allocation");
        }
        call(0x8008AFF0,fighter);
        if(at(fighter+0x1D8)[0]!=1)fail("normal body close did not mark its bound shapes");
        rebuild();
        if(word(head)||half(contact::free_count+8)!=capacity||registry(fighter,0)!=Registry{})
            fail("normal body close did not empty the registered buckets and restore cells");
        u32 returned=0,tail=0;
        for(auto cur=word(contact::free_head+16);cur;cur=word(cur+4)){
            if(cur<cells||cur>=cells+capacity*cell_stride||(cur-cells)%cell_stride||at(cur)[0]||at(cur+2)[0]!=4)
                fail("normal body did not return the exact original kind-4 cells");
            const u32 bit=1u<<((cur-cells)/cell_stride);
            if(returned&bit)fail("normal body free list cycled");
            returned|=bit;tail=cur;
        }
        if(std::popcount(returned)!=int(capacity)||word(contact::free_tail+16)!=tail)
            fail("normal body free list lost a cell after close");
        return out;
    }
};
} // namespace

int main(int argc,char** argv){
    auto mode=Mode::registry;
    if(argc==4&&!std::strcmp(argv[3],"--geometry"))mode=Mode::spheres;
    if(argc==4&&!std::strcmp(argv[3],"--capsules"))mode=Mode::capsules;
    if(argc==4&&!std::strcmp(argv[3],"--wall"))mode=Mode::wall;
    if(argc!=3&&mode==Mode::registry)
        fail("usage: body-registry-fixture qualified.dol unmodified-recomp.so [--geometry|--capsules|--wall] < qualified-normal-queues");
    auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    if(mode==Mode::wall)retail_declared_stage_grid::bind(mod,original,wall_stage,true,100.f,2000.f);
    Probe probe(mod,original);
    unsigned prior=0;
    for(unsigned i=0;i<models;++i){
        std::array<u8,2> id{};
        contact::HitBlock hit{};
        if(std::fread(id.data(),1,id.size(),stdin)!=id.size()||
           std::fread(hit.data(),1,hit.size(),stdin)!=hit.size())fail("normal body registry input extent");
        const unsigned model=(unsigned(id[0])<<8)|id[1];
        if((i&&model<=prior)||model>0x0FFF)fail("normal body registry input identity");
        prior=model;
        // Fail closed before executing any unqualified descriptor as an original shape kind or matrix reference.
        for(unsigned n=0;n<slots&&hit[n]<0x80;++n){
            const unsigned d=32+descriptor_size*hit[n];
            if(hit[n]>=33||hit[d]>5||(hit[d+1]>=32&&hit[d+1]!=0x80)||
               (hit[d+3]&0x81)||hit[d+6]||hit[d+7]||hit[d+8]!=255||hit[d+9]!=255)
                fail("normal body descriptor outside verified kind, role or phase bounds");
        }
        if(mode!=Mode::registry){probe.run(u16(model),hit,0,32,mode);continue;}
        for(unsigned capacity:{32u,1u,0u}){
            const auto row=probe.run(u16(model),hit,0,capacity);
            for(unsigned seat=1;capacity==32&&seat<seats;++seat)
                if(probe.run(u16(model),hit,seat,capacity)!=row)fail("normal body registry depended on the physical seat");
            for(unsigned view=0;view<views;++view){
                std::printf("body-registry: %04X %u %u %u %u",model,capacity,view,row.bound,row.special);
                for(auto registry:row.visits[view])std::printf(" %08X",registry);
                std::putchar('\n');
            }
        }
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("trailing normal body registry payload");
}
