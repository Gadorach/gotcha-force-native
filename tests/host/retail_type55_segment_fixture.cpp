// Original type-55 timing and its type-11 segment's four-shape group.
#include "retail_stationary_contact.hpp"
#include <array>
#include <cmath>

namespace {
using namespace retail_fixture;
namespace pair=retail_stationary_contact;
using pair::Point;
using pair::HitBlock;
using pair::ActorSpec;
constexpr u32 carrier=0x81030000,segment=0x81030800,shapes=0x81032000,shape_stride=0x200;
constexpr u32 descriptors=0x81033000,descriptor_row=0x81033800,hit_block=0x81034000;
constexpr u32 factory=0x8018EEA4,update=0x8018EFA4,transform=0x8018F3FC;
constexpr u32 segment_factory=0x80139A6C,segment_update=0x80139B80,segment_transform=0x80139CC0;
constexpr u32 attack_count=0x8043B5A0-21328,attack_registry=0x803C477C,owner_table=0x80351C78;
constexpr unsigned steps[]={13,28,29,30};
struct Case {unsigned slots,y;bool spare;float rate;};
constexpr Case cases[]={{0,210,true,1},{1,210,true,1},{2,210,true,1},{3,210,true,1},{4,210,true,1},
                        {2,60,true,1},{4,60,true,1},{4,210,false,1},{4,210,false,0.5}};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){return pair::at(ram,va);}
    u32 word(u32 va){return be32(at(va));}
    u16 half(u32 va){return pair::half(ram,va);}
    float scalar(u32 va){return std::bit_cast<float>(word(va));}
    Point point(u32 va){return {scalar(va),scalar(va+4),scalar(va+8)};}
    constexpr static u32 shape_cell(unsigned n){return shapes+n*shape_stride;}
    void shape_pool(unsigned start,unsigned count){
        if(start+count>4||half(pair::free_count+8)||word(pair::free_head+16))fail("type-55 shape pool extent");
        put32(at(pair::free_head+16),count?shape_cell(start):0);
        put32(at(pair::free_tail+16),count?shape_cell(start+count-1):0);
        put16(at(pair::free_count+8),u16(count));
        for(unsigned i=0;i<count;++i){
            const auto cell=shape_cell(start+i);
            at(cell)[2]=4;
            put32(at(cell+4),i+1<count?shape_cell(start+i+1):0);
        }
    }
    void pools(bool spare,unsigned count){
        put32(at(0x80436380),descriptor_row);
        put32(at(descriptor_row+12),descriptors);put16(at(descriptor_row+18),2);
        put32(at(descriptors),descriptors+0x80);
        put32(at(pair::free_head+12),carrier);put32(at(pair::free_tail+12),spare?segment:carrier);
        put16(at(pair::free_count+6),spare?2:1);
        at(carrier)[2]=3;at(segment)[2]=3;put32(at(carrier+4),spare?segment:0);
        shape_pool(0,count);
    }
    void parent_pose(){
        // Run the original matrix builder to the first renderer write.
        CPUState late{};late.gpr[3]=carrier;
        mod.run_to(late,ram,transform,{0x800076D0});
        if(late.lr!=0x8018F4D0||scalar(carrier+0x120)!=100.f||
           scalar(carrier+0x130)!=200.f||scalar(carrier+0x140)!=300.f)
            fail("type-55 carrier matrix or renderer boundary changed");
    }
    void publish() {
        CPUState matrix{};matrix.gpr[3]=segment;
        mod.run(matrix,ram,segment_transform);
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState pass{};
            mod.run(pass,ram,entry,1024);
        }
    }
    void advance(bool due){
        CPUState tick{};tick.gpr[3]=carrier;
        CallObservation request{segment_factory};
        mod.run_observed(tick,ram,update,request,1024);
        if(request.count!=unsigned(due)||(due&&(request.caller!=0x8018F384||request.arguments[0]!=carrier||
            request.arguments[1]!=carrier+0x38||request.arguments[2]!=carrier+0x9C||request.arguments[3]!=8))||
           half(actor+0x784)!=5)
            fail("type-55 segment clock or request operands changed");
    }
    void shape_identities(unsigned count){
        if(half(attack_count)!=count||word(pair::active_head+49*4)!=(count?shape_cell(count-1):0))
            fail("type-55 shape count disagreed with the admitted group prefix");
        for(unsigned i=0;i<count;++i){
            const auto cell=shape_cell(i);
            if(word(cell+0x20)!=segment||word(cell+0x24)!=actor||word(cell+0x2C)!=owner_table||
               word(cell+0x28)!=hit_block+0xAA0+steps[i]*80||at(cell)[0x12]!=steps[i]||
               at(cell)[0x14]!=8||at(cell)[0x11]!=26||word(attack_registry+4*i)!=shape_cell(count-1-i))
                fail("type-55 shape prefix or reverse registry order changed");
        }
    }
public:
    Check(const Module& m,const std::vector<u8>& input):mod(m),ram(input){}
    void observe(unsigned scenario,const HitBlock& hit,const HitBlock& body,const ActorSpec& owner_spec,const ActorSpec& body_spec){
        const auto rule=cases[scenario];
        pools(rule.spare,rule.slots);
        put32(at(0x803C4E84),actor);
        for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
        auto* owner=at(actor);
        owner[0]=1;owner[0x18]=1;
        put16(owner+0x3E8,0x0704);put16(owner+0x94,0x0704);put16(owner+0x784,5);
        put32(owner+0x90,actor);put32(owner+0x27C,owner_table);put32(owner+0x3FC,hit_block);
        put_float(owner+0xB4,1.f);put_float(owner+0xC0,1.f);put_float(owner+0x1DC8,rule.rate);
        std::memcpy(at(hit_block),hit.data(),hit.size());
        auto* joint=owner+0x8D4;
        for(unsigned axis=0;axis<3;++axis)put_float(joint+20*axis,1.f);
        put_float(joint+12,100.f);put_float(joint+28,200.f);put_float(joint+44,300.f);
        constexpr u32 visual=0x8103F000,link=visual+0x40,table=visual+0x80,empty=visual+0x400;
        put32(at(0x803BB4C0+0x600),visual);put32(at(visual+16),link);put32(at(link),table);
        for(unsigned id:{7u,8u})put32(at(table+4*id),empty);
        CPUState ctor{};ctor.gpr[3]=actor;ctor.gpr[4]=0;
        mod.run(ctor,ram,factory);
        if(ctor.gpr[3]!=carrier||at(carrier)[0x10]!=55||at(carrier)[0x11]||at(carrier)[0x18]||
           word(carrier+0xC)!=update||word(carrier+0x8C)!=actor||word(carrier+0x90)!=actor||
           word(pair::active_head+9*4)!=carrier||half(pair::free_count+6)!=unsigned(rule.spare)||half(descriptor_row+18))
            fail("type-55 carrier publication changed");
        advance(false);
        if(at(carrier)[0x18]!=1||at(carrier)[0x19]||half(carrier+0x154)||half(carrier+0x156)!=10||
           point(carrier+0x20)!=Point{100,200,300}||word(pair::active_head+49*4))
            fail("type-55 initializer did not set the first socket and timer");
        parent_pose();
        for(unsigned n=1;n<=10;++n){
            advance(false);
            if(at(carrier)[0x18]!=1||at(carrier)[0x19]!=unsigned(n==10)||
               half(carrier+0x154)!=(n==10?0:n)||half(carrier+0x156)!=(n==10?60:10))
                fail("type-55 warmup transitioned on the wrong visit");
            parent_pose();
        }
        advance(true);
        if(!rule.spare){
            for(unsigned n=12;n<=69;++n){
                advance(true);
                if(word(carrier+8)||at(carrier)[0x18]!=1||half(carrier+0x154)!=n-10)
                    fail("type-55 refused stream changed its request cadence");
            }
            advance(false);
            if(at(carrier)[0x18]!=2||half(carrier+0x154)!=60||word(carrier+8)||half(pair::free_count+8)!=rule.slots)
                fail("type-55 emitting window did not close without a shape");
            std::printf("type55-window: %u 10 59 70\n",scenario);
            return;
        }
        if(word(carrier+8)!=segment||at(segment)[0x10]!=11||at(segment)[0x11]!=8||at(segment)[0x18]||
           word(segment+0x8C)!=carrier||word(segment+0x90)!=actor||
           word(segment+0x144)!=carrier+0x38||word(segment+0x148)!=carrier+0x9C)
            fail("type-55 segment creation changed");
        CPUState init{};init.gpr[3]=segment;
        CallObservation bind{0x8008AC80};
        mod.run_observed(init,ram,segment_update,bind);
        if(bind.count!=1||bind.caller!=0x80139BE0||bind.arguments[0]!=segment||bind.arguments[1]!=8||
           at(segment)[0x18]!=(rule.slots?1:2)||half(pair::free_count+8))
            fail("type-55 segment did not consume the admitted group's shape cells");
        if(!rule.slots){
            shape_pool(0,4);
            CPUState reject{};reject.gpr[3]=segment;
            mod.run_observed(reject,ram,segment_update,bind);
            if(at(segment)[0x18]!=3||bind.count!=1||word(pair::active_head+49*4)||half(pair::free_count+8)!=4)
                fail("type-55 refused segment retried when shape cells became available");
            std::printf("type55-shapes: %u 0 0 0 0 0\n",scenario);
            return;
        }
        publish();
        shape_identities(rule.slots);
        const auto near=point(carrier+0x38);
        if(half(segment+0x26C)!=1)fail("type-55 opening shapes did not retain one segment token");
        for(unsigned i=0;i<rule.slots;++i){
            if(point(shape_cell(i)+0x64)!=Point{near[0],near[1]+30.f+60.f*i,near[2]}||
               scalar(shape_cell(i)+0x7C)!=30.f)
                fail("type-55 near sphere did not keep its per-descriptor offset");
        }
        parent_pose();
        advance(true);
        const auto far=point(carrier+0x9C);
        const Point center{near[0],near[1]+float(rule.y),(near[2]+far[2])*0.5f};
        pair::bind_sasuke(mod,ram,center,body,owner_spec,body_spec,300);
        shape_pool(rule.slots,4-rule.slots);
        CPUState sweep{};sweep.gpr[3]=segment;
        mod.run_observed(sweep,ram,segment_update,bind);
        if(at(segment)[0x18]!=2||point(segment+0x20)!=far||bind.count!=1||far[2]-near[2]!=15000.f){
            std::fprintf(stderr,"type-55 phase=%u bind=%u z near/far/child %.5f %.5f %.5f\n",
                         at(segment)[0x18],bind.count,double(near[2]),double(far[2]),double(scalar(segment+0x28)));
            fail("type-55 swept sample or no-rebind boundary changed");
        }
        publish();
        shape_identities(rule.slots);
        unsigned overlaps=0;
        if(half(pair::receiver_count)!=1||word(pair::receiver_registry)!=pair::body||
           point(pair::body+0x64)!=center||scalar(pair::body+0x7C)!=22.f||half(pair::free_count+8)!=4-rule.slots)
            fail("type-55 receiver or later free shape pool changed");
        for(unsigned i=0;i<rule.slots;++i){
            const auto cell=shape_cell(i);
            if(point(cell+0x3C)!=Point{near[0],near[1]+30.f+60.f*i,near[2]}||
               point(cell+0x30)!=Point{far[0],far[1]+30.f+60.f*i,far[2]}||scalar(cell+0x88)!=30.f)
                fail("type-55 swept sphere did not keep its per-descriptor offset");
            overlaps+=unsigned(std::abs(scalar(cell+0x34)-center[1])<52.f);
        }
        const auto amount=pair::pair_hp(mod,ram,segment,owner_table+26*24,bool(overlaps));
        const unsigned hp=half(pair::receiver+0x1C6),token=half(pair::receiver+0x20C);
        if(hp!=180-amount||half(actor+0x1C6)!=300||word(actor+0x418)!=amount||word(pair::receiver+0x41C)!=amount||
           half(segment+0x26C)!=1||word(pair::receiver+0x290)!=(overlaps?segment:0)||
           at(pair::receiver)[0x26E]!=unsigned(bool(overlaps))||word(carrier+0x290)||at(carrier)[0x1D9]||
           token!=unsigned(bool(overlaps))||at(segment)[0x1D9]!=(overlaps?16:0))
            fail("type-55 multi-shape overlap used the wrong actor, HP or token");
        if(overlaps){
            pair::pair_hp(mod,ram,segment,owner_table+26*24,false);
            if(half(pair::receiver+0x1C6)!=hp||at(pair::receiver)[0x26E]!=1||
               word(actor+0x418)!=amount||word(pair::receiver+0x41C)!=amount)
                fail("type-55 same-segment token wrote damage twice");
        }
        std::printf("type55-shapes: %u %u %u %u %u %u\n",scenario,rule.slots,overlaps,amount,hp,token);
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: type55-segment-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    HitBlock attack{},body{};
    ActorSpec owner{},receiver{};
    for(auto* b:{&attack,&body})
        if(std::fread(b->data(),1,b->size(),stdin)!=b->size())fail("type-55 hit block extent");
    for(auto* b:{&owner,&receiver})
        if(std::fread(b->data(),1,b->size(),stdin)!=b->size())fail("type-55 spec extent");
    if(std::getc(stdin)!=EOF)fail("unexpected type-55 payload");
    for(unsigned i=0;i<std::size(cases);++i){
        Check check(mod,original);
        check.observe(i,attack,body,owner,receiver);
    }
}
