#pragma once
#include <array>
#include <bit>
#include "engine/assets/story_guide.hpp"
namespace gf {
//F8F4 caller fields and DECLARED editor-state observations. Not genuine native
//editor/GObj readiness; this value plan MUST NOT lift the outer controller gate.
struct StoryEditorGuideState {u32 completed68=0,source54=0,selector84=0;u8 phase2=0,editor148=0;s16 counter8=0;bool operator==(const StoryEditorGuideState&)const=default;};
struct StoryEditorGuideObservation {s16 primary10=0,local12=0;};
struct StoryEditorGuidePlan {StoryEditorGuideState state;bool request_text_bank1=false,request_panel_bank1=false,request_popup0=false;};
inline bool story_editor_guide_plan(const StoryEditorGuideState& before,const StoryEditorGuideObservation& observation,StoryEditorGuidePlan* out){
    if(!out)return false;
    StoryEditorGuidePlan plan;plan.state=before;auto& s=plan.state;
    if(s.completed68||!s.source54||s.phase2>2){*out=plan;return true;}
    if(s.phase2==0){if(observation.local12!=1){*out=plan;return true;}s.phase2=1;s.editor148=1;plan.request_text_bank1=plan.request_panel_bank1=true;}
    if(s.phase2==1){if(observation.primary10!=1){*out=plan;return true;}s.phase2=2;s.counter8=40;}
    //Signed16 store BEFORE compare; 8000 ->7FFF, not immediate completion.
    s.counter8=std::bit_cast<s16>(u16(u16(s.counter8)-1));if(s.counter8<1){++s.phase2;s.selector84=4;plan.request_popup0=true;}
    *out=plan;return true;
}
struct StoryGuideReference {u32 bank_fingerprint=0,byte_offset=0;u8 slot=0;bool operator==(const StoryGuideReference&)const=default;};
inline bool story_guide_reference_valid(const StoryGuideReference& ref,const StoryGuideBank& bank){StoryGuideCommand command;return bank.ready()&&ref.bank_fingerprint==bank.fingerprint()&&bank.command(ref.slot,ref.byte_offset,&command);}
inline bool story_guide_reference_encode(const StoryGuideReference& ref,const StoryGuideBank& bank,std::span<u8> bytes){if(bytes.size()!=16||!story_guide_reference_valid(ref,bank))return false;store_be32(bytes.data(),0x47464752);store_be16(bytes.data()+4,1);bytes[6]=ref.slot;bytes[7]=0;store_be32(bytes.data()+8,ref.bank_fingerprint);store_be32(bytes.data()+12,ref.byte_offset);return true;}
inline bool story_guide_reference_decode(ByteView bytes,const StoryGuideBank& bank,StoryGuideReference* out){if(!out||!bytes.data||bytes.size!=16||bytes.be32(0)!=0x47464752||bytes.be16(4)!=1||bytes.data[7])return false;StoryGuideReference ref{bytes.be32(8),bytes.be32(12),bytes.data[6]};if(!story_guide_reference_valid(ref,bank))return false;*out=ref;return true;}
} //namespace gf
