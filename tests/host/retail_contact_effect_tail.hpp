// Shared bounded observation of the original post-HP contact-effect gate.
#pragma once
#include "retail_stationary_contact.hpp"

namespace retail_contact_effect_tail {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
inline constexpr unsigned case_count=5,miss_case=4;
inline constexpr u32 helper=0x80019550,contact_point=0x803B0720;
inline constexpr u32 effect=0x81062000,cells=0x81063000,pool=0x81064000,active_head=0x803C706C;

inline void observe(const Module& mod,std::vector<u8>& ram,unsigned source_type,u16 model,unsigned scenario,
                    u32 attack,u32 record,unsigned selector,unsigned cost,unsigned family,unsigned loss_on_hit){
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto half=[&](u32 address){return contact::half(ram,address);};
    const auto point=[&](u32 address){
        return contact::Point{std::bit_cast<float>(word(address)),std::bit_cast<float>(word(address+4)),
                              std::bit_cast<float>(word(address+8))};
    };
    if(scenario>=case_count||cost<1||cost>8||half(contact::receiver+0x1C6)!=180||
       at(contact::receiver)[0x2AF]||half(contact::receiver+0x20C)||
       !half(attack+0x26C)||half(contact::free_count+4)||word(active_head+31*4))
        fail("contact-effect replay needs an unresolved published pair and empty class-2 pool");
    const bool hit=scenario!=miss_case,held=scenario==1,has_cell=scenario!=2;
    const unsigned offered=scenario==3?cost-1:cost;
    const bool asked=hit&&!held,made=asked&&has_cell&&offered==cost;
    const unsigned gate=held||asked?4:0, damage=hit?loss_on_hit:0;
    const unsigned attack_token=half(attack+0x26C),shape_free=half(contact::free_count+8);
    at(contact::receiver)[0x2AF]=held?4:0;
    put32(at(0x80436380),pool);put32(at(pool+12),offered?cells:0);put16(at(pool+18),u16(offered));
    for(unsigned i=0;i<offered;++i)put32(at(cells+0x80*i),i+1<offered?cells+0x80*(i+1):0);
    put32(at(contact::free_head+8),has_cell?effect:0);put32(at(contact::free_tail+8),has_cell?effect:0);
    put16(at(contact::free_count+4),u16(has_cell));at(effect)[2]=2;
    const auto saved=ram;

    CPUState amount{};CallObservation amount_call{contact::amount_entry};
    const auto amount_front=mod.run_to_or_return(amount,ram,contact::pair_pass,{contact::amount_entry},&amount_call);
    if(amount_front!=(hit?contact::amount_entry:0)||amount_call.count!=unsigned(hit)||
       (hit&&(amount_call.caller!=0x8002F108||amount_call.arguments[0]!=record||
              amount_call.arguments[1]!=attack||amount_call.arguments[2]!=contact::receiver))||
       half(contact::receiver+0x1C6)!=180)
        fail("contact-effect replay did not start from the exact attack amount record");
    std::memcpy(ram.data(),saved.data(),ram.size());

    CPUState visual{};CallObservation request{helper};
    const auto cut=mod.run_to_or_return(visual,ram,contact::pair_pass,{helper},&request);
    if(cut!=(asked?helper:0)||request.count!=unsigned(asked)||
       (asked&&(request.caller!=0x8002F87C||request.arguments[0]!=attack||
                request.arguments[1]!=contact_point||request.arguments[2]!=selector))||
       half(contact::receiver+0x1C6)!=180-damage||
       at(contact::receiver)[0x2AF]!=gate||half(pool+18)!=offered||at(effect)[0])
        fail("contact-effect gate changed before the effect allocation");
    const auto requested_point=asked?point(contact_point):contact::Point{};
    std::memcpy(ram.data(),saved.data(),ram.size());

    CPUState finish{};CallObservation effect_call{helper};
    mod.run_observed(finish,ram,contact::pair_pass,effect_call,4096);
    if(effect_call.count!=unsigned(asked)||(asked&&(effect_call.caller!=0x8002F87C||
       effect_call.arguments[1]!=contact_point||effect_call.arguments[2]!=selector))||
       half(contact::receiver+0x1C6)!=180-damage||word(actor+0x418)!=damage||word(contact::receiver+0x41C)!=damage||
       half(contact::receiver+0x20C)!=(hit?attack_token:0)||at(contact::receiver)[0x26E]!=unsigned(hit)||
       at(contact::receiver)[0x2AF]!=gate||half(contact::free_count+4)!=unsigned(has_cell&&!made)||
       half(contact::free_count+8)!=shape_free||half(pool+18)!=offered-cost*unsigned(made)||
       word(active_head+31*4)!=(made?effect:0)||bool(at(effect)[0])!=made){
        std::fprintf(stderr,"impact model=%04X case=%u calls=%u gate=%u hp=%u "
                     "ledgers=%u/%u slots=%u/%u/%u live=%u bucket=%08X\n",model,scenario,
                     effect_call.count,at(contact::receiver)[0x2AF],half(contact::receiver+0x1C6),
                     word(actor+0x418),word(contact::receiver+0x41C),half(contact::free_count+4),
                     half(contact::free_count+8),half(pool+18),at(effect)[0],word(active_head+31*4));
        fail("contact-effect gate or HP depended on the allocator result");
    }
    if(made&&(at(effect)[0x10]||at(effect)[0x11]!=family||at(effect)[0x12]||at(effect)[0x13]!=cost||at(effect)[0x18]||
       word(effect+0x0C)!=0x800196D0||word(effect+0x8C)!=attack||word(effect+0x90)!=actor||
       point(effect+0x20)!=requested_point))
        fail("contact-effect constructor lost its source point, family or credit");

    const auto final_hp=half(contact::receiver+0x1C6),left=half(pool+18);
    if(hit){
        // Reopen just the visual latch on the retained pair. A duplicate
        // attack token must not reach either HP or the fresh visual gate.
        at(contact::receiver)[0x2AF]=0;
        CPUState duplicate{};CallObservation second{helper};
        const auto tail=mod.run_to_or_return(duplicate,ram,contact::pair_pass,{contact::hp_write,helper},&second);
        if(tail||second.count||at(contact::receiver)[0x2AF]||half(contact::receiver+0x1C6)!=final_hp||
           half(pool+18)!=left||half(contact::receiver+0x20C)!=attack_token||
           word(actor+0x418)!=damage||word(contact::receiver+0x41C)!=damage)
            fail("contact-effect duplicate reached HP or a reopened effect gate");
    }
    std::printf("impact-tail: %u %u %u %u %u %u %u %u %u %u %u %.5f %.5f %.5f\n",
                source_type,model,scenario,selector,damage,final_hp,unsigned(asked),unsigned(made),
                gate,left,hit?attack_token:0,double(requested_point[0]),double(requested_point[1]),double(requested_point[2]));
}
}
