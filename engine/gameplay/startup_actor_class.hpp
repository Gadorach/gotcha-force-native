#pragma once
// Source8018CE1C/CEF8/CF54 and8006F708/F7C0/F88C, ONLY0615 and000A.
// Class callbacks, not80057B78 construction/general preupdate/world placement.
#include <vector>
#include "engine/assets/startup_actor_class.hpp"
namespace gf {
struct StartupActorClassState {
    u16 model_id=0;bool callbacks_initialized=false;
    u32 flags5e0=0;
    u8 field4a8=0,field6ee=0,field582=0,field58e=0,field7c8=0;
    u8 field6e9=0,field6ec=0,fieldaf=0;
    u16 field6f6=0,field272=0; s16 field1dfc=0,angle18e2=0;
};
enum class StartupActorClassEffectKind {AuxiliaryC74ECRequired,Auxiliary185F6CRequired,Attachment4D6DCRequired,Effect1AB6CRequired};
struct StartupActorClassEffect {StartupActorClassEffectKind kind;u8 id;};
inline bool startup_actor_class_bound(const StartupActorClassProfile& p,const StartupActorClassState& s){
    return startup_actor_class_profile_valid(p)&&(s.model_id==p.model_ids[0]||s.model_id==p.model_ids[1]);
}
// Requests mean attempted source allocations/calls, NOT objects created/rendered.
// Missing parent writes remain unchanged, exactly as these callbacks do.
inline bool startup_actor_class_initialize(const StartupActorClassProfile& p,const StartupActorClassState& input,
                                          StartupActorClassState* out,std::vector<StartupActorClassEffect>* effects){
    if(effects)effects->clear();
    if(!out||!effects||!startup_actor_class_bound(p,input))return false;
    auto next=input;std::vector<StartupActorClassEffect> requests;
    if(input.model_id==p.model_ids[0]){next.field4a8=1;next.field6f6=0;
        requests={{StartupActorClassEffectKind::AuxiliaryC74ECRequired,0x28},{StartupActorClassEffectKind::AuxiliaryC74ECRequired,0x29},
                  {StartupActorClassEffectKind::Auxiliary185F6CRequired,0x18},{StartupActorClassEffectKind::Auxiliary185F6CRequired,0x19}};
    }else requests={{StartupActorClassEffectKind::Attachment4D6DCRequired,6},{StartupActorClassEffectKind::Attachment4D6DCRequired,7}};
    next.callbacks_initialized=true;*out=next;*effects=std::move(requests);return true;
}
inline bool startup_actor_class_pre(const StartupActorClassProfile& p,const StartupActorClassState& input,StartupActorClassState* out){
    if(!out||!input.callbacks_initialized||!startup_actor_class_bound(p,input))return false;
    auto next=input;
    if(input.model_id==p.model_ids[0]){
        if(!(input.flags5e0&1u)){next.field6ee=0;next.field1dfc=0;const f32 angle=f32(f32(input.angle18e2)*p.angle_retention);next.angle18e2=s16(s32(angle));}
    }else{
        next.field582=0; // model3 branch is outside the admitted identities
        if(!(input.flags5e0&1u)){next.field1dfc=0;next.field6ee=0;}
        if(!(input.flags5e0&0x1000u))next.field272=u16(input.field272&0xfffbu);
        next.field58e=u8(input.field58e&0xdfu);
    }
    *out=next;return true;
}
inline bool startup_actor_class_post(const StartupActorClassProfile& p,const StartupActorClassState& input,
                                    StartupActorClassState* out,std::vector<StartupActorClassEffect>* effects){
    if(effects)effects->clear();
    if(!out||!effects||!input.callbacks_initialized||!startup_actor_class_bound(p,input))return false;
    auto next=input;std::vector<StartupActorClassEffect> requests;
    // Source lbz+extsb:80..FF are negative, NOT positive event selectors.
    if(input.model_id==p.model_ids[0]){
        if(input.field6e9==2)next.fieldaf=255;
        else if(input.field6e9==1){next.fieldaf=1;requests.push_back({StartupActorClassEffectKind::Effect1AB6CRequired,0x32});}
        next.field6e9=0;
    }else{
        if(input.flags5e0&0x02000000u)next.field7c8=0;
        if(input.field6e9>0&&input.field6e9<128)next.field6ec=u8((unsigned(input.field6ec)+1u)&3u);
        // Sasuke does NOT consume/clear6E9 here, unlike G-Red.
    }
    *out=next;*effects=std::move(requests);return true;
}
} // namespace gf
