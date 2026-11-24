#pragma once
#include <exception>
#include "engine/gameplay/challenge_setup_flow.hpp"
#include "engine/gameplay/challenge_setup_scene.hpp"
#include "engine/gameplay/challenge_box_load_driver.hpp"
#include "engine/graphics/screen_fade.hpp"
#include "engine/gameplay/challenge_exit_visual.hpp"
#include "engine/gameplay/challenge_native_notice.hpp"
#include "engine/gameplay/challenge_force_scene.hpp"
#include "engine/gameplay/challenge_force_notices.hpp"
#include "engine/gameplay/challenge_setup_choice.hpp"
namespace gf {
//Default: entry→difficulty→players→FIRST box→selection REQUEST. Fresh explicit
//opt-ins add dialogs and retained Force Selection. One setup archive/cache/chrome;
//box readiness is an explicit native observation, not source DMA timing.
//A further fresh opt-in joins the second box. Editor graphics, live RNG ownership
//and battle remain external; selected-player candidates are not battle ACKs.
class ChallengeSetupPresentation {
public:
    bool open(ByteView bank,const ChallengeDifficultyArt& common,const ChallengeSetupWidgetsArt& widgets,const AttachmentAlternateProfile& style,const ChallengeEntryProfile& entry,const ChallengeDifficultyProfile& difficulty,const ChallengeSetupPagesProfile& pages,const ForceCatalog& catalog,u8 participants,std::string* error){
        *this={};if(!bank.data||bank.size<32||bank.size>(64u<<20)||!common.valid()||!widgets.valid()||widgets.bank_hash!=common.bank_hash||common.limits!=difficulty.energy||!attachment_alternate_profile_valid(style)||!catalog.ready()||!flow_.open(entry,difficulty,pages,participants))return fail(error,"entry/profile/bank/catalog domain");
        bytes_.assign(bank.data,bank.data+bank.size);common_=common;widgets_art_=widgets;style_=style;catalog_=catalog;opened_=true;return true;
    }
    bool enable_dialogs(const ChallengeExitDialogArt& art,std::string* error){
        if(!opened_||failed_||widgets_||art.bank_hash!=common_.bank_hash||!flow_.enable_exit_dialog(art))return fail(error,"fresh source-bound dialogs required");
        exit_art_=art;dialogs_enabled_=true;return true;
    }
    //Fresh opt-in. Legacy open/step/take_inventory stays request-only. Icons are
    //explicitly prepared native assets, not a fabricated source DMA/read ACK.
    bool enable_force_selection(const ChallengeForceSelectionProfile& profile,const ChallengeForceArt& art,const ChallengeForceCardsArt& cards,const ChallengeForceNoticesArt& notices,const HsdRecordAnimationBank& icons,const ForceEditorArt& mapping,const SourceFont& font,RenderEngine& render,std::string* error){
        if(!opened_||failed_||widgets_||force_enabled_||flow_.updates()||!dialogs_enabled_||art.bank_hash!=common_.bank_hash||cards.bank_hash!=common_.bank_hash||notices.bank_hash!=common_.bank_hash||!art.valid()||!cards.valid()||!notices.valid()||!flow_.enable_force_selection(profile)||!force_scene_.load_prepared(icons,mapping,common_,font,render,error))return fail(error,"fresh source-bound Force Selection assets required");
        force_art_=art;force_cards_=cards;force_notices_art_=notices;force_enabled_=true;return true;
    }
    //Separate fresh opt-in: the first-force-only mode still ends at its request.
    bool enable_second_box(std::string* error){
        if(!opened_||failed_||widgets_||flow_.updates()||!force_enabled_||second_enabled_||!flow_.enable_second_box())return fail(error,"fresh retained second-box owner required");
        second_enabled_=true;choice_batch_=std::make_shared<challenge_choice_detail::Batch>();return true;
    }
    //Explicit fresh-normal entry producer, BEFORE any setup callback. Owns the
    //represented common64 clear and fresh95FBC context fields, not all caller/
    //common01/menu-transition/resource work. Old/default APIs retain UNKNOWN
    //preparation entry state. No public flag setter, late seed or replay route.
    //The V2 recipe additionally owns the fresh enclosing common00/caller4
    //lineage. V1 does not infer it from the Challenge label.
    bool initialize_normal_preparation_entry(const ChallengePreparationEntryProfile& profile,std::string* error){
        if(!opened_||failed_||!second_enabled_||!choice_batch_||choice_batch_->ready||choice_batch_->claimed||flow_.updates()||flow_.phase()!=ChallengeSetupPhase::Entry||preparation_entry_||!profile.ready()){if(error)*error="Challenge setup: fresh unstepped normal preparation entry/profile required";return false;}
        try{auto next=std::make_shared<challenge_preparation_entry_detail::Storage>();if(profile.normal_dispatch()){next->state.common00=1;next->state.caller4=0;}if(profile.challenge_mode_lineage())next->state.common32=1;preparation_entry_=next;choice_batch_->preparation_entry=next;if(error)error->clear();return true;}
        catch(const std::exception&){return fail(error,"uncertain normal entry allocation; no replay");}
    }
    const ChallengePreparationEntryState* preparation_entry_state()const{return !failed_&&preparation_entry_&&preparation_entry_->live?&preparation_entry_->state:nullptr;}
    bool bind_normal_halfword_rng(const SourceHalfwordRngOwner& rng,std::string* error){
        if(!opened_||failed_||!preparation_entry_||!preparation_entry_->live||preparation_entry_->state.common32!=1||preparation_entry_->halfword_rng||flow_.updates()||flow_.phase()!=ChallengeSetupPhase::Entry||!rng.ready()){if(error)*error="Challenge setup: fresh mode1 entry and explicit independent halfword stream required";return false;}
        preparation_entry_->halfword_rng=rng.snapshot();if(error){error->clear();}return true;
    }
    bool step(const std::array<ChallengeSetupPad,4>& pads,bool box_resources_ready,ForceInventoryStore* first_store,ChallengeSetupEvent* out,std::string* error,ForceInventoryStore* second_store=nullptr){
        if(!out)return false;
        *out={};if(!opened_||failed_||flow_.terminal())return false;
        auto& driver=current_driver();
        if(driver.pending()&&!driver.observe_notice(pads))return fail(error,"native notice request expired");
        const auto receipt=driver.receipt();ChallengeSetupEvent event;
        if(!flow_.step(resources_ready_,box_resources_ready,pads,receipt.valid()?&receipt:nullptr,&event,force_enabled_&&display_inventory_ready()?&display_inventory():nullptr))return fail(error,"setup callback");
        if(event.entry.request_resources){if(!resources_.load({bytes_.data(),bytes_.size()},error,true)||resources_.hash()!=common_.bank_hash)return fail(error,"native resources");resources_ready_=true;bytes_.clear();++preparations_;}
        const bool creating=event.entry.build_widgets;
        if(creating){if(!visual_.open(common_,widgets_art_)||!scene_.load_prepared(resources_,common_,style_,error)||(force_enabled_&&!force_notices_.open(force_notices_art_)))return fail(error,"widget construction");widgets_=true;++constructions_;}
        if(event.exit_dialog_created){if(!dialogs_enabled_||!exit_visual_.open(resources_,exit_art_,common_.operands[8],u16(flow_.updates()),error))return fail(error,"exit widget construction");}
        else if(dialogs_enabled_&&exit_visual_.live()&&!exit_visual_.update(flow_.exit_dialog().state(),flow_.exit_dialog().selection(),u16(flow_.updates()),error))return fail(error,"exit widget update");
        if(event.box.request_load){
            if(!first_store)return fail(error,"native gameplay store required for requested read");
            try{if(!driver.begin(event.box.load,catalog_,*first_store,second_store,error))return fail(error,"native load request");}
            catch(const std::exception&){return fail(error,"uncertain native read; claimed request is not replayed");}
        }
        if(event.selection_requested){
            const bool second=flow_.visual_input().context==2;auto& destination=second?second_inventory_:inventory_;const ForceInventory* first=second?&inventory_:nullptr;
            const bool published=event.box.loaded?driver.publish_loaded(flow_.box(),destination,first):challenge_box_fallback(flow_.box(),catalog_,first,destination);
            if(!published)return fail(error,"inventory publication");
            if(second)second_inventory_ready_=true;else inventory_ready_=true;
            if(force_enabled_){if(!flow_.accept_selection_inventory(destination)||(second?!force_visual_.switch_inventory(inventory_,destination):!force_visual_.open(force_art_,force_cards_,destination)))return fail(error,"Force Selection inventory handoff");displayed_player_=second?1:0;force_opened_=true;event.force_opened=true;}
        }
        if(second_enabled_&&event.force.second_box_requested){if(!capture_choice(0,event.force)||!flow_.admit_second_box())return fail(error,"first acceptance/second box handoff");event.second_box_opened=true;}
        if(second_enabled_&&event.force.finish_requested){if(!capture_choice(flow_.visual_input().context==2?1:0,event.force))return fail(error,"selected force handoff");choice_batch_->count=flow_.team_up()?2:1;choice_batch_->ready=true;event.selections_ready=true;}
        if(event.entry.retire_widgets){widgets_=false;scene_={};visual_={};resources_={};resources_ready_=false;exit_visual_={};if(force_enabled_){force_visual_={};force_scene_={};force_notices_={};force_opened_=false;inventory_ready_=false;}if(second_enabled_){second_inventory_ready_=false;choices_={};}}
        if(widgets_&&!creating&&!visual_.step(flow_.visual_input()))return fail(error,"widget update");
        if(force_enabled_&&widgets_&&!creating){
            const auto in=flow_.visual_input();if(!force_notices_.step({in.visible,in.interactive,in.closing,flow_.team_up(),in.context,flow_.box_ports()}))return fail(error,"source force notices");
            if(force_opened_&&flow_.phase()!=ChallengeSetupPhase::ForceEditorRequested){const auto& page=flow_.force_selection();const auto selected=page.cursor()<0?s16(-1):s16(page.selected_force());if(!force_visual_.step(display_inventory(),{(in.visible&4)!=0,selected,flow_.energy()}))return fail(error,"force widget update");}
        }
        *out=event;return true;
    }
    bool draw(RenderEngine& render,std::string* error){
        if(!opened_||failed_)return false;
        if(widgets_){
            if(force_enabled_){auto poses=visual_.draws();const auto& extra=exit_visual_.draws();poses.insert(poses.end(),extra.begin(),extra.end());
                if(force_opened_&&flow_.phase()!=ChallengeSetupPhase::ForceEditorRequested){if(!force_scene_.draw(render,scene_,poses,force_visual_,display_inventory(),error,force_notices_.draws()))return fail(error,"force widget draw");}
                else{const auto& notice=force_notices_.draws();poses.insert(poses.end(),notice.begin(),notice.end());if(!scene_.draw(render,poses,error))return fail(error,"retained widget draw");}
            }
            else if(dialogs_enabled_){auto poses=visual_.draws();const auto& extra=exit_visual_.draws();poses.insert(poses.end(),extra.begin(),extra.end());if(!scene_.draw(render,poses,error))return fail(error,"widget draw");}
            else if(!scene_.draw(render,visual_.draws(),error))return fail(error,"widget draw");
        }
        if(dialogs_enabled_&&!draw_challenge_native_notice(render,notice()))return fail(error,"native-file notice draw");
        if(!draw_screen_fade(render,flow_.fade(),640,480))return fail(error,"fade draw");
        return true;
    }
    //Only a cancelled external exit-confirmation owner can resume this same box;
    //this method is not a load/card/next-screen acceptance receipt.
    bool cancel_exit_request(){return opened_&&!failed_&&flow_.cancel_exit_request();}
    const ChallengeSetupFlow& flow()const{return flow_;}const ChallengeSetupVisual* visual()const{return widgets_&&!failed_?&visual_:nullptr;}
    ChallengeNativeLoadNotice notice()const{const auto& driver=current_driver();return driver.pending()?driver.notice():ChallengeNativeLoadNotice::None;}
    const ForceInventory* inventory()const{return inventory_ready_&&!failed_?&inventory_:nullptr;}
    const ForceInventory* player_inventory(unsigned player)const{return failed_||player>1?nullptr:player?(second_inventory_ready_?&second_inventory_:nullptr):(inventory_ready_?&inventory_:nullptr);}
    const ForceInventory* current_inventory()const{return !failed_&&display_inventory_ready()?&display_inventory():nullptr;}
    bool take_inventory(ForceInventory* out){if(!out||!inventory_ready_||failed_||flow_.phase()!=ChallengeSetupPhase::SelectionRequested)return false;*out=std::move(inventory_);inventory_ready_=false;return true;}
    bool failed()const{return failed_;}unsigned preparations()const{return preparations_;}unsigned constructions()const{return constructions_;}std::size_t models()const{return scene_.models();}
    const ChallengeExitVisual* exit_visual()const{return dialogs_enabled_&&!failed_?&exit_visual_:nullptr;}
    const ChallengeForceVisual* force_visual()const{return force_opened_&&!failed_?&force_visual_:nullptr;}
    const ChallengeForceNotices* force_notices()const{return force_enabled_&&widgets_&&!failed_?&force_notices_:nullptr;}
    bool begin_force_editor(ChallengeForceEditorBridge& bridge,const ChallengeForceRequest& request,const ForceSupplement& names,ForceInventoryStore& store){return force_enabled_&&!failed_&&flow_.phase()==ChallengeSetupPhase::ForceEditorRequested&&bridge.begin(request,display_inventory(),names,store);}
    bool step_force_editor(ChallengeForceEditorBridge& bridge,ForceEditorHit hit,u32 repeated,u32 pressed,int adjustment=0){return force_enabled_&&!failed_&&flow_.phase()==ChallengeSetupPhase::ForceEditorRequested&&bridge.step(display_inventory(),hit,repeated,pressed,adjustment);}
    bool choose_force_in_editor(ChallengeForceEditorBridge& bridge,int direction){return force_enabled_&&!failed_&&flow_.phase()==ChallengeSetupPhase::ForceEditorRequested&&bridge.choose_force(display_inventory(),direction);}
    const ForceEditorSession* force_editor_session(const ChallengeForceEditorBridge& bridge)const{return force_enabled_&&!failed_&&flow_.phase()==ChallengeSetupPhase::ForceEditorRequested?bridge.session(display_inventory()):nullptr;}
    bool complete_force_editor(ChallengeForceEditorBridge& bridge,const ChallengeSetupButtons& history,std::string* error){
        if(!force_enabled_||failed_||!flow_.complete_force_editor(bridge,display_inventory(),history))return false;
        if(!force_visual_.rebind_inventory(display_inventory()))return fail(error,"editor-return visual binding");
        return true;
    }
    bool take_force_roster(ForceBattleRoster* out){if(out)*out={};return !second_enabled_&&force_enabled_&&!failed_&&inventory_ready_&&flow_.take_force_roster(inventory_,out);}
    bool take_random_request(ChallengeForceRequest* out){if(out)*out={};return !second_enabled_&&force_enabled_&&!failed_&&inventory_ready_&&flow_.take_random_request(inventory_,out);}
    bool take_choice(unsigned player,ChallengeSetupChoice* out){
        if(out)*out={};
        const auto* owner=player_inventory(player);
        if(!out||!second_enabled_||!owner||flow_.phase()!=ChallengeSetupPhase::ForceFinished||choice_taken_[player]||!choices_[player].current(*owner))return false;
        *out=choices_[player];choice_taken_[player]=true;return true;
    }
private:
    friend class ChallengeFinalSetup;
    bool retained_final_callback(bool clear_input,bool retire,std::string* error){
        if(!opened_||failed_||!flow_.retained_final_callback(clear_input,retire))return fail(error,"retained final callback");
        if(retire){widgets_=false;scene_={};visual_={};resources_={};resources_ready_=false;exit_visual_={};force_visual_={};force_scene_={};force_notices_={};force_opened_=false;return true;}
        if(widgets_&&!visual_.step(flow_.visual_input()))return fail(error,"retained final chrome");
        if(force_enabled_&&widgets_){const auto in=flow_.visual_input();if(!force_notices_.step({in.visible,in.interactive,in.closing,flow_.team_up(),in.context,flow_.box_ports()}))return fail(error,"retained final notices");if(force_opened_){const auto& page=flow_.force_selection();const auto selected=page.cursor()<0?s16(-1):s16(page.selected_force());if(!force_visual_.step(display_inventory(),{(in.visible&4)!=0,selected,flow_.energy()}))return fail(error,"retained final force widgets");}}
        return true;
    }
    bool capture_choice(unsigned player,const ChallengeForceEvent& event){
        if(player>1||choices_[player].kind()!=ChallengeSetupChoiceKind::None)return false;
        auto& choice=choices_[player];choice.player_=u8(player);choice.port_=event.port;choice.energy_=flow_.energy();choice.batch_=choice_batch_;
        const auto* owner=player_inventory(player);if(!owner)return false;
        if(event.random){if(!flow_.take_random_request(*owner,&choice.random_))return false;choice.kind_=ChallengeSetupChoiceKind::RandomRequest;}
        else{if(!flow_.take_force_roster(*owner,&choice.stored_))return false;choice.kind_=ChallengeSetupChoiceKind::Stored;}
        return true;
    }
    ChallengeBoxLoadDriver& current_driver(){return flow_.visual_input().context==2?second_driver_:driver_;}
    const ChallengeBoxLoadDriver& current_driver()const{return flow_.visual_input().context==2?second_driver_:driver_;}
    ForceInventory& display_inventory(){return displayed_player_?second_inventory_:inventory_;}
    const ForceInventory& display_inventory()const{return displayed_player_?second_inventory_:inventory_;}
    bool display_inventory_ready()const{return displayed_player_?second_inventory_ready_:inventory_ready_;}
    bool fail(std::string* error,const char* why){failed_=true;widgets_=false;inventory_ready_=false;if(choice_batch_)choice_batch_->live=false;if(preparation_entry_)preparation_entry_->live=false;if(error&&error->empty())*error=std::string("Challenge setup presentation: ")+why;return false;}
    ChallengeSetupFlow flow_;ChallengeDifficultyArt common_;ChallengeSetupWidgetsArt widgets_art_;AttachmentAlternateProfile style_;ForceCatalog catalog_;ForceInventory inventory_;ChallengeBoxLoadDriver driver_;
    HsdRecordAnimationBank resources_;ChallengeSetupVisual visual_;ChallengeSetupScene scene_;std::vector<u8> bytes_;unsigned preparations_=0,constructions_=0;bool opened_=false,failed_=false,resources_ready_=false,widgets_=false,inventory_ready_=false;
    ChallengeExitDialogArt exit_art_;ChallengeExitVisual exit_visual_;bool dialogs_enabled_=false;
    ChallengeForceArt force_art_;ChallengeForceCardsArt force_cards_;ChallengeForceNoticesArt force_notices_art_;ChallengeForceVisual force_visual_;ChallengeForceScene force_scene_;ChallengeForceNotices force_notices_;bool force_enabled_=false,force_opened_=false;
    ForceInventory second_inventory_;ChallengeBoxLoadDriver second_driver_;std::array<ChallengeSetupChoice,2> choices_;std::array<bool,2> choice_taken_{};u8 displayed_player_=0;bool second_enabled_=false,second_inventory_ready_=false;
    std::shared_ptr<challenge_choice_detail::Batch> choice_batch_;
    std::shared_ptr<challenge_preparation_entry_detail::Storage> preparation_entry_;
};
} // namespace gf
