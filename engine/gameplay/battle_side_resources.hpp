#pragma once
#include "engine/assets/battle_side_archive.hpp"
#include "engine/gameplay/challenge_stage_data.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
namespace gf {
//42748/4279C consume the already-owned final side payload. No second file read,
//DMA acknowledgement or side-object construction. Archive allocation is ordered
//in the SAME scene pool; failed allocations are not silently unwound/replayed.
class BattleSideResources {
public:
    BattleSideResources()=default;BattleSideResources(const BattleSideResources&)=delete;BattleSideResources& operator=(const BattleSideResources&)=delete;
    BattleSideResources(BattleSideResources&&)=default;BattleSideResources& operator=(BattleSideResources&&)=default;
    bool initialize(std::shared_ptr<const ChallengeNativeStageData> data,SceneArchivePool& archives,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("battle side resources: ")+why;return false;};
        if(attempted_||!data||!archives.ready())return fail("fresh final data/current archive pool required");
        attempted_=true;
        try{
            auto next=std::make_unique<Storage>();next->data=std::move(data);
            if(next->data->selection().payloads[1].id==0xffffffffu){
                if(next->data->slots(1))return fail("absent side selection has payload");
            }else{
                if(next->data->slots(1)!=1)return fail("one raw sector archive required");
                const auto raw=next->data->slot(1,0);
                if(!battle_side_archive_layout(raw,&next->layout,error))return false;
                for(unsigned i=0;i<next->layout.count;++i){
                    const auto slice=next->layout.models[i];HsdRecordAnimationBank bank;
                    if(!bank.load({raw.data+slice.offset,slice.bytes},error,true))return false;
                    if(archives.acquire(&next->leases[i],error)!=SceneArchiveAcquire::Acquired)return fail("side archive allocation failed; no replay");
                    if(!archives.bind_borrowed_bank(next->leases[i],bank,error))return false;
                }
            }
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return fail("uncertain side decode/allocation; no replay");}
    }
    bool current()const{if(!storage_)return false;for(unsigned i=0;i<storage_->layout.count;++i)if(!storage_->leases[i].bank())return false;return true;}
    unsigned count()const{return current()?storage_->layout.count:0;}
    ByteView placement()const{if(!current()||!storage_->layout.placement.bytes)return {};const auto raw=storage_->data->slot(1,0);const auto s=storage_->layout.placement;return {raw.data+s.offset,s.bytes};}
    const HsdRecordAnimationBank* bank(unsigned i)const{return current()&&i<storage_->layout.count?storage_->leases[i].bank():nullptr;}
    const SceneArchiveRecord* record(unsigned i)const{return current()&&i<storage_->layout.count?storage_->leases[i].inspect():nullptr;}
    void retire(){storage_.reset();attempted_=true;}
private:
    friend class BattleStageConstruction;
    struct Storage {std::shared_ptr<const ChallengeNativeStageData> data;BattleSideArchiveLayout layout;std::array<SceneArchiveLease,8> leases;};
    std::unique_ptr<Storage> storage_;bool attempted_=false;
};
} // namespace gf
