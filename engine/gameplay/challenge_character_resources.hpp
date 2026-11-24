#pragma once
#include <exception>
#include "engine/assets/battle_character_resources.hpp"
#include "engine/gameplay/challenge_initial_party.hpp"
#include "engine/core/backends.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/gameplay/battle_effect_resources.hpp"
namespace gf {
enum class ChallengeCharacterResourceStatus {Closed,Loading,DescriptorAllocated,Failed,Retired,Queued};
enum class ChallengeCharacterWorkKind {LoadRow,PublishRow,LoadCommonEffects,BindCommonEffectTextures};
using ChallengeCharacterCellKind=SceneResourceCellKind;
using ChallengeCharacterCell=SceneResourceCell;
struct ChallengeCharacterRowState {
    std::array<ChallengeCharacterCell,14> cells;
    std::array<ByteView,4> published{}; //data,battle,motion,selected wrapper
    //410BC's initial C30 clear images, not a live packed memory codec. +600 is
    //a typed archive lease; later419EC cache words have their own typed owner.
    std::array<u8,0x600> cache_prefix{};std::array<u8,0x62c> cache_suffix{};
    SceneArchiveLease archive;bool active=false,loaded=false,published_complete=false;
};
struct ChallengeCharacterWork {ChallengeCharacterWorkKind kind=ChallengeCharacterWorkKind::LoadRow;u8 row=0;};
struct ChallengeCharacterQueueState {
    std::array<ChallengeCharacterRowState,6> rows;
    std::array<ChallengeCharacterWork,14> items{};unsigned count=0,cursor=0;
    std::array<SceneResourceArgument,128> arguments{};u8 read=0,write=0;unsigned pending_arguments=0;
};
namespace challenge_character_resource_detail {
struct Batch {
    bool live=true;ChallengeInitialPartyCandidate party;std::array<std::shared_ptr<const BattleCharacterResource>,6> resources;std::array<u8,6> selected_member_indices{255,255,255,255,255,255};mutable std::array<bool,6> construction_claimed{};mutable bool battle_roster_claimed=false;
    bool queued=false,processing=false;ChallengeCharacterQueueState queue;
    std::array<BattleCharacterResourceRow,6> recipes{};std::array<u8,6> variants{};
    std::array<std::shared_ptr<const BattleCharacterResource>,6> loaded_rows;
    std::weak_ptr<SceneArchivePool> archives;const SceneArchivePoolSnapshot* archive_identity=nullptr;std::string directory;
    std::weak_ptr<challenge_preparation_entry_detail::Storage> origin;
    std::shared_ptr<BattleEffectResources> effects;
    mutable std::array<std::shared_ptr<const SceneModelMaterialContext>,6> model_contexts;
    bool current(const std::array<const ForceInventory*,2>& owners)const{
        if(!live||!party.current(owners))return false;
        if(!queued)return true;
        const auto pool=archives.lock();const auto entry=origin.lock();if(!entry||!entry->live||!entry->character_load_claimed||entry->archives.lock()!=pool||!party.preparation_entry_state(owners)||!pool||!pool->ready()||pool->snapshot()!=archive_identity)return false;
        for(const auto& row:queue.rows)if(row.published_complete&&!row.archive.current())return false;
        if(effects&&!effects->current())return false;
        return true;
    }
};
}
class ChallengeCharacterResources;
class ChallengeCharacterDescriptorReceipt {
public:
    std::shared_ptr<const BattleCharacterResource> data(const std::array<const ForceInventory*,2>& owners)const{const auto b=batch_.lock();return b&&b->current(owners)&&slot_<6?b->resources[slot_]:nullptr;}
    bool valid(const std::array<const ForceInventory*,2>& owners)const{return bool(data(owners));}
 private:friend class ChallengeCharacterResources;friend class BattleActorConstruction;friend class ChallengeBattleRoster;friend class BattlePreparationBootstrap;std::weak_ptr<const challenge_character_resource_detail::Batch> batch_;u8 slot_=255;
};
//Explicit resource preparation for selected members of a genuine current party.
//Member indices are supplied, NOT assumed to be the original first-member policy.
//One fresh read per occupied slot; native ALL-batch publication is stronger than
//source per-slot callback timing and is not an original asynchronous ACK.
class ChallengeCharacterResources {
public:
    ChallengeCharacterResources()=default;ChallengeCharacterResources(const ChallengeCharacterResources&)=delete;ChallengeCharacterResources& operator=(const ChallengeCharacterResources&)=delete;
    ChallengeCharacterResources(ChallengeCharacterResources&&)=default;ChallengeCharacterResources& operator=(ChallengeCharacterResources&&)=default;
    ~ChallengeCharacterResources(){retire();}
    bool begin(const ChallengeInitialPartyCandidate& party,const std::array<const ForceInventory*,2>& owners,const std::array<u8,6>& selected,const BattleCharacterResourceProfile& profile,IFileSystem& files,const std::string& directory,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("Challenge character resources: ")+why;return false;};
        if(status_!=ChallengeCharacterResourceStatus::Closed)return reject("fresh one-shot owner required; no retry");
        status_=ChallengeCharacterResourceStatus::Loading;auto next=std::make_shared<challenge_character_resource_detail::Batch>();next->party=party;batch_=next;const auto indices=selected;
        auto fail=[&](const char* why){next->live=false;next->resources={};if(status_!=ChallengeCharacterResourceStatus::Retired)status_=ChallengeCharacterResourceStatus::Failed;return reject(why);};
        if(!next->party.current(owners)||!owners[0]||!profile.for_catalog(*owners[0]->catalog())||directory.empty())return fail("current party/catalog/profile/native directory required");
        std::array<BattleCharacterResourceRow,6> rows{};std::array<bool,6> active{};std::array<u8,6> variants{};
        for(unsigned slot=0;slot<6;++slot){active[slot]=(next->party.active()&(1u<<slot))!=0;if(!active[slot]){if(indices[slot]!=255)return fail("inactive member index must be absent");continue;}const auto members=next->party.members(slot);if(indices[slot]>=members.size())return fail("selected member outside current roster");const auto& fields=members[indices[slot]].fields;const auto* row=profile.find(fields.model);if(!row||fields.variant>=6)return fail("selected model/variant not bound by profile");rows[slot]=*row;variants[slot]=fields.variant;}
        next->selected_member_indices=indices;
        try{for(unsigned slot=0;slot<6;++slot){if(!active[slot])continue;
                auto resource=read_resource(rows[slot],variants[slot],files,directory,[&](){return next->live&&batch_==next&&next->party.current(owners);},error);
                if(!resource){return fail("native read/identity/lifetime refusal");}next->resources[slot]=std::move(resource);
            }}catch(const std::exception&){return fail("uncertain native read/allocation; request not replayed");}
        if(!next->live||batch_!=next||!next->party.current(owners))return fail("party/owner retired before publication");
        status_=ChallengeCharacterResourceStatus::DescriptorAllocated;if(error)error->clear();return true;
    }
    //Native source-ordered work queue. It does not import an original DVD
    //scheduler/control byte or claim hardware timing. Load items retain their
    //current row; publication items consume slot/variant from one argument FIFO.
    bool begin_queued(const ChallengeInitialPartyCandidate& party,const std::array<const ForceInventory*,2>& owners,const std::array<u8,6>& selected,const BattleCharacterResourceProfile& profile,const std::shared_ptr<SceneArchivePool>& archives,const std::string& directory,std::string* error){
        if(status_!=ChallengeCharacterResourceStatus::Closed||!party.preparation_entry_state(owners)||!owners[0]||!profile.for_catalog(*owners[0]->catalog())||!archives||!archives->ready()||directory.empty())return reject(error,"fresh normal origin/catalog/archive domain/path required");
        const auto entry=party.preparation_entry_.lock();
        if(!entry||!entry->live||!entry->prefix_claimed||!entry->branch_claimed||entry->character_load_claimed||entry->archives.lock()!=archives)return reject(error,"same unclaimed normal scene load epoch/archive pool required");
        status_=ChallengeCharacterResourceStatus::Loading;
        try{auto next=std::make_shared<challenge_character_resource_detail::Batch>();next->party=party;next->queued=true;next->origin=entry;next->archives=archives;next->archive_identity=archives->snapshot();next->directory=directory;next->selected_member_indices=selected;
            for(unsigned slot=0;slot<6;++slot){auto& row=next->queue.rows[slot];row.active=(party.active()&(1u<<slot))!=0;
                if(!row.active){if(selected[slot]!=255)return reject_failed(error,"inactive selected member must be absent");continue;}
                const auto members=party.members(slot);if(selected[slot]>=members.size())return reject_failed(error,"selected member outside roster");
                const auto& member=members[selected[slot]].fields;const auto* recipe=profile.find(member.model);
                if(!recipe||recipe->slots>14||member.variant>=6)return reject_failed(error,"bounded14-cell normal package/variant required");
                next->recipes[slot]=*recipe;next->variants[slot]=member.variant;
                for(unsigned cell:{0u,2u,3u,4u+member.variant})row.cells[cell].kind=ChallengeCharacterCellKind::Destination;
                auto& q=next->queue;q.items[q.count++]={ChallengeCharacterWorkKind::LoadRow,u8(slot)};
                q.arguments[q.write]=SceneResourceArgument::number(slot);q.write=u8((q.write+1)&127);q.arguments[q.write]=SceneResourceArgument::number(member.variant);q.write=u8((q.write+1)&127);q.pending_arguments+=2;
                q.items[q.count++]={ChallengeCharacterWorkKind::PublishRow,u8(slot)};
            }
            entry->character_load_claimed=true;batch_=std::move(next);status_=ChallengeCharacterResourceStatus::Queued;if(error)error->clear();return true;
        }catch(const std::exception&){return reject_failed(error,"uncertain queue construction; no replay");}
    }
    //42878 appends to the SAME item/argument stream after all six slot visits.
    //Its four pointer arguments bind typed destinations in this batch, not a
    //copy of the not-yet-loaded texture bytes or guest addresses.
    bool append_common_effects(const BattleEffectResourceProfile& profile,const std::string& path,const std::array<const ForceInventory*,2>& owners,std::string* error){
        if(status_!=ChallengeCharacterResourceStatus::Queued||!batch_||!batch_->current(owners)||batch_->processing||batch_->queue.cursor||batch_->effects||!profile.ready()||path.empty())return reject(error,"current untouched queue/source effect profile/path required");
        const auto* display=batch_->party.display_setup();if(!display||display->layout29>1)return reject(error,"current Challenge display selection required");
        try{auto effects=std::make_shared<BattleEffectResources>();if(!effects->begin(profile,display->layout29,path,error))return false;
            auto& q=batch_->queue;if(q.count+2>q.items.size()||q.pending_arguments+4>q.arguments.size())return reject(error,"bounded shared queue capacity");
            q.items[q.count++]={ChallengeCharacterWorkKind::LoadCommonEffects,0};
            using K=SceneResourceArgumentKind;
            for(const auto kind:{K::EffectTextureInput,K::EffectTextureBindings,K::EffectTextureObject,K::EffectPaletteObject}){q.arguments[q.write]={kind,0};q.write=u8((q.write+1)&127);}
            q.pending_arguments+=4;q.items[q.count++]={ChallengeCharacterWorkKind::BindCommonEffectTextures,0};
            batch_->effects=std::move(effects);if(error)error->clear();return true;
        }catch(const std::exception&){batch_->live=false;return reject_failed(error,"uncertain effect enqueue; no replay");}
    }
    //One actual native load OR publication operation. No caller-provided
    //completion flags. A failure closes the batch but preserves consumed pool
    //occupancy and the completed prefix; cancellation is not successful drain.
    bool advance_queued(IFileSystem& files,const std::array<const ForceInventory*,2>& owners,std::string* error){
        const auto b=batch_;
        if(status_!=ChallengeCharacterResourceStatus::Queued||!b||!b->current(owners)||b->processing||b->queue.cursor>=b->queue.count)return reject(error,"current non-reentrant queued operation required");
        b->processing=true;
        auto fail=[&](const char* why){b->live=false;b->processing=false;if(status_!=ChallengeCharacterResourceStatus::Retired)status_=ChallengeCharacterResourceStatus::Failed;return reject(error,why);};
        try{const auto item=b->queue.items[b->queue.cursor];auto& row=b->queue.rows[item.row];
            if(item.kind==ChallengeCharacterWorkKind::LoadCommonEffects){
                if(!b->effects||!b->effects->load(files,error)||batch_!=b||!b->current(owners)||status_!=ChallengeCharacterResourceStatus::Queued)return fail("effect load/owner lifetime failed");
            }else if(item.kind==ChallengeCharacterWorkKind::BindCommonEffectTextures){
                auto& q=b->queue;using K=SceneResourceArgumentKind;
                const std::array<K,4> kinds{K::EffectTextureInput,K::EffectTextureBindings,K::EffectTextureObject,K::EffectPaletteObject};
                if(q.pending_arguments<4||!b->effects)return fail("effect argument/source owner missing");
                for(unsigned i=0;i<4;++i)if(q.arguments[(q.read+i)&127].kind!=kinds[i])return fail("effect callback argument origin mismatch");
                q.read=u8((q.read+4)&127);q.pending_arguments-=4;
                if(!b->effects->bind_textures(error))return fail("effect texture callback refused");
            }else if(item.kind==ChallengeCharacterWorkKind::LoadRow){
                auto data=read_resource(b->recipes[item.row],b->variants[item.row],files,b->directory,[&](){return batch_==b&&b->current(owners)&&status_==ChallengeCharacterResourceStatus::Queued;},error);
                if(!data)return fail("queued native read/row lifetime failed; no replay");
                //A5A0 clears zero-size cells even when their old value was SKIP.
                for(unsigned cell=0;cell<data->selection().slots;++cell){auto& c=row.cells[cell];const auto bytes=data->block(cell);if(!bytes.size){c.kind=ChallengeCharacterCellKind::Empty;c.bytes={};}
                    else if(c.kind==ChallengeCharacterCellKind::Destination){c.kind=ChallengeCharacterCellKind::Loaded;c.bytes=bytes;}}
                b->loaded_rows[item.row]=std::move(data);row.loaded=true;
            }else{
                auto& q=b->queue;if(q.pending_arguments<2)return fail("publication argument underflow");
                if(q.arguments[q.read].kind!=SceneResourceArgumentKind::Integer||q.arguments[(q.read+1)&127].kind!=SceneResourceArgumentKind::Integer)return fail("character publication argument types");
                const u32 slot=q.arguments[q.read].integer,variant=q.arguments[(q.read+1)&127].integer;
                if(slot!=item.row||variant>=6||variant!=b->variants[item.row]||!row.loaded||!b->loaded_rows[item.row])return fail("publication arguments/current row mismatch");
                q.read=u8((q.read+2)&127);q.pending_arguments-=2; //A234 consumes before callback
                const std::array<unsigned,4> cells{0,2,3,4+variant};
                for(unsigned i=0;i<4;++i){const auto& c=row.cells[cells[i]];if(c.kind!=ChallengeCharacterCellKind::Loaded&&c.kind!=ChallengeCharacterCellKind::Empty)return fail("publication reads an unavailable row cell");row.published[i]=c.bytes;}
                const auto pool=b->archives.lock();if(!pool||pool->snapshot()!=b->archive_identity)return fail("publication archive domain retired");
                if(pool->acquire(&row.archive,error)!=SceneArchiveAcquire::Acquired)return fail("publication archive allocation failed; prefix retained");
                b->resources[item.row]=b->loaded_rows[item.row];row.published_complete=true;
            }
            ++b->queue.cursor;b->processing=false;
            if(b->queue.cursor==b->queue.count){if(b->queue.pending_arguments)return fail("unconsumed publication arguments");status_=ChallengeCharacterResourceStatus::DescriptorAllocated;}
            if(error){error->clear();}return true;
        }catch(const std::exception&){return fail("uncertain queued operation; no replay");}
    }
    bool current(const std::array<const ForceInventory*,2>& owners)const{return batch_&&batch_->current(owners)&&(status_==ChallengeCharacterResourceStatus::Queued||status_==ChallengeCharacterResourceStatus::DescriptorAllocated);}
    u8 selected_member(unsigned slot,const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&slot<6?batch_->selected_member_indices[slot]:255;}
    const ChallengeCharacterQueueState* queue_state(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&batch_->queued?&batch_->queue:nullptr;}
    const BattleEffectResources* common_effects(const std::array<const ForceInventory*,2>& owners)const{return current(owners)?batch_->effects.get():nullptr;}
    const SceneModelMaterialContext* model_context(unsigned slot,const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&slot<6?batch_->model_contexts[slot].get():nullptr;}
    bool successful_drain(const std::array<const ForceInventory*,2>& owners)const{return current(owners)&&batch_->queued&&status_==ChallengeCharacterResourceStatus::DescriptorAllocated&&batch_->queue.cursor==batch_->queue.count&&!batch_->queue.pending_arguments;}
    ChallengeCharacterDescriptorReceipt receipt(unsigned slot)const{ChallengeCharacterDescriptorReceipt r;if((status_==ChallengeCharacterResourceStatus::DescriptorAllocated||status_==ChallengeCharacterResourceStatus::Queued)&&slot<6&&batch_&&batch_->live&&batch_->resources[slot]){r.batch_=batch_;r.slot_=u8(slot);}return r;}
    ChallengeCharacterResourceStatus status()const{return status_;}
    void retire(){if(batch_){batch_->live=false;if(batch_->effects)batch_->effects->retire();}batch_.reset();status_=ChallengeCharacterResourceStatus::Retired;}
private:
    friend class BattlePreparationBootstrap;
    static bool reject(std::string* error,const char* why){if(error)*error=std::string("Challenge character resources: ")+why;return false;}
    bool reject_failed(std::string* error,const char* why){status_=ChallengeCharacterResourceStatus::Failed;return reject(error,why);}
    template<class Current> static std::shared_ptr<const BattleCharacterResource> read_resource(const BattleCharacterResourceRow& row,u8 variant,IFileSystem& files,const std::string& directory,Current current,std::string* error){
        const auto path=directory+"/"+std::to_string(row.member)+".gfnb";u64 size=0;std::vector<u8> bytes;
        if(!files.file_size(path,&size)||!current()||size!=row.bytes||!files.read(path,bytes)||!current()||bytes.size()!=row.bytes){reject(error,"missing/short/retired native read");return {};}
        auto data=std::make_shared<BattleCharacterResource>();if(!data->load(row,variant,{bytes.data(),bytes.size()},error))return {};
        return data;
    }
    std::shared_ptr<challenge_character_resource_detail::Batch> batch_;ChallengeCharacterResourceStatus status_=ChallengeCharacterResourceStatus::Closed;
};
} // namespace gf
