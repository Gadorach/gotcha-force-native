#pragma once
// Source 801BACD8/BAF6C/BB2D0/BB69C: grid pick/drop and operation menu.
// Hit observations come from the presentation's BF144-style cursor/wheel path;
// this controller does not invent a flat-list replacement for that geometry.
#include "engine/gameplay/force_inventory.hpp"
namespace gf {
struct ForceEditorHit {s16 box=-1;s8 member=-1;};
enum class ForceEditorRequest {None,Exit,Warehouse,Components,Search,Name,Spec};
struct ForceEditorEvents {
    ForceEditorRequest request=ForceEditorRequest::None;s16 spec_box=-1;
    std::array<u16,2> sounds{};unsigned sound_count=0;s8 wheel_direction=0;
    void sound(u16 cue){if(sound_count<sounds.size())sounds[sound_count++]=cue;}
};
enum class ForceEditorGridPhase {Grid,Operations,Discard,Waiting};
class ForceEditorGrid {
public:
    bool open(const ForceInventory& owner,unsigned force){
        *this={};if(!owner.ready()||force>=kForceCount)return false;
        scope_=owner.instance_binding();revision_=owner.revision();force_=force;ready_=true;return true;
    }
    bool current(const ForceInventory& owner)const{return ready_&&owner.ready()&&scope_.lock()==owner.instance_binding().lock()&&revision_==owner.revision();}
    unsigned force()const{return force_;}s16 held()const{return held_;}
    unsigned menu_row()const{return row_;}unsigned wait_updates()const{return wait_;}
    ForceEditorGridPhase phase()const{return phase_;}
    bool select_force(const ForceInventory& owner,unsigned force){
        if(!current(owner)||force>=kForceCount||held_>=0||phase_!=ForceEditorGridPhase::Grid)return false;
        force_=force;return true;
    }
    // Returning subviews may have edited this SAME inventory. A held reference
    // cannot survive unrelated mutations: explicit reopen clears that carry.
    bool resume(const ForceInventory& owner){
        if(!ready_||!owner.ready()||scope_.lock()!=owner.instance_binding().lock()||phase_!=ForceEditorGridPhase::Waiting)return false;
        if(held_>=0&&revision_!=owner.revision())return false;
        revision_=owner.revision();phase_=ForceEditorGridPhase::Grid;pending_=ForceEditorRequest::None;return true;
    }
    bool accept_search_pick(const ForceInventory& owner,unsigned box){
        if(!ready_||!owner.ready()||scope_.lock()!=owner.instance_binding().lock()||phase_!=ForceEditorGridPhase::Waiting||pending_!=ForceEditorRequest::Search||held_>=0||box>=kBoxBorgs)return false;
        const auto& item=owner.state()->box[box];if(!item.value.present()||(item.memberships&(u32(1)<<force_)))return false;
        revision_=owner.revision();held_=s16(box);phase_=ForceEditorGridPhase::Grid;pending_=ForceEditorRequest::None;return true;
    }
    u32 disabled_operations(const ForceInventory& owner)const{
        if(!current(owner))return 0x3f;
        const auto& s=*owner.state();u32 disabled=held_>=0?0xf:0;
        if(held_<0){if(!s.forces[force_].count)disabled|=0xc;if(s.force_count==kForceCount)disabled|=4;}
        const s16 item=held_>=0?held_:selected(owner);
        if(item<0)return disabled|0x30;
        if(s.warehouse_count==kWarehouseBorgs)disabled|=0x10;
        const auto model=s.box[unsigned(item)].value.model;if(model==0x615||model==0x629)disabled|=0x20;
        return disabled;
    }
    // Edges/repeats are already resolved for the controlling port. Original pad
    // bits are retained (A100/B200/X400/Y800/Start80/L40/R20). Navigation precedes
    // action in menus; grid geometry/navigation is supplied before this call.
    bool step(ForceInventory& owner,ForceEditorHit hit,u32 repeated,u32 pressed,ForceEditorEvents* out){
        if(!out)return false;
        *out={};if(!current(owner)||phase_==ForceEditorGridPhase::Waiting)return false;
        if(wait_){--wait_;return true;}
        if(phase_!=ForceEditorGridPhase::Grid)return menu_step(owner,repeated,pressed,*out);
        if(hit.box<-1||hit.box>=s16(kBoxBorgs)||hit.member<-1||hit.member>=s8(kForceSlots)||(hit.member>=0&&hit.box>=0))return false;
        hit_=hit;
        if(held_>=0){
            if(pressed&0x200){held_=-1;out->sound(123);return true;}
            if(pressed&0x400){request(ForceEditorRequest::Spec,*out,held_);return true;}
            if(pressed&0x800){open_operations();out->sound(122);return true;}
            if(pressed&0x100)return drop(owner,*out);
            return true;
        }
        if(pressed&0x80)request(ForceEditorRequest::Exit,*out);
        else if(pressed&0x40)request(ForceEditorRequest::Components,*out);
        else if(pressed&0x20)request(ForceEditorRequest::Warehouse,*out);
        else if(pressed&0x400){const auto item=selected(owner);if(item<0)out->sound(125);else request(ForceEditorRequest::Spec,*out,item);}
        else if(pressed&0x800){open_operations();out->sound(122);}
        else if(pressed&0x100){
            const auto item=selected(owner);if(item<0)return true;
            if(hit_.member<0&&(owner.state()->box[unsigned(item)].memberships&(u32(1)<<force_)))return true;
            if(hit_.member>=0&&!mutated(owner,owner.remove(force_,unsigned(hit_.member))))return false;
            held_=item;if(!seen(owner,item))return false;out->sound(122);
        }
        return true;
    }
private:
    //Not a public revision bypass: session calls only immediately after its
    //bound stats editor's successful single-slot mutation. Generic resume keeps
    //rejecting unrelated changes while holding a Borg.
    bool adopt_stats_edit(const ForceInventory& owner,u64 prior,bool warehouse,unsigned slot,s16 model,u32 acquired){
        if(!ready_||phase_!=ForceEditorGridPhase::Waiting||scope_.lock()!=owner.instance_binding().lock()||prior==~u64(0)||owner.revision()!=prior+1||revision_>prior)return false;
        if(held_>=0&&(revision_!=prior||warehouse||slot!=unsigned(held_)))return false;
        const auto* b=warehouse?(slot<kWarehouseBorgs?&owner.state()->warehouse[slot]:nullptr):(slot<kBoxBorgs?&owner.state()->box[slot].value:nullptr);if(!b||b->model!=model||b->acquired!=acquired)return false;revision_=owner.revision();return true;
    }
    friend class ForceEditorSession;
    s16 selected(const ForceInventory& owner)const{
        if(hit_.member>=0)return owner.state()->forces[force_].slots[unsigned(hit_.member)];
        return hit_.box>=0&&owner.state()->box[unsigned(hit_.box)].value.present()?hit_.box:s16(-1);
    }
    bool mutated(const ForceInventory& owner,ForceChange result){
        if(result!=ForceChange::Applied&&result!=ForceChange::Unchanged){ready_=false;return false;}
        revision_=owner.revision();return true;
    }
    bool seen(ForceInventory& owner,s16 item){return mutated(owner,owner.mark_seen(0,unsigned(item)));}
    void request(ForceEditorRequest value,ForceEditorEvents& events,s16 spec=-1){events.request=value;events.spec_box=spec;events.sound(122);phase_=ForceEditorGridPhase::Waiting;pending_=value;}
    void open_operations(){phase_=ForceEditorGridPhase::Operations;row_=0;wait_=10;}
    void close_operations(){phase_=ForceEditorGridPhase::Grid;wait_=10;}
    bool drop(ForceInventory& owner,ForceEditorEvents& events){
        if(hit_.member<0){
            if(hit_.box<0)return true;
            if(hit_.box==held_)held_=-1;
            else{
                if(!mutated(owner,owner.swap_box(unsigned(held_),unsigned(hit_.box))))return false;
                const auto& b=owner.state()->box[unsigned(held_)];if(!b.value.present()||(b.memberships&(u32(1)<<force_)))held_=-1;
            }
            events.sound(122);
        }else if(!(owner.state()->box[unsigned(held_)].memberships&(u32(1)<<force_))){
            const auto old=selected(owner);
            if(old<0){if(!mutated(owner,owner.place(force_,unsigned(hit_.member),unsigned(held_))))return false;held_=-1;}
            else{if(!mutated(owner,owner.replace(force_,unsigned(hit_.member),unsigned(held_))))return false;held_=old;}
            events.sound(122);
        }
        return held_<0||seen(owner,held_);
    }
    bool menu_step(ForceInventory& owner,u32 repeated,u32 pressed,ForceEditorEvents& events){
        const unsigned size=phase_==ForceEditorGridPhase::Discard?2:6;const auto old=row_;
        if(repeated&0x8008)row_=(row_+size-1)%size;
        else if(repeated&0x4004)row_=(row_+1)%size;
        if(row_!=old)events.sound(119);
        if(phase_==ForceEditorGridPhase::Discard){
            if(pressed&0x200){phase_=ForceEditorGridPhase::Operations;row_=5;wait_=10;events.sound(123);return true;}
            if(!(pressed&0x100))return true;
            if(row_==0){const auto item=held_>=0?held_:selected(owner);if(item<0||!mutated(owner,owner.erase_box(unsigned(item))))return false;held_=-1;}
            close_operations();events.sound(122);return true;
        }
        if(pressed&0xa00){close_operations();events.sound(123);return true;}
        if(!(pressed&0x100))return true;
        if(disabled_operations(owner)&(u32(1)<<row_)){events.sound(125);return true;}
        if(row_==0){request(ForceEditorRequest::Search,events);return true;}
        if(row_==1){request(ForceEditorRequest::Name,events);return true;}
        if(row_==2){s16 destination=-1;if(!mutated(owner,owner.copy_force(force_,&destination)))return false;const unsigned delta=(unsigned(destination)+kForceCount-force_)%kForceCount;events.wheel_direction=delta<11?-1:1;force_=unsigned(destination);hit_={};}
        else if(row_==3){if(!mutated(owner,owner.delete_force(force_)))return false;}
        else if(row_==4){const auto item=held_>=0?held_:selected(owner);if(!seen(owner,item)||!mutated(owner,owner.to_warehouse(unsigned(item))))return false;held_=-1;}
        else if(row_==5){if(held_<0&&!seen(owner,selected(owner)))return false;phase_=ForceEditorGridPhase::Discard;row_=1;wait_=10;events.sound(122);return true;}
        close_operations();events.sound(122);return true;
    }
    std::weak_ptr<const ForceInventoryScope> scope_;u64 revision_=0;bool ready_=false;
    unsigned force_=0,row_=0,wait_=0;s16 held_=-1;ForceEditorHit hit_;
    ForceEditorGridPhase phase_=ForceEditorGridPhase::Grid;
    ForceEditorRequest pending_=ForceEditorRequest::None;
};
} // namespace gf
