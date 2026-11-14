#pragma once
#include "engine/gameplay/story_force_outer.hpp"
#include "engine/gameplay/story_force_editor_bridge.hpp"
namespace gf {
enum class StoryForceOuterDestination {None,MissionPreparation,MapEntry};
namespace story_force_outer_detail {struct Scope {bool live=true;};}
class StoryForceOuterHandoff {
public:
    bool current(const ForceInventory& owner)const{const auto scope=scope_.lock();const auto binding=owner_.lock();return scope&&scope->live&&binding&&owner.ready()&&binding==owner.instance_binding().lock()&&owner.revision()==revision_&&(!has_roster_||owner.current(roster_));}
    StoryForceOuterDestination destination()const{return destination_;}const StoryForceOuterFields& fields()const{return fields_;}const StoryForceSetupFacts& facts()const{return facts_;}bool has_stored_roster()const{return has_roster_;}const ForceBattleRoster& stored()const{return roster_;}
private:
    friend class StoryForceOuterController;std::weak_ptr<story_force_outer_detail::Scope> scope_;std::weak_ptr<const ForceInventoryScope> owner_;u64 revision_=0;StoryForceOuterDestination destination_=StoryForceOuterDestination::None;StoryForceOuterFields fields_;StoryForceSetupFacts facts_;ForceBattleRoster roster_;bool has_roster_=false;
};
struct StoryForceOuterInput {std::array<SourceSetupPad,4> setup{};ForceEditorHit editor_hit;u32 editor_repeated=0,editor_pressed=0;int editor_force_direction=0;};
struct StoryForceOuterEvent {StoryForceNativeEvent setup;bool editor_entered=false,editor_returned=false;u8 editor_context149=0;StoryForceOuterDestination destination=StoryForceOuterDestination::None;};
//Source caller/editor lifecycle around the actual native setup and shared editor
//controllers. No caller editor-completed boolean, no prepared-Enter shortcut.
//Actual editor graphics/GObj/helper F8F4 and mission/map constructors stay separate.
class StoryForceOuterController {
public:
    StoryForceOuterController()=default;StoryForceOuterController(const StoryForceOuterController&)=delete;StoryForceOuterController& operator=(const StoryForceOuterController&)=delete;
    bool open(const StoryForceOuterFields& fields,const StoryMapRebuildProfile& profile,const StoryForceSetupFacts& facts,const ForceInventory& owner,u8 controller_mask,const StoryForceNativeSpec& spec,const ForceSupplement& names,ForceInventoryStore& store){
        close();StoryForceSetupPlan plan;if(!story_force_outer_fields_valid(fields)||fields.major!=5||fields.minor||!story_force_setup_plan(profile,facts,&plan)||!owner.ready()||!names.for_catalog(*owner.catalog()))return false;
        fields_=fields;profile_=profile;facts_=facts;spec_=spec;names_=names;store_=&store;mask_=controller_mask;owner_=owner.instance_binding();revision_=owner.revision();catalog_=owner.catalog()->fingerprint();scope_=std::make_shared<story_force_outer_detail::Scope>();ready_=true;return true;
    }
    bool step(IFileSystem& files,RenderEngine& render,ForceInventory& owner,const StoryForceOuterInput& input,StoryForceOuterEvent* out,std::string* error){
        if(!out)return false;
        *out={};if(!ready_||terminal_)return false;if(!bound(owner)||(fields_.major!=4&&owner.revision()!=revision_)||callbacks_>=8192||(render_&&render_!=&render))return fail(error,"owner/renderer/callback domain");render_=&render;StoryForceOuterEvent event;
        if(fields_.major==5){
            if(fields_.minor==2){StoryForceOuterFields next;if(!story_force_outer_selected_advance_plan(fields_,&next))return fail(error,"selector advance");fields_=next;publish(owner,StoryForceOuterDestination::MissionPreparation);event.destination=handoff_.destination_;}
            else{
                if(fields_.minor==0){StoryForceSetupPlan plan;if(!story_force_setup_plan(profile_,facts_,&plan))return fail(error,"setup facts");if(!plan.selector_enabled){StoryForceOuterFields next;if(!story_force_outer_result_plan(fields_,StoryForceNativeChoiceKind::Bypass,0,&next))return fail(error,"bypass");fields_=next;publish(owner,StoryForceOuterDestination::MissionPreparation);event.destination=handoff_.destination_;}
                    else{if(!native_.open(profile_,facts_,owner,fields_.force4c,fields_.port3c,mask_,spec_))return fail(error,"native setup birth");fields_.minor=1;}}
                if(!terminal_){if(!native_.step(files,render,owner,input.setup,&event.setup,error))return fail(error,"native setup");if(event.setup.destination!=StoryForceNativeChoiceKind::None){StoryForceNativeChoice choice;if(!native_.take_choice(owner,&choice)||!choice.current(owner)||choice.facts()!=facts_||choice.port()!=fields_.port3c)return fail(error,"same captured native setup choice");StoryForceOuterFields next;if(!story_force_outer_result_plan(fields_,choice.kind(),choice.source_result(),&next))return fail(error,"source result route");fields_=next;
                        if(choice.kind()==StoryForceNativeChoiceKind::Editor)editor_request_=choice.editor();
                        else{if(choice.kind()==StoryForceNativeChoiceKind::Stored){roster_=choice.stored();has_roster_=true;}native_.close();if(fields_.major==3){publish(owner,StoryForceOuterDestination::MapEntry);event.destination=handoff_.destination_;}}
                    }}
            }
        }else if(fields_.major==4){
            if(fields_.minor==2){StoryForceOuterFields next;if(!story_force_editor_exit_plan(fields_,&next))return fail(error,"editor exit route");fields_=next;editor_=StoryForceEditorBridge{};}
            else{
                if(fields_.minor==0){StoryForceEditorEntryPlan plan;if(!story_force_editor_entry_plan(fields_,&plan)||plan.helper_f8f4_pending)return fail(error,"source helper F8F4 is unfulfilled, not a native editor ACK");if(!store_||!editor_.begin(editor_request_,owner,names_,*store_))return fail(error,"real shared editor birth");fields_=plan.fields;event.editor_entered=true;event.editor_context149=plan.context149;}
                if(input.editor_force_direction&&(!editor_.choose_force(owner,input.editor_force_direction)))return fail(error,"editor force choice");
                if(!editor_.step(owner,input.editor_hit,input.editor_repeated,input.editor_pressed))return fail(error,"actual editor callback");
                const auto* session=editor_.session(owner);if(!session)return fail(error,"live editor session");
                if(session->view()==ForceEditorView::Selection){StoryForceEditorOuterReturn returned;if(!editor_.take_outer_return(owner,&returned)||!returned.current(owner)||returned.facts()!=facts_||returned.port()!=fields_.port3c)return fail(error,"genuine scoped editor return");StoryForceOuterFields next;if(!story_force_editor_return_plan(fields_,returned.force(),&next))return fail(error,"editor returned force");const auto scope=returned.scope_.lock();if(!scope||scope->claimed)return fail(error,"one-shot editor return");scope->claimed=true;fields_=next;revision_=owner.revision();event.editor_returned=true;native_.close();editor_request_={};}
            }
        }else return fail(error,"supported outer setup/editor dispatch");
        if(!bound(owner)||(fields_.major!=4&&owner.revision()!=revision_))return fail(error,"owner changed during dispatch");
        ++callbacks_;*out=event;if(error)error->clear();return true;
    }
    bool draw_setup(RenderEngine& render,const ForceInventory& owner,std::string* error){if(!ready_||!bound(owner)||(render_&&render_!=&render))return false;return fields_.major!=5||fields_.minor!=1||native_.draw(render,owner,error);}
    const ForceEditorSession* editor_session(const ForceInventory& owner)const{return ready_&&fields_.major==4?editor_.session(owner):nullptr;}
    bool take_handoff(const ForceInventory& owner,StoryForceOuterHandoff* out){if(out)*out={};if(!out||!ready_||!terminal_||taken_||!handoff_.current(owner))return false;*out=handoff_;taken_=true;return true;}
    void close(){if(scope_)scope_->live=false;scope_.reset();native_.close();editor_=StoryForceEditorBridge{};editor_request_={};owner_.reset();ready_=terminal_=taken_=has_roster_=false;store_=nullptr;render_=nullptr;callbacks_=0;roster_={};handoff_={};}
    const StoryForceOuterFields& fields()const{return fields_;}bool ready()const{return ready_;}bool terminal()const{return terminal_;}unsigned callbacks()const{return callbacks_;}
private:
    bool bound(const ForceInventory& owner)const{const auto binding=owner_.lock();return scope_&&scope_->live&&binding&&owner.ready()&&owner.instance_binding().lock()==binding&&owner.catalog()->fingerprint()==catalog_;}
    bool fail(std::string* error,const char* why){if(scope_)scope_->live=false;ready_=false;native_.close();editor_=StoryForceEditorBridge{};if(error&&error->empty())*error=std::string("Story force outer owner: ")+why;return false;}
    void publish(const ForceInventory& owner,StoryForceOuterDestination destination){handoff_.scope_=scope_;handoff_.owner_=owner_;handoff_.revision_=owner.revision();handoff_.destination_=destination;handoff_.fields_=fields_;handoff_.facts_=facts_;handoff_.roster_=roster_;handoff_.has_roster_=has_roster_;terminal_=true;}
    StoryForceOuterFields fields_;StoryMapRebuildProfile profile_;StoryForceSetupFacts facts_;StoryForceNativeSpec spec_;ForceSupplement names_;ForceInventoryStore* store_=nullptr;RenderEngine* render_=nullptr;StoryForceNativePresentation native_;StoryForceEditorBridge editor_;StoryForceEditorRequest editor_request_;ForceBattleRoster roster_;StoryForceOuterHandoff handoff_;
    std::weak_ptr<const ForceInventoryScope> owner_;std::shared_ptr<story_force_outer_detail::Scope> scope_;u64 revision_=0;u32 catalog_=0,callbacks_=0;u8 mask_=0;bool ready_=false,terminal_=false,taken_=false,has_roster_=false;
};
} //namespace gf
