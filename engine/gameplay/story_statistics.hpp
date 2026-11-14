#pragma once
//Pointer-free Story+1434..150Bhex, source803F0/804FC/8258C/825D8/82648.
//Field candidates only: no full mission handler, result authority, wall-clock
//conversion, reward calculation or successful native campaign-load receipt.
#include <array>
#include <span>
#include "engine/gameplay/story_start.hpp"
namespace gf {
inline constexpr std::size_t kStoryStatisticsBytes=0xd8;
struct StoryStatistics {
    u32 result_true=0,result_false=0,callback_remainder=0,callback_carries=0;
    std::array<u32,16> partner_true{},partner_false{},partner_use{};
    u32 counter1504=0,counter1508=0;
    bool operator==(const StoryStatistics&)const=default;
};
inline bool story_statistics_decode_declared(ByteView b,StoryStatistics* out){
    if(!out||!b.data||b.size!=kStoryStatisticsBytes)return false;
    StoryStatistics next;next.result_true=b.be32(0);next.result_false=b.be32(4);next.callback_remainder=b.be32(8);next.callback_carries=b.be32(12);
    for(unsigned i=0;i<16;++i){next.partner_true[i]=b.be32(0x10+i*4);next.partner_false[i]=b.be32(0x50+i*4);next.partner_use[i]=b.be32(0x90+i*4);}
    next.counter1504=b.be32(0xd0);next.counter1508=b.be32(0xd4);*out=next;return true;
}
inline bool story_statistics_encode(const StoryStatistics& value,std::span<u8> out){
    if(out.size()!=kStoryStatisticsBytes)return false;
    store_be32(out.data(),value.result_true);store_be32(out.data()+4,value.result_false);store_be32(out.data()+8,value.callback_remainder);store_be32(out.data()+12,value.callback_carries);
    for(unsigned i=0;i<16;++i){store_be32(out.data()+0x10+i*4,value.partner_true[i]);store_be32(out.data()+0x50+i*4,value.partner_false[i]);store_be32(out.data()+0x90+i*4,value.partner_use[i]);}
    store_be32(out.data()+0xd0,value.counter1504);store_be32(out.data()+0xd4,value.counter1508);return true;
}
inline bool story_statistics_mode_valid(StoryStartAction action){return unsigned(action)<=unsigned(StoryStartAction::Special);}
inline bool story_statistics_callback_plan(const StoryStatistics& before,StoryStartAction action,
                                           u8 source_major_state,StoryStatistics* out){
    if(!out||!story_statistics_mode_valid(action))return false;
    auto next=before;const auto major=std::bit_cast<s8>(source_major_state);
    if(action!=StoryStartAction::Special&&major>1&&major<9){
        ++next.callback_remainder; //source u32 store/wrap precedes unsigned comparison
        if(next.callback_remainder>=3600){next.callback_remainder=0;++next.callback_carries;}
    }
    *out=next;return true;
}
//Combine only represented statistics writes from803F0 and804FC. Other result
//effects (event progress, history, partner state, inventory...) remain unhandled.
//The bool is a declared source predicate, not an opaque genuine result receipt.
inline bool story_statistics_result_fields_plan(const StoryStatistics& before,StoryStartAction action,
                                                bool source_result_is_one,u32 mask150c,u32 mask1510,
                                                StoryStatistics* out){
    if(!out||!story_statistics_mode_valid(action))return false;
    auto next=before;
    if(action!=StoryStartAction::Special){
        if(source_result_is_one)++next.result_true;else ++next.result_false;
        for(unsigned i=1;i<=13;++i){
            if(mask150c&(1u<<i))++next.partner_use[i];
            if(mask1510&(1u<<i)){if(source_result_is_one)++next.partner_true[i];else ++next.partner_false[i];}
        }
    }
    *out=next;return true;
}
inline bool story_statistics_award_field_plan(const StoryStatistics& before,StoryStartAction action,
                                             s32 source_argument,StoryStatistics* out){
    if(!out||!story_statistics_mode_valid(action))return false;
    auto next=before;if(action!=StoryStartAction::Special){if(source_argument)++next.counter1508;else ++next.counter1504;}
    *out=next;return true;
}
//Exact statistics subrange of8017B8C0's new-run reset. Not the whole constructor
//and never permission to clear replay history/reward counters/collected Borgs.
inline bool story_statistics_new_run_clear_plan(StoryStatistics* out){if(!out)return false;*out={};return true;}
} //namespace gf
