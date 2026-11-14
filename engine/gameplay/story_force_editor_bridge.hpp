#pragma once
#include "engine/gameplay/story_force_selection.hpp"
#include "engine/gameplay/force_editor_session.hpp"
namespace gf {
class StoryForceOuterController;
namespace story_force_editor_outer_detail {struct Scope {bool claimed=false;};}
class StoryForceEditorOuterReturn {
public:
    bool current(const ForceInventory& owner)const{const auto scope=scope_.lock();return scope&&!scope->claimed&&request_.valid(owner)&&owner.revision()==revision_;}
    unsigned force()const{return force_;}u8 port()const{return request_.port();}const StoryForceSetupFacts& facts()const{return request_.facts();}
private:
    friend class StoryForceEditorBridge;friend class StoryForceOuterController;
    std::weak_ptr<story_force_editor_outer_detail::Scope> scope_;StoryForceEditorRequest request_;u64 revision_=0;unsigned force_=0;
};
//Adapted collected Challenge bridge: same real editor, inventory, Full budget,
//genuine take_return and zero selection-mode save. No caller success boolean.
//Host still owns editor graphics/input normalization and final carried history.
class StoryForceEditorBridge {
public:
    StoryForceEditorBridge()=default;
    StoryForceEditorBridge(const StoryForceEditorBridge&)=delete;StoryForceEditorBridge& operator=(const StoryForceEditorBridge&)=delete;
    StoryForceEditorBridge(StoryForceEditorBridge&&)=default;StoryForceEditorBridge& operator=(StoryForceEditorBridge&&)=default;
    bool begin(const StoryForceEditorRequest& request,ForceInventory& owner,const ForceSupplement& names,ForceInventoryStore& store){
        if(active_||!request.current(owner))return false;
        const auto scope=request.scope_.lock();if(!scope||scope->claimed)return false;
        if(!editor_.open_for_selection(owner,names,store,request.force(),request.energy(),ForceCostMode::Full))return false;
        return_scope_.reset();scope->claimed=true;request_=request;owner_=&owner;active_=true;return true;
    }
    bool step(ForceInventory& owner,ForceEditorHit hit,u32 repeated,u32 pressed,int adjustment=0){
        if(!bound(owner))return false;
        if(editor_.editing_borg())return editor_.step_borg_editor(repeated,pressed,adjustment);
        if(editor_.warehouse_tools())return editor_.step_warehouse_tools(repeated,pressed,adjustment);
        return editor_.step(hit,repeated,pressed);
    }
    bool choose_force(ForceInventory& owner,int direction){return bound(owner)&&editor_.choose_force(direction);}
    bool complete(StoryForceSelection& page,ForceInventory& owner,const SourceSetupButtons& history){
        if(!bound(owner)||!page.can_return(owner,request_)||editor_.entry()!=ForceEditorEntry::Selection||editor_.view()!=ForceEditorView::Selection)return false;
        ForceEditorReturn returned;if(!editor_.take_return(&returned)||returned.destination!=ForceEditorDestination::Selection||returned.saved||!page.resume_editor(owner,request_,returned.force,history))return false;
        active_=false;return true;
    }
    const ForceEditorSession* session(const ForceInventory& owner)const{return bound(owner)?&editor_:nullptr;}
    //F77C outer dispatch owns the return/reentry. Unlike complete(), this does
    //NOT shortcut back to the old prepared selector's Enter phase.
    bool take_outer_return(const ForceInventory& owner,StoryForceEditorOuterReturn* out){
        if(out)*out={};
        if(!out||!bound(owner)||editor_.entry()!=ForceEditorEntry::Selection||editor_.view()!=ForceEditorView::Selection)return false;
        ForceEditorReturn returned;if(!editor_.take_return(&returned)||returned.destination!=ForceEditorDestination::Selection||returned.saved||returned.force>=kForceCount)return false;
        return_scope_=std::make_shared<story_force_editor_outer_detail::Scope>();StoryForceEditorOuterReturn result;result.scope_=return_scope_;result.request_=request_;result.revision_=owner.revision();result.force_=returned.force;*out=result;active_=false;return true;
    }
private:
    bool bound(const ForceInventory& owner)const{return active_&&owner_==&owner&&request_.valid(owner);}
    ForceEditorSession editor_;StoryForceEditorRequest request_;ForceInventory* owner_=nullptr;bool active_=false;
    std::shared_ptr<story_force_editor_outer_detail::Scope> return_scope_;
};
} //namespace gf
