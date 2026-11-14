#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_new_run_fields.hpp"
namespace {unsigned checks=0;void check(bool x){++checks;if(!x)std::abort();}}
int main(){using namespace gf;StoryNewRunFields before;before.start={8,7,true,true};check(StoryReplayContext::from_declared_index(5,&before.replay));
    before.resume_marker20=101;before.weighted70=0xffffffff;before.accumulated4624=0x8abc1234;
    before.source38=2;before.source44=4;before.source50=1;before.matches6c=999;before.source74=42;before.source78=17;before.partner8c=12;before.unlock90=0xffff;before.location_mask114=0xaaaa;before.location118=2;before.previous11c=3;before.stable120=4;before.cycle124=2;
    for(unsigned i=0;i<16;++i){before.partner_tiers94[i]=u8(i+1);before.map_states130[i]=u8(i+11);before.map_parameters140[i]=u8(i+31);}
    for(unsigned i=0;i<384;++i)before.unclassified12b4[i]=u8(i*71+19);
    for(unsigned i=0;i<256;++i){before.history.bytes[i]=u8(i*29+7);before.events.events[i].fill(u8(i+1));}for(auto& row:before.events.gates)row.fill(255);
    before.rewards.active_count=1536;before.rewards.backup_count=0;for(unsigned i=0;i<1536;++i){before.rewards.active[i]=u32(i)*0x9e3779b9;before.rewards.backup[i]=~before.rewards.active[i];}
    before.statistics.result_true=83;before.statistics.callback_remainder=3599;before.statistics.partner_use.fill(55);before.statistics.counter1508=71;
    for(bool present:{false,true}){StoryNewRunFieldPlan plan;check(story_new_run_field_reset_plan(before,{true,present},&plan));const auto& f=plan.fields;
        check(f.source74==(present?500u:300u)&&f.source78==f.source74);check(f.location118==10&&f.previous11c==10&&f.stable120==10&&f.cycle124==0&&f.location_mask114==0x400);
        check(f.source38==0&&f.source44==1&&f.source50==0&&f.matches6c==0&&f.partner8c==1&&f.unlock90==0);
        check(f.partner_tiers94==std::array<u8,16>{}&&f.map_states130==std::array<u8,16>{}&&f.events==StoryEventProgress{}&&f.statistics==StoryStatistics{});
        check(f.map_parameters140==before.map_parameters140&&f.unclassified12b4==before.unclassified12b4);
        check(f.start==before.start&&f.replay==before.replay&&f.resume_marker20==101&&f.weighted70==before.weighted70&&f.accumulated4624==before.accumulated4624);
        check(f.history==before.history&&f.rewards==before.rewards);
        check(plan.external_prefix[0]==StoryNewRunPending::ClearCollectionByte0&&plan.external_prefix[1]==StoryNewRunPending::ClearBoxByte2174);
        check(plan.external_suffix[0]==StoryNewRunPending::InitializeEventPrerequisites&&plan.external_suffix[1]==StoryNewRunPending::InitializeGatesWithoutAdvance&&plan.external_suffix[2]==StoryNewRunPending::ResetScreenFade);
        check(plan.requested_minor==1&&plan.requested_phase==0);
    }
    for(unsigned play=0;play<8;++play)for(unsigned saved=0;saved<8;++saved)for(unsigned completed=0;completed<9;++completed){auto input=before;input.start.saved_play=saved;input.start.completed_plays=completed;check(StoryReplayContext::from_declared_index(play,&input.replay));StoryNewRunFieldPlan plan;
        check(story_new_run_field_reset_plan(input,{true,true},&plan)&&plan.fields.start==input.start&&plan.fields.replay==input.replay&&plan.fields.rewards==input.rewards&&plan.fields.history==input.history);
    }
    StoryNewRunFieldPlan unchanged;unchanged.fields=before;check(!story_new_run_field_reset_plan(before,{false,false},&unchanged)&&unchanged.fields==before);
    auto invalid=before;invalid.start.completed_plays=9;check(!story_new_run_field_reset_plan(invalid,{true,false},&unchanged)&&unchanged.fields==before);
    invalid=before;invalid.replay={};check(!story_new_run_field_reset_plan(invalid,{true,false},&unchanged)&&unchanged.fields==before);
    invalid=before;invalid.rewards.active_count=1537;check(!story_new_run_field_reset_plan(invalid,{true,false},&unchanged)&&unchanged.fields==before);
    check(!story_new_run_field_reset_plan(before,{true,false},nullptr));
    std::printf("Story new-run represented field reset: %u owned checks PASS; retention and explicit unfulfilled initialization requirements\n",checks);
}
