#pragma once
#include "engine/gameplay/menu.hpp"
#include "engine/gameplay/option_menu.hpp"
#include "engine/gameplay/option_transition.hpp"
#include "engine/gameplay/force_editor_session.hpp"
namespace gf {
enum class EditorFrontendView {Main,Option,Cheats,Editor,Unavailable,More,CreditsDelay,Credits,CheatDetails,CheatSaveError};
// Native host handoff, not a simulated retail load/save-card transaction.
// One inventory/store and settings owner survive every menu/editor round-trip.
class EditorFrontend {
public:
    bool open(ForceInventory& owner,const ForceSupplement& names,const ForceSpecificationCatalog& data,ForceInventoryStore& inventory_store,
              OptionSettingsStore& option_store,CheatSettingsStore& cheat_store,const MenuNavigationConfig& navigation,const OptionTransitionConfig& camera,unsigned initial){
        *this={};if(!owner.ready()||!names.for_catalog(*owner.catalog())||!data.for_catalog(*owner.catalog())||!menu_.configure_navigation(navigation)||!menu_.enter_main_menu(initial)||!transition_.configure(camera)||!options_.configure(navigation.repeat_limit,navigation.repeat_restart))return false;
        owner_=&owner;names_=names;data_=data;inventory_store_=&inventory_store;option_store_=&option_store;cheat_store_=&cheat_store;navigation_=navigation;options_.configure_more();
        const auto loaded=cheat_store.load(&cheats_);return loaded==CheatSettingsLoad::Loaded||loaded==CheatSettingsLoad::Missing;
    }
    bool configure_credits(unsigned wait){if(wait>600)return false;credits_wait_=wait;credits_available_=true;return true;}
    bool finish_credits(){if(view_!=EditorFrontendView::Credits)return false;view_=EditorFrontendView::More;more_row_=1;return true;}
    bool enable_mouse_control(){if(view_!=EditorFrontendView::Main)return false;mouse_control_=true;return true;}
    OptionMouseArea mouse_area()const{if(!mouse_control_)return OptionMouseArea::None;if(view_==EditorFrontendView::More)return OptionMouseArea::More;if(view_==EditorFrontendView::Cheats)return OptionMouseArea::Cheats;if(view_==EditorFrontendView::CheatDetails)return OptionMouseArea::Details;if(view_==EditorFrontendView::CheatSaveError)return OptionMouseArea::CheatSaveError;if(view_==EditorFrontendView::Credits)return OptionMouseArea::Credits;if(view_==EditorFrontendView::Option&&transition_.phase()==OptionTransitionPhase::UI){const auto phase=options_.phase();return phase==OptionMenuPhase::Editing?OptionMouseArea::Settings:phase==OptionMenuPhase::Saved?OptionMouseArea::Saved:phase==OptionMenuPhase::SaveError?OptionMouseArea::SaveError:OptionMouseArea::None;}return OptionMouseArea::None;}
    bool update(const std::array<GamePadState,4>& pads,ForceEditorHit hit,u32 editor_repeated,u32 editor_pressed,int adjustment=0,s16 main_pointer_item=-1,const OptionMouseCommand& mouse={},const EditorListMouseCommand& list_mouse={}){
        sounds_.clear();bgm_immediate_=false;if(!owner_||!owner_->ready()||main_pointer_item< -1||main_pointer_item>=s16(kMenuItemCount)||!option_mouse_valid(mouse))return false;
        bool digital=false;for(const auto& pad:pads)digital=digital||menu_pad_bits(pad,48)!=0;const auto pointed=mouse_control_&&!digital&&mouse.area==mouse_area()?mouse:OptionMouseCommand{};
        const auto bits=menu_pad_bits(pads[0],48);const auto input=menu_repeat_step(history_,bits,navigation_);u32 pressed=input.pressed|pointed.pressed,direction=menu_direction_events(input.pressed|input.repeated);
        if(view_==EditorFrontendView::Unavailable){menu_.observe_ports(pads);options_.observe(pads);if(pressed&0x100){if(unavailable_from_more_){view_=EditorFrontendView::More;unavailable_from_more_=false;}else{menu_.resume_main_menu();block_main_confirm(pads);view_=EditorFrontendView::Main;}sounds_.push_back(123);}return true;}
        if(view_==EditorFrontendView::Editor){menu_.observe_ports(pads);const auto list_pointed=mouse_control_&&!digital&&!editor_repeated&&!editor_pressed&&!adjustment?list_mouse:EditorListMouseCommand{};const bool ok=editor_.editing_borg()?editor_.step_borg_editor(editor_repeated,editor_pressed,adjustment):editor_.warehouse_tools()?editor_.step_warehouse_tools(editor_repeated,editor_pressed,adjustment,list_pointed):editor_.step(hit,editor_repeated,editor_pressed,list_pointed);if(!ok)return false;for(auto sound:editor_.sounds())sounds_.push_back(sound);ForceEditorReturn result;if(editor_.take_return(&result)){if(result.destination!=ForceEditorDestination::MainMenu||!result.saved)return false;menu_.resume_main_menu();block_main_confirm(pads);view_=EditorFrontendView::Main;}return true;}
        if(view_==EditorFrontendView::More){menu_.observe_ports(pads);options_.observe(pads);const auto row=more_row_;if(pointed.row>=0)more_row_=unsigned(pointed.row);if(direction==8||direction==4)more_row_=1-more_row_;if(row!=more_row_)sounds_.push_back(119);if(pressed&0x200){view_=EditorFrontendView::Option;sounds_.push_back(123);}else if(pressed&0x100){sounds_.push_back(122);if(!more_row_){view_=EditorFrontendView::Cheats;cheat_row_=0;notice_.clear();}else if(credits_available_){credits_left_=credits_wait_;view_=credits_left_?EditorFrontendView::CreditsDelay:EditorFrontendView::Credits;}else{view_=EditorFrontendView::Unavailable;unavailable_from_more_=true;}}return true;}
        if(view_==EditorFrontendView::CreditsDelay){menu_.observe_ports(pads);options_.observe(pads);if(credits_left_)--credits_left_;if(!credits_left_)view_=EditorFrontendView::Credits;return true;}
        if(view_==EditorFrontendView::Credits){menu_.observe_ports(pads);options_.observe(pads);if(pressed&0x200){view_=EditorFrontendView::More;more_row_=1;}return true;}
        if(view_==EditorFrontendView::CheatDetails){menu_.observe_ports(pads);options_.observe(pads);if(pressed&0x300){view_=EditorFrontendView::Cheats;sounds_.push_back(123);}return true;}
        if(view_==EditorFrontendView::CheatSaveError){menu_.observe_ports(pads);options_.observe(pads);if(pressed&0x200){notice_.clear();view_=EditorFrontendView::Cheats;sounds_.push_back(123);}else if(pressed&0x100)return leave_cheats();return true;}
        if(view_==EditorFrontendView::Cheats){menu_.observe_ports(pads);options_.observe(pads);const auto row=cheat_row_;if(pointed.row>=0)cheat_row_=unsigned(pointed.row);if(direction==8)cheat_row_=(cheat_row_+6)%7;else if(direction==4)cheat_row_=(cheat_row_+1)%7;if(row!=cheat_row_)sounds_.push_back(119);if(pointed.value>=0)direction=pointed.value==0?1:2;
            if(pressed&0x200)return leave_cheats();
            if(pressed&0x800){view_=EditorFrontendView::CheatDetails;sounds_.push_back(122);return true;}
            if(cheat_row_<2&&(direction==1||direction==2||(pressed&0x100))){auto& flag=cheat_row_?cheats_.borg_editor:cheats_.warehouse_selector;const bool before=flag;flag=direction==1?true:direction==2?false:!flag;if(flag!=before)sounds_.push_back(119);}return true;
        }
        if(view_==EditorFrontendView::Option){menu_.observe_ports(pads);
            if(transition_.phase()==OptionTransitionPhase::UI){const auto e=pointed.area==OptionMouseArea::None?options_.update(*option_store_,pads):options_.update_pointed(*option_store_,pads,pointed);bgm_immediate_=e.bgm_immediate;if(e.open_more){view_=EditorFrontendView::More;more_row_=0;notice_.clear();}else if(e.leave)transition_.request_leave();}
            else{options_.observe(pads);if(transition_.update(0)){menu_.resume_main_menu();block_main_confirm(pads);view_=EditorFrontendView::Main;}}
            return true;
        }
        auto main_pads=pads;for(unsigned port=0;port<pads.size();++port)if(main_confirm_blocked_[port]){if(!pads[port].a&&!pads[port].start)main_confirm_blocked_[port]=false;else main_pads[port].a=main_pads[port].start=false;}
        const auto selected=menu_.selection();if(main_pointer_item>=0&&!menu_.point_main_item(unsigned(main_pointer_item)))return false;
        UiSoundBatch sounds;menu_.update_ports_with_sound(main_pads,&sounds);if(menu_.selection()!=selected)sounds_.push_back(119);
        if(menu_.screen()==MenuScreen::Option){options_.begin(*option_store_,pads);if(!transition_.begin(0))return false;view_=EditorFrontendView::Option;sounds_.push_back(122);}
        else if(menu_.screen()==MenuScreen::EditForce){if(!editor_.open(*owner_,names_,*inventory_store_)||!editor_.configure_tools(cheats_,data_))return false;view_=EditorFrontendView::Editor;sounds_.push_back(124);}
        else if(menu_.screen()!=MenuScreen::MainMenu){view_=EditorFrontendView::Unavailable;sounds_.push_back(124);}
        return true;
    }
    EditorFrontendView view()const{return view_;}const MainMenu& menu()const{return menu_;}const OptionMenu& options()const{return options_;}const OptionTransition& transition()const{return transition_;}
    ForceEditorSession& editor(){return editor_;}const ForceEditorSession& editor()const{return editor_;}
    CheatSettings cheats()const{return cheats_;}unsigned cheat_row()const{return cheat_row_;}const std::string& notice()const{return notice_;}std::span<const u16> sounds()const{return sounds_;}
    unsigned more_row()const{return more_row_;}unsigned credits_delay()const{return credits_left_;}
    // Actual OPTION event, including same-value defaults; not inferred from a
    // changed preference or a generic confirm sound. Cleared on every update.
    bool bgm_immediate()const{return bgm_immediate_;}
private:
    bool leave_cheats(){if(!cheat_store_->write(cheats_)){notice_="CHEAT SETTINGS SAVE FAILED";if(mouse_control_)view_=EditorFrontendView::CheatSaveError;return true;}notice_.clear();view_=EditorFrontendView::More;more_row_=0;sounds_.push_back(123);return true;}
    //Host Enter may change its A/Start mapping across views. Consume the whole
    //confirm family until physical release; do not clear histories into a fresh
    //press or let the same held button activate MAIN on return.
    void block_main_confirm(const std::array<GamePadState,4>& pads){for(unsigned port=0;port<pads.size();++port)main_confirm_blocked_[port]=pads[port].a||pads[port].start;}
    ForceInventory* owner_=nullptr;ForceInventoryStore* inventory_store_=nullptr;OptionSettingsStore* option_store_=nullptr;CheatSettingsStore* cheat_store_=nullptr;
    ForceSupplement names_;ForceSpecificationCatalog data_;MainMenu menu_;OptionMenu options_;OptionTransition transition_;ForceEditorSession editor_;CheatSettings cheats_;
    MenuNavigationConfig navigation_;MenuPadHistory history_;std::array<bool,4> main_confirm_blocked_{};EditorFrontendView view_=EditorFrontendView::Main;unsigned cheat_row_=0,more_row_=0,credits_wait_=0,credits_left_=0;bool unavailable_from_more_=false,credits_available_=false,bgm_immediate_=false,mouse_control_=false;std::string notice_;std::vector<u16> sounds_;
};
} // namespace gf
