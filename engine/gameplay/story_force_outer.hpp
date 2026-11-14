#pragma once
#include "engine/gameplay/story_force_native_presentation.hpp"
#include "engine/gameplay/story_map_entry.hpp"
namespace gf {
//Represented F9F4/F77C caller bytes/words only, NOT a Story runtime/checkpoint.
struct StoryForceOuterFields {
    u8 major=5,minor=0,phase2=0,entry_flag3=0,return_major6=5,last_map_minor7=6,port3c=0;
    u32 reason44=0,force4c=0,source54=0,completed68=0;
    bool operator==(const StoryForceOuterFields&)const=default;
};
struct StoryForceEditorEntryPlan {StoryForceOuterFields fields;u8 port=0,controller_mask=0,context149=0;unsigned force=0;bool helper_f8f4_pending=false;};
inline bool story_force_outer_fields_valid(const StoryForceOuterFields& f){return f.major<=10&&f.port3c<4&&f.force4c<kForceCount&&f.completed68<=8&&f.return_major6<=10&&f.last_map_minor7<=10;}
//Scalar source plans remain declared values. The native owner below must bind
//actual UI/editor results before publishing them; no caller "success" grants.
inline bool story_force_outer_result_plan(const StoryForceOuterFields& before,StoryForceNativeChoiceKind kind,u32 result,StoryForceOuterFields* out){
    if(!out||!story_force_outer_fields_valid(before)||before.major!=5)return false;
    auto f=before;
    if(kind==StoryForceNativeChoiceKind::Bypass){if(before.minor!=0||result)return false;f.major=6;f.minor=0;}
    else{
        if(before.minor!=1)return false;
        if(kind==StoryForceNativeChoiceKind::Stored){if(result<1||result>20)return false;f.force4c=result-1;f.minor=2;}
        else if(kind==StoryForceNativeChoiceKind::Editor){if(result<0x8001||result>0x8014)return false;f.major=4;f.minor=0;f.entry_flag3=0;f.return_major6=5;f.force4c=(result&0x7fff)-1;}
        else if(kind==StoryForceNativeChoiceKind::Back){if(result!=0xffffffffu)return false;if(before.last_map_minor7==6||before.last_map_minor7==7){f.major=3;f.minor=0;f.phase2=0;f.reason44=before.last_map_minor7==7?4:3;}else f.minor=0;}
        else return false;
    }
    *out=f;return true;
}
inline bool story_force_outer_selected_advance_plan(const StoryForceOuterFields& before,StoryForceOuterFields* out){if(!out||!story_force_outer_fields_valid(before)||before.major!=5||before.minor!=2)return false;auto f=before;f.major=6;f.minor=0;*out=f;return true;}
inline bool story_force_editor_entry_plan(const StoryForceOuterFields& before,StoryForceEditorEntryPlan* out){
    if(!out||!story_force_outer_fields_valid(before)||before.major!=4||before.minor!=0)return false;
    StoryForceEditorEntryPlan plan;plan.fields=before;plan.fields.minor=1;plan.fields.phase2=0;plan.port=before.port3c;plan.controller_mask=u8(1u<<before.port3c);plan.force=before.force4c;plan.context149=before.entry_flag3?2:3;plan.helper_f8f4_pending=before.completed68==0&&before.source54!=0;*out=plan;return true;
}
inline bool story_force_editor_return_plan(const StoryForceOuterFields& before,unsigned force,StoryForceOuterFields* out){if(!out||!story_force_outer_fields_valid(before)||before.major!=4||before.minor!=1||force>=kForceCount)return false;auto f=before;f.minor=2;f.force4c=force;*out=f;return true;}
inline bool story_force_editor_exit_plan(const StoryForceOuterFields& before,StoryForceOuterFields* out){if(!out||!story_force_outer_fields_valid(before)||before.major!=4||before.minor!=2)return false;auto f=before;f.major=before.return_major6;f.minor=0;f.phase2=before.return_major6>=7&&before.return_major6<=8?4:0;f.source54=0;*out=f;return true;}
//Map constructor is still pending. Preserve the whole represented cache and
//active row; do not replace it with a cursor/highlight on editor/Back return.
inline bool story_force_outer_map_entry_plan(const StoryForceOuterFields& fields,const StoryForceSetupFacts& facts,const StoryMapEntryFields& before,StoryMapEntryFields* out){
    if(!out||!story_force_outer_fields_valid(fields)||fields.major!=3||fields.minor||fields.phase2||(fields.reason44!=3&&fields.reason44!=4)||!story_map_entry_fields_valid(before)||before.action!=facts.action||before.bound_event_row!=facts.active_event_row)return false;
    auto f=before;f.minor=0;f.phase=0;f.reason44=fields.reason44;*out=std::move(f);return true;
}
} //namespace gf
