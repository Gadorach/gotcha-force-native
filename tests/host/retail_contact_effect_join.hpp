// One accepted pair through the model update of the effect it actually allocated.
#pragma once
#include "retail_contact_effect_archive.hpp"
#include "retail_contact_effect_tail.hpp"

namespace retail_contact_effect_join {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace art=retail_contact_effect_archive;
namespace tail=retail_contact_effect_tail;
inline constexpr u32 owner=0x81065140,visual_header=0x81065100,view=0x81065000;
inline constexpr u32 attack_count=0x8043B5A0-21328;

inline void observe(const Module& mod,std::vector<u8>& ram,const art::Source& source,bool type3,
                    unsigned source_type,u16 model,u32 attack,u32 record,
                    unsigned selector,unsigned cost,unsigned family,unsigned damage){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto point=[&](u32 address){
        return contact::Point{std::bit_cast<float>(word(address)),std::bit_cast<float>(word(address+4)),
                              std::bit_cast<float>(word(address+8))};
    };
    constexpr auto effect=tail::effect,pool=tail::pool,cells=tail::cells,body=contact::receiver;
    constexpr u32 bucket_head=tail::active_head+4*art::bucket,visual_row=pool+3*28;
    const auto attack_token=half(attack+0x26C),shape_free=half(contact::free_count+8);
    const auto attacks=half(attack_count),receivers=half(contact::receiver_count);
    if(half(body+0x1C6)!=180||half(body+0x20C)||at(body+0x2AF)[0]||!attack_token||
       half(contact::free_count+4)||word(bucket_head)||cost<1||cost>8)
        fail("joined contact needs one unresolved hit and an empty class-2 inlet");
    put32(at(0x80436380),pool);
    put32(at(pool+12),cells);put16(at(pool+18),u16(cost));
    for(unsigned i=0;i<cost;++i)put32(at(cells+0x80*i),i+1<cost?cells+0x80*(i+1):0);
    put32(at(contact::free_head+8),effect);put32(at(contact::free_tail+8),effect);
    put16(at(contact::free_count+4),1);at(effect)[2]=2;

    // Validate the exact original amount record in a separate entry cut.
    {
        auto fork=ram;
        CPUState cpu{};CallObservation amount{contact::amount_entry};
        mod.run_to(cpu,fork,contact::pair_pass,{contact::amount_entry},&amount);
        if(amount.count!=1||amount.caller!=0x8002F108||amount.arguments[0]!=record||
           amount.arguments[1]!=attack||amount.arguments[2]!=body)
            fail("joined contact chose a different attack amount record");
    }
    CPUState pair{};CallObservation requested{tail::helper};
    mod.run_observed(pair,ram,contact::pair_pass,requested,4096);
    const auto hp_matches=[&](){
        return half(body+0x1C6)==180-damage&&word(actor+0x418)==damage&&word(body+0x41C)==damage&&
               half(body+0x20C)==attack_token&&at(body+0x26E)[0]==1&&at(body+0x2AF)[0]==4;
    };
    const auto saved_point=point(tail::contact_point);
    if(requested.count!=1||requested.caller!=0x8002F87C||requested.arguments[0]!=attack||
       requested.arguments[1]!=tail::contact_point||requested.arguments[2]!=selector||!hp_matches()||
       word(bucket_head)!=effect||at(effect)[0]!=1||at(effect+0x11)[0]!=family||at(effect+0x13)[0]!=cost||
       at(effect+0x18)[0]||at(effect+0x88)[0]!=at(attack+0x88)[0]||word(effect+0x8C)!=attack||word(effect+0x90)!=actor||
       half(contact::free_count+4)||half(pool+18)||point(effect+0x20)!=saved_point)
        fail("joined contact did not allocate the exact effect at the accepted hit point");

    // Advance the exact returned effect, not a newly issued helper request.
    // Overwrite just the shared collision scratch to rule out rebinding it.
    constexpr contact::Point scratch{655.f,-750.f,-32.f};
    for(unsigned i=0;i<scratch.size();++i)put_float(at(tail::contact_point+4*i),scratch[i]);
    art::Setup setup(mod,ram);
    setup.init_heap();
    put32(at(0x8043B5A0-21024),pool);
    if(type3)setup.init_visual_header(pool,visual_header);
    setup.publish(source,pool,owner,view);
    setup.call(0x80254288);
    const auto scene=word(0x8043B5A0-31852);
    if(!scene||half(scene+484)||half(scene+486))fail("joined contact needs empty starting scene counters");
    if(selector==1){
        // Bound loading to one real child joint, as in the previous receipt.
        put16(at(scene+484),31);put16(at(scene+486),511);
    }
    const unsigned lane=at(effect+0x88)[0],mode=at(scene+81)[0];
    const bool initialized=type3||selector==0;
    const unsigned model_count=!initialized?0:selector==8?2:1;
    const std::array<unsigned,2> expected_records{
        selector==0?21:selector==8?53:2+(mode==3?0:lane),selector==8?54u:0u
    };
    if(selector==1&&lane>1)fail("joined burst has an unsupported parent lane");
    u32 roots[2]{};
    const auto stable_hit=[&](){
        return hp_matches()&&point(effect+0x20)==saved_point&&point(tail::contact_point)==scratch&&
               half(contact::free_count+8)==shape_free&&half(attack_count)==attacks&&half(contact::receiver_count)==receivers;
    };
    const auto tick=[&](CallObservation& models){
        CPUState cpu{};cpu.gpr[3]=art::bucket;
        if(mod.run_to_or_return(cpu,ram,0x80089A94,{0x80264290},&models,4096))
            fail("joined visual crossed an unsupported model assertion boundary");
        setup.prepare_view_zero();
        if(!stable_hit()||word(art::source_globals)!=owner||half(owner+26)!=157)
            fail("joined visual update changed the accepted hit or archive owner");
    };
    CallObservation models{0x8024F834};
    tick(models);
    auto descriptor=word(art::draw_heads+5*4);
    const auto phase=at(effect+0x18)[0];
    if(models.count!=model_count||phase!=(initialized?1:2)||at(effect+0x82)[0]!=unsigned(initialized)||
       word(bucket_head)!=effect||half(pool+18)||half(contact::free_count+4))
        fail("joined contact effect initialized or refused at the wrong boundary");
    if(model_count){
        if(models.caller!=(selector==0?0x80006FF8u:selector==8?0x800072B0u:0x800071F8u))
            fail("joined contact took a different model loader");
        const auto records=word(word(owner+16));
        for(unsigned i=model_count;i>0;--i){
            const auto index=i-1,cell=cells+0x80*index;
            const auto root=models.first_arguments[index],joint=word(cell+12);
            if(descriptor!=cell||word(cell+20)!=expected_records[index]||
               root<art::source_bank+32||root>art::source_bank+source.bytes.size()-64||
               word(word(records+4*expected_records[index]))!=root||joint<art::block||joint>=art::arena_end)
                fail("joined contact did not enqueue an original source-archive model");
            roots[index]=root-art::source_bank-32;
            descriptor=word(descriptor+40);
        }
    }
    if(descriptor||half(scene+484)!=(selector==1?31u+unsigned(initialized):0)||
       half(scene+486)!=(selector==1?511u+unsigned(initialized):0))
        fail("joined visual made a different queue or scene debit");
    const auto claimed_type3=selector!=0&&initialized;
    if(word(effect+0xDC)!=(claimed_type3?visual_header:0)||
       half(visual_row+18)!=unsigned(type3&&!claimed_type3))
        fail("joined visual used the wrong type-3 storage policy");
    unsigned returned=0;
    if(!initialized){
        // A later cell offer must not retry this already refused emitter.
        setup.init_visual_header(pool,visual_header);
        for(unsigned visit=2;visit<=3;++visit){
            CallObservation later{0x8024F834};
            tick(later);
            if(later.count||word(art::draw_heads+5*4)||word(effect+0xDC)||half(visual_row+18)!=1||
               word(visual_row+12)!=visual_header||half(contact::free_count+4)!=unsigned(visit==3)||
               word(bucket_head)!=(visit==3?0:effect)||half(pool+18)!=(visit==3?cost:0))
                fail("joined refused emitter retried or returned its battle storage late");
        }
        returned=3;
    }
    std::printf("impact-art: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %.5f %.5f %.5f\n",
                source.slot,source_type,model,unsigned(type3),selector,damage,half(body+0x1C6),attack_token,
                unsigned(at(body+0x2AF)[0]),lane,mode,unsigned(phase),model_count,roots[0],roots[1],returned,
                unsigned(claimed_type3),double(saved_point[0]),double(saved_point[1]),double(saved_point[2]));
}
}
