#pragma once
//8004D7FC/D850/D978 and801860C4/6118/62D4;660E8 retirement then table dispatch.
// Pure child bookkeeping with EXPLICIT parent observations, no scene allocator.
#include "engine/assets/startup_attachments.hpp"
#include <algorithm>
#include "engine/gameplay/attachment_transform.hpp"
namespace gf {
struct StartupAttachmentParent {
    u16 model_id=0;u8 mode6e8=0,retire662=0,alternate_ae=0,selector3ec=0; s16 reload784=0;
};
struct StartupAttachmentState {
    u8 phase=0,mode=0,render_group=0;f32 draw_scale=0;bool allocated=true;
    AttachmentBoneTransformInput transform;
    bool operator==(const StartupAttachmentState&)const=default;
};
struct StartupAttachmentUpdate {
    StartupAttachmentState state;bool unregister=false,released=false,visible=false,alternate=false;
};
namespace startup_attachment_state_detail {
//Shared numeric implementation; wrappers keep their separate index contracts.
inline bool step(const StartupAttachmentProfile& p,unsigned index,u8 stored_parent_index,u8 maximum_parent_index,
                                         const StartupAttachmentParent& parent,const StartupAttachmentState& input,
                                         StartupAttachmentUpdate* out,std::string* error){
    if(out)*out={};
    if(error)error->clear();
    auto fail=[&](const char* why){if(error)*error=std::string("startup attachment state: ")+why;return false;};
    if(!out||!startup_attachment_profile_valid(p)||index>=2||stored_parent_index>maximum_parent_index||input.phase>3||(!input.allocated&&input.phase!=3)||!std::isfinite(input.draw_scale)||input.draw_scale<0||input.draw_scale>p.one)return fail("profile/state/index");
    const auto& e=p.entries[index];
    if((input.phase==0&&(input.mode||input.render_group||input.draw_scale!=0))||(input.phase>=2&&input.render_group)||(!p.kind&&input.draw_scale!=0)||(p.kind&&input.mode))return fail("constructor/retired field domain");
    if(input.phase==1){
        if(input.mode>=p.mode_count||input.render_group!=stored_parent_index+0x41)return fail("live mode/render group");
        const auto& old=e.modes[input.mode];
        if(input.transform.flags!=e.flags||input.transform.bone!=old.bone||input.transform.angles!=old.angles||input.transform.local_position!=old.position||input.transform.scale!=std::array<f32,3>{p.one,p.one,p.one})return fail("live transform must retain initialized source fields");
    }
    auto s=input;StartupAttachmentUpdate result;
    if(!s.allocated){result.state=s;*out=result;return true;}
    //660E8 changesphase to2 BEFORE the update callback's table lookup.
    if(s.phase<=1&&(parent.retire662||parent.model_id!=p.model_id)){s.phase=2;s.render_group=0;result.unregister=true;}
    if(s.phase==2){s.phase=3;result.state=s;*out=result;return true;}
    if(s.phase==3){s.allocated=false;result.released=true;result.state=s;*out=result;return true;}
    if(s.phase==0){s.phase=1;s.render_group=u8(stored_parent_index+0x41);s.draw_scale=p.zero;}
    if(!p.kind){s.mode=u8(parent.mode6e8&0x7fu);if(s.mode>=p.mode_count)return fail("unimported Sasuke attachment mode");}
    const auto& mode=e.modes[p.kind?0:s.mode];s.transform.flags=e.flags;s.transform.bone=mode.bone;s.transform.local_position=mode.position;s.transform.angles=mode.angles;s.transform.scale.fill(p.one);
    if(p.kind){s.draw_scale=f32(s.draw_scale+(parent.mode6e8==e.grow_mode?p.growth:-p.growth));s.draw_scale=std::clamp(s.draw_scale,p.zero,p.one);result.visible=s.draw_scale>p.zero;result.alternate=result.visible&&parent.alternate_ae!=0;}
    else{result.visible=!(e.id&1u)||(s.mode==2&&parent.reload784>0);result.alternate=result.visible&&(parent.alternate_ae!=0||parent.selector3ec==4);}
    result.state=s;*out=result;if(error)error->clear();return true;
}
}
inline bool startup_attachment_state_step(const StartupAttachmentProfile& p,unsigned index,u8 stored_parent_index,const StartupAttachmentParent& parent,const StartupAttachmentState& input,StartupAttachmentUpdate* out,std::string* error){
    return startup_attachment_state_detail::step(p,index,stored_parent_index,1,parent,input,out,error);
}
} // namespace gf
