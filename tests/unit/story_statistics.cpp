#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include "engine/gameplay/story_statistics.hpp"
namespace {unsigned checks=0;void check(bool x){++checks;if(!x)std::abort();}}
int main(){using namespace gf;std::vector<u8> bytes(kStoryStatisticsBytes);for(unsigned i=0;i<bytes.size();++i)bytes[i]=u8(i*71+13);StoryStatistics initial;
    check(story_statistics_decode_declared({bytes.data(),bytes.size()},&initial));std::vector<u8> encoded(bytes.size());check(story_statistics_encode(initial,encoded)&&encoded==bytes);
    for(unsigned action=0;action<3;++action)for(unsigned major=0;major<256;++major)for(u32 remainder:{0u,3598u,3599u,3600u,0xfffffffeu,0xffffffffu}){
        auto before=initial;before.callback_remainder=remainder;before.callback_carries=0xffffffffu;auto expected=before;
        const bool enabled=action!=2&&major>=2&&major<=8;
        if(enabled){const u64 sum=u64(remainder)+1;const u32 stored=u32(sum&0xffffffffu);expected.callback_remainder=stored>=3600?0:stored;expected.callback_carries=stored>=3600?0:0xffffffffu;}
        StoryStatistics next;check(story_statistics_callback_plan(before,StoryStartAction(action),u8(major),&next)&&next==expected);
    }
    for(unsigned bit=0;bit<32;++bit)for(unsigned action=0;action<3;++action)for(unsigned outcome=0;outcome<2;++outcome){
        auto before=initial;before.result_true=before.result_false=0xffffffffu;before.partner_true.fill(0xffffffffu);before.partner_false.fill(0xffffffffu);before.partner_use.fill(0xffffffffu);
        auto expected=before;if(action!=2){if(outcome)expected.result_true=0;else expected.result_false=0;if(bit>=1&&bit<=13){expected.partner_use[bit]=0;if(outcome)expected.partner_true[bit]=0;else expected.partner_false[bit]=0;}}
        StoryStatistics next;check(story_statistics_result_fields_plan(before,StoryStartAction(action),outcome!=0,1u<<bit,1u<<bit,&next)&&next==expected);
    }
    for(u32 mask=0;mask<65536;++mask){
        auto expected=initial;for(unsigned i=1;i<=13;++i)if(mask&(1u<<i)){++expected.partner_use[i];++expected.partner_true[i];}++expected.result_true;
        auto next=initial;check(story_statistics_result_fields_plan(next,StoryStartAction::Replay,true,mask,mask,&next)&&next==expected);
    }
    {auto expected=initial;++expected.result_false;++expected.partner_use[1];++expected.partner_false[2];StoryStatistics next;
        check(story_statistics_result_fields_plan(initial,StoryStartAction::Continue,false,2,4,&next)&&next==expected);}
    for(auto argument:{std::numeric_limits<s32>::min(),-1,0,1,std::numeric_limits<s32>::max()})for(unsigned action=0;action<3;++action){
        auto before=initial;before.counter1504=before.counter1508=0xffffffffu;auto expected=before;if(action!=2){if(argument)expected.counter1508=0;else expected.counter1504=0;}
        StoryStatistics next;check(story_statistics_award_field_plan(before,StoryStartAction(action),argument,&next)&&next==expected);
    }
    auto next=initial;check(story_statistics_new_run_clear_plan(&next)&&next==StoryStatistics{});
    check(!story_statistics_callback_plan(initial,StoryStartAction(3),2,&next)&&next==StoryStatistics{});
    check(!story_statistics_result_fields_plan(initial,StoryStartAction(3),true,0,0,&next)&&next==StoryStatistics{});
    check(!story_statistics_award_field_plan(initial,StoryStartAction(3),0,&next)&&next==StoryStatistics{});
    check(!story_statistics_callback_plan(initial,StoryStartAction::Continue,2,nullptr));
    check(!story_statistics_result_fields_plan(initial,StoryStartAction::Continue,true,0,0,nullptr));
    check(!story_statistics_award_field_plan(initial,StoryStartAction::Continue,0,nullptr));check(!story_statistics_new_run_clear_plan(nullptr));
    next=initial;check(!story_statistics_decode_declared({bytes.data(),bytes.size()-1},&next)&&next==initial);check(!story_statistics_decode_declared({bytes.data(),0x4628},&next)&&next==initial);check(!story_statistics_decode_declared({nullptr,bytes.size()},&next)&&next==initial);check(!story_statistics_decode_declared({bytes.data(),bytes.size()},nullptr));
    const auto old=encoded;check(!story_statistics_encode(initial,std::span<u8>(encoded.data(),encoded.size()-1))&&encoded==old);
    std::printf("Story statistics storage/field candidates: %u owned checks PASS; source callbacks not measured time/result authority\n",checks);
}
