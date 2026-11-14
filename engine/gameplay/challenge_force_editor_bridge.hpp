#pragma once
#include "engine/gameplay/challenge_force_selection.hpp"
#include "engine/gameplay/force_editor_session.hpp"
namespace gf {
//Owns the real shared native editor, not a caller-supplied success boolean.
//The scene/input host remains responsible for rendering the existing editor and
//supplying its qualified normalized input and final carried setup history.
class ChallengeForceEditorBridge {
public:
    bool begin(const ChallengeForceRequest& request,ForceInventory& owner,const ForceSupplement& names,ForceInventoryStore& store){
        if(active_||request.kind()!=ChallengeForceRequestKind::Editor||!request.current(owner))return false;
        const auto scope=request.scope_.lock();if(!scope||scope->claimed)return false;
        if(!editor_.open_for_selection(owner,names,store,request.force(),request.energy(),ForceCostMode::Full))return false;
        scope->claimed=true;request_=request;owner_=&owner;active_=true;return true;
    }
    bool step(ForceInventory& owner,ForceEditorHit hit,u32 repeated,u32 pressed,int adjustment=0){
        if(!bound(owner))return false;
        if(editor_.editing_borg())return editor_.step_borg_editor(repeated,pressed,adjustment);
        if(editor_.warehouse_tools())return editor_.step_warehouse_tools(repeated,pressed,adjustment);
        return editor_.step(hit,repeated,pressed);
    }
    bool choose_force(ForceInventory& owner,int direction){return bound(owner)&&editor_.choose_force(direction);}
    bool complete(ChallengeForceSelection& page,ForceInventory& owner,const ChallengeSetupButtons& history){
        if(!bound(owner)||!page.can_return(owner,request_)||editor_.entry()!=ForceEditorEntry::Selection||editor_.view()!=ForceEditorView::Selection)return false;
        ForceEditorReturn returned;if(!editor_.take_return(&returned)||returned.destination!=ForceEditorDestination::Selection||returned.saved||!page.resume_editor(owner,request_,returned.force,history))return false;
        active_=false;return true;
    }
    const ForceEditorSession* session(const ForceInventory& owner)const{return bound(owner)?&editor_:nullptr;}
private:
    bool bound(const ForceInventory& owner)const{return active_&&owner_==&owner&&request_.valid(owner);}
    ForceEditorSession editor_;ChallengeForceRequest request_;ForceInventory* owner_=nullptr;bool active_=false;
};
} // namespace gf
