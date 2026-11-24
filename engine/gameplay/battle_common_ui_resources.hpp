#pragma once
#include "engine/assets/battle_common_ui_resources.hpp"
#include "engine/assets/challenge_finalization.hpp"
#include "engine/core/backends.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
#include <exception>
namespace gf {
struct BattlePlayerFaceSlot {SceneArchiveLease archive;ByteView data;u8 selected2c=255;};
//439A0(1)→40EDC/40EB4: one shared decoded archive and an actual same-domain
//descriptor. Model instances and UI callbacks are separate consumers.
class BattleCommonUiResources {
public:
    BattleCommonUiResources()=default;BattleCommonUiResources(const BattleCommonUiResources&)=delete;BattleCommonUiResources& operator=(const BattleCommonUiResources&)=delete;
    BattleCommonUiResources(BattleCommonUiResources&&)=default;BattleCommonUiResources& operator=(BattleCommonUiResources&&)=default;
    bool load(const BattleCommonUiProfile& profile,IFileSystem& files,const std::string& path,const std::shared_ptr<SceneArchivePool>& pool,std::string* error){
        if(attempted_||!profile.ready()||path.empty()||!pool||!pool->ready())return fail(error,"fresh profile/current pool/path required");
        attempted_=true;const auto recipe=profile;const auto archives=pool;
        try{const auto scope=std::make_shared<const ReadScope>();read_scope_=scope;auto live=[&](){return read_scope_==scope&&!retired_;};
            std::vector<u8> bytes;u64 size=0;if(!files.file_size(path,&size)||!live()||size!=recipe.bytes()||!files.read(path,bytes)||!live())return fail(error,"missing/short/retired read; no replay");
            const ByteView b{bytes.data(),bytes.size()};if(b.size!=recipe.bytes()||!b.contains(0,32)||b.be32(0)!=b.size||challenge_payload_hash(b)!=recipe.hash())return fail(error,"archive byte identity/extent");
            auto next=std::make_unique<Storage>();if(!next->bank.load(b,error,true))return false;
            if(!archives->ready())return fail(error,"archive domain retired during read");
            if(archives->acquire(&next->archive,error)!=SceneArchiveAcquire::Acquired)return fail(error,"archive allocation failed; prefix retained");
            if(!archives->bind_borrowed_bank(next->archive,next->bank,error))return false;
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain resource read/bind; no replay");}
    }
    bool current()const{const auto* bank=storage_?storage_->archive.bank():nullptr;return bank&&bank->same_source(storage_->bank);}
    const HsdRecordAnimationBank* bank()const{return current()?&storage_->bank:nullptr;}
    const SceneArchiveRecord* archive_record()const{return current()?storage_->archive.inspect():nullptr;}
    void retire(){read_scope_.reset();storage_.reset();attempted_=retired_=true;}
private:
    friend class BattlePreparationBootstrap;
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("battle common UI resources: ")+why;return false;}
    struct ReadScope {};struct Storage {HsdRecordAnimationBank bank;SceneArchiveLease archive;};
    std::shared_ptr<const ReadScope> read_scope_;std::unique_ptr<Storage> storage_;bool attempted_=false,retired_=false;
};
} // namespace gf
