#pragma once
#include "engine/assets/battle_preparation_models.hpp"
#include "engine/assets/hsd_joint_instances.hpp"
#include "engine/gameplay/battle_preparation_widgets.hpp"
#include "engine/core/backends.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
namespace gf {
struct BattlePreparationModelScope {};
struct BattlePreparationModelBinding {u16 attempt=0;u8 descriptor_slot=0;u16 record=0,instance=0;bool operator==(const BattlePreparationModelBinding&)const=default;};
enum class BattlePreparationModelsStatus {Closed,Prepared,Failed,Retired,ArchiveBound};
//Actual immutable bank bytes + separate normal root state copies for successful
//widget allocations. Resources + explicit same-domain archive slot ONLY: no model
//descriptor0C bind, global scene availability, phase/counter/pose or199C74 ACK.
class BattlePreparationModels {
public:
    BattlePreparationModels()=default;BattlePreparationModels(const BattlePreparationModels&)=delete;BattlePreparationModels& operator=(const BattlePreparationModels&)=delete;
    BattlePreparationModels(BattlePreparationModels&&)=default;BattlePreparationModels& operator=(BattlePreparationModels&&)=default;~BattlePreparationModels(){retire();}
    bool begin(BattlePreparationWidgets& widgets,const BattlePreparationModelProfile& profile,IFileSystem& files,const std::string& path,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle preparation models: ")+why;return false;};
        if(attempted_||!profile.ready()||path.empty()||!widgets.current()||widgets.storage_->models_preparation_claimed)return reject("fresh/current unclaimed widgets and byte profile required");
        attempted_=true;status_=BattlePreparationModelsStatus::Failed;const auto context=widgets.storage_;context->models_preparation_claimed=true;
        try{auto next=std::make_unique<Storage>();next->widgets=context;next->profile=profile;u64 size=0;std::vector<u8> bytes;
            if(!files.file_size(path,&size)||size!=profile.bytes()||!files.read(path,bytes)||!profile.matches({bytes.data(),bytes.size()}))return reject("missing/short/foreign bytes; claimed read not replayed");
            if(!context->current())return reject("widgets retired during read");
            if(!next->bank.load({bytes.data(),bytes.size()},error,true))return false;
            if(next->bank.hash()!=profile.hash())return reject("shared bank identity mismatch");
            std::vector<u16> records;
            for(unsigned a=0;a<context->attempts.size();++a){const auto& attempt=context->attempts[a];if(attempt.result!=SceneObjectAcquire::Allocated)continue;const auto* row=context->profile.find(attempt.type);if(!row||row->descriptors!=attempt.descriptors)return reject("captured template changed");
                for(unsigned slot=0;slot<row->descriptors;++slot){const s8 record=row->model_records[slot];if(record<0)continue;if(unsigned(record)>=next->bank.records()||records.size()>=256)return reject("record/budget outside bank");next->bindings.push_back({u16(a),u8(slot),u16(record),u16(records.size())});records.push_back(u16(record));}
            }
            if(!next->instances.construct_normal_records(next->bank,records,error))return false;
            if(!context->current())return reject("widgets retired before resource publication");
            storage_=std::move(next);status_=BattlePreparationModelsStatus::Prepared;if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native read/construction; no replay");}
    }
    bool current()const{if((status_!=BattlePreparationModelsStatus::Prepared&&status_!=BattlePreparationModelsStatus::ArchiveBound)||!storage_)return false;const auto s=storage_->widgets.lock();const auto* archive=storage_->archive_bound?storage_->archive.bank():nullptr;return s&&s->current()&&storage_->bank.hash()==storage_->profile.hash()&&storage_->instances.instances()==storage_->bindings.size()&&(!storage_->archive_bound||(archive&&archive->same_source(storage_->bank)));}
    //4104C's197A0C(1)→40EB4/06C88 against an ACTUAL same-domain archive slot.
    //No descriptor-root attachment, widget phase/counter change or scene ACK.
    bool advance_archive_binding(SceneArchivePool& pool,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle preparation archive: ")+why;return false;};
        if(status_!=BattlePreparationModelsStatus::Prepared||!current()||storage_->archive_attempted)return reject("prepared/current one-shot resource cursor required");
        const auto context=storage_->widgets.lock();if(!context||!pool.matches_descriptor_owner(context->pool->storage_->descriptors))return reject("same current descriptor domain required");
        storage_->archive_attempted=true;
        if(pool.acquire(&storage_->archive,error)!=SceneArchiveAcquire::Acquired){status_=BattlePreparationModelsStatus::Failed;return false;}
        if(!pool.bind_borrowed_bank(storage_->archive,storage_->bank,error)||!context->current()){status_=BattlePreparationModelsStatus::Failed;return false;}
        storage_->archive_bound=true;status_=BattlePreparationModelsStatus::ArchiveBound;if(error)error->clear();return true;
    }
    const HsdRecordAnimationBank* bank()const{return current()?&storage_->bank:nullptr;}
    const HsdJointInstanceBank* instances()const{return current()?&storage_->instances:nullptr;}
    std::span<const BattlePreparationModelBinding> bindings()const{return current()?std::span<const BattlePreparationModelBinding>{storage_->bindings}:std::span<const BattlePreparationModelBinding>{};}
    bool archive_pool1_join_pending()const{return !current()||!storage_->archive_bound;}bool callbacks_and_descriptor_roots_pending()const{return true;}
    const SceneArchiveRecord* archive_record()const{return current()&&storage_->archive_bound?storage_->archive.inspect():nullptr;}
    BattlePreparationModelsStatus status()const{return status_;}
    void retire(){storage_.reset();status_=BattlePreparationModelsStatus::Retired;attempted_=true;}
private:
    friend class BattlePreparationContext;
    struct Storage {std::shared_ptr<const BattlePreparationModelScope> identity=std::make_shared<const BattlePreparationModelScope>();std::weak_ptr<BattlePreparationWidgets::Storage> widgets;BattlePreparationModelProfile profile;HsdRecordAnimationBank bank;HsdJointInstanceBank instances;std::vector<BattlePreparationModelBinding> bindings;SceneArchiveLease archive;bool archive_attempted=false,archive_bound=false;};
    std::unique_ptr<Storage> storage_;bool attempted_=false;BattlePreparationModelsStatus status_=BattlePreparationModelsStatus::Closed;
};
} // namespace gf
