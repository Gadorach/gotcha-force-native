#pragma once
#include "engine/gameplay/force_search.hpp"
namespace gf {
enum class ForceEditorListPhase {Browse,Sort,Tribe,Item,Discard,Waiting};
enum class ForceEditorListRequest {None,Grid,Warehouse,Components,Exit,Spec,Picked};
struct ForceEditorListEvents {
    ForceEditorListRequest request=ForceEditorListRequest::None;
    ForceListDomain domain=ForceListDomain::Box;s16 slot=-1;
    std::array<u16,2> sounds{};unsigned sound_count=0;
    void sound(u16 cue){if(sound_count<sounds.size())sounds[sound_count++]=cue;}
};
// Shared source list navigation/menu operations: BB/BC/BD/BE handlers. Screen
// transitions and rendering consume the explicit requests, never raw storage.
class ForceEditorList {
public:
    bool open(const ForceInventory& owner,ForceListDomain domain,const ForceSupplement& names,unsigned force=0,
              ForceSortSpec sort={{{ForceSortKey::Acquired,false},{ForceSortKey::Number,false},{ForceSortKey::Name,false},{ForceSortKey::Acquired,false}}}){
        *this={};if(!owner.ready()||unsigned(domain)>2||force>=kForceCount||!names.for_catalog(*owner.catalog()))return false;
        scope_=owner.instance_binding();domain_=domain;force_=force;names_=names;sort_=sort;display_=domain==ForceListDomain::Components?2:1;
        return rebuild(owner);
    }
    bool current(const ForceInventory& owner)const{return view_.current(owner);}
    bool open_selected(const ForceInventory& owner,ForceListDomain domain,const ForceSupplement& names,unsigned force,s16 slot){
        if(slot<0||!open(owner,domain,names,force))return false;
        const auto found=std::find(view_.slots.begin(),view_.slots.end(),slot);if(found==view_.slots.end())return false;cursor_=unsigned(found-view_.slots.begin());first_=cursor_>=visible_rows()?cursor_+1-visible_rows():0;return true;
    }
    const ForceSearchList& view()const{return view_;}unsigned cursor()const{return cursor_;}unsigned first_row()const{return first_;}
    unsigned visible_rows()const{return domain_==ForceListDomain::Components?10:13;}
    unsigned menu_row()const{return row_;}unsigned menu_first()const{return menu_first_;}unsigned wait_updates()const{return wait_;}
    ForceEditorListPhase phase()const{return phase_;}u8 display_flags()const{return display_;}
    const ForceSortSpec& sort()const{return sort_;}u32 tribes()const{return tribes_;}
    s16 selected()const{return cursor_<view_.count?view_.slots[cursor_]:s16(-1);}
    bool resume(const ForceInventory& owner){if(phase_!=ForceEditorListPhase::Waiting||!same_owner(owner))return false;phase_=ForceEditorListPhase::Browse;return rebuild(owner);}
    bool step(ForceInventory& owner,u32 repeated,u32 pressed,ForceEditorListEvents* out,s16 pointed_row=-1){
        if(!out)return false;
        *out={};out->domain=domain_;if(!current(owner)||phase_==ForceEditorListPhase::Waiting)return false;
        if(wait_){--wait_;return true;}
        if(phase_!=ForceEditorListPhase::Browse)return menu(owner,repeated,pressed,*out,pointed_row);
        if(pointed_row< -1||(pointed_row>=0&&(unsigned(pointed_row)<first_||unsigned(pointed_row)>=std::min(first_+visible_rows(),view_.count))))return false;
        if(pointed_row>=0&&cursor_!=unsigned(pointed_row)){cursor_=unsigned(pointed_row);out->sound(119);}
        navigate(repeated,*out);const s16 item=selected();
        if(domain_==ForceListDomain::Box){
            if(pressed&0x200){request(ForceEditorListRequest::Grid,*out,123);return true;}
        }else{
            if(pressed&0x80){request(ForceEditorListRequest::Exit,*out);return true;}
            if(pressed&0x40){request(domain_==ForceListDomain::Warehouse?ForceEditorListRequest::Grid:ForceEditorListRequest::Warehouse,*out);return true;}
            if(pressed&0x20){request(domain_==ForceListDomain::Warehouse?ForceEditorListRequest::Components:ForceEditorListRequest::Grid,*out);return true;}
        }
        if(pressed&0x400){if(item<0)out->sound(125);else{out->slot=item;request(ForceEditorListRequest::Spec,*out);}return true;}
        if(pressed&0x800){phase_=ForceEditorListPhase::Sort;row_=0;wait_=10;out->sound(122);return true;}
        if(pressed&0x10){display_^=domain_==ForceListDomain::Components?2:1;out->sound(122);return true;}
        if(!(pressed&0x100)||item<0)return true;
        if(domain_==ForceListDomain::Box&&(owner.state()->box[unsigned(item)].memberships&(u32(1)<<force_)))return true;
        const auto changed=owner.mark_seen(unsigned(domain_),unsigned(item));
        if(changed!=ForceChange::Applied&&changed!=ForceChange::Unchanged)return false;
        view_.revision=owner.revision(); // Only the new-marker changed; retain permutation.
        if(domain_==ForceListDomain::Box){out->slot=item;request(ForceEditorListRequest::Picked,*out);}
        else{phase_=ForceEditorListPhase::Item;row_=0;wait_=10;out->sound(122);}
        return true;
    }
private:
    bool resume_stats_selection(const ForceInventory& owner,s16 slot){if(!resume(owner))return false;const auto found=std::find(view_.slots.begin(),view_.slots.end(),slot);if(found==view_.slots.end())return false;cursor_=unsigned(found-view_.slots.begin());first_=std::min(first_,cursor_);if(cursor_>=first_+visible_rows())first_=cursor_+1-visible_rows();return true;}
    friend class ForceEditorSession;
    bool same_owner(const ForceInventory& owner)const{return owner.ready()&&scope_.lock()==owner.instance_binding().lock();}
    bool rebuild(const ForceInventory& owner){
        if(!same_owner(owner)||!force_search(owner,domain_,tribes_,sort_,&names_,&view_))return false;
        if(!view_.count){cursor_=first_=0;return true;}
        cursor_=std::min(cursor_,view_.count-1);first_=std::min(first_,view_.count>visible_rows()?view_.count-visible_rows():0);
        if(cursor_<first_)first_=cursor_;else if(cursor_>=first_+visible_rows())first_=cursor_+1-visible_rows();
        return true;
    }
    void navigate(u32 bits,ForceEditorListEvents& events){
        const auto old_cursor=cursor_,old_first=first_;const unsigned page=visible_rows();
        if(!view_.count){cursor_=first_=0;return;}
        if(bits&0x8008)cursor_=cursor_?cursor_-1:view_.count-1;
        else if(bits&0x4004)cursor_=(cursor_+1)%view_.count;
        else if(bits&0x1001){if(!cursor_&&!first_){cursor_=view_.count-1;first_=cursor_>=page?cursor_+1-page:0;}else{cursor_=cursor_>page?cursor_-page:0;first_=first_>page?first_-page:0;}}
        else if(bits&0x2002){const auto last_first=view_.count>page?view_.count-page:0;if(cursor_<view_.count-1||first_<last_first){cursor_=std::min(cursor_+page,view_.count-1);first_=std::min(first_+page,last_first);}else cursor_=first_=0;}
        if(cursor_<first_)first_=cursor_;else if(cursor_>=first_+page)first_=cursor_+1-page;
        if(cursor_!=old_cursor||first_!=old_first)events.sound(119);
    }
    void request(ForceEditorListRequest value,ForceEditorListEvents& events,u16 sound=122){events.request=value;events.sound(sound);phase_=ForceEditorListPhase::Waiting;}
    void close(){phase_=ForceEditorListPhase::Browse;wait_=10;}
    bool menu(ForceInventory& owner,u32 repeated,u32 pressed,ForceEditorListEvents& events,s16 pointed_row){
        const bool parts=domain_==ForceListDomain::Components;
        const unsigned size=phase_==ForceEditorListPhase::Sort?(parts?4:7):phase_==ForceEditorListPhase::Tribe?21:phase_==ForceEditorListPhase::Item?(parts?1:2):2;
        if(pointed_row< -1||(pointed_row>=0&&(unsigned(pointed_row)>=size||(phase_==ForceEditorListPhase::Tribe&&(unsigned(pointed_row)<menu_first_||unsigned(pointed_row)>=menu_first_+8)))))return false;
        const auto old=row_;if(pointed_row>=0)row_=unsigned(pointed_row);if(repeated&0x8008)row_=(row_+size-1)%size;else if(repeated&0x4004)row_=(row_+1)%size;
        if(phase_==ForceEditorListPhase::Tribe){if(row_<menu_first_)menu_first_=row_;else if(row_>=menu_first_+8)menu_first_=row_-7;}
        if(old!=row_)events.sound(119);
        const bool back=(pressed&0x200)||((pressed&0x800)&&phase_==ForceEditorListPhase::Sort);
        if(back){
            if(phase_==ForceEditorListPhase::Discard){phase_=ForceEditorListPhase::Item;row_=parts?0:1;wait_=10;}
            else if(phase_==ForceEditorListPhase::Tribe){phase_=ForceEditorListPhase::Sort;row_=parts?3:5;wait_=10;}
            else close();
            events.sound(123);return true;
        }
        if(!(pressed&0x100))return true;
        if(phase_==ForceEditorListPhase::Sort){
            if(row_==size-1){phase_=ForceEditorListPhase::Tribe;row_=menu_first_=0;wait_=10;}
            else{
                // Source BE key/direction pairs:80383750/8038378C/80435A44.
                static constexpr std::array<ForceSortTerm,6> borg={{{ForceSortKey::LevelExperience,true},{ForceSortKey::Acquired,false},{ForceSortKey::Rarity,true},{ForceSortKey::Cost,true},{ForceSortKey::Number,false},{ForceSortKey::Name,false}}};
                static constexpr std::array<ForceSortTerm,3> part={{{ForceSortKey::Acquired,false},{ForceSortKey::Rarity,true},{ForceSortKey::Number,false}}};
                const auto term=parts?part[row_]:borg[row_];
                if(sort_[0].key==term.key)sort_[0].descending=!sort_[0].descending;
                else{for(unsigned i=3;i>0;--i)sort_[i]=sort_[i-1];sort_[0]=term;}
                if(!force_resort(owner,sort_,&names_,&view_))return false;
                close();
            }
        }else if(phase_==ForceEditorListPhase::Tribe){tribes_=row_?u32(1)<<(row_-1):~u32(0);cursor_=first_=0;if(!rebuild(owner))return false;close();}
        else if(phase_==ForceEditorListPhase::Item){
            const auto item=selected();if(item<0)return false;
            if(!parts&&row_==0){if(owner.state()->box_count==kBoxBorgs){events.sound(125);return true;}if(owner.from_warehouse(unsigned(item))!=ForceChange::Applied||!rebuild(owner))return false;close();}
            else{
                if(!parts){const auto model=owner.state()->warehouse[unsigned(item)].model;if(model==0x615||model==0x629){events.sound(125);return true;}}
                phase_=ForceEditorListPhase::Discard;row_=1;wait_=10;
            }
        }else{
            if(row_==0){const auto item=selected();if(item<0)return false;const auto change=parts?owner.erase_part(unsigned(item)):owner.erase_warehouse(unsigned(item));if(change!=ForceChange::Applied||!rebuild(owner))return false;}
            close();
        }
        events.sound(122);return true;
    }
    std::weak_ptr<const ForceInventoryScope> scope_;ForceSupplement names_;ForceSearchList view_;
    ForceListDomain domain_=ForceListDomain::Box;ForceSortSpec sort_{};u32 tribes_=~u32(0);
    unsigned force_=0,cursor_=0,first_=0,row_=0,menu_first_=0,wait_=0;u8 display_=1;
    ForceEditorListPhase phase_=ForceEditorListPhase::Browse;
};
} // namespace gf
