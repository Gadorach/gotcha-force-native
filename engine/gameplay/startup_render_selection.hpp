#pragma once
//80056AA8 AE assignment +80010514 resolved-viewer predicate ONLY. Inputs are
// explicit scene observations, not recovered global/player/actor lifetimes.
#include <span>
#include <string>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
struct StartupRenderViewerObservation {u8 viewer_slot=0,actor_slot=0,mode=0,behaviour43d=0;};
struct StartupRenderSelectionInput {
    bool parent_callback_called=false;u8 state4a0=0,force59a=0,stored_actor_index=0;
    s8 viewer_slot=-1;u16 mask270=0;u32 scene_counter=0;f32 timer720=0;
    std::span<const StartupRenderViewerObservation> viewers;
};
struct StartupRenderSelection {u8 alternate_ae=0;bool written=false;};
inline u32 startup_render_viewer_query(const StartupRenderViewerObservation& v){
    const unsigned behaviour=v.behaviour43d&0x7fu;
    return (v.mode==2?0x80000000u:0u)|(behaviour>=2&&behaviour<=9?(1u<<(behaviour-2)):0u);
}
inline bool startup_render_selection(const StartupRenderSelectionInput& input,u8 previous_ae,
                                     StartupRenderSelection* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup render selection: ")+why;return false;};
    if(!out)return fail("destination");
    StartupRenderSelection result{previous_ae,false};
    // No registration call, or56AA8's4A0==2 early return: AE is NOT reset.
    if(!input.parent_callback_called||input.state4a0==2){*out=result;if(error)error->clear();return true;}
    if(input.stored_actor_index>5||input.viewers.size()>6||!std::isfinite(input.timer720)||std::abs(input.timer720)>1e7f)return fail("actor/viewer/timer budget");
    bool alternate=false;
    if(input.viewer_slot>=0){
        if(input.viewer_slot>=6)return fail("unqualified viewer index");
        const StartupRenderViewerObservation* viewer=nullptr;
        for(const auto& v:input.viewers)if(v.viewer_slot==u8(input.viewer_slot)){
            if(viewer||v.actor_slot>=6)return fail("ambiguous/unbound viewer actor");
            viewer=&v;
        }
        if(!viewer)return fail("missing resolved viewer");
        alternate=startup_render_viewer_query(*viewer)==0&&(input.mask270&(1u<<u8(input.viewer_slot)));
    }
    if(input.force59a)alternate=true;
    // PPC add wraps; only bit1 of the source scene counter+stored slot is read.
    if(input.timer720>0&&!alternate&&((input.scene_counter+u32(input.stored_actor_index))&2u))alternate=true;
    result={u8(alternate),true};*out=result;if(error)error->clear();return true;
}
} // namespace gf
