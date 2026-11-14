#pragma once
#include <cmath>
#include <vector>
#include "engine/assets/source_adx_control.hpp"
namespace gf {
//Typed field/call candidates only, NOT original68B objects or native playback.
struct SourceAdxPlayerFields {
    u8 active0=0,paused1=0,source2=0,bank3=0,priority18=0,pause19=0,pause1a=0,requested1b=0,stop23=0,flag40=0,volume_kind=2;
    s16 member4=0,archive6=0,observed8=0,requested_a=0;s32 caption20=0,backend_status30=0;
    f32 current_c=0,maximum10=0,minimum14=0,slope1c=0;u32 started_tick24=0,elapsed28=0,caption_ref34=0,fade_count38=0,fade_duration3c=0;
    bool operator==(const SourceAdxPlayerFields&)const=default;
};
struct SourceAdxRing {std::array<u32,64> words{};std::array<u8,64> known{};u32 write=0,read=0;bool operator==(const SourceAdxRing&)const=default;};
struct SourceAdxControlFields {std::array<SourceAdxPlayerFields,3> players{};SourceAdxRing ring;u32 enabled=0;std::array<bool,3> handle_present{};s32 staged_archive=-1,staged_member=-1;bool operator==(const SourceAdxControlFields&)const=default;};
enum class SourceAdxActionKind:u8 {SetSdkVolume,StopHandle,UnpauseHandle,StartArchiveMember,PauseHandle};
struct SourceAdxPendingAction {SourceAdxActionKind kind=SourceAdxActionKind::StopHandle;u32 player=0;s32 value=0,archive=0,member=0;bool operator==(const SourceAdxPendingAction&)const=default;};
struct SourceAdxCommand {u32 opcode=0;std::array<u32,4> args{};u8 known=0;};
enum class SourceAdxEnqueueOutcome:u8 {Disabled,Dropped,Enqueued};
struct SourceAdxEnqueuePlan {SourceAdxRing ring;SourceAdxEnqueueOutcome outcome=SourceAdxEnqueueOutcome::Disabled;bool source_empty_alias=false;};
inline bool source_adx_enqueue_plan(const SourceAdxControlProfile& profile,const SourceAdxRing& before,u32 enabled,const SourceAdxCommand& command,SourceAdxEnqueuePlan* out){
    if(!out||!profile.valid()||command.opcode>=4||command.known>15)return false;
    const u32 length=profile.lengths[command.opcode];const u8 required=command.opcode==0||command.opcode==3?7:1;if((command.known&required)!=required)return false;SourceAdxEnqueuePlan p;p.ring=before;
    if(!enabled){*out=p;return true;}
    if(before.read>=64||before.write>=64)return false;
    //E91D4 checks +1..length-1, NOT the final +length position. Preserve
    //exact-fill write==read alias; do not replace it with a63-word capacity rule.
    for(u32 i=1;i<length;++i)if((before.write+i)%64==before.read){p.outcome=SourceAdxEnqueueOutcome::Dropped;*out=p;return true;}
    p.ring.words[before.write]=command.opcode;p.ring.known[before.write]=1;for(u32 i=1;i<length;++i){const auto at=(before.write+i)%64;const bool known=(command.known&(1u<<(i-1)))!=0;p.ring.words[at]=known?command.args[i-1]:0;p.ring.known[at]=u8(known);}p.ring.write=(before.write+length)%64;p.outcome=SourceAdxEnqueueOutcome::Enqueued;p.source_empty_alias=p.ring.write==p.ring.read;*out=p;return true;
}
inline bool source_adx_initialize_fields_plan(const SourceAdxControlProfile& profile,const SourceAdxControlFields& before,SourceAdxControlFields* out){
    if(!out||!profile.valid())return false;
    auto p=before;for(unsigned i=0;i<3;++i){auto& s=p.players[i];s.current_c=s.maximum10=profile.full;s.minimum14=s.slope1c=0;s.stop23=0;s.fade_count38=0;s.active0=s.requested1b=s.priority18=s.flag40=s.source2=s.pause19=s.pause1a=s.paused1=0;s.volume_kind=i==2?0:2;}*out=p;return true;
}
inline bool source_adx_choose_voice_slot(const SourceAdxControlFields& fields,u32* out){
    if(!out)return false;
    const auto& a=fields.players[0];const auto& b=fields.players[1];if(a.requested1b&&std::bit_cast<s8>(a.bank3)==14)*out=0;else if(b.requested1b&&std::bit_cast<s8>(b.bank3)==14)*out=1;else if(!a.requested1b)*out=0;else if(!b.requested1b)*out=1;else *out=a.started_tick24<b.started_tick24?0:1;return true;
}
struct SourceAdxGainPlan {SourceAdxControlFields fields;std::vector<SourceAdxPendingAction> actions;};
inline bool source_adx_gain_plan(const SourceAdxControlProfile& profile,const SourceAdxControlFields& before,unsigned player,f32 desired,SourceAdxGainPlan* out){
    if(!out||!profile.valid()||player>=3)return false;
    SourceAdxGainPlan p;p.fields=before;if(before.enabled){auto& s=p.fields.players[player];if(!std::isfinite(desired)||!std::isfinite(s.maximum10)||!std::isfinite(s.minimum14)||std::abs(desired)>32767||std::abs(s.maximum10)>32767||std::abs(s.minimum14)>32767)return false;if(s.maximum10<desired)desired=s.maximum10;if(desired<s.minimum14)desired=s.minimum14;s.current_c=desired;if(before.handle_present[player])p.actions.push_back({SourceAdxActionKind::SetSdkVolume,player,profile.volume[u32(s32(desired))&127]/10,0,0});}*out=std::move(p);return true;
}
struct StoryGuideVoiceStartPlan {SourceAdxControlFields fields;std::vector<SourceAdxPendingAction> actions;std::array<SourceAdxEnqueueOutcome,2> enqueue{};std::array<bool,2> source_empty_alias{};u32 player=0;};
//Fresh DECLARED call-time fields for E9F0C's bank14/no-caption/default-gain
//branch. Do not retrofit old unfulfilled requests with later settings/state.
inline bool story_guide_voice_start_plan(const SourceAdxControlProfile& profile,const SourceAdxControlFields& before,u32 source_key,u8 voice_setting_index,StoryGuideVoiceStartPlan* out){
    if(!out||!profile.valid()||(source_key!=60&&source_key!=61)||voice_setting_index>4)return false;
    StoryGuideVoiceStartPlan p;p.fields=before;if(!source_adx_choose_voice_slot(before,&p.player))return false;auto& s=p.fields.players[p.player];const f32 product=127.f*profile.gains[p.player][voice_setting_index];const auto byte=std::bit_cast<s8>(u8(s32(product)));s.maximum10=f32(byte);s.minimum14=s.maximum10*profile.half;s.stop23=0;s.fade_count38=0;s.slope1c=0;s.requested1b=1;s.caption_ref34=0;s.bank3=14;s.requested_a=std::bit_cast<s16>(u16(u16(s.requested_a)+1));SourceAdxGainPlan gain;if(!source_adx_gain_plan(profile,p.fields,p.player,f32(byte),&gain))return false;p.fields=gain.fields;p.actions=std::move(gain.actions);
    const std::array<SourceAdxCommand,2> commands{{{0,{p.player,0,1803+source_key,0},7},{1,{p.player,0,0,0},1}}};for(unsigned i=0;i<2;++i){SourceAdxEnqueuePlan queued;if(!source_adx_enqueue_plan(profile,p.fields.ring,p.fields.enabled,commands[i],&queued))return false;p.fields.ring=queued.ring;p.enqueue[i]=queued.outcome;p.source_empty_alias[i]=queued.source_empty_alias;}
    //EA170 applies these software fields after the source call returns. They
    //do not prove either independent ring enqueue or any backend start succeeded.
    p.fields.players[p.player].priority18=254;p.fields.players[p.player].flag40=0;*out=std::move(p);return true;
}
struct SourceAdxConsumePlan {SourceAdxControlFields fields;std::vector<SourceAdxPendingAction> actions;bool consumed=false;u32 opcode=0;};
inline bool source_adx_consume_plan(const SourceAdxControlProfile& profile,const SourceAdxControlFields& before,SourceAdxConsumePlan* out){
    if(!out||!profile.valid()||before.ring.read>=64||before.ring.write>=64)return false;
    SourceAdxConsumePlan p;p.fields=before;const auto& q=before.ring;if(q.read==q.write){*out=std::move(p);return true;}if(q.known[q.read]!=1)return false;const u32 opcode=q.words[q.read];if(opcode>=4)return false;const auto length=profile.lengths[opcode];const auto available=(q.write+64-q.read)%64;if(length>available)return false;std::array<u32,4> args{};for(u32 i=1;i<length;++i){const auto at=(q.read+i)%64;if((opcode!=3||i<4)&&q.known[at]!=1)return false;args[i-1]=q.words[at];}const u32 slot=args[0];if(slot>=3)return false;auto& s=p.fields.players[slot];const bool handle=before.handle_present[slot];
    if(opcode==0){if(handle)p.actions.push_back({SourceAdxActionKind::StopHandle,slot});p.fields.staged_archive=std::bit_cast<s32>(args[1]);p.fields.staged_member=std::bit_cast<s32>(args[2]);}
    else if(opcode==1){if(before.staged_archive>=0&&before.staged_member>=0){if(handle){p.actions.push_back({SourceAdxActionKind::UnpauseHandle,slot,0});p.actions.push_back({SourceAdxActionKind::StartArchiveMember,slot,0,before.staged_archive,before.staged_member});s.active0=1;s.source2=0;s.member4=std::bit_cast<s16>(u16(before.staged_member));s.archive6=std::bit_cast<s16>(u16(before.staged_archive));}p.fields.staged_archive=p.fields.staged_member=-1;}}
    else if(opcode==2){if(handle)p.actions.push_back({SourceAdxActionKind::StopHandle,slot});s.active0=s.requested1b=s.priority18=s.flag40=s.source2=0;s.slope1c=0;s.stop23=0;s.fade_count38=0;s.pause19=s.pause1a=s.paused1=0;}
    else {if(args[1]>1)return false;const u8 value=u8(args[2]);if(args[1])s.pause1a=value;else s.pause19=value;s.paused1=u8(u32(s.pause19)+s.pause1a);if((args[2]!=0||s.paused1==0)&&handle)p.actions.push_back({SourceAdxActionKind::PauseHandle,slot,std::bit_cast<s32>(args[2])});}
    p.fields.ring.read=(q.read+length)%64;p.opcode=opcode;p.consumed=true;*out=std::move(p);return true;
}
} //namespace gf
