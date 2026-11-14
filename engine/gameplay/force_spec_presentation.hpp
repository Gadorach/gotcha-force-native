#pragma once
#include "engine/gameplay/force_spec_request.hpp"
#include "engine/gameplay/force_spec_lifecycle.hpp"
#include "engine/gameplay/force_spec_scene.hpp"
#include "engine/gameplay/character_preview_frame.hpp"
namespace gf {
struct ForceSpecPresentationEvent {u16 sound=0;bool released=false;u32 unhandled_actor_actions=0;bool actor_reset=false;std::vector<CharacterPreviewSoundRequirement> sound_requirements;bool actor_demo_started=false;};
// Joined original UI + two reviewed normal idle actors. No demonstration/action
// acknowledgement or broader class/variant admission is implied by this owner.
class ForceSpecPresentation {
public:
    ForceSpecPresentation()=default;
    ForceSpecPresentation(const ForceSpecPresentation&)=delete;
    ForceSpecPresentation& operator=(const ForceSpecPresentation&)=delete;
    ForceSpecPresentation(ForceSpecPresentation&&)=default;
    ForceSpecPresentation& operator=(ForceSpecPresentation&&)=default;
    bool load(ByteView bank,const ForceSpecArt& art,const SourceMessageBank& messages,const SourceFont& font,
              const ForceSpecLifecycleProfile& lifecycle,const CharacterPreviewProfile& preview,RenderEngine& render,std::string* error){
        return load_impl(bank,art,messages,font,lifecycle,preview,render,error,false);
    }
    bool load_with_horizon_fog(ByteView bank,const ForceSpecArt& art,const SourceMessageBank& messages,const SourceFont& font,
                              const ForceSpecLifecycleProfile& lifecycle,const CharacterPreviewProfile& preview,RenderEngine& render,std::string* error){
        return load_impl(bank,art,messages,font,lifecycle,preview,render,error,true);
    }
private:
    bool load_impl(ByteView bank,const ForceSpecArt& art,const SourceMessageBank& messages,const SourceFont& font,
                   const ForceSpecLifecycleProfile& lifecycle,const CharacterPreviewProfile& preview,RenderEngine& render,std::string* error,bool fog){
        reset();loaded_=false;
        if(!lifecycle.valid()||!character_preview_profile_valid(preview)||lifecycle.bank_hash!=art.bank_hash||preview.scene_hash!=art.bank_hash)return fail(error,"scene/profile binding");
        if(!(fog?ui_.load_with_horizon_fog(bank,art,messages,font,render,error):ui_.load(bank,art,messages,font,render,error)))return false;
        lifecycle_profile_=lifecycle;preview_profile_=preview;loaded_=true;return true;
    }
public:
    bool open(const ForceInventory& owner,const std::shared_ptr<const ForceSpecRequest>& request,std::string* error){
        reset();if(!loaded_||!request)return fail(error,"loaded scene and editor request required");
        const auto* view=request->snapshot().view(owner);const auto* value=request->snapshot().value(owner);
        if(!view||!value||value->variant||(value->model!=0x615&&value->model!=10))return fail(error,"normal G-Red/Sasuke preview scope");
        if(!lifecycle_.open(lifecycle_profile_,view->page_mask,ForceSpecPage::Statistics))return fail(error,"initial page availability");
        request_=request;active_=true;angles_={preview_profile_.pitch,preview_profile_.yaw};return true;
    }
    // Real CPU parsing/actor construction and texture upload establish readiness;
    // callers cannot assert a successful allocation with a boolean observation.
    bool prepare(const ForceInventory& owner,const CharacterPreviewAssets& assets,ByteView nameplate,
                  RenderEngine& render,CharacterPreviewOutput* out,std::string* error){
        return prepare_impl(owner,assets,nameplate,render,out,error);
    }
    bool prepare_with_actor_actions(const ForceInventory& owner,const CharacterPreviewAssets& assets,ByteView nameplate,
                                    const CharacterPoseHeightProfile& height,const ForceSpecActorActionProfile& actions,ByteView events,ByteView parts,
                                    RenderEngine& render,CharacterPreviewOutput* out,std::string* error,
                                    CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly){
        return prepare_impl(owner,assets,nameplate,render,out,error,&height,&actions,events,parts,root_mode);
    }
    //Explicit scene demo opt-in; reset-only preparation keeps its old A policy.
    //A start is an observed request, never ForceSpecCompletion or VM-done proof.
    bool prepare_with_demo_actor_actions(const ForceInventory& owner,const CharacterPreviewAssets& assets,ByteView nameplate,
                                         const CharacterPoseHeightProfile& height,const ForceSpecActorActionProfile& actions,ByteView events,ByteView parts,
                                         RenderEngine& render,CharacterPreviewOutput* out,std::string* error,
                                         CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly){
        return prepare_impl(owner,assets,nameplate,render,out,error,&height,&actions,events,parts,root_mode,true);
    }
private:
    bool prepare_impl(const ForceInventory& owner,const CharacterPreviewAssets& assets,ByteView nameplate,RenderEngine& render,CharacterPreviewOutput* out,std::string* error,
                      const CharacterPoseHeightProfile* height=nullptr,const ForceSpecActorActionProfile* actions=nullptr,ByteView events={},ByteView parts={},
                       CharacterMotionRootObservationMode root_mode=CharacterMotionRootObservationMode::AObjOnly,bool demo=false){
        if(out)*out={};
        const auto request=request_.lock();if(!out||!current(owner)||prepared_||lifecycle_.phase()!=ForceSpecPhase::Resources)return refuse(error,"resource preparation order/owner");
        auto actor=std::make_unique<CharacterPreviewFrame>();CharacterPreviewOutput result;
        const auto* borg=request->snapshot().value(owner);
        if(demo&&(!height||!actions))return refuse(error,"explicit demo actor profiles required");
        if(!(demo?actor->load_with_scene_demo_controls(assets,preview_profile_,borg->variant,*height,events,parts,*actions,lifecycle_,&result,error,root_mode):height&&actions?actor->load_with_scene_actor_controls(assets,preview_profile_,borg->variant,*height,events,parts,*actions,lifecycle_,&result,error,root_mode):actor->load(assets,preview_profile_,CharacterPreviewMode::OpeningIdleWithAuxiliaryRequirements,borg->variant,&result,error)))return refuse(error,"preview construction");
        if(!actor->state()||actor->state()->scalars.klass.model_id!=u16(borg->model))return refuse(error,"requested instance/model resources differ");
        if(!ui_.bind_nameplate(u16(borg->model),nameplate,render,error))return refuse(error,"nameplate upload");
        actor->upload_textures_shared(render,borg->model==10?40512:40000);
        if(render.shared_texture_upload_failed())return refuse(error,"actor texture upload");
        requirements_=result.auxiliary_requirements;actor_=std::move(actor);prepared_=true;actor_actions_mode_=height&&actions;*out=std::move(result);return true;
    }
public:
    bool can_request_borg_editor(const ForceInventory& owner)const{return current(owner)&&prepared_&&actor_&&actor_->ready()&&!edit_requested_&&lifecycle_.phase()==ForceSpecPhase::Active&&lifecycle_.navigation().page()==ForceSpecPage::Statistics;}
    bool request_borg_editor(const ForceInventory& owner,const ForceSpecEditRequest& request){if(!can_request_borg_editor(owner)||request.scope_.expired()||request.request_.lock()!=request_.lock())return false;edit_scope_=request.scope_;edit_requested_=true;return true;}
    bool step(const ForceInventory& owner,u32 held,u32 pressed,ForceSpecPresentationEvent* out,std::string* error){
        if(out)*out={};
        if(!out||!current(owner)||lifecycle_.phase()==ForceSpecPhase::Done)return refuse(error,"inactive/stale presentation");
        if(edit_requested_)pressed=0x200u; //Native edit intent uses the real cover/retire/release path.
        if(actor_actions_mode_)return step_actions_impl(owner,held,pressed,out,error);
        const auto before=lifecycle_.phase();ForceSpecLifecycleEvents event;
        if(!lifecycle_.step(prepared_,pressed,&event))return refuse(error,"scene lifecycle");
        if(event.retire){if(!actor_)return refuse(error,"missing actor retirement");actor_->retire();}
        if(event.release){actor_.reset();prepared_=false;out->released=true;}
        if(lifecycle_.visible()){
            if(!actor_||!actor_->ready()||callbacks_==~u32(0))return refuse(error,"visible actor/callback domain");
            //1DCFA0 updates angle accumulators BEFORE testing6B.6B=1 holds
            // camera/yaw through entry, but does not discard held input history.
            angles_=force_spec_rotate(angles_,held);const auto phase=lifecycle_.phase();
            const auto controls=phase==ForceSpecPhase::Opening||phase==ForceSpecPhase::RevealWait?ForceSpecAngles{preview_profile_.pitch,preview_profile_.yaw}:angles_;
            CharacterPreviewOutput output;if(!actor_->step_opening(controls,lifecycle_.actor_z(),&output,error))return refuse(error,"preview callback");
            ++callbacks_;
        }
        out->sound=event.sound;if(before==ForceSpecPhase::Active&&!(pressed&0x200))out->unhandled_actor_actions=pressed&0xd00u;return true;
    }
    bool draw(const ForceInventory& owner,RenderEngine& render,std::string* error){
        if(!current(owner)||render.model_pass())return refuse(error,"draw owner/pass");
        if(lifecycle_.phase()==ForceSpecPhase::Resources||lifecycle_.phase()==ForceSpecPhase::Done)return true;
        const SceneLighting* old=render.scene_lighting();const bool had=old!=nullptr;const SceneLighting saved=had?*old:SceneLighting{};
        if(!ui_.lighting()||!render.set_scene_lighting(*ui_.lighting()))return refuse(error,"scene lighting");
        bool ok=background(render,lifecycle_.visible()?Tint{64,166,192,255}:Tint{0,0,0,255});
        if(ok&&lifecycle_.visible()){
            const auto request=request_.lock();ForceSpecWidgetInput input;input.borg=*request->snapshot().view(owner);input.experience=request->snapshot().value(owner)->experience;input.edited=request->snapshot().value(owner)->edits.modified;input.callbacks=callbacks_;input.page=lifecycle_.navigation().page();input.blink=lifecycle_.navigation().blink();
            ok=actor_&&actor_->ready()&&ui_.draw_background(render,input,error);
            if(ok)for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass)){ok=false;break;}if(actor_actions_mode_){unsigned count=0;ok=actor_->draw_scene(render,lifecycle_,&count,error);}else actor_->draw(render);if(!render.end_model_pass())ok=false;if(!ok)break;}
            if(ok)ok=ui_.draw_foreground(render,input,error);
        }
        if(ok&&lifecycle_.fade().draw()){const auto rgba=lifecycle_.fade().colour();ok=background(render,{u8(rgba>>24),u8(rgba>>16),u8(rgba>>8),u8(rgba)});}
        if(had){if(!render.set_scene_lighting(saved))ok=false;}else render.clear_scene_lighting();
        return ok?true:refuse(error,"scene submission");
    }
    bool take_completion(const ForceInventory& owner,ForceSpecCompletion* out){
        if(!out)return false;
        *out={};if(!current(owner)||lifecycle_.phase()!=ForceSpecPhase::Done||completion_taken_)return false;
        if(edit_requested_&&edit_scope_.expired())return false;
        out->request_=request_;out->for_edit_=edit_requested_;out->edit_scope_=edit_scope_;completion_taken_=true;return true;
    }
    bool current(const ForceInventory& owner)const{const auto request=request_.lock();return loaded_&&active_&&request&&request->snapshot().current(owner);}
    const ForceSpecLifecycle* lifecycle()const{return active_?&lifecycle_:nullptr;}
    const CharacterPreviewState* actor_state()const{return active_&&actor_?actor_->state():nullptr;}
    unsigned callbacks()const{return callbacks_;}
    std::span<const StartupActorClassEffect> auxiliary_requirements()const{return requirements_;}
    void reset(){active_=prepared_=completion_taken_=actor_actions_mode_=edit_requested_=false;edit_scope_.reset();request_.reset();actor_.reset();lifecycle_={};callbacks_=0;requirements_.clear();angles_={};}
private:
    bool step_actions_impl(const ForceInventory& owner,u32 held,u32 pressed,ForceSpecPresentationEvent* out,std::string* error){
        if(!current(owner))return refuse(error,"scene action inventory owner");
        auto next=lifecycle_;ForceSpecLifecycleEvents event;if(!next.step(prepared_,pressed,&event))return refuse(error,"scene action lifecycle");
        if(event.retire){if(!actor_)return refuse(error,"missing scene actor retirement");actor_->retire();}
        if(event.release){actor_.reset();prepared_=false;out->released=true;}
        if(next.visible()){
            if(!actor_||!actor_->ready()||callbacks_==~u32(0))return refuse(error,"visible scene actor/callback domain");
            CharacterPreviewActionOutput action;if(!actor_->step_scene_actor(next,held,pressed,&action,error))return refuse(error,"source scene actor callback");
            if(action.performed==ForceSpecActorAction::Reset&&!actor_->reset_action_current(action))return refuse(error,"source reset receipt/frame binding");
            if(action.performed==ForceSpecActorAction::Demonstrate&&!actor_->action_start_current(action))return refuse(error,"source demo start receipt/frame binding");
            out->actor_demo_started=action.performed==ForceSpecActorAction::Demonstrate;out->actor_reset=action.performed==ForceSpecActorAction::Reset;out->unhandled_actor_actions=action.unfulfilled_actions;out->sound_requirements=std::move(action.preview.sound_requirements);++callbacks_;
        }
        lifecycle_=std::move(next);out->sound=event.sound;return true;
    }
    static bool background(RenderEngine& render,Tint colour){
        return ForceSpecScene::draw_backdrop(render,colour);
    }
    static bool fail(std::string* error,const char* why){if(error&&error->empty())*error=std::string("spec presentation: ")+why;return false;}
    bool refuse(std::string* error,const char* why){reset();return fail(error,why);}
    ForceSpecScene ui_;ForceSpecLifecycleProfile lifecycle_profile_;CharacterPreviewProfile preview_profile_;ForceSpecLifecycle lifecycle_;
    std::weak_ptr<const ForceSpecRequest> request_;std::unique_ptr<CharacterPreviewFrame> actor_;std::vector<StartupActorClassEffect> requirements_;
    ForceSpecAngles angles_;unsigned callbacks_=0;bool loaded_=false,active_=false,prepared_=false,completion_taken_=false,actor_actions_mode_=false,edit_requested_=false;std::weak_ptr<const ForceSpecEditScope> edit_scope_;
};
} // namespace gf
