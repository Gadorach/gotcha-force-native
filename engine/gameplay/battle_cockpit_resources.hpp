#pragma once
#include "engine/assets/battle_cockpit_resources.hpp"
#include "engine/assets/challenge_finalization.hpp"
#include "engine/core/backends.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/assets/battle_initial_graphics_archives.hpp"
#include <exception>
#include <memory>
namespace gf {
//42520: raw native archive ownership and an explicitly null header slot.
//No HSD instance, cockpit widget, camera or draw authority is granted here.
class BattleCockpitResources {
public:
    BattleCockpitResources()=default;BattleCockpitResources(const BattleCockpitResources&)=delete;BattleCockpitResources& operator=(const BattleCockpitResources&)=delete;
    BattleCockpitResources(BattleCockpitResources&&)=default;BattleCockpitResources& operator=(BattleCockpitResources&&)=default;
    bool load(const BattleCockpitResourceProfile& profile,u8 mode,IFileSystem& files,const std::string& directory,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("battle cockpit resources: ")+why;return false;};
        const auto* selected=profile.find(mode);if(attempted_||!selected||directory.empty())return fail("fresh supported selection/profile/path required");
        const auto row=*selected;
        attempted_=true;
        try{const auto scope=std::make_shared<const ReadScope>();read_scope_=scope;auto live=[&](){return !retired_&&read_scope_==scope;};
            auto next=std::make_unique<Storage>();next->selection=row;next->mode=mode;const auto path=directory+"/"+std::to_string(row.member)+".hsd";u64 bytes=0;
            if(!files.file_size(path,&bytes)||!live()||bytes!=row.bytes||!files.read(path,next->bytes)||!live())return fail("missing/short/retired read; no replay");
            const ByteView b{next->bytes.data(),next->bytes.size()};
            if(b.size!=row.bytes||!b.contains(0,32)||b.be32(0)!=b.size||challenge_payload_hash(b)!=row.hash)return fail("native archive extent/identity");
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return fail("uncertain archive read; no replay");}
    }
    bool ready()const{return bool(storage_);}
    ByteView input()const{return ready()?ByteView{storage_->bytes.data(),storage_->bytes.size()}:ByteView{};}
    const BattleCockpitResourceRow* selection()const{return ready()?&storage_->selection:nullptr;}
    const SceneArchiveRecord* archive_record()const{return ready()?storage_->header.inspect():nullptr;}
    const HsdRecordAnimationBank* bound_bank()const{const auto* bank=ready()?storage_->header.bank():nullptr;return bank&&bank->same_source(storage_->bank)?bank:nullptr;}
    const HsdInitialCockpitLightInputs* initial_light_inputs()const{return bound_bank()&&storage_->light_inputs.generation==1?&storage_->light_inputs:nullptr;}
    void retire(){read_scope_.reset();storage_.reset();attempted_=retired_=true;}
private:
    friend class BattlePreparationBootstrap;
    bool bind_initial_graphics(const BattleInitialGraphicsArchivesProfile& profile,SceneArchivePool& archives,std::string* error){
        if(!profile.ready()||!ready()||storage_->graphics_attempted||storage_->mode!=1||storage_->selection.member!=2708||!archives.ready()){if(error)*error="initial cockpit archive: fresh loaded split-mode source required";return false;}
        storage_->graphics_attempted=true;
        if(!storage_->bank.load(input(),error,true)||storage_->bank.hash()!=0x2fbc80c9)return false;
        if(archives.acquire(&storage_->header,error)!=SceneArchiveAcquire::Acquired)return false;
        if(!archives.bind_borrowed_bank(storage_->header,storage_->bank,error))return false;
        return storage_->bank.read_initial_cockpit_light_inputs(&storage_->light_inputs,error);
    }
    struct ReadScope {};
    struct Storage {BattleCockpitResourceRow selection;u8 mode=0;std::vector<u8> bytes;SceneArchiveLease header;HsdRecordAnimationBank bank;HsdInitialCockpitLightInputs light_inputs;bool graphics_attempted=false;};
    std::shared_ptr<const ReadScope> read_scope_;std::unique_ptr<Storage> storage_;bool attempted_=false,retired_=false;
};
} // namespace gf
