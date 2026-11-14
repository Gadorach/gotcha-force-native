#pragma once
#include "engine/assets/scene_descriptor_bootstrap.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
namespace gf {
//16B pool2/3 record common fields ONLY. This is typed native fresh storage, not
//a guest image. Allocation/header constructors and cleanup hooks stay unexposed.
struct SceneSmallDescriptorRecord {s32 link0=-1,link4=-1;u8 pool08=0,allocated09=0,flag0a=0,flag0b=0;u32 unresolved_header0c=0;bool operator==(const SceneSmallDescriptorRecord&)const=default;};
struct SceneSmallDescriptorPoolSnapshot {s32 free_head=-1,live_head=-1;s16 free_count=0;std::vector<SceneSmallDescriptorRecord> records;bool operator==(const SceneSmallDescriptorPoolSnapshot&)const=default;};
class SceneDescriptorBootstrap {
public:
    SceneDescriptorBootstrap()=default;SceneDescriptorBootstrap(const SceneDescriptorBootstrap&)=delete;SceneDescriptorBootstrap& operator=(const SceneDescriptorBootstrap&)=delete;
    SceneDescriptorBootstrap(SceneDescriptorBootstrap&&)=default;SceneDescriptorBootstrap& operator=(SceneDescriptorBootstrap&&)=default;~SceneDescriptorBootstrap(){retire();}
    bool begin_fresh(const SceneDescriptorBootstrapProfile& profile,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("descriptor bootstrap: ")+why;return false;};if(attempted_||!profile.ready())return reject("fresh one-shot owner/profile required");attempted_=true;
        try{auto next=std::make_unique<Storage>();next->profile=profile;next->models=std::make_shared<SceneDescriptorPool>();next->archives=std::make_shared<SceneArchivePool>();
            if(!next->models->begin_fresh(profile.model_profile(),error)||!next->archives->begin_fresh(profile.archive_profile(),next->models,error))return false;
            for(unsigned i=2;i<4;++i){auto& s=next->small[i-2];const auto& row=*profile.row(i);const auto count=row.bytes/row.stride;s.free_head=0;s.free_count=s16(count);s.records.resize(count);for(unsigned j=0;j<count;++j){auto& r=s.records[j];r.pool08=u8(i);r.link0=j+1<count?s32(j+1):-1;}}
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native fresh allocation; no retry or source reset");}
    }
    bool current()const{return storage_&&storage_->models&&storage_->models->ready()&&storage_->archives&&storage_->archives->matches_descriptor_owner(storage_->models);}
    std::shared_ptr<SceneDescriptorPool> model_pool()const{return current()?storage_->models:nullptr;}
    std::shared_ptr<SceneArchivePool> archive_pool()const{return current()?storage_->archives:nullptr;}
    const SceneSmallDescriptorPoolSnapshot* small_pool(unsigned index)const{return current()&&index>=2&&index<4?&storage_->small[index-2]:nullptr;}
    const SceneDescriptorBootstrapProfile* recipe()const{return current()?&storage_->profile:nullptr;}
    //The next88398 call has no intervening descriptor consumer. Capacity alone
    //cannot prove freshness after allocate/free or a claimed class2 owner.
    bool pristine_for_physical_join()const{
        if(!current()||storage_->models->storage_->class2_claimed){return false;}const auto* m=storage_->models->snapshot();const auto* a=storage_->archives->snapshot();
        if(m->free_count!=1489||m->free_head!=0||m->live_head!=-1||a->free_count!=32||a->free_head!=0||a->live_head!=-1)return false;
        for(unsigned i=0;i<m->records.size();++i){const auto& r=m->records[i];if(r.generation||r.allocated09||r.native_root0c||r.link0!=(i+1<m->records.size()?s32(i+1):-1)||r.link4!=-1)return false;}
        for(unsigned i=0;i<a->records.size();++i){const auto& r=a->records[i];if(r.generation||r.allocated09||r.header_owned||r.link0!=(i+1<a->records.size()?s32(i+1):-1)||r.link4!=-1)return false;}
        return true;
    }
    bool small_pool_allocation_consumers_pending()const{return true;}
    void retire(){storage_.reset();attempted_=true;} //native lifetime invalidation, not SDK bulk cleanup
private:struct Storage {SceneDescriptorBootstrapProfile profile;std::shared_ptr<SceneDescriptorPool> models;std::shared_ptr<SceneArchivePool> archives;std::array<SceneSmallDescriptorPoolSnapshot,2> small;~Storage(){if(archives)archives->retire();if(models)models->retire();}};std::unique_ptr<Storage> storage_;bool attempted_=false;
};
} // namespace gf
