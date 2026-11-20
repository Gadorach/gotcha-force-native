#pragma once
#include <array>
#include "engine/core/types.hpp"

namespace gf {
struct NormalSamuraiSlashSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return !variant&&slot<6&&(model==0x0700||model==0x0701);}
    constexpr bool demon()const{return model==0x0701;}
};
enum class SamuraiSlashPosture:u8 {Neutral,OrdinaryAirborne};
struct SamuraiSlashRequest {
    bool requested=false;
    u8 selector=0,class_state580=0,substate581=0;
};
// Caller supplies the reviewed posture classification and conditioned X
// leaf. Neither posture here uses a weapon-stock debit.
inline bool normal_samurai_slash_request(NormalSamuraiSlashSource source,SamuraiSlashPosture posture,u32 commands5d4,
                                         u8 flags58e,bool reached_reviewed_input,SamuraiSlashRequest* out){
    using P=SamuraiSlashPosture;
    if(!out||!source.reviewed()||!reached_reviewed_input||(posture!=P::Neutral&&posture!=P::OrdinaryAirborne))return false;
    const bool air=posture==P::OrdinaryAirborne;
    *out=(commands5d4&0x80u)&&!(flags58e&4u)?SamuraiSlashRequest{true,u8(air?45:44),2,u8(air?3:0)}:SamuraiSlashRequest{};
    return true;
}

struct SamuraiSlashProgram {
    bool requested=false;
    u8 mask=0,group=0,slot=0;
};
inline bool normal_samurai_slash_start_program(NormalSamuraiSlashSource source,SamuraiSlashPosture posture,SamuraiSlashProgram* out){
    using P=SamuraiSlashPosture;
    if(!out||!source.reviewed()||(posture!=P::Neutral&&posture!=P::OrdinaryAirborne))return false;
    *out={true,15,4,u8(posture==P::OrdinaryAirborne&&!source.demon()?3:0)};return true;
}

struct SamuraiSlashCounters {
    s16 active548=0,reserve54a=0,contribution54c=0;
    bool operator==(const SamuraiSlashCounters&)const=default;
};
struct SamuraiSlashActiveEntry {
    SamuraiSlashCounters counters;
    SamuraiSlashProgram program;
};
// Reviewed neutral phase-two handoff, not a VM clock.
inline bool normal_samurai_slash_active_entry(NormalSamuraiSlashSource source,bool lower_done,SamuraiSlashActiveEntry* out){
    if(!out||!source.reviewed())return false;
    SamuraiSlashActiveEntry next;
    if(lower_done)next={source.demon()?SamuraiSlashCounters{0,30,8}:SamuraiSlashCounters{30,40,8},{true,15,4,1}};
    *out=next;return true;
}

// Shared 80149AA4 transfer: one update for any nonzero fresh-edge count,
// not one per pressed button and not an RNG selection.
inline bool normal_samurai_slash_button_step(NormalSamuraiSlashSource source,const SamuraiSlashCounters& in,u32 pressed5bc,
                                             SamuraiSlashCounters* out){
    if(!out||!source.reviewed()||in.active548<0||in.reserve54a<0||in.active548+in.reserve54a>(source.demon()?30:70)||
       in.contribution54c<2||in.contribution54c>8)return false;
    auto next=in;
    if(pressed5bc&0xf00u){
        const s16 transfer=in.contribution54c<in.reserve54a?in.contribution54c:in.reserve54a;
        next.contribution54c=s16(in.contribution54c>2?in.contribution54c-1:2);
        next.reserve54a=s16(in.reserve54a-transfer);
        next.active548=s16(in.active548+transfer);
    }
    *out=next;return true;
}
struct SamuraiSlashProgress {
    s16 active548=0;
    u8 phase540=3;
    SamuraiSlashProgram program;
};
// Lane event at the neutral slot-1 tail. Only strictly negative after
// subtraction selects the next program; zero retains phase 3.
inline bool normal_samurai_slash_progress(NormalSamuraiSlashSource source,s16 active548,u8 lane1b01,
                                          SamuraiSlashProgress* out){
    if(!out||!source.reviewed()||active548<0||active548>(source.demon()?30:70))return false;
    SamuraiSlashProgress next{active548,3,{}};
    if(lane1b01){
        next.active548=s16(active548-10);
        if(next.active548<0){next.phase540=4;next.program={true,15,4,2};}
    }
    *out=next;return true;
}

struct SamuraiSlashShapeRequest {
    bool requested=false;
    u8 carrier_slot=0,credited_slot=0,shape_group=0,hit_row=0,descriptor_count=0;
    std::array<u8,3> descriptors{};
    s16 start=0,end=0;
};
// Source group selection and descriptor references only. These are not
// world-space shapes, hits, damage, or credit to the caster's aim target.
inline bool normal_samurai_slash_shape(NormalSamuraiSlashSource source,u8 program_slot,u8 lane_mask,
                                       bool reached_reviewed_opcode,SamuraiSlashShapeRequest* out){
    if(!out||!source.reviewed()||!reached_reviewed_opcode||(program_slot!=1&&program_slot!=2))return false;
    SamuraiSlashShapeRequest next;
    if(lane_mask&1u){
        if(program_slot==1)next=source.demon()?SamuraiSlashShapeRequest{true,source.slot,source.slot,7,19,3,{7,13,17},0,-2}:
            SamuraiSlashShapeRequest{true,source.slot,source.slot,7,19,1,{7},9,-2};
        else next=source.demon()?SamuraiSlashShapeRequest{true,source.slot,source.slot,8,20,1,{28},0,1}:
            SamuraiSlashShapeRequest{true,source.slot,source.slot,8,20,2,{13,28},1,3};
    }
    *out=next;return true;
}
struct SamuraiSlashVisualRequest {
    bool requested=false;
    u8 owner_slot=0;
    std::array<u8,2> keys{};
};
inline bool normal_samurai_slash_visual(NormalSamuraiSlashSource source,u8 lane_mask,bool observed_key3_hook,
                                        SamuraiSlashVisualRequest* out){
    if(!out||!source.reviewed())return false;
    *out=(lane_mask&1u)&&observed_key3_hook?SamuraiSlashVisualRequest{true,source.slot,
        source.demon()?std::array<u8,2>{19,18}:std::array<u8,2>{6,5}}:SamuraiSlashVisualRequest{};
    return true;
}
}
