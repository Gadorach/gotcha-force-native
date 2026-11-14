#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <vector>
#include "engine/gameplay/story_reward_counters.hpp"
namespace {unsigned checks=0;void check(bool x){++checks;if(!x)std::abort();}}
int main(){using namespace gf;StoryRewardCounters initial;
    for(unsigned i=0;i<kStoryRewardCounterCapacity;++i){initial.active[i]=(u32(i)*0x9e3779b9u)^0xffffffffu;initial.backup[i]=(u32(i)*0x7f4a7c15u)^0x01234567u;}
    check(story_reward_counters_valid(initial));
    std::vector<u8> encoded(kStoryRewardCounterBytes),other(encoded.size());
    for(unsigned count=0;count<=kStoryRewardCounterCapacity;++count){
        auto before=initial;before.active_count=count;before.backup_count=u32(kStoryRewardCounterCapacity-count);StoryRewardCounters snapshot,restored,decoded;
        check(story_reward_counters_snapshot_plan(before,&snapshot));check(snapshot.active==before.active&&snapshot.backup==before.active&&snapshot.active_count==count&&snapshot.backup_count==count);
        check(story_reward_counters_restore_plan(before,&restored));check(restored.active==before.backup&&restored.backup==before.backup&&restored.active_count==before.backup_count&&restored.backup_count==before.backup_count);
        auto alias=before;check(story_reward_counters_snapshot_plan(alias,&alias)&&alias==snapshot);alias=before;check(story_reward_counters_restore_plan(alias,&alias)&&alias==restored);
        check(story_reward_counters_encode(before,encoded));check(load_be32(encoded.data()+0x1800)==before.active_count&&load_be32(encoded.data()+0x3004)==before.backup_count);
        check(story_reward_counters_decode_declared({encoded.data(),encoded.size()},&decoded)&&decoded==before);check(story_reward_counters_encode(decoded,other)&&encoded==other);
    }
    //Even with zero active/backup counts, all physically stored words copy.
    StoryRewardCounters copied;check(story_reward_counters_snapshot_plan(initial,&copied)&&copied.backup.back()==initial.active.back());check(story_reward_counters_restore_plan(initial,&copied)&&copied.active.back()==initial.backup.back());
    for(u32 count:{1537u,std::numeric_limits<u32>::max()})for(unsigned which=0;which<2;++which){
        auto invalid=initial;if(which)invalid.backup_count=count;else invalid.active_count=count;auto unchanged=initial;
        check(!story_reward_counters_valid(invalid));check(!story_reward_counters_snapshot_plan(invalid,&unchanged)&&unchanged==initial);check(!story_reward_counters_restore_plan(invalid,&unchanged)&&unchanged==initial);
        std::fill(encoded.begin(),encoded.end(),173);const auto old=encoded;check(!story_reward_counters_encode(invalid,encoded)&&encoded==old);
        check(story_reward_counters_encode(initial,encoded));store_be32(encoded.data()+(which?0x3004:0x1800),count);
        check(!story_reward_counters_decode_declared({encoded.data(),encoded.size()},&unchanged)&&unchanged==initial);
    }
    auto unchanged=initial;check(!story_reward_counters_decode_declared({nullptr,kStoryRewardCounterBytes},&unchanged)&&unchanged==initial);
    check(!story_reward_counters_decode_declared({encoded.data(),encoded.size()-1},&unchanged)&&unchanged==initial);
    check(!story_reward_counters_decode_declared({encoded.data(),0x4628},&unchanged)&&unchanged==initial);
    check(!story_reward_counters_decode_declared({encoded.data(),encoded.size()},nullptr));
    check(!story_reward_counters_snapshot_plan(initial,nullptr));check(!story_reward_counters_restore_plan(initial,nullptr));
    const auto old=encoded;check(!story_reward_counters_encode(initial,std::span<u8>(encoded.data(),encoded.size()-1))&&encoded==old);
    std::printf("Story reward-counter persistence/copy: %u owned checks PASS; no rate/RNG/result or full-save authority\n",checks);
}
