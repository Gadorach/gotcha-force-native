#pragma once
#include "engine/gameplay/force_spec_request.hpp"
#include "engine/gameplay/force_spec_lifecycle.hpp"
#include "engine/gameplay/force_spec_scene.hpp"
namespace gf {
struct ForceStatsUiEvent {u16 sound=0;bool released=false;u32 unavailable_actor_actions=0;};
//An explicitly UI-only request owns original stats/moves/description pages.
//It never loads an actor/VM, receives actor resources or mints actor completion.
class ForceStatsUiPresentation {
public:
    ForceStatsUiPresentation()=default;
    ForceStatsUiPresentation(const ForceStatsUiPresentation&)=delete;ForceStatsUiPresentation& operator=(const ForceStatsUiPresentation&)=delete;
    ForceStatsUiPresentation(ForceStatsUiPresentation&&)=default;ForceStatsUiPresentation& operator=(ForceStatsUiPresentation&&)=default;
    bool load(ByteView bank,const ForceSpecArt& art,const SourceMessageBank& messages,const SourceFont& font,const ForceSpecLifecycleProfile& profile,RenderEngine& render,std::string* error){
        reset();loaded_=false;if(!profile.valid()||profile.bank_hash!=art.bank_hash||!ui_.load_with_horizon_fog(bank,art,messages,font,render,error)||!text_.load(font,render,7301))return false;
        profile_=profile;loaded_=true;return true;
    }
    bool open(const ForceInventory& owner,const std::shared_ptr<const ForceSpecRequest>& request,std::string* error){
        reset();if(!loaded_||!request||request->kind()!=ForceSpecPresentationKind::StatisticsUiOnly)return fail(error,"explicit UI-only request required");
        const auto* view=request->snapshot().view(owner);if(!view||!request->snapshot().value(owner)||!life_.open(profile_,view->page_mask,ForceSpecPage::Statistics))return fail(error,"current stored Borg/pages required");request_=request;active_=true;return true;
    }
    bool prepare_nameplate(const ForceInventory& owner,ByteView plate,RenderEngine& render,std::string* error){
        if(!current(owner)||prepared_||life_.phase()!=ForceSpecPhase::Resources)return refuse(error,"nameplate preparation order");
        const auto request=request_.lock();const auto* borg=request->snapshot().value(owner);if(!borg||!ui_.bind_nameplate(u16(borg->model),plate,render,error)||render.shared_texture_upload_failed())return refuse(error,"nameplate UI resource");prepared_=true;return true;
    }
    bool can_request_borg_editor(const ForceInventory& owner)const{return current(owner)&&prepared_&&!edit_requested_&&life_.phase()==ForceSpecPhase::Active&&life_.navigation().page()==ForceSpecPage::Statistics;}
    bool request_borg_editor(const ForceInventory& owner,const ForceSpecEditRequest& request){if(!can_request_borg_editor(owner)||request.scope_.expired()||request.request_.lock()!=request_.lock())return false;edit_scope_=request.scope_;edit_requested_=true;return true;}
    bool step(const ForceInventory& owner,u32 pressed,ForceStatsUiEvent* out,std::string* error){
        if(out)*out={};
        if(!out||!current(owner)||life_.phase()==ForceSpecPhase::Done)return refuse(error,"inactive/stale UI owner");
        if(edit_requested_)pressed=0x200;
        const auto before=life_.phase();ForceSpecLifecycleEvents event;if(!life_.step(prepared_,pressed,&event))return refuse(error,"UI lifecycle");
        if(life_.visible()){if(callbacks_==~u32(0))return refuse(error,"widget callback budget");++callbacks_;}
        if(event.release){prepared_=false;out->released=true;}out->sound=event.sound;if(before==ForceSpecPhase::Active&&!(pressed&0x200))out->unavailable_actor_actions=pressed&0xd00;return true;
    }
    bool draw(const ForceInventory& owner,RenderEngine& render,std::string* error){
        if(!current(owner)||render.model_pass())return refuse(error,"UI draw owner/pass");
        if(life_.phase()==ForceSpecPhase::Resources||life_.phase()==ForceSpecPhase::Done)return true;
        const auto* old=render.scene_lighting();const bool had=old!=nullptr;const auto saved=had?*old:SceneLighting{};if(!ui_.lighting()||!render.set_scene_lighting(*ui_.lighting()))return refuse(error,"UI lighting");
        const auto* fog=ui_.horizon_fog();bool ok=fog&&ForceSpecScene::draw_backdrop(render,life_.visible()?Tint{fog->colour[0],fog->colour[1],fog->colour[2],255}:Tint{0,0,0,255});
        if(ok&&life_.visible()){const auto request=request_.lock();ForceSpecWidgetInput input;input.borg=*request->snapshot().view(owner);const auto* borg=request->snapshot().value(owner);input.experience=borg->experience;input.edited=borg->edits.modified;input.callbacks=callbacks_;input.page=life_.navigation().page();input.blink=life_.navigation().blink();
            ok=ui_.draw_background(render,input,error)&&ui_.draw_stats_ui_foreground(render,input,error)&&label(render,"3D PREVIEW UNAVAILABLE",32,224,16,8)&&label(render,"LEFT / RIGHT NEXT PAGE - B BACK",24,456,14,7);}
        if(ok&&life_.fade().draw()){const auto rgba=life_.fade().colour();ok=ForceSpecScene::draw_backdrop(render,{u8(rgba>>24),u8(rgba>>16),u8(rgba>>8),u8(rgba)});}
        if(had){if(!render.set_scene_lighting(saved))ok=false;}else render.clear_scene_lighting();return ok?true:refuse(error,"UI submission");
    }
    bool take_completion(const ForceInventory& owner,ForceSpecCompletion* out){if(!out)return false;*out={};if(!current(owner)||life_.phase()!=ForceSpecPhase::Done||taken_||(edit_requested_&&edit_scope_.expired()))return false;out->request_=request_;out->ui_only_=true;out->for_edit_=edit_requested_;out->edit_scope_=edit_scope_;taken_=true;return true;}
    bool current(const ForceInventory& owner)const{const auto request=request_.lock();return loaded_&&active_&&request&&request->kind()==ForceSpecPresentationKind::StatisticsUiOnly&&request->snapshot().current(owner);}
    const ForceSpecLifecycle* lifecycle()const{return active_?&life_:nullptr;}
    unsigned callbacks()const{return callbacks_;}
    void reset(){active_=prepared_=taken_=edit_requested_=false;request_.reset();edit_scope_.reset();life_={};callbacks_=0;}
private:
    bool label(RenderEngine& render,const char* ascii,f32 x,f32 y,f32 extent,f32 advance){std::string encoded;return text_.font().encode_ascii(ascii,&encoded)&&text_.draw_screen(render,encoded,x,y,extent,advance,1,1,0x000000ff,0xffffffffu);}
    static bool fail(std::string* error,const char* why){if(error&&error->empty())*error=std::string("stats UI: ")+why;return false;}
    bool refuse(std::string* error,const char* why){reset();return fail(error,why);}
    ForceSpecScene ui_;SourceTextRenderer text_;ForceSpecLifecycleProfile profile_;ForceSpecLifecycle life_;std::weak_ptr<const ForceSpecRequest> request_;std::weak_ptr<const ForceSpecEditScope> edit_scope_;unsigned callbacks_=0;bool loaded_=false,active_=false,prepared_=false,taken_=false,edit_requested_=false;
};
} //namespace gf
