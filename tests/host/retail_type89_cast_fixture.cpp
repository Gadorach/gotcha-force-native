// Original indexed type-89 phase-two requests, factory admission, and opening shapes.
#include "retail_stationary_contact.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
using namespace retail_fixture;
constexpr u32 update=0x800B8D6C,factory=0x8016CC24;
constexpr u32 objects=0x81030000,object_stride=0x800,descriptors=0x81034000,desc_stride=0x80;
constexpr u32 descriptor_row=0x81035000;
constexpr u32 free_head=0x803C716C,free_tail=0x803C718C,free_count=0x803C71AC,active_head=0x803C706C;
constexpr u32 shapes=0x81036000,shape_stride=0x200,hit_block=0x81038000;
constexpr u32 bind=0x8008AC80,visit_bucket=0x80089A94,carrier_update=0x8016CD1C;
constexpr u32 attack_count=0x8043B5A0-21328,attack_registry=0x803C477C,active_tail=0x803C6F6C;
namespace contact_replay=retail_stationary_contact;
using contact_replay::receiver;
using contact_replay::body;
using contact_replay::scene;
using contact_replay::receiver_count;
using contact_replay::receiver_registry;
constexpr u32 make_area=0x80085E00,area_update=0x80086048;
constexpr u32 common_bank=0x81048000,area_shape=shapes+shape_stride,area_record=0x802C4988;
constexpr unsigned max_cells=8;
struct Capacity {unsigned objects,descriptors;};
constexpr Capacity capacities[]={{8,8},{1,8},{8,1},{0,8},{8,0},{0,0}};
constexpr unsigned refill_case=std::size(capacities)-1;
constexpr u16 pilots[]={0x0503,0x0506,0x050F,0x0512};
constexpr unsigned singles[]={3,6,4,7,5,8};
using HitBlock=std::array<u8,7840>;
using ActorSpec=std::array<u8,432>;
using CommonBank=std::array<u8,21504>;
enum class AreaCase {no_spare,original_pose,offset_pose};
struct Followup {AreaCase scenario;const CommonBank& bank;};
struct Contact {
    unsigned distance,owner_hp;const HitBlock& body;const ActorSpec& owner;const ActorSpec& target;
    const Followup* follow=nullptr;
};
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
    void place_receiver(const std::array<float,3>& center){
        contact_replay::place_receiver(ram,center);
    }
    void contact_inlet(const Contact& inlet,u32 attack){
        const unsigned axis=at(attack)[0x11]==3?2:0;
        std::array<float,3> center{};
        for(unsigned i=0;i<3;++i)
            center[i]=(scalar(attack+0x20+4*i)+scalar(shapes+0x30+4*i))*0.5f+float(i==axis?inlet.distance:0);
        contact_replay::bind_sasuke(mod,ram,center,inlet.body,inlet.owner,inlet.target,inlet.owner_hp);
    }
    unsigned pair_hp(u32 attack,u32 record,bool expected){
        return contact_replay::pair_hp(mod,ram,attack,record,expected);
    }
    unsigned contact_write(u16 model,const Contact& inlet,u32 attack,u32 record){
        const bool expected=inlet.distance<52;
        const bool pair=paired(model);
        for(unsigned axis=0;axis<3;++axis){
            const auto current=scalar(shapes+0x30+4*axis),previous=scalar(shapes+0x3C+4*axis);
            const auto delta=pair?current-previous:previous-current;
            if(current!=scalar(attack+0x20+4*axis)||(axis==(pair?1u:0u)?!(delta>9999.f&&delta<10001.f):delta!=0))
                fail("type-89 contact sweep endpoints changed");
        }
        if(half(attack_count)!=1||word(attack_registry)!=shapes||half(receiver_count)!=1||word(receiver_registry)!=body||
           scalar(shapes+0x88)!=30.f||scalar(shapes+0x7C)<5000.f||scalar(body+0x7C)!=22.f||
           scalar(body+0x64)-scalar(shapes+0x64)!=float(pair?inlet.distance:0)||
           scalar(body+0x68)!=scalar(shapes+0x68)||scalar(body+0x6C)-scalar(shapes+0x6C)!=float(pair?0:inlet.distance)||
           at(attack)[0x1D9]||at(attack)[0x17C]||half(receiver+0x1C6)!=180){
            std::fprintf(stderr,"type-89 contact regs=%u/%u radius=%.5f/%.5f delta=%.5f/%.5f/%.5f flags=%u/%u hp=%u\n",
                         half(attack_count),half(receiver_count),scalar(shapes+0x7C),scalar(body+0x7C),
                         scalar(body+0x64)-scalar(shapes+0x64),scalar(body+0x68)-scalar(shapes+0x68),
                         scalar(body+0x6C)-scalar(shapes+0x6C),at(attack)[0x1D9],at(attack)[0x17C],half(receiver+0x1C6));
            fail("type-89 contact geometry, registration or HP inlet mismatch");
        }
        const unsigned damage=pair_hp(attack,record,expected);
        if(!expected){
            if(at(attack)[0x1D9]||half(receiver+0x1C6)!=180||half(receiver+0x1C8)!=0xFFFF||
               half(receiver+0x20C)||at(receiver)[0x26E]||word(attack+0x290)||word(actor+0x290)||word(receiver+0x290)||
               word(actor+0x418)||word(receiver+0x41C))
                fail("type-89 miss called amount, latched contact or wrote a ledger");
            return 0;
        }
        const unsigned hp=half(receiver+0x1C6);
        const auto token=half(attack+0x26C);
        if(damage!=(model<0x050F?17u:8u)||hp!=180-damage||half(receiver+0x1C8)!=180||
           at(attack)[0x1D9]!=16||half(receiver+0x20C)!=token||!token||at(receiver)[0x26E]!=1||
           word(attack+0x290)!=receiver||word(actor+0x290)!=receiver||word(receiver+0x290)!=attack||
           word(actor+0x418)!=damage||word(receiver+0x41C)!=damage||half(actor+0x1C6)!=inlet.owner_hp)
            fail("type-89 contact HP, credit or token mismatch");
        if(pair_hp(attack,record,false)||half(receiver+0x1C6)!=hp||
           at(receiver)[0x26E]!=1||word(actor+0x418)!=damage||word(receiver+0x41C)!=damage)
            fail("type-89 repeated resolution did not refuse the same token");
        return damage;
    }
    void follow_area(u16 model,const Followup& options,u32 parent,u32 descendant,u32 parent_visual,unsigned direct){
        const bool spare=options.scenario!=AreaCase::no_spare,moved=options.scenario==AreaCase::offset_pose;
        const unsigned initial_hp=180-direct;
        const std::array center{scalar(parent+0x2A0),scalar(parent+0x2A4),scalar(parent+0x2A8)};
        if(!descendant||!direct||word(active_head+49*4)!=shapes||half(free_count+8)||
           half(receiver+0x20C)!=1||half(receiver+0x20E)||at(receiver)[0x26E]!=1)
            fail("type-89 follow-up did not start from genuine parent contact");
        std::memcpy(at(common_bank),options.bank.data(),options.bank.size());
        put32(at(scene+152),common_bank);
        constexpr u32 table=0x8103F000+0x80,empty=0x8103F000+0x400;
        for(unsigned id:{18u,22u,23u})put32(at(table+4*id),empty);
        if(spare){
            at(area_shape)[2]=4;
            put32(at(free_head+16),area_shape);put32(at(free_tail+16),area_shape);put16(at(free_count+8),1);
        }
        if(moved)place_receiver({center[0],center[1],center[2]+100.f});
        const std::array<u32,2> visual{parent_visual,0};
        unsigned first=0,unlink=0,retired=0,parent_retired=0,hp_loss=0;
        float initial_radius=0,hit_radius=0,peak_radius=0;
        unsigned area_token=0;
        for(unsigned n=33;n<=90;++n){
            CPUState visit{};visit.gpr[3]=8;
            CallObservation bind_area{0x8008AE3C};
            mod.run_observed(visit,ram,visit_bucket,bind_area,4096);
            if(bind_area.count!=unsigned(n==33)||
               (n==33&&(bind_area.caller!=0x800860E0||bind_area.arguments[0]!=actor||
                       bind_area.arguments[1]!=descendant||bind_area.arguments[2]!=110||
                       at(descendant)[0x18]!=(spare?1:2)||at(descendant)[0x86]!=unsigned(spare)||
                       be32(at(descendant+0x8C))!=parent||be32(at(descendant+0x90))!=actor)))
                fail("type-89 follow-up group bind or refusal mismatch");
            if(n==33&&(!at(shapes)[0]||!at(parent)[0x1D8]||half(free_count+8)||
                      word(active_head+49*4)!=(spare?area_shape:shapes)||
                      (spare&&word(area_shape+8)!=shapes)))
                fail("type-89 area bind did not precede the parent shape's return");
            publish(visual,at(parent)[0]&&word(parent+0x100)==0x8016D398?1u:0u);
            const bool bound=word(active_head+49*4)==area_shape;
            if((word(active_head+49*4)!=0&&!bound)||(!spare&&bound)||
               (bound&&(word(area_shape+0x20)!=descendant||word(area_shape+0x24)!=actor||
                       word(area_shape+0x28)!=common_bank+0x400+150*80||word(area_shape+0x2C)!=0x802C4760||
                       at(area_shape)[0x14]!=110||at(area_shape)[0x12]!=150)))
                fail("type-89 follow-up took wrong shape, credit or bank");
            const unsigned expected=spare&&n>33&&n<50;
            if(half(attack_count)!=expected||(expected&&word(attack_registry)!=area_shape)||
               half(receiver_count)!=1||word(receiver_registry)!=body||half(actor+0x774)!=5||
               half(free_count+8)!=unsigned(!bound)+unsigned(spare))
                fail("type-89 follow-up registry or shape availability mismatch");
            if(bound){
                if(half(descendant+0x26C)!=2)fail("type-89 area did not keep a fresh token");
                area_token=half(descendant+0x26C);
                const float radius=scalar(area_shape+0x7C);
                if(n==33)initial_radius=radius;
                peak_radius=std::max(peak_radius,radius);
                for(unsigned axis=0;axis<3;++axis)
                    if(scalar(area_shape+0x64+axis*4)!=center[axis])
                        fail("type-89 area shape did not stay at the saved point");
                if(moved&&(scalar(body+0x64)!=center[0]||std::abs(scalar(body+0x68)-center[1])>0.001f||
                           scalar(body+0x6C)!=center[2]+100.f))
                    fail("type-89 displaced area receiver geometry changed");
                if(!moved&&(std::abs(scalar(body+0x64)-center[0]-(paired(model)?22.f:0.f))>0.002f||
                            std::abs(scalar(body+0x68)-center[1])>0.002f||
                            std::abs(scalar(body+0x6C)-center[2]-(paired(model)?0.f:22.f))>0.002f))
                    fail("type-89 original area receiver geometry changed");
            }
            if(n==34&&spare&&!bound)fail("type-89 area missing on first growth visit");
            if(spare&&!bound&&!unlink)unlink=n;
            const bool hit=spare&&n==(moved?36:34);
            const auto applied=pair_hp(descendant,area_record,hit);
            if(hit){
                first=n;hp_loss=applied;hit_radius=scalar(area_shape+0x7C);
                if(applied!=17||half(receiver+0x1C8)!=initial_hp||
                   word(descendant+0x290)!=receiver||word(receiver+0x290)!=descendant||word(actor+0x290)!=receiver)
                    fail("type-89 area got the wrong HP, recipient or credit");
                pair_hp(descendant,area_record,false);
            }
            if(half(receiver+0x1C6)!=initial_hp-hp_loss||word(actor+0x418)!=direct+hp_loss||
               word(receiver+0x41C)!=direct+hp_loss||half(receiver+0x20C)!=1||
               half(receiver+0x20E)!=(hp_loss?2:0)||at(receiver)[0x26E]!=1+unsigned(bool(hp_loss)))
                fail("type-89 follow-up changed the wrong HP or token ledger");
            if(!retired&&!at(descendant)[0])retired=n;
            if(!parent_retired&&!at(parent)[0]){
                parent_retired=n;
                if(spare&&(!bound||retired||word(active_head+9*4)!=descendant||
                           half(free_count+6)!=2||half(descriptor_row+18)!=2))
                    fail("type-89 follow-up did not retain its resources independently of the parent");
            }
            if(retired&&parent_retired)break;
        }
        if(!retired||!parent_retired||word(active_head+9*4)||half(free_count+6)!=3||half(descriptor_row+18)!=4||
           (spare&&(std::abs(initial_radius-3.1f)>0.001f||std::abs(peak_radius-387.5f)>0.001f||unlink!=50))){
            std::fprintf(stderr,"type-89 follow-up first=%u unlink=%u retirement=%u/%u pool=%u/%u radii=%.4f/%.4f/%.4f\n",
                         first,unlink,parent_retired,retired,half(free_count+6),half(descriptor_row+18),
                         double(initial_radius),double(hit_radius),double(peak_radius));
            fail("type-89 follow-up curve or pool retirement mismatch");
        }
        std::printf("type89-followup: %u %u %u %u %u %u %u %u %u %u %.4f %.4f %.4f\n",
                    model,unsigned(options.scenario),direct,hp_loss,half(receiver+0x1C6),area_token,first,unlink,retired,parent_retired,
                    double(initial_radius),double(hit_radius),double(peak_radius));
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
    void opening(u16 model,unsigned scenario,const HitBlock& hit,Opening inlet,const Contact* target=nullptr){
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
        unsigned damage=0;
        // Resolve only the first admitted carrier; the second was refused.
        const auto contact_carrier=objects+(created-1)*object_stride;
        // Number bucket visits from one (the initializer). Nothing else
        // schedules the owner, recipients, a type-5 area, or collision resolution.
        for(unsigned n=2;n<=33;++n){
            CPUState tick{};tick.gpr[3]=8;
            CallObservation area_request{make_area};
            if(target&&n==33){
                const auto before=target->follow?ram:std::vector<u8>{};
                const auto cut=mod.run_to_or_return(tick,ram,visit_bucket,{area_update},&area_request);
                const u32 descendant=word(contact_carrier+8);
                if(area_request.count!=unsigned(bool(damage))||bool(cut)!=bool(damage)||bool(descendant)!=bool(damage)||
                   (damage&&(area_request.caller!=0x8016D30C||area_request.arguments[0]!=contact_carrier||
                             area_request.arguments[1]!=contact_carrier+0x2A0||area_request.arguments[2]!=16||
                             !descendant||tick.gpr[3]!=descendant||at(descendant)[0x10]!=5||at(descendant)[0x11]!=16||
                             word(descendant+0x8C)!=contact_carrier||word(descendant+0x90)!=actor))||
                   half(receiver+0x1C6)!=180-damage||half(actor+0x774)!=5)
                    fail("type-89 accepted contact did not select the scoped area request");
                if(damage)
                    for(unsigned axis=0;axis<3;++axis)
                        if(word(descendant+0x20+4*axis)!=word(contact_carrier+0x2A0+4*axis))
                            fail("type-89 area did not inherit the saved contact point");
                if(target->follow){
                    std::memcpy(ram.data(),before.data(),ram.size());
                    follow_area(model,*target->follow,contact_carrier,descendant,admitted[0],damage);
                    return;
                }
                std::printf("type89-contact: %u %u %u %u %u %u %u %u %.4f %.4f %.4f\n",
                            model,target->distance,at(contact_carrier)[0x11],damage,half(receiver+0x1C6),
                            half(receiver+0x20C),at(receiver)[0x26E],area_request.count,
                            damage?scalar(contact_carrier+0x2A0)-scalar(contact_carrier+0x20):0.f,
                            damage?scalar(contact_carrier+0x2A4)-scalar(contact_carrier+0x24):0.f,
                            damage?scalar(contact_carrier+0x2A8)-scalar(contact_carrier+0x28):0.f);
                return;
            }
            mod.run_observed(tick,ram,visit_bucket,area_request,4096);
            if(target&&n==32)contact_inlet(*target,contact_carrier);
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
                if(target)damage=contact_write(model,*target,contact_carrier,reactions+3*24);
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
    const bool contact=argc==4&&!std::strcmp(argv[3],"--contact");
    const bool followup=argc==4&&!std::strcmp(argv[3],"--followup");
    if(argc!=3&&!opening&&!contact&&!followup)
        fail("usage: type89-cast-fixture qualified.dol unmodified-recomp.so [--opening|--contact|--followup]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(opening||contact||followup){
        std::array<HitBlock,std::size(pilots)> hits{};
        for(auto& hit:hits)
            if(std::fread(hit.data(),1,hit.size(),stdin)!=hit.size())fail("type-89 opening hit block extent");
        if(contact||followup){
            HitBlock body_hit{};
            std::array<ActorSpec,std::size(pilots)> attacker{};
            ActorSpec defender{};
            if(std::fread(body_hit.data(),1,body_hit.size(),stdin)!=body_hit.size())fail("type-89 body block extent");
            for(auto& record:attacker)
                if(std::fread(record.data(),1,record.size(),stdin)!=record.size())fail("type-89 attacker spec extent");
            if(std::fread(defender.data(),1,defender.size(),stdin)!=defender.size())
                fail("type-89 receiver spec extent");
            constexpr unsigned hp[]={2500,2000,1250,1000};
            if(followup){
                CommonBank bank{};
                if(std::fread(bank.data(),1,bank.size(),stdin)!=bank.size()||std::getc(stdin)!=EOF)
                    fail("type-89 follow-up common bank extent");
                for(unsigned i=0;i<std::size(pilots);++i)
                    for(auto scenario:{AreaCase::no_spare,AreaCase::original_pose,AreaCase::offset_pose}){
                        Check test(mod,original);
                        const Followup options{scenario,bank};
                        const Contact target{51,hp[i],body_hit,attacker[i],defender,&options};
                        test.opening(pilots[i],0,hits[i],{{3,4},1},&target);
                    }
                return 0;
            }
            if(std::getc(stdin)!=EOF)fail("unexpected type-89 contact payload");
            for(unsigned i=0;i<std::size(pilots);++i)
                for(unsigned distance:{30u,51u,52u,53u}){
                    Check test(mod,original);
                    const Contact target{distance,hp[i],body_hit,attacker[i],defender};
                    test.opening(pilots[i],0,hits[i],{{3,4},1},&target);
                }
            return 0;
        }
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
