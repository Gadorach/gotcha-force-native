#pragma once
// Shared native box/warehouse/force owner. Original inventory kernels live at
// 801CA948..801CCE38. Slot order and per-instance references are preserved;
// rendering, card I/O, controller input and battle simulation are not owners.
#include <array>
#include <bit>
#include <memory>
#include <span>
#include <string_view>
#include "engine/assets/force_catalog.hpp"
#include "engine/gameplay/borg_edits.hpp"
#include "engine/gameplay/cheat_settings.hpp"
#include "engine/gameplay/force_cost.hpp"
namespace gf {
inline constexpr unsigned kForceCount=20,kForceSlots=30,kBoxBorgs=200;
inline constexpr unsigned kWarehouseBorgs=2000,kWarehouseParts=1000;
struct ForceBorg {
    s16 model=-1;u8 variant=0,level=0;u32 acquired=0,experience=0;
    u8 fresh=0,reserved13=0;u16 reserved14=0;
    BorgEdits edits;
    bool operator==(const ForceBorg&)const=default;
    bool present()const{return model>=0;}
};
inline s32 force_borg_cost(const ForceCatalog& catalog,const ForceBorg& borg){const auto* row=catalog.find(u16(borg.model));return row&&borg.variant<6?(borg.edits.values[0]>=0?borg.edits.values[0]:row->cost[borg.variant]):0;}
struct ForceBoxBorg {ForceBorg value;u32 memberships=0;std::array<u32,3> extension{};};
struct ForcePart {
    // BE source word: model:u16, component:u8, variant:u8 (CD534/CD858).
    u32 key=0xffffffffu,acquired=0;u8 fresh=0;std::array<u8,3> reserved{};
    bool present()const{return std::bit_cast<s32>(key)>=0;}
    u16 model()const{return u16(key>>16);}u8 variant()const{return u8(key);}u8 component()const{return u8(key>>8);}
};
struct ForceDefinition {
    std::array<u8,41> name{};u8 count=0;u16 reserved42=0;s32 cost=0;
    std::array<s16,kForceSlots> slots=[](){std::array<s16,kForceSlots> x{};x.fill(-1);return x;}();
};
struct ForceInventoryState {
    std::array<ForceDefinition,kForceCount> forces;
    std::array<ForceBoxBorg,kBoxBorgs> box;
    std::array<ForceBorg,kWarehouseBorgs> warehouse;
    std::array<ForcePart,kWarehouseParts> parts;
    u16 force_count=0,box_count=0,warehouse_count=0,part_count=0;
    std::array<u8,16> box_name{};std::array<u8,12> box_tail{};
    std::array<u8,600> profile{}; // Preserved, with only qualified fields modified.
    u32 next_acquisition()const{return load_be32(profile.data()+0x50);}
};
enum class ForceChange {Applied,Unchanged,Invalid,Empty,Full,Occupied,AlreadyMember,NotReady};
//Same scalar edit kernel for immediate tools and a staged stats-menu draft.
//It touches one Borg only; inventory ownership/revision/publication stay outside.
inline ForceChange force_borg_edit_value(ForceBorg& b,BorgEditField field,s32 value){
    if(!b.present())return ForceChange::Empty;
    if(field==BorgEditField::Variant){if(value<0||value>=6)return ForceChange::Invalid;if(b.variant==value)return ForceChange::Unchanged;b.variant=u8(value);}
    else if(field==BorgEditField::Level){if(value<1||value>10)return ForceChange::Invalid;if(b.level+1==value)return ForceChange::Unchanged;b.level=u8(value-1);}
    else{const unsigned id=field==BorgEditField::Cost?0:field==BorgEditField::HP?1:unsigned(field)-2;if(id>=8||value< -1||value>32767||(id==1&&value==0)||(id>=2&&id<=5&&value>10))return ForceChange::Invalid;if(b.edits.values[id]==value)return ForceChange::Unchanged;b.edits.values[id]=s16(value);}
    b.edits.modified=true;return ForceChange::Applied;
}
struct ForceInventoryScope {};
struct ForceBattleMember {ForceBorg borg;u16 box_slot=0;u8 force_slot=0;};
class ForceBattleRoster {
public:
    std::span<const ForceBattleMember> members()const{return {members_.data(),count_};}
    unsigned force()const{return force_;}u64 revision()const{return revision_;}
    u32 cost()const{return cost_;}s32 source_slot_prefix()const{return prefix_;}
    const std::array<u8,41>& name()const{return name_;}
private:
    friend class ForceInventory;
    std::weak_ptr<const ForceInventoryScope> scope_;u64 revision_=0;
    std::array<ForceBattleMember,kForceSlots> members_;std::array<u8,41> name_{};
    unsigned force_=0,count_=0;u32 cost_=0;s32 prefix_=0;
};
class ForceInventory {
public:
    ForceInventory()=default;ForceInventory(const ForceInventory&)=delete;ForceInventory& operator=(const ForceInventory&)=delete;
    ForceInventory(ForceInventory&&)=default;ForceInventory& operator=(ForceInventory&&)=default;
    static bool valid(const ForceInventoryState& s,const ForceCatalog& catalog){
        if(!catalog.ready())return false;
        auto borg_valid=[&](const ForceBorg& b){return b.edits.valid()&&((b.model==-1&&b.edits==BorgEdits{})||
            (b.present()&&catalog.find(u16(b.model))&&b.variant<6&&b.level<=9&&b.fresh<=1));};
        std::array<u32,kBoxBorgs> membership{};unsigned forces=0,box=0,warehouse=0,parts=0;
        for(const auto& b:s.box){if(!borg_valid(b.value))return false;box+=b.value.present();}
        for(const auto& b:s.warehouse){if(!borg_valid(b))return false;warehouse+=b.present();}
        for(const auto& p:s.parts){if(p.key!=0xffffffffu&&(!p.present()||!catalog.find(p.model())||p.variant()>=6||p.fresh>1))return false;parts+=p.present();}
        for(unsigned f=0;f<kForceCount;++f){const auto& force=s.forces[f];unsigned count=0;u32 cost=0;std::array<bool,kBoxBorgs> seen{};
            if(std::find(force.name.begin(),force.name.end(),u8(0))==force.name.end())return false;
            for(s16 slot:force.slots){if(slot==-1)continue;if(slot<0||unsigned(slot)>=kBoxBorgs||seen[unsigned(slot)]||!s.box[unsigned(slot)].value.present())return false;
                const auto& b=s.box[unsigned(slot)].value;seen[unsigned(slot)]=true;membership[unsigned(slot)]|=u32(1)<<f;++count;cost+=u32(force_borg_cost(catalog,b));}
            if(force.count!=count||force.cost!=s32(cost))return false;
            forces+=count!=0;
        }
        for(unsigned i=0;i<kBoxBorgs;++i)if(s.box[i].memberships!=membership[i])return false;
        return s.force_count==forces&&s.box_count==box&&s.warehouse_count==warehouse&&s.part_count==parts&&
            std::find(s.box_name.begin(),s.box_name.end(),u8(0))!=s.box_name.end();
    }
    bool load_state(const ForceCatalog& catalog,const ForceInventoryState& state){
        if(!valid(state,catalog))return false;
        auto data=std::make_unique<ForceInventoryState>(state);auto library=std::make_shared<const ForceCatalog>(catalog);
        auto scope=std::make_shared<const ForceInventoryScope>();state_=std::move(data);catalog_=std::move(library);scope_=std::move(scope);revision_=1;return true;
    }
    bool create(const ForceCatalog& catalog,bool source_starters=true){
        if(!catalog.ready())return false;
        auto next=std::make_unique<ForceInventoryState>();
        std::copy(catalog.owner_name().begin(),catalog.owner_name().end(),next->box_name.begin());
        if(source_starters){
            unsigned i=0;for(auto id:catalog.starters()){auto& b=next->box[i].value;b.model=s16(id);b.acquired=next_serial(*next);b.fresh=id==0x615?0:1;record_collection(*next,catalog,id,0);++i;}
            next->forces[0].slots[0]=0;put_name(next->forces[0].name,catalog.starter_name());
            std::copy(catalog.owner_name().begin(),catalog.owner_name().end(),next->profile.begin());store_be32(next->profile.data()+0x54,0x41);
        }
        refresh(*next,catalog);return load_state(catalog,*next);
    }
    bool ready()const{return state_&&catalog_&&scope_;}
    const ForceInventoryState* state()const{return state_.get();}
    const ForceCatalog* catalog()const{return catalog_.get();}
    u64 revision()const{return revision_;}
    std::weak_ptr<const ForceInventoryScope> instance_binding()const{return scope_;}
    ForceChange place(unsigned force,unsigned slot,unsigned item){
        return change([&](auto& s){if(force>=kForceCount||slot>=kForceSlots||item>=kBoxBorgs)return ForceChange::Invalid;
            if(!s.box[item].value.present())return ForceChange::Empty;
            if(s.forces[force].slots[slot]>=0)return ForceChange::Occupied;
            if(s.box[item].memberships&(u32(1)<<force))return ForceChange::AlreadyMember;
            s.forces[force].slots[slot]=s16(item);return ForceChange::Applied;});
    }
    ForceChange remove(unsigned force,unsigned slot){
        return change([&](auto& s){if(force>=kForceCount||slot>=kForceSlots)return ForceChange::Invalid;
            if(s.forces[force].slots[slot]<0)return ForceChange::Empty;
            s.forces[force].slots[slot]=-1;return ForceChange::Applied;});
    }
    ForceChange replace(unsigned force,unsigned slot,unsigned item,s16* displaced=nullptr){
        s16 old=-1;auto result=change([&](auto& s){if(force>=kForceCount||slot>=kForceSlots||item>=kBoxBorgs)return ForceChange::Invalid;
            if(!s.box[item].value.present()||s.forces[force].slots[slot]<0)return ForceChange::Empty;
            if(s.box[item].memberships&(u32(1)<<force))return ForceChange::AlreadyMember;
            old=s.forces[force].slots[slot];s.forces[force].slots[slot]=s16(item);return ForceChange::Applied;});
        if(displaced)*displaced=result==ForceChange::Applied?old:-1;
        return result;
    }
    ForceChange swap_box(unsigned a,unsigned b){
        return change([&](auto& s){if(a>=kBoxBorgs||b>=kBoxBorgs)return ForceChange::Invalid;if(a==b)return ForceChange::Unchanged;
            std::swap(s.box[a],s.box[b]);for(auto& f:s.forces)for(auto& slot:f.slots){if(slot==s16(a))slot=s16(b);else if(slot==s16(b))slot=s16(a);}return ForceChange::Applied;});
    }
    // 801CC7B4 copies members/count/cost, retaining the destination name.
    ForceChange copy_force(unsigned from,s16* destination=nullptr){
        s16 dest=-1;auto result=change([&](auto& s){if(from>=kForceCount)return ForceChange::Invalid;if(!s.forces[from].count)return ForceChange::Empty;
            for(unsigned i=0;i<kForceCount;++i)if(i!=from&&!s.forces[i].count){s.forces[i].slots=s.forces[from].slots;dest=s16(i);return ForceChange::Applied;}
            return ForceChange::Full;});
        if(destination)*destination=result==ForceChange::Applied?dest:-1;
        return result;
    }
    ForceChange delete_force(unsigned force){
        return change([&](auto& s){if(force>=kForceCount)return ForceChange::Invalid;if(!s.forces[force].count)return ForceChange::Empty;s.forces[force].slots.fill(-1);return ForceChange::Applied;});
    }
    ForceChange rename_force(unsigned force,std::string_view encoded){
        return change([&](auto& s){if(force>=kForceCount||encoded.size()>40||encoded.find('\0')!=std::string_view::npos)return ForceChange::Invalid;put_name(s.forces[force].name,encoded);return ForceChange::Applied;});
    }
    // UI eligibility (e.g. protected G Red/Neo G Red) is separate from these
    // source storage primitives. Transfers are allowed to retire box membership.
    ForceChange erase_box(unsigned item){return change([&](auto& s){if(item>=kBoxBorgs)return ForceChange::Invalid;if(!s.box[item].value.present())return ForceChange::Empty;remove_references(s,item);clear(s.box[item].value);s.box[item].memberships=0;return ForceChange::Applied;});}
    ForceChange erase_warehouse(unsigned item){return change([&](auto& s){if(item>=kWarehouseBorgs)return ForceChange::Invalid;if(!s.warehouse[item].present())return ForceChange::Empty;clear(s.warehouse[item]);return ForceChange::Applied;});}
    ForceChange erase_part(unsigned item){return change([&](auto& s){if(item>=kWarehouseParts)return ForceChange::Invalid;if(!s.parts[item].present())return ForceChange::Empty;clear(s.parts[item]);return ForceChange::Applied;});}
    ForceChange to_warehouse(unsigned item,s16* destination=nullptr){
        s16 dest=-1;auto result=change([&](auto& s){if(item>=kBoxBorgs)return ForceChange::Invalid;if(!s.box[item].value.present())return ForceChange::Empty;if(s.warehouse_count==kWarehouseBorgs)return ForceChange::Full;
            for(unsigned i=0;i<kWarehouseBorgs;++i)if(!s.warehouse[i].present()){s.warehouse[i]=s.box[item].value;remove_references(s,item);clear(s.box[item].value);s.box[item].memberships=0;dest=s16(i);return ForceChange::Applied;}
            return ForceChange::Invalid;});
        if(destination)*destination=result==ForceChange::Applied?dest:-1;
        return result;
    }
    ForceChange from_warehouse(unsigned item,s16* destination=nullptr){
        s16 dest=-1;auto result=change([&](auto& s){if(item>=kWarehouseBorgs)return ForceChange::Invalid;if(!s.warehouse[item].present())return ForceChange::Empty;if(s.box_count==kBoxBorgs)return ForceChange::Full;
            for(unsigned i=0;i<kBoxBorgs;++i)if(!s.box[i].value.present()){s.box[i].value=s.warehouse[item];s.box[i].memberships=0;clear(s.warehouse[item]);dest=s16(i);return ForceChange::Applied;}
            return ForceChange::Invalid;});
        if(destination)*destination=result==ForceChange::Applied?dest:-1;
        return result;
    }
    // Actual earned items are supplied by the future reward producer; no reward
    // odds or combat contribution formula is invented here. Fusion uses this
    // same acquisition kernel inside a joint inventory transaction.
    ForceChange acquire(ForceBorg value){return change([&](auto& s){return acquire_into(s,*catalog_,value);});}
    ForceChange pick_warehouse(CheatSettings cheats,u16 model,u8 variant){
        return change([&](auto& s){if(!cheats.warehouse_selector||!catalog_->find(model)||variant>=6)return ForceChange::Invalid;
            if(s.warehouse_count==kWarehouseBorgs)return ForceChange::Full;
            for(auto& b:s.warehouse)if(!b.present()){b={};b.model=s16(model);b.variant=variant;b.acquired=next_serial(s);b.fresh=1;record_collection(s,*catalog_,model,variant);return ForceChange::Applied;}
            return ForceChange::Invalid;});
    }
    // Scope/revision guards are caller-visible so an old detail selection can
    // never edit a newly occupying instance. All shared force costs refresh.
    ForceChange edit_borg(CheatSettings cheats,bool warehouse,unsigned slot,u64 expected_revision,BorgEditField field,s32 value){
        if(!cheats.borg_editor||expected_revision!=revision_)return ForceChange::Invalid;
        return change([&](auto& s){if((warehouse&&slot>=kWarehouseBorgs)||(!warehouse&&slot>=kBoxBorgs))return ForceChange::Invalid;
            auto& b=warehouse?s.warehouse[slot]:s.box[slot].value;if(!b.present())return ForceChange::Empty;
            const auto result=force_borg_edit_value(b,field,value);if(result==ForceChange::Applied&&field==BorgEditField::Variant)record_collection(s,*catalog_,u16(b.model),b.variant);return result;});
    }
    ForceChange acquire_part(u32 key){return change([&](auto& s){ForcePart p;p.key=key;if(!p.present()||!catalog_->find(p.model())||p.variant()>=6)return ForceChange::Invalid;if(s.part_count==kWarehouseParts)return ForceChange::Full;for(auto& x:s.parts)if(!x.present()){x.key=key;x.acquired=next_serial(s);x.fresh=1;return ForceChange::Applied;}return ForceChange::Invalid;});}
    ForceChange mark_seen(unsigned domain,unsigned item){return change([&](auto& s){u8* flag=nullptr;
        if(domain==0&&item<kBoxBorgs&&s.box[item].value.present())flag=&s.box[item].value.fresh;
        if(domain==1&&item<kWarehouseBorgs&&s.warehouse[item].present())flag=&s.warehouse[item].fresh;
        if(domain==2&&item<kWarehouseParts&&s.parts[item].present())flag=&s.parts[item].fresh;
        if(!flag)return ForceChange::Invalid;
        if(!*flag)return ForceChange::Unchanged;
        *flag=0;return ForceChange::Applied;});}
    ForceChange age_new_markers(){return change([&](auto& s){bool changed=false;const s32 limit=std::bit_cast<s32>(s.next_acquisition()-50u);auto age=[&](auto& x){if(x.present()&&x.fresh&&std::bit_cast<s32>(x.acquired)<limit){x.fresh=0;changed=true;}};for(auto& x:s.box)age(x.value);for(auto& x:s.warehouse)age(x);for(auto& x:s.parts)age(x);return changed?ForceChange::Applied:ForceChange::Unchanged;});}
    // 801BE9EC exit: clear empty names, default unnamed forces, compact holes.
    ForceChange finish_edit(){return change([&](auto& s){for(unsigned i=0;i<kForceCount;++i){auto& f=s.forces[i];if(!f.count)f.name.fill(0);else if(!f.name[0]){put_name(f.name,catalog_->unnamed_name());f.name[catalog_->letter_byte()]=u8(f.name[catalog_->letter_byte()]+i);}unsigned at=0;for(auto slot:f.slots)if(slot>=0)f.slots[at++]=slot;while(at<kForceSlots)f.slots[at++]=-1;}return ForceChange::Applied;});}
    bool budget_prefix(unsigned force,s32 budget,ForceCostMode mode,s32* out)const{
        if(!out||!ready()||force>=kForceCount||(mode!=ForceCostMode::Full&&mode!=ForceCostMode::TwoThirds))return false;
        if(budget<1){*out=-1;return true;}
        s32 prefix=0,total=0;for(unsigned i=0;i<kForceSlots;++i){const auto slot=state_->forces[force].slots[i];if(slot>=0){const auto& b=state_->box[unsigned(slot)].value;total+=cost(b,mode);}if(total<=budget)prefix=s32(i+1);if(budget<=total)break;}*out=prefix;return true;
    }
    bool total_cost(unsigned force,ForceCostMode mode,s32* out)const{
        if(!out)return false;
        *out=0;if(!ready()||force>=kForceCount||(mode!=ForceCostMode::Full&&mode!=ForceCostMode::TwoThirds))return false;
        for(auto slot:state_->forces[force].slots)if(slot>=0)*out+=cost(state_->box[unsigned(slot)].value,mode);
        return true;
    }
    bool battle_roster(unsigned force,s32 budget,ForceCostMode mode,ForceBattleRoster* out)const{
        if(!out)return false;
        *out={};s32 prefix=0;if(!budget_prefix(force,budget,mode,&prefix)||!state_->forces[force].count||prefix==0)return false;
        ForceBattleRoster next;next.scope_=scope_;next.revision_=revision_;next.force_=force;next.prefix_=prefix;next.name_=state_->forces[force].name;
        for(unsigned i=0;i<(prefix<0?kForceSlots:unsigned(prefix));++i){auto slot=state_->forces[force].slots[i];if(slot<0)continue;const auto& b=state_->box[unsigned(slot)].value;if(b.edits.modified)return false;next.members_[next.count_++]={b,u16(slot),u8(i)};next.cost_+=u32(cost(b,mode));}
        if(!next.count_)return false;
        *out=std::move(next);return true;
    }
    bool current(const ForceBattleRoster& roster)const{return ready()&&roster.count_&&roster.scope_.lock()==scope_&&roster.revision_==revision_;}
private:
    friend class ForceFusion;
    friend class BorgStatsEditor;
    //One checked commit for a stats-menu draft. Cancellation never calls this.
    ForceChange commit_stats_draft(CheatSettings cheats,bool warehouse,unsigned slot,u64 revision,const ForceBorg& original,const ForceBorg& draft){
        if(!ready()||!cheats.borg_editor||revision!=revision_)return ForceChange::Invalid;
        return change([&](auto& s){
            if((warehouse&&slot>=kWarehouseBorgs)||(!warehouse&&slot>=kBoxBorgs))return ForceChange::Invalid;
            auto& actual=warehouse?s.warehouse[slot]:s.box[slot].value;if(actual!=original)return ForceChange::Invalid;
            ForceBorg next=original;
            for(unsigned field=0;field<10;++field){const s32 value=field==0?draft.variant:field==2?s32(draft.level)+1:draft.edits.values[field==1?0:field-2];const auto result=force_borg_edit_value(next,BorgEditField(field),value);if(result!=ForceChange::Applied&&result!=ForceChange::Unchanged)return result;}
            const bool changed=next.variant!=original.variant||next.level!=original.level||next.edits.values!=original.edits.values;
            next.edits.modified=original.edits.modified||changed;if(next!=draft)return ForceChange::Invalid;
            if(!changed)return ForceChange::Unchanged;
            actual=next;if(actual.variant!=original.variant)record_collection(s,*catalog_,u16(actual.model),actual.variant);return ForceChange::Applied;
        });
    }
    static ForceChange acquire_into(ForceInventoryState& s,const ForceCatalog& catalog,ForceBorg value){
        if(!value.present()||!catalog.find(u16(value.model))||value.variant>=6||value.level>9||!value.edits.valid())return ForceChange::Invalid;
        ForceBorg* at=nullptr;
        if(s.box_count<kBoxBorgs){for(auto& x:s.box)if(!x.value.present()){at=&x.value;x.memberships=0;break;}}
        else if(s.warehouse_count<kWarehouseBorgs){for(auto& x:s.warehouse)if(!x.present()){at=&x;break;}}
        if(!at)return ForceChange::Full;
        at->model=value.model;at->variant=value.variant;at->level=value.level;at->acquired=next_serial(s);at->experience=value.experience;at->fresh=1;at->edits=value.edits;record_collection(s,catalog,u16(value.model),value.variant);return ForceChange::Applied;
    }
    static void put_name(std::array<u8,41>& out,std::string_view text){std::copy(text.begin(),text.end(),out.begin());out[text.size()]=0;}
    static void clear(ForceBorg& b){b.model=-1;b.variant=b.level=b.fresh=0;b.acquired=b.experience=0;b.edits={};}
    static void clear(ForcePart& p){p.key=0xffffffffu;p.acquired=0;p.fresh=0;}
    static void remove_references(ForceInventoryState& s,unsigned item){for(auto& f:s.forces)for(auto& slot:f.slots)if(slot==s16(item))slot=-1;}
    static u32 next_serial(ForceInventoryState& s){const u32 value=s.next_acquisition();store_be32(s.profile.data()+0x50,value+1);return value;}
    static void record_collection(ForceInventoryState& s,const ForceCatalog& catalog,u16 model,u8 variant){
        const auto number=catalog.find(model)->number;const u8 flag=u8(1u<<variant);s.profile[0x58+number]|=flag;s.profile[0x158+number]|=flag;
        for(const auto& p:catalog.collection_pairs())if(model==p.first||model==p.second){const auto a=catalog.find(p.first)->number,b=catalog.find(p.second)->number,c=catalog.find(p.result)->number;if(s.profile[0x58+a]&&s.profile[0x58+b]){s.profile[0x58+c]|=s.profile[0x58+a];s.profile[0x158+c]|=s.profile[0x58+a];}}
    }
    static void refresh(ForceInventoryState& s,const ForceCatalog& catalog){
        s.force_count=s.box_count=s.warehouse_count=s.part_count=0;
        for(auto& b:s.box){b.memberships=0;s.box_count+=b.value.present();}for(const auto& b:s.warehouse)s.warehouse_count+=b.present();for(const auto& p:s.parts)s.part_count+=p.present();
        for(unsigned i=0;i<kForceCount;++i){auto& f=s.forces[i];f.count=0;f.cost=0;for(auto slot:f.slots)if(slot>=0){auto& b=s.box[unsigned(slot)];++f.count;f.cost+=force_borg_cost(catalog,b.value);b.memberships|=u32(1)<<i;}s.force_count+=f.count!=0;}
    }
    s32 cost(const ForceBorg& b,ForceCostMode mode)const{return force_cost_for_mode(force_borg_cost(*catalog_,b),mode);}
    template<class Operation> ForceChange change(Operation operation){
        if(!ready())return ForceChange::NotReady;
        if(revision_==~u64(0))return ForceChange::Invalid;
        auto next=std::make_unique<ForceInventoryState>(*state_);const auto result=operation(*next);if(result!=ForceChange::Applied)return result;
        refresh(*next,*catalog_);if(!valid(*next,*catalog_))return ForceChange::Invalid;
        state_=std::move(next);++revision_;return result;
    }
    std::unique_ptr<ForceInventoryState> state_;std::shared_ptr<const ForceCatalog> catalog_;
    std::shared_ptr<const ForceInventoryScope> scope_;u64 revision_=0;
};
} // namespace gf
