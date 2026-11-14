#pragma once
#include "engine/gameplay/challenge_load_box.hpp"
#include "engine/save/force_inventory.hpp"
namespace gf {
enum class ChallengeNativeLoadNotice {None,Loaded,Missing,Unavailable,Invalid,Cancelled};
//Native inventory-file adapter, NOT a GameCube memory-card emulator. Source box
//contexts0/1 use distinct backend namespaces, EACH logical native gameplay slot0.
//Never use OPTION's native slot1 for player2. No write/erase is performed here.
class ChallengeBoxLoadDriver {
public:
    bool begin(const ChallengeBoxLoadRequest& request,const ForceCatalog& catalog,ForceInventoryStore& first,ForceInventoryStore* second,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("Challenge native load: ")+why;return false;};
        if(pending_||read_done_||!request.valid()||!catalog.ready()||request.slot()>1)return fail("fresh one-read owner/live source request/catalog required");
        ForceInventoryStore* store=request.slot()?second:&first;
        if(!store||(request.slot()&&(store==&first||store->same_namespace(first))))return fail("second player requires distinct native gameplay namespace");
        auto scope=request.scope_.lock();if(!scope||scope->claimed)return fail("read request already claimed");
        scope->claimed=true;pending_=true;read_done_=false;taken_=false;request_=request;loaded_=std::make_unique<ForceInventory>();catalog_key_=catalog.fingerprint();
        read_scope_=std::make_shared<challenge_box_detail::ReadScope>();waiting_release_=true;prior_buttons_=0;
        const auto result=store->load(catalog,loaded_.get());
        notice_=result==ForceInventoryLoad::Loaded?ChallengeNativeLoadNotice::Loaded:result==ForceInventoryLoad::Missing?ChallengeNativeLoadNotice::Missing:result==ForceInventoryLoad::Unavailable?ChallengeNativeLoadNotice::Unavailable:ChallengeNativeLoadNotice::Invalid;
        read_done_=true;if(error)error->clear();return true;
    }
    //Explicit native-notice policy: release launching A/Start/B; fresh A accepts,
    //fresh B cancels use of loaded data (or dismisses a failure). B wins A.
    //This is not inferred original card-dialog timing/text.
    bool observe_notice(const std::array<ChallengeSetupPad,4>& pads){
        if(!pending_||!read_done_||!request_.valid()||!read_scope_||read_scope_->acknowledged)return false;
        const auto& pad=pads[request_.port()];const u16 held=pad.connected?challenge_setup_fold_buttons(pad.source_buttons):0;
        const u16 actions=u16(held&0x380u);
        if(waiting_release_){prior_buttons_=held;if(!actions)waiting_release_=false;return true;}
        const u16 pressed=u16(held&u16(~prior_buttons_));prior_buttons_=held;
        if(pressed&0x300u){
            if((pressed&0x200u)&&notice_==ChallengeNativeLoadNotice::Loaded)notice_=ChallengeNativeLoadNotice::Cancelled;
            read_scope_->acknowledged=true;pending_=false;
        }
        return true;
    }
    ChallengeBoxLoadReceipt receipt()const{
        ChallengeBoxLoadReceipt out;if(!read_done_||!read_scope_||!read_scope_->acknowledged)return out;
        out.request_=request_;out.driver_=read_scope_;out.loaded_=notice_==ChallengeNativeLoadNotice::Loaded;return out;
    }
    bool publish_loaded(ChallengeLoadBox& box,ForceInventory& destination,const ForceInventory* first_player=nullptr){
        if(taken_||!read_done_||notice_!=ChallengeNativeLoadNotice::Loaded||!loaded_||!loaded_->ready()||!read_scope_||!read_scope_->acknowledged||box.phase_!=ChallengeBoxPhase::SelectionRequested||!box.loaded_||box.published_||box.scope_!=request_.scope_.lock()||box.scope_->sequence!=request_.sequence_)return false;
        if(box.context_==2&&(!first_player||first_player==&destination||!first_player->ready()||first_player->catalog()->fingerprint()!=catalog_key_))return false;
        if(!destination.load_state(*loaded_->catalog(),*loaded_->state()))return false;
        taken_=true;box.published_=true;read_scope_->acknowledged=false;loaded_.reset();return true;
    }
    ChallengeNativeLoadNotice notice()const{return notice_;}bool pending()const{return pending_;}
private:
    ChallengeBoxLoadRequest request_;std::shared_ptr<challenge_box_detail::ReadScope> read_scope_;std::unique_ptr<ForceInventory> loaded_;
    ChallengeNativeLoadNotice notice_=ChallengeNativeLoadNotice::None;u32 catalog_key_=0;u16 prior_buttons_=0;bool pending_=false,read_done_=false,taken_=false,waiting_release_=true;
};
//80190370: first-player Skip/failed load uses source starter initialization;
//second-player Skip/failed load copies first player's images. Native tool
//provenance is additionally retained; no edited Borg gains battle admission.
inline bool challenge_box_fallback(ChallengeLoadBox& box,const ForceCatalog& catalog,const ForceInventory* first_player,ForceInventory& destination){
    if(box.phase_!=ChallengeBoxPhase::SelectionRequested||box.loaded_||box.published_||!catalog.ready())return false;
    bool accepted=false;
    if(box.context_==1)accepted=destination.create(catalog);
    else if(first_player&&first_player!=&destination&&first_player->ready()&&first_player->catalog()->fingerprint()==catalog.fingerprint())accepted=destination.load_state(catalog,*first_player->state());
    if(accepted)box.published_=true;
    return accepted;
}
} // namespace gf
