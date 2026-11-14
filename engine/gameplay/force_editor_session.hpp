#pragma once
#include "engine/gameplay/force_editor_grid.hpp"
#include "engine/gameplay/force_editor_list.hpp"
#include "engine/gameplay/force_selection.hpp"
#include "engine/gameplay/force_name_keyboard.hpp"
#include "engine/gameplay/force_spec_request.hpp"
#include "engine/gameplay/warehouse_tools.hpp"
#include "engine/gameplay/borg_stats_editor.hpp"
#include "engine/save/force_inventory.hpp"
#include "engine/gameplay/editor_list_mouse.hpp"
namespace gf {
enum class ForceEditorView {Grid,List,Spec,Name,SaveError,Selection,ExitBlocked,SelectionExit,Returned,WarehouseTools,BorgEdit};
enum class ForceEditorEntry {Standalone,Selection};
enum class ForceEditorDestination {MainMenu,Selection};
enum class ForceEditorStoragePage {Editor,Warehouse,Picker,Items,Other};
struct ForceEditorReturn {ForceEditorDestination destination=ForceEditorDestination::MainMenu;unsigned force=0;bool saved=false;};
// Joined native editing/warehouse/save/selection controller. Original card
// messaging and combat are not simulated by successful native file writes.
class ForceEditorSession {
public:
    bool open(ForceInventory& owner,const ForceSupplement& names,ForceInventoryStore& store,unsigned force=0){
        return open_impl(owner,names,store,force,ForceEditorEntry::Standalone,-1,ForceCostMode::Full);
    }
    bool open_for_selection(ForceInventory& owner,const ForceSupplement& names,ForceInventoryStore& store,unsigned force,s32 budget,ForceCostMode mode){
        return open_impl(owner,names,store,force,ForceEditorEntry::Selection,budget,mode);
    }
    // Story/Challenge/VS hosts supply their current energy setting. A negative
    // limit hides the source LIMIT/REMAIN panels; no mode-specific value is guessed.
    bool open_with_limit(ForceInventory& owner,const ForceSupplement& names,ForceInventoryStore& store,unsigned force,s32 budget){
        return open_impl(owner,names,store,force,ForceEditorEntry::Standalone,budget,ForceCostMode::Full);
    }
    ForceEditorView view()const{return view_;}const ForceEditorGrid& grid()const{return grid_;}const ForceEditorList& list()const{return list_;}
    ForceEditorView background_view()const{return view_==ForceEditorView::ExitBlocked?return_:view_;}
    ForceEditorEntry entry()const{return entry_;}unsigned notice_wait()const{return notice_wait_;}
    s32 energy_limit()const{return energy_limit_;}
    ForceCostMode cost_mode()const{return cost_mode_;}
    bool configure_tools(CheatSettings cheats,const ForceSpecificationCatalog& data){
        if(!bound()||view_!=ForceEditorView::Grid||!data.for_catalog(*owner_->catalog()))return false;
        cheats_=cheats;tool_data_=data;tools_configured_=true;return true;
    }
    CheatSettings cheats()const{return cheats_;}
    const WarehouseTools* warehouse_tools()const{return view_==ForceEditorView::WarehouseTools?&tools_:nullptr;}
    ForceEditorStoragePage storage_page()const{
        if(view_==ForceEditorView::Grid)return ForceEditorStoragePage::Editor;
        if(view_==ForceEditorView::WarehouseTools)return tools_.page()==WarehouseToolPage::PickerModels||tools_.page()==WarehouseToolPage::PickerVariants?ForceEditorStoragePage::Picker:ForceEditorStoragePage::Warehouse;
        if(view_==ForceEditorView::List)return list_.view().domain==ForceListDomain::Components?ForceEditorStoragePage::Items:list_.view().domain==ForceListDomain::Warehouse?ForceEditorStoragePage::Warehouse:ForceEditorStoragePage::Other;
        return ForceEditorStoragePage::Other;
    }
    EditorListMouseContext list_mouse_context()const{
        EditorListMouseContext c;if(!bound()||!list_mouse_scope_)return c;
        c.session=list_mouse_scope_;c.owner=owner_->instance_binding();c.serial=list_mouse_serial_;c.revision=owner_->revision();
        if(view_==ForceEditorView::WarehouseTools&&tools_.current(*owner_)&&tools_.page()!=WarehouseToolPage::Edit){c.active=true;c.modal=tools_.page()==WarehouseToolPage::Actions;c.waiting=tools_.wait_updates()!=0;c.count=c.modal?2:tools_.page()==WarehouseToolPage::Models?unsigned(tools_.models().size()):tools_.page()==WarehouseToolPage::Instances?unsigned(tools_.instances().size()):tools_.page()==WarehouseToolPage::PickerModels?unsigned(owner_->catalog()->entries().size()):6;c.first=c.modal?0:tools_.cursor()/10*10;c.visible=c.modal?2:10;}
        else if(view_==ForceEditorView::List&&list_.current(*owner_)&&list_.phase()!=ForceEditorListPhase::Waiting){c.active=true;c.modal=list_.phase()!=ForceEditorListPhase::Browse;c.waiting=list_.wait_updates()!=0;const bool parts=list_.view().domain==ForceListDomain::Components;c.count=!c.modal?list_.view().count:list_.phase()==ForceEditorListPhase::Sort?(parts?4:7):list_.phase()==ForceEditorListPhase::Tribe?21:list_.phase()==ForceEditorListPhase::Item?(parts?1:2):2;c.first=!c.modal?list_.first_row():list_.phase()==ForceEditorListPhase::Tribe?list_.menu_first():0;c.visible=c.modal?8:list_.visible_rows();}
        return c;
    }
    bool step_warehouse_tools(u32 repeated,u32 pressed,int adjustment,const EditorListMouseCommand& mouse={}){
        sounds_count_=0;if(!bound()||view_!=ForceEditorView::WarehouseTools)return false;
        s16 pointed=-1;if(!list_mouse_input(mouse,repeated,pressed,&pointed)||!advance_list_mouse_serial())return false;
        if(tools_.page()!=WarehouseToolPage::Actions&&!tools_.wait_updates()&&!(pressed&0x200u)&&(pressed&0x60u))return cycle_storage(pressed&0x40u?-1:1);
        WarehouseToolEvent event;if(!tools_.step(*owner_,repeated,pressed,adjustment,&event,stats_ui_fallback_,pointed))return false;
        if(event.sound)sound(event.sound);
        if(event.specification)return begin_specification(ForceEditorView::WarehouseTools,ForceListDomain::Warehouse,event.slot);
        if(event.legacy_list){if(!(event.slot>=0?list_.open_selected(*owner_,ForceListDomain::Warehouse,names_,grid_.force(),event.slot):list_.open(*owner_,ForceListDomain::Warehouse,names_,grid_.force())))return false;view_=ForceEditorView::List;if(event.sort_menu){ForceEditorListEvents sorting;if(!list_.step(*owner_,0,0x800,&sorting)||list_.phase()!=ForceEditorListPhase::Sort)return false;}return true;}
        if(event.close){if(!grid_.resume(*owner_))return false;view_=ForceEditorView::Grid;}
        return true;
    }
    bool configure_name_keyboard(const ForceNameArt& art,const SourceFont& font){
        if(!bound()||view_!=ForceEditorView::Grid||!art.valid_for(font))return false;
        name_art_=art;name_font_=font;name_configured_=true;return true;
    }
    const ForceNameKeyboard* name_keyboard()const{return name_configured_&&view_==ForceEditorView::Name?&keyboard_:nullptr;}
    bool configure_specification(const ForceSpecificationCatalog& data){
        if(!bound()||view_!=ForceEditorView::Grid||!data.for_catalog(*owner_->catalog()))return false;
        spec_data_=data;spec_configured_=true;preview_spec_only_=stats_ui_fallback_=false;return true;
    }
    // Opt-in rollout: other models/colours retain the provisional detail path,
    // not an invisible actor or falsely admitted constructor.
    bool configure_preview_specification(const ForceSpecificationCatalog& data){if(!configure_specification(data))return false;preview_spec_only_=true;return true;}
    //Original pages for every stored metadata record; qualified two-model actor
    //requests stay on their old path. Other requests are distinctly UI-only.
    bool configure_stored_specification(const ForceSpecificationCatalog& data){if(!configure_preview_specification(data))return false;stats_ui_fallback_=true;return true;}
    std::shared_ptr<const ForceSpecRequest> spec_request()const{return bound()&&view_==ForceEditorView::Spec?spec_request_:nullptr;}
    bool editing_borg()const{return view_==ForceEditorView::BorgEdit;}
    const BorgStatsEditor* borg_editor()const{return bound()&&editing_borg()&&borg_editor_.current(*owner_)?&borg_editor_:nullptr;}
    bool borg_edit_available()const{const auto* b=specification();return bound()&&view_==ForceEditorView::Spec&&tools_configured_&&cheats_.borg_editor&&!edit_scope_&&spec_domain_!=ForceListDomain::Components&&owner_->revision()==spec_revision_&&b&&b->present()&&b->model==spec_model_&&b->acquired==spec_acquired_;}
    bool request_borg_editor(ForceSpecEditRequest* out){if(out)*out={};if(!out||!borg_edit_available()||!spec_request_||!spec_request_->snapshot().current(*owner_))return false;edit_scope_=std::make_shared<const ForceSpecEditScope>();out->request_=spec_request_;out->scope_=edit_scope_;return true;}
    bool cancel_borg_editor_request(const ForceSpecEditRequest& request){if(!edit_scope_||request.scope_.lock()!=edit_scope_||request.request_.lock()!=spec_request_)return false;edit_scope_.reset();return true;}
    bool begin_provisional_borg_editor(){return borg_edit_available()&&!spec_request_&&open_borg_editor();}
    bool step_borg_editor(u32 repeated,u32 pressed,int adjustment){
        sounds_count_=0;if(!bound()||!editing_borg())return false;BorgStatsEditEvent event;if(!borg_editor_.step(*owner_,repeated,pressed,adjustment,&event))return false;
        if(event.changed){if(!grid_.adopt_stats_edit(*owner_,event.prior_revision,spec_domain_==ForceListDomain::Warehouse,unsigned(spec_slot_),spec_model_,spec_acquired_))return false;spec_revision_=owner_->revision();spec_edited_=true;}
        if(event.sound)sound(event.sound);
        if(event.close){borg_editor_.close();return begin_specification(return_,spec_domain_,spec_slot_,true);}return true;
    }
    bool complete_specification(const ForceSpecCompletion& completion){
        if(!bound()||view_!=ForceEditorView::Spec||!spec_request_||completion.request_.lock()!=spec_request_||!spec_request_->snapshot().current(*owner_)||completion.ui_only_!=(spec_request_->kind()==ForceSpecPresentationKind::StatisticsUiOnly))return false;
        if(completion.for_edit_){if(!edit_scope_||completion.edit_scope_.lock()!=edit_scope_||!cheats_.borg_editor||!open_borg_editor())return false;spec_request_.reset();edit_scope_.reset();return true;}
        if(edit_scope_)return false;
        if(!resume())return false;
        spec_request_.reset();return true;
    }
    unsigned force()const{return view_==ForceEditorView::Returned?return_result_.force:view_==ForceEditorView::Selection||view_==ForceEditorView::SelectionExit?selection_.force():grid_.force();}
    s16 held()const{return grid_.held();}bool exit_yes()const{return yes_;}
    const std::string& notice()const{return notice_;}const std::string& name_input()const{return name_;}
    const ForceBorg* specification()const{
        if(!bound()||spec_slot_<0)return nullptr;
        if(spec_domain_==ForceListDomain::Box)return &owner_->state()->box[unsigned(spec_slot_)].value;
        if(spec_domain_==ForceListDomain::Warehouse)return &owner_->state()->warehouse[unsigned(spec_slot_)];
        return nullptr;
    }
    const ForcePart* part_specification()const{return bound()&&spec_domain_==ForceListDomain::Components&&spec_slot_>=0?&owner_->state()->parts[unsigned(spec_slot_)]:nullptr;}
    void name_text(std::string_view ascii){if(name_configured_||view_!=ForceEditorView::Name)return;for(unsigned char c:ascii)if(c>=32&&c<=126&&name_.size()<40)name_+=char(c);}
    void name_backspace(){if(!name_configured_&&view_==ForceEditorView::Name&&!name_.empty())name_.pop_back();}
    bool choose_force(int direction){
        if(!bound()||(direction!=-1&&direction!=1))return false;
        const auto next=unsigned((int(force())+direction+20)%20);
        if(view_==ForceEditorView::Selection){const bool ok=selection_.choose_nonempty(*owner_,direction);if(ok)return_pending_=false;return ok;}
        return view_==ForceEditorView::Grid&&grid_.select_force(*owner_,next);
    }
    bool reenter(){
        if(view_!=ForceEditorView::Selection||!bound())return false;
        unsigned force=0;if(!selection_.begin_edit(*owner_,&force)||!grid_.open(*owner_,force))return false;
        view_=ForceEditorView::Grid;notice_.clear();return_pending_=false;return true;
    }
    bool take_return(ForceEditorReturn* out){
        if(!out)return false;
        *out={};const bool ok=bound()&&return_pending_&&return_revision_==owner_->revision();if(ok)*out=return_result_;return_pending_=false;return ok;
    }
    bool take_battle_roster(ForceBattleRoster* out){
        if(!out)return false;
        *out={};return bound()&&view_==ForceEditorView::Selection&&selection_.take(*owner_,out);
    }
    bool step(ForceEditorHit hit,u32 repeated,u32 pressed,const EditorListMouseCommand& mouse={}){
        sounds_count_=0;if(!bound()||!store_)return false;
        if(view_==ForceEditorView::WarehouseTools)return step_warehouse_tools(repeated,pressed,0,mouse);
        s16 pointed=-1;if(!list_mouse_input(mouse,repeated,pressed,&pointed)||!advance_list_mouse_serial())return false;
        if(view_==ForceEditorView::Grid){ForceEditorEvents e;if(!grid_.step(*owner_,hit,repeated,pressed,&e))return false;for(unsigned i=0;i<e.sound_count;++i)sound(e.sounds[i]);
            if(e.request==ForceEditorRequest::None)return true;
            if(e.request==ForceEditorRequest::Warehouse&&tools_configured_){if(!tools_.open(*owner_,tool_data_,cheats_))return false;view_=ForceEditorView::WarehouseTools;}
            else if(e.request==ForceEditorRequest::Search||e.request==ForceEditorRequest::Warehouse||e.request==ForceEditorRequest::Components){const auto domain=e.request==ForceEditorRequest::Search?ForceListDomain::Box:e.request==ForceEditorRequest::Warehouse?ForceListDomain::Warehouse:ForceListDomain::Components;if(!list_.open(*owner_,domain,names_,grid_.force()))return false;view_=ForceEditorView::List;}
            else if(e.request==ForceEditorRequest::Spec){if(!begin_specification(ForceEditorView::Grid,ForceListDomain::Box,e.spec_box))return false;}
            else if(e.request==ForceEditorRequest::Name){
                if(name_configured_){const auto& bytes=owner_->state()->forces[grid_.force()].name;const auto end=std::find(bytes.begin(),bytes.end(),u8(0));
                    if(end==bytes.end()||!keyboard_.open(name_art_,name_font_,{reinterpret_cast<const char*>(bytes.data()),std::size_t(end-bytes.begin())}))return false;
                    name_revision_=owner_->revision();
                }
                view_=ForceEditorView::Name;name_.clear();
            }
            else if(e.request==ForceEditorRequest::Exit){return_=ForceEditorView::Grid;return finish();}
        }else if(view_==ForceEditorView::WarehouseTools){return step_warehouse_tools(repeated,pressed,0);
        }else if(view_==ForceEditorView::List){
            if(tools_configured_&&list_.phase()==ForceEditorListPhase::Browse&&list_.view().domain!=ForceListDomain::Box&&!(pressed&0x80u)&&(pressed&0x60u))return cycle_storage(pressed&0x40u?-1:1);
            ForceEditorListEvents e;if(!list_.step(*owner_,repeated,pressed,&e,pointed))return false;for(unsigned i=0;i<e.sound_count;++i)sound(e.sounds[i]);
            if(e.request==ForceEditorListRequest::Grid){if(!grid_.resume(*owner_))return false;view_=ForceEditorView::Grid;}
            else if(e.request==ForceEditorListRequest::Warehouse&&tools_configured_){if(!tools_.open(*owner_,tool_data_,cheats_))return false;view_=ForceEditorView::WarehouseTools;}
            else if(e.request==ForceEditorListRequest::Warehouse||e.request==ForceEditorListRequest::Components){if(!list_.open(*owner_,e.request==ForceEditorListRequest::Warehouse?ForceListDomain::Warehouse:ForceListDomain::Components,names_,grid_.force()))return false;}
            else if(e.request==ForceEditorListRequest::Picked){if(!grid_.accept_search_pick(*owner_,unsigned(e.slot)))return false;view_=ForceEditorView::Grid;}
            else if(e.request==ForceEditorListRequest::Spec){if(!begin_specification(ForceEditorView::List,e.domain,e.slot))return false;}
            else if(e.request==ForceEditorListRequest::Exit){return_=ForceEditorView::List;return finish();}
        }else if(view_==ForceEditorView::BorgEdit){return step_borg_editor(repeated,pressed,0);
        }else if(view_==ForceEditorView::Spec){
            if(spec_request_)return spec_request_->snapshot().current(*owner_); // Presentation owns page/fade/input callbacks.
            if(pressed&0x300){if(!resume())return false;sound(123);}
        }else if(view_==ForceEditorView::Name){
            if(name_configured_){
                if(owner_->revision()!=name_revision_)return false;
                ForceNameEvents event;if(!keyboard_.step(repeated,pressed,&event))return false;
                if(event.committed){if(owner_->rename_force(grid_.force(),keyboard_.text())!=ForceChange::Applied)return false;name_revision_=owner_->revision();}
                if(event.sound)sound(event.sound);
                if(keyboard_.phase()==ForceNamePhase::Done){if(!grid_.resume(*owner_))return false;view_=ForceEditorView::Grid;}
                return true;
            }
            if(pressed&0x200){if(!grid_.resume(*owner_))return false;view_=ForceEditorView::Grid;sound(123);}
            else if(pressed&0x100){if(owner_->rename_force(grid_.force(),name_)!=ForceChange::Applied||!grid_.resume(*owner_))return false;view_=ForceEditorView::Grid;sound(122);}
        }else if(view_==ForceEditorView::ExitBlocked){
            if(notice_wait_){--notice_wait_;return true;}
            if(pressed&0x300)return resume();
        }else if(view_==ForceEditorView::SaveError){
            if(owner_->revision()!=exit_revision_)return false;
            if(pressed&0x200)return resume();
            if(pressed&0x100)return save_and_return();
        }else if(view_==ForceEditorView::SelectionExit){
            if(repeated&0xf00f){yes_=!yes_;sound(119);}
            if(pressed&0x200){view_=ForceEditorView::Selection;sound(123);}
            else if(pressed&0x100){if(!yes_)view_=ForceEditorView::Selection;else{publish_return(ForceEditorDestination::MainMenu,selection_.force(),false);view_=ForceEditorView::Returned;}sound(122);}
        }else if(view_==ForceEditorView::Selection){
            if(pressed&0x200){if(!selection_.choose(*owner_,selection_.force()))return false;yes_=false;view_=ForceEditorView::SelectionExit;return_pending_=false;}
            else if(pressed&0x400)return reenter(); // Retail SELECT A FORCE: X, not B/Y.
            else if(pressed&0x100){ForceBattleRoster roster;if(selection_.confirm(*owner_)&&selection_.preview(*owner_,&roster))notice_="ROSTER READY: "+std::to_string(roster.members().size())+" BORGS / COST "+std::to_string(roster.cost());else notice_="THIS FORCE IS EMPTY";sound(122);}
        }
        return true;
    }
    std::span<const u16> sounds()const{return {sounds_.data(),sounds_count_};}
private:
    bool advance_list_mouse_serial(){if(list_mouse_serial_==~u64(0))return false;++list_mouse_serial_;return true;}
    bool list_mouse_input(const EditorListMouseCommand& command,u32& repeated,u32& pressed,s16* row)const{
        *row=-1;if(!command.context.active)return command.target.row==-1&&!command.target.pressed&&!command.target.repeated;
        const auto c=list_mouse_context();if(!command.context.same(c)||!editor_list_mouse_target_valid(command.target))return false;
        if(c.waiting)return true;
        const auto& t=command.target;if(t.row>=0&&(unsigned(t.row)<c.first||unsigned(t.row)>=std::min(c.first+c.visible,c.count)))return false;
        //Page ownership, not the target, decides whether an ordinal identifies
        //stored inventory or a virtual Picker entry. Old controllers mutate.
        if(c.modal&&(t.pressed&~0x300u||t.repeated&~0xcu))return false;
        *row=t.row;repeated|=t.repeated;pressed|=t.pressed;return true;
    }
    //Owner-requested storage ring: Editor→Warehouse→optional Picker→Items→Editor.
    //Grid already entered its Waiting state before opening a storage page. Keep
    //that same grid/held-instance owner until resume, not a fresh grid open.
    bool cycle_storage(int direction){
        if(!tools_configured_||(direction!=-1&&direction!=1))return false;
        if((view_==ForceEditorView::WarehouseTools&&!tools_.current(*owner_))||(view_==ForceEditorView::List&&!list_.current(*owner_)))return false;
        const std::array<ForceEditorStoragePage,4> pages={ForceEditorStoragePage::Editor,ForceEditorStoragePage::Warehouse,cheats_.warehouse_selector?ForceEditorStoragePage::Picker:ForceEditorStoragePage::Items,ForceEditorStoragePage::Items};
        const unsigned count=cheats_.warehouse_selector?4:3;const auto page=storage_page();unsigned current=count;for(unsigned i=0;i<count;++i)if(pages[i]==page)current=i;
        if(current==count)return false;
        const auto next=pages[unsigned((int(current)+direction+int(count))%int(count))];
        if(next==ForceEditorStoragePage::Editor){if(!grid_.resume(*owner_))return false;view_=ForceEditorView::Grid;}
        else if(next==ForceEditorStoragePage::Items){if(!list_.open(*owner_,ForceListDomain::Components,names_,grid_.force()))return false;view_=ForceEditorView::List;}
        else{const bool opened=next==ForceEditorStoragePage::Picker?tools_.open_picker(*owner_,tool_data_,cheats_):tools_.open(*owner_,tool_data_,cheats_);if(!opened)return false;view_=ForceEditorView::WarehouseTools;}
        sound(122);return true;
    }
    bool begin_specification(ForceEditorView prior,ForceListDomain domain,s16 slot,bool refresh=false){
        spec_request_.reset();
        edit_scope_.reset();if(!refresh)spec_edited_=false;
        if(domain!=ForceListDomain::Components){const auto* b=domain==ForceListDomain::Box?(slot>=0&&unsigned(slot)<kBoxBorgs?&owner_->state()->box[unsigned(slot)].value:nullptr):(slot>=0&&unsigned(slot)<kWarehouseBorgs?&owner_->state()->warehouse[unsigned(slot)]:nullptr);if(!b||!b->present())return false;spec_revision_=owner_->revision();spec_model_=b->model;spec_acquired_=b->acquired;}
        if(spec_configured_&&domain!=ForceListDomain::Components){
            auto request=std::make_shared<ForceSpecRequest>();
            if(slot<0||!request->snapshot_.bind(*owner_,spec_data_,domain==ForceListDomain::Box?ForceSpecDomain::Box:ForceSpecDomain::Warehouse,unsigned(slot)))return false;
            const auto* value=request->snapshot_.value(*owner_);
            const bool supported=!value->variant&&(value->model==0x615||value->model==10);
            if(stats_ui_fallback_&&!supported)request->kind_=ForceSpecPresentationKind::StatisticsUiOnly;
            if(!preview_spec_only_||supported||stats_ui_fallback_)spec_request_=std::move(request);
        }
        return_=prior;spec_domain_=domain;spec_slot_=slot;view_=ForceEditorView::Spec;return true;
    }
    bool bound()const{return owner_&&owner_->ready()&&scope_.lock()==owner_->instance_binding().lock();}
    bool open_borg_editor(){if(!bound()||view_!=ForceEditorView::Spec||spec_domain_==ForceListDomain::Components||!tools_configured_||!cheats_.borg_editor||owner_->revision()!=spec_revision_)return false;const auto* b=specification();if(!b||b->model!=spec_model_||b->acquired!=spec_acquired_||!borg_editor_.open(*owner_,tool_data_,cheats_,spec_domain_==ForceListDomain::Box?ForceSpecDomain::Box:ForceSpecDomain::Warehouse,unsigned(spec_slot_)))return false;view_=ForceEditorView::BorgEdit;return true;}
    bool open_impl(ForceInventory& owner,const ForceSupplement& names,ForceInventoryStore& store,unsigned force,ForceEditorEntry entry,s32 budget,ForceCostMode mode){
        *this={};if(!owner.ready()||!names.for_catalog(*owner.catalog())||!grid_.open(owner,force))return false;
        if(entry==ForceEditorEntry::Selection){unsigned selected=0;if(!selection_.open(owner,force,budget,mode)||!selection_.begin_edit(owner,&selected))return false;}
        owner_=&owner;scope_=owner.instance_binding();list_mouse_scope_=std::make_shared<const EditorListMouseScope>();names_=names;store_=&store;entry_=entry;energy_limit_=budget;cost_mode_=mode;view_=ForceEditorView::Grid;return true;
    }
    void publish_return(ForceEditorDestination destination,unsigned force,bool saved){return_result_={destination,force,saved};return_revision_=owner_->revision();return_pending_=true;}
    bool save_and_return(){
        if(!store_->write(*owner_)){view_=ForceEditorView::SaveError;notice_="NATIVE SAVE FAILED - A RETRY / B RETURN";return true;}
        publish_return(ForceEditorDestination::MainMenu,grid_.force(),true);view_=ForceEditorView::Returned;notice_="NATIVE INVENTORY SAVED";return true;
    }
    bool finish(){
        //801BFBF4/801BE8C8: do not normalize/save/quit if every force is empty.
        if(!owner_->state()->force_count){view_=ForceEditorView::ExitBlocked;notice_wait_=10;notice_="AT LEAST ONE FORCE MUST CONTAIN A BORG";return true;}
        if(owner_->finish_edit()!=ForceChange::Applied)return false;
        exit_revision_=owner_->revision();
        if(entry_==ForceEditorEntry::Standalone)return save_and_return();
        if(!selection_.return_from_edit_nonempty(*owner_,grid_.force()))return false;
        view_=ForceEditorView::Selection;notice_.clear();publish_return(ForceEditorDestination::Selection,selection_.force(),false);return true;
    }
    void sound(u16 id){if(sounds_count_<sounds_.size())sounds_[sounds_count_++]=id;}
    bool resume(){if(return_==ForceEditorView::Grid){if(!grid_.resume(*owner_))return false;}else if(return_==ForceEditorView::WarehouseTools){if(!tools_.resume_stats_selection(*owner_,spec_slot_))return false;}else if(view_==ForceEditorView::Spec&&spec_edited_){if(!list_.resume_stats_selection(*owner_,spec_slot_))return false;}else if(!list_.resume(*owner_))return false;view_=return_;notice_.clear();return true;}
    ForceInventory* owner_=nullptr;ForceInventoryStore* store_=nullptr;ForceSupplement names_;
    std::weak_ptr<const ForceInventoryScope> scope_;ForceEditorEntry entry_=ForceEditorEntry::Standalone;
    ForceEditorReturn return_result_;bool return_pending_=false;u64 return_revision_=0,exit_revision_=0;unsigned notice_wait_=0;
    s32 energy_limit_=-1;
    ForceCostMode cost_mode_=ForceCostMode::Full;
    ForceEditorGrid grid_;ForceEditorList list_;ForceSelection selection_;
    ForceNameArt name_art_;SourceFont name_font_;ForceNameKeyboard keyboard_;bool name_configured_=false;u64 name_revision_=0;
    ForceSpecificationCatalog spec_data_;bool spec_configured_=false,preview_spec_only_=false,stats_ui_fallback_=false;std::shared_ptr<const ForceSpecRequest> spec_request_;
    BorgStatsEditor borg_editor_;std::shared_ptr<const ForceSpecEditScope> edit_scope_;u64 spec_revision_=0;u32 spec_acquired_=0;s16 spec_model_=-1;bool spec_edited_=false;
    WarehouseTools tools_;CheatSettings cheats_;ForceSpecificationCatalog tool_data_;bool tools_configured_=false;
    ForceEditorView view_=ForceEditorView::Grid,return_=ForceEditorView::Grid;bool yes_=false;
    ForceListDomain spec_domain_=ForceListDomain::Box;s16 spec_slot_=-1;std::string name_,notice_;
    std::array<u16,2> sounds_{};unsigned sounds_count_=0;
    std::shared_ptr<const EditorListMouseScope> list_mouse_scope_;u64 list_mouse_serial_=0;
};
} // namespace gf
