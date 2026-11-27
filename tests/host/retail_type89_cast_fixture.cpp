// Original indexed type-89 phase-two requests, factory admission, and opening shapes.
#include "retail_recomp_fixture.hpp"
#include <algorithm>
#include <array>

namespace {
using namespace retail_fixture;
constexpr u32 update=0x800B8D6C,factory=0x8016CC24;
constexpr u32 objects=0x81030000,object_stride=0x800,descriptors=0x81034000,desc_stride=0x80;
constexpr u32 descriptor_row=0x81035000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 shapes=0x81036000,shape_stride=0x200,hit_block=0x81038000;
constexpr u32 bind=0x8008AC80,visit_bucket=0x80089A94,carrier_update=0x8016CD1C;
constexpr u32 attack_count=0x8043B5A0-21328,attack_registry=0x803C477C,active_tail=0x803C6F6C;
constexpr unsigned max_cells=8;
struct Capacity {unsigned objects,descriptors;};
constexpr Capacity capacities[]={{8,8},{1,8},{8,1},{0,8},{8,0},{0,0}};
constexpr unsigned refill_case=std::size(capacities)-1;
constexpr u16 pilots[]={0x0503,0x0506,0x050F,0x0512};
constexpr unsigned singles[]={3,6,4,7,5,8};
using HitBlock=std::array<u8,7840>;
struct Opening {Capacity cap;unsigned shapes;};
constexpr Opening openings[]={{{2,2},0},{{2,2},1},{{2,2},2},{{1,2},1},{{2,1},1}};
bool paired(u16 model){return model==0x0503||model==0x050F;}

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 address){
        if(address<GC_RAM_BASE||address-GC_RAM_BASE>ram.size()-4)fail("type-89 cast scratch extent");
        return ram.data()+address-GC_RAM_BASE;
    }
    u32 word(u32 address){return be32(at(address));}
    u16 half(u32 address){const auto* p=at(address);return u16(u16(p[0])<<8|p[1]);}
    float scalar(u32 address){return std::bit_cast<float>(word(address));}
    void pool(Capacity cap){
        if(cap.objects>max_cells||cap.descriptors>max_cells)fail("type-89 cast pool extent");
        put32(at(free_head+12),cap.objects?objects:0);
        put32(at(free_tail+12),cap.objects?objects+(cap.objects-1)*object_stride:0);
        put16(at(free_count+6),u16(cap.objects));
        for(unsigned i=0;i<cap.objects;++i){
            const auto cell=objects+i*object_stride;
            at(cell)[2]=3;
            put32(at(cell+4),i+1<cap.objects?cell+object_stride:0);
        }
        put32(at(0x80436380),descriptor_row);
        put32(at(descriptor_row+12),cap.descriptors?descriptors:0);
        put16(at(descriptor_row+18),u16(cap.descriptors));
        for(unsigned i=0;i<cap.descriptors;++i)
            put32(at(descriptors+i*desc_stride),i+1<cap.descriptors?descriptors+(i+1)*desc_stride:0);
    }
    void owner_inlet(u16 model,float clock){
        auto* owner=at(actor);
        owner[0]=1;owner[0x18]=1;owner[0x540]=2;
        // No actor animation cue or floor snapping is needed by the supplied phase.
        owner[0x1DB]=1;owner[0x7C9]=1;
        put16(owner+0x3E8,model);put16(owner+0x94,model);
        put32(owner+0x90,actor);
        for(unsigned offset:{0xB4,0xB8,0xC0,0xC4,0x1DC8})put_float(owner+offset,1.f);
        put16(owner+0x774,5);
        put_float(owner+0x558,clock);
    }
    void shape_pool(unsigned limit){
        if(limit>2)fail("type-89 opening shape extent");
        put32(at(free_head+16),limit?shapes:0);
        put32(at(free_tail+16),limit?shapes+(limit-1)*shape_stride:0);
        put16(at(free_count+8),u16(limit));
        for(unsigned i=0;i<limit;++i){
            const auto cell=shapes+i*shape_stride;
            at(cell)[2]=4;
            put32(at(cell+4),i+1<limit?cell+shape_stride:0);
        }
    }
    void clear_stage(){
        constexpr u32 stage=0x81050000,grid=stage+0x100,refs=stage+0x200,side=32,empty=refs+side*side*4;
        put32(at(0x8043B5A0-21316),stage);
        for(unsigned i=0;i<3;++i)put32(at(stage+28+4*i),grid);
        put_float(at(stage+52),9000.f);put_float(at(stage+56),15000.f);
        put_float(at(stage+60),-1000.f);put_float(at(stage+64),15000.f);put_float(at(stage+68),-1000.f);
        put32(at(grid),500);put32(at(grid+4),500);put32(at(grid+8),side);put32(at(grid+12),side);
        put32(at(grid+16),u32(-1000));put32(at(grid+20),u32(-1000));
        put32(at(grid+24),refs);put32(at(empty),0xFFFFFFFF);
        for(unsigned i=0;i<side*side;++i)put32(at(refs+4*i),empty);
    }
    void publish(const std::array<u32,2>& admitted,unsigned accepted){
        // The original late transform requires a render node. Give it
        // inert nodes only for that pass, then remove the scratch pointers
        // before any original initializer can replace a real render node.
        for(unsigned i=0;i<accepted;++i){
            if(word(admitted[i]+12))fail("type-89 empty visual descriptor mismatch");
            put32(at(admitted[i]+12),0x81041000+0x200*i);
        }
        CallObservation late{0x8016D398};
        CPUState late_cpu{};mod.run_observed(late_cpu,ram,0x80089554,late,1024);
        if(late.count!=accepted)fail("type-89 late transform count mismatch");
        for(unsigned i=0;i<accepted;++i)put32(at(admitted[i]+12),0);
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState pass{};mod.run(pass,ram,entry,1024);
        }
    }
public:
    Check(const Module& m,const std::vector<u8>& input):mod(m),ram(input){}
    void observe(u16 model,unsigned scenario,Capacity cap){
        pool(cap);
        owner_inlet(model,2.5f);
        auto* owner=at(actor);
        const bool pair_bank=paired(model);
        const unsigned steps=6+unsigned(pair_bank);
        unsigned cursor=0,allocated=0;
        float clock=2.5f;
        std::array<unsigned,max_cells> keys{};
        for(unsigned visit=1;visit<=81;++visit){
            if(scenario==refill_case&&visit==5){
                // Both pools gain one cell between the first timed step and the next update.
                cap={1,1};
                pool(cap);
            }
            CPUState cpu{};cpu.gpr[3]=actor;
            CallObservation requests{factory};
            const auto before=allocated;
            const bool end=visit==4+11*steps;
            if(end){
                // Stop at the original actor-return request after the negative pair.
                mod.run_to(cpu,ram,update,{0x8006A474},&requests);
                if(cpu.lr!=0x800B8EE8||cpu.gpr[3]!=actor||owner[0x1CEF])
                    fail("type-89 cast sentinel did not reach the scoped return request");
            }else{
                mod.run_observed(cpu,ram,update,requests,1024);
            }
            if(clock>0){
                clock-=1.f;
                if(requests.count)fail("type-89 cast escaped positive timer");
            }else{
                clock=10.f;
                const bool sentinel=cursor==steps,pair=pair_bank&&cursor==0;
                const unsigned count=sentinel?0:pair?2:1;
                if(requests.count!=count)fail("type-89 cast pair length or sentinel mismatch");
                for(unsigned k=0;k<count;++k){
                    const unsigned key=pair?9+k:singles[cursor-unsigned(pair_bank)];
                    if(requests.first_arguments[k]!=actor||requests.second_arguments[k]!=key||
                       requests.caller!=0x800B8E84)
                        fail("type-89 cast order or input mismatch");
                    if(allocated<cap.objects&&allocated<cap.descriptors)keys[allocated++]=key;
                }
                std::printf("type89-cast: %u %u %u %u %u %u %u %u\n",model,scenario,visit,cursor,count,
                            count?requests.second_arguments[0]:0,count==2?requests.second_arguments[1]:0,allocated-before);
                if(!sentinel)++cursor;
            }
            if(owner[0x6EB]!=cursor||word(actor+0x558)!=std::bit_cast<u32>(clock)||half(actor+0x774)!=5||
               half(free_count+6)!=cap.objects-allocated||half(descriptor_row+18)!=cap.descriptors-allocated||
               word(active_head+9*4)!=(allocated?objects+(allocated-1)*object_stride:0)||word(active_head+49*4))
                fail("type-89 cast cursor, timer, stock or pool mismatch");
            for(unsigned i=0;i<allocated;++i){
                const auto cell=objects+i*object_stride;
                const auto* child=at(cell);
                if(child[0x10]!=89||child[0x11]!=keys[i]||child[0x18]||child[0x17D]||
                   word(cell+0xC)!=0x8016CD1C||word(cell+0x8C)!=actor||word(cell+0x90)!=actor||
                   word(cell+0xE0)!=descriptors+i*desc_stride)
                    fail("type-89 admitted carrier identity, ownership or order mismatch");
            }
            if(end){
                if(cursor!=steps||requests.count)fail("type-89 cast did not stop at sentinel");
                return;
            }
        }
        fail("type-89 cast visit budget");
    }
    void opening(u16 model,unsigned scenario,const HitBlock& hit,Opening inlet){
        const auto cap=inlet.cap;
        pool(cap);
        shape_pool(inlet.shapes);
        clear_stage();
        owner_inlet(model,0.f);
        const u32 reactions=model<0x050F?0x802FEF08:0x802FEFB0;
        put32(at(actor+0x3FC),hit_block);put32(at(actor+0x27C),reactions);
        std::memcpy(at(hit_block),hit.data(),hit.size());
        put32(at(0x803C4E84),actor);
        for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
        for(unsigned id:{5u,12u}){
            auto* joint=at(actor+0x8D4+48*id);
            for(unsigned axis=0;axis<3;++axis)put_float(joint+20*axis,1.f);
            put_float(joint+12,1000.f);put_float(joint+28,200.f);put_float(joint+44,300.f);
        }
        // Let admitted initializers complete without rendering or audio playback.
        constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400;
        constexpr u32 audio=0x8103E000;
        put32(at(0x803BB374),visual);put32(at(visual+16),link);put32(at(link),table);
        for(unsigned id:{7u,8u,9u})put32(at(table+4*id),empty);
        put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
        at(0x803D0000+15080+852)[0]=1;

        CPUState release{};release.gpr[3]=actor;
        CallObservation request{factory};
        mod.run_observed(release,ram,update,request,1024);
        const bool pair=paired(model);
        const unsigned count=pair?2:1,created=std::min(count,std::min(cap.objects,cap.descriptors));
        const std::array<unsigned,2> keys{pair?9u:3u,pair?10u:0u};
        if(!created||request.count!=count||request.caller!=0x800B8E84||at(actor)[0x6EB]!=1||
           half(free_count+6)!=cap.objects-created||half(descriptor_row+18)!=cap.descriptors-created||
           word(active_head+9*4)!=objects+(created-1)*object_stride||word(active_head+49*4))
            fail("type-89 opening carrier publication mismatch");
        for(unsigned i=0;i<count;++i)
            if(request.first_arguments[i]!=actor||request.second_arguments[i]!=keys[i])
                fail("type-89 opening cast operands mismatch");
        for(unsigned i=0;i<created;++i){
            const auto cell=objects+i*object_stride;
            if(at(cell)[0x10]!=89||at(cell)[0x11]!=keys[i]||at(cell)[0x18]||
               at(cell)[0x17D]||word(cell+0xC)!=carrier_update||word(cell+0x90)!=actor)
                fail("type-89 opening did not start with an uninitialized carrier");
        }

        CPUState visitor{};visitor.gpr[3]=8;
        CallObservation opening{bind};
        mod.run_observed(visitor,ram,visit_bucket,opening,1024);
        const unsigned accepted=std::min(inlet.shapes,created);
        if(opening.count!=created||opening.caller!=0x8016CE0C||
           half(free_count+8)!=inlet.shapes-accepted||
           word(active_head+49*4)!=(accepted?shapes+(accepted-1)*shape_stride:0)||
           word(active_tail+49*4)!=(accepted?shapes:0))
            fail("type-89 opening bind count, head, or tail mismatch");
        std::array<unsigned,2> attempt_keys{};
        std::array<u32,2> admitted{};
        for(unsigned i=0;i<created;++i){
            const unsigned ordinal=created-1-i;
            const auto cell=objects+ordinal*object_stride;
            const auto* child=at(cell);
            attempt_keys[i]=child[0x11];
            if(opening.first_arguments[i]!=cell||opening.second_arguments[i]!=3||
               attempt_keys[i]!=keys[ordinal]||child[0x18]!=(i<accepted?1:2)||
               child[0x17D]!=(i<accepted?16:0))
                fail("type-89 opening bind attempt order or gate mismatch");
            if(i>=accepted)continue;
            const auto shape=shapes+i*shape_stride;
            const auto* published=at(shape);
            const float x=child[0x11]==9?800.f:child[0x11]==10?1200.f:1000.f;
            if(published[0]!=1||published[1]!=49||published[2]!=4||published[0x12]!=3||published[0x14]!=3||
               word(shape+0x20)!=cell||word(shape+0x24)!=actor||word(shape+0x28)!=hit_block+0xAA0+80*3||
               word(shape+0x2C)!=reactions||word(shape+4)!=(i+1<accepted?shape+shape_stride:0)||
               word(shape+8)!=(i?shape-shape_stride:0)||word(shape+0x60)!=cell+0x114||scalar(cell+0x20)!=x||
               scalar(cell+0x24)!=(pair?600.f:200.f)||scalar(cell+0x28)!=300.f)
                fail("type-89 opening shape identity, credit, or origin mismatch");
            const auto visual_descriptor=word(cell+0xE0);
            if(visual_descriptor!=descriptors+ordinal*desc_stride||word(visual_descriptor+12))
                fail("type-89 empty visual descriptor mismatch");
            admitted[i]=visual_descriptor;
        }
        publish(admitted,accepted);
        const unsigned opening_registered=half(attack_count);
        if(opening_registered||word(attack_registry)||half(actor+0x774)!=5)
            fail("type-89 opening late transform, attack registry or stock mismatch");
        for(unsigned i=0;i<accepted;++i){
            const auto shape=shapes+i*shape_stride,cell=word(shape+0x20);
            // The late transform placed the raw origin sphere at the carrier root.
            for(unsigned axis=0;axis<3;++axis)
                if(scalar(cell+0x114+12+16*axis)!=scalar(cell+0x20+4*axis)||
                   scalar(shape+0x30+4*axis)!=scalar(cell+0x20+4*axis))
                    fail("type-89 opening publication matrix did not follow the carrier root");
        }
        unsigned first_registered=0;
        std::array<unsigned,2> registry_keys{};
        // Number bucket visits from one (the initializer). Nothing else
        // schedules the owner, recipients, a type-5 area, or collision resolution.
        for(unsigned n=2;n<=33;++n){
            CPUState tick{};tick.gpr[3]=8;
            CallObservation area_request{0x80085E00};
            mod.run_observed(tick,ram,visit_bucket,area_request,4096);
            publish(admitted,accepted);
            const unsigned entries=half(attack_count);
            if(entries!=(n==32?accepted:0)||area_request.count||half(actor+0x774)!=5)
                fail("type-89 opening activation or clear-stage unlink disagreed");
            if(n<33){
                if(half(free_count+8)!=inlet.shapes-accepted||
                   word(active_head+49*4)!=(accepted?shapes+(accepted-1)*shape_stride:0))
                    fail("type-89 held or acquired unexpected shape slots before unlink");
                for(unsigned i=0;i<accepted;++i)
                    if(half(word(shapes+i*shape_stride+0x20)+0x272)!=(n==32?0:0xFFFF))
                        fail("type-89 exclusion mask changed outside its timed transition");
            }
            if(entries){
                first_registered=n;
                for(unsigned i=0;i<entries;++i){
                    const auto shape=word(attack_registry+4*i);
                    if(shape!=shapes+(accepted-1-i)*shape_stride||half(word(shape+0x20)+0x272))
                        fail("type-89 activated registry order or exclusion mask mismatch");
                    registry_keys[i]=at(word(shape+0x20))[0x11];
                }
            }
        }
        if(word(active_head+49*4)||half(free_count+8)!=inlet.shapes)
            fail("type-89 clear-stage shape unlink did not return pool slots");
        std::printf("type89-opening: %u %u %u %u %u %u %u %u %u %u %u\n",model,scenario,count,created,
                    attempt_keys[0],attempt_keys[1],accepted,opening_registered,first_registered,registry_keys[0],registry_keys[1]);
    }
};
}

int main(int argc,char** argv){
    const bool opening=argc==4&&!std::strcmp(argv[3],"--opening");
    if(argc!=3&&!opening)fail("usage: type89-cast-fixture qualified.dol unmodified-recomp.so [--opening]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(opening){
        std::array<HitBlock,std::size(pilots)> hits{};
        for(auto& hit:hits)
            if(std::fread(hit.data(),1,hit.size(),stdin)!=hit.size())fail("type-89 opening hit block extent");
        if(std::getc(stdin)!=EOF)fail("unexpected type-89 opening payload");
        for(unsigned p=0;p<std::size(pilots);++p)
            for(unsigned i=0;i<std::size(openings);++i){
                Check test(mod,original);
                test.opening(pilots[p],i,hits[p],openings[i]);
            }
        return 0;
    }
    for(const auto model:pilots)
        for(unsigned i=0;i<std::size(capacities);++i){
            Check test(mod,original);
            test.observe(model,i,capacities[i]);
        }
}
