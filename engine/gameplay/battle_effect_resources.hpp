#pragma once
#include "engine/assets/battle_effect_resources.hpp"
#include "engine/assets/challenge_finalization.hpp"
#include "engine/assets/tpl.hpp"
#include "engine/core/backends.hpp"
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/gameplay/scene_resource_work.hpp"
#include "engine/assets/battle_initial_graphics_archives.hpp"
#include <exception>
namespace gf {
enum class BattleEffectResourceStatus {Closed,Queued,Loaded,TexturesBound,Failed,Retired};
struct BattleEffectTextureBinding {
    TplImage image;ByteView source_pixels,source_palette;
    u32 wrap_s=0,wrap_t=0,min_filter=1,mag_filter=1; //8BED8 explicit clamp/linear
    f32 min_lod=0,max_lod=0,lod_bias=0;bool mipmaps=false;
};
class BattleEffectResources {
public:
    BattleEffectResources()=default;BattleEffectResources(const BattleEffectResources&)=delete;BattleEffectResources& operator=(const BattleEffectResources&)=delete;
    BattleEffectResources(BattleEffectResources&&)=default;BattleEffectResources& operator=(BattleEffectResources&&)=default;
    bool begin(const BattleEffectResourceProfile& profile,u8 mode,const std::string& path,std::string* error){
        if(status_!=BattleEffectResourceStatus::Closed||!profile.ready()||mode>3||path.empty())return fail(error,"fresh source profile/display mode/path required");
        status_=BattleEffectResourceStatus::Failed;
        try{auto next=std::make_unique<Storage>();next->profile=profile;next->path=path;next->selected=mode?2:1;
            next->cells[0].kind=next->cells[next->selected].kind=SceneResourceCellKind::Destination;
            storage_=std::move(next);status_=BattleEffectResourceStatus::Queued;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain queued-row initialization; no replay");}
    }
    bool load(IFileSystem& files,std::string* error){
        if(status_!=BattleEffectResourceStatus::Queued||!storage_)return fail(error,"current unconsumed queued load required");
        status_=BattleEffectResourceStatus::Failed;
        //Local candidate survives retirement/move during a filesystem callback.
        try{auto next=std::make_unique<Storage>();next->profile=storage_->profile;next->path=storage_->path;next->selected=storage_->selected;next->cells=storage_->cells;
            const auto scope=std::make_shared<const ReadScope>();read_scope_=scope;
            auto current=[&](){return read_scope_==scope&&status_==BattleEffectResourceStatus::Failed;};
            u64 size=0;if(!files.file_size(next->path,&size)||!current()||size!=next->profile.bytes()||!files.read(next->path,next->bytes)||!current())return fail(error,"missing/short/retired effect read; no replay");
            const ByteView b{next->bytes.data(),next->bytes.size()};
            if(b.size!=next->profile.bytes()||!challenge_native_bundle_valid(b)||b.be32(8)!=3||challenge_payload_hash(b)!=next->profile.hash())return fail(error,"effect bundle extent/identity");
            std::size_t at=40;for(unsigned i=0;i<3;++i){const auto n=b.be32(20+8*i);if(n!=next->profile.block_bytes(i))return fail(error,"effect block extent");
                if(next->cells[i].kind==SceneResourceCellKind::Destination){next->cells[i]={SceneResourceCellKind::Loaded,{b.data+at,n}};}at+=n;}
            const auto model=next->cells[next->selected].bytes;if(model.be32(0)<32||model.be32(0)>model.size)return fail(error,"selected effect HSD extent");
            next->model={model.data,model.be32(0)};storage_=std::move(next);status_=BattleEffectResourceStatus::Loaded;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain effect input ownership; no replay");}
    }
    bool bind_textures(std::string* error){
        if(status_!=BattleEffectResourceStatus::Loaded||!storage_)return fail(error,"current loaded effect row required");
        status_=BattleEffectResourceStatus::Failed;
        try{const auto input=storage_->cells[0].bytes;
            if(!input.contains(0,12)||input.be32(0)!=kTplMagic||input.be32(4)!=1)return fail(error,"one authored effect texture destination required");
            detail::TplLayout layout;BattleEffectTextureBinding next;
            if(!detail::tpl_validate(input,0,&layout,error)||!tpl_decode(input,0,&next.image,error))return false;
            next.source_pixels={input.data+layout.pixel_offset,std::size_t(layout.pixel_bytes)};
            if(layout.palette_bytes)next.source_palette={input.data+layout.palette_offset,std::size_t(layout.palette_bytes)};
            storage_->texture=std::move(next);status_=BattleEffectResourceStatus::TexturesBound;if(error)error->clear();return true;
        }catch(const std::exception&){return fail(error,"uncertain effect texture construction; no replay");}
    }
    bool current()const{return storage_&&(status_==BattleEffectResourceStatus::Queued||status_==BattleEffectResourceStatus::Loaded||status_==BattleEffectResourceStatus::TexturesBound);}
    const std::array<SceneResourceCell,3>* cells()const{return current()?&storage_->cells:nullptr;}
    ByteView model_input()const{return current()?storage_->model:ByteView{};}
    const BattleEffectTextureBinding* texture()const{return current()&&status_==BattleEffectResourceStatus::TexturesBound?&storage_->texture:nullptr;}
    const SceneArchiveRecord* archive_record()const{return current()?storage_->archive.inspect():nullptr;}
    const HsdRecordAnimationBank* bound_bank()const{const auto* bank=current()?storage_->archive.bank():nullptr;return bank&&bank->same_source(storage_->bank)?bank:nullptr;}
    u8 selected_model_block()const{return current()?storage_->selected:255;}
    BattleEffectResourceStatus status()const{return status_;}
    void retire(){read_scope_.reset();storage_.reset();status_=BattleEffectResourceStatus::Retired;}
private:
    friend class BattlePreparationBootstrap;
    bool bind_initial_graphics(const BattleInitialGraphicsArchivesProfile& profile,SceneArchivePool& archives,std::string* error){
        if(!profile.ready()||status_!=BattleEffectResourceStatus::TexturesBound||!storage_||storage_->graphics_attempted||storage_->selected!=2||!archives.ready()){if(error)*error="initial effect archive: current unconsumed split-model source required";return false;}
        storage_->graphics_attempted=true;
        if(!storage_->bank.load(storage_->model,error,true)||storage_->bank.hash()!=0x6dd5d3b2)return false;
        if(archives.acquire(&storage_->archive,error)!=SceneArchiveAcquire::Acquired)return false;
        return archives.bind_borrowed_bank(storage_->archive,storage_->bank,error);
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("battle effect resources: ")+why;return false;}
    struct ReadScope {};
    struct Storage {BattleEffectResourceProfile profile;std::string path;std::vector<u8> bytes;std::array<SceneResourceCell,3> cells;u8 selected=0;ByteView model;BattleEffectTextureBinding texture;SceneArchiveLease archive;HsdRecordAnimationBank bank;bool graphics_attempted=false;};
    std::shared_ptr<const ReadScope> read_scope_;std::unique_ptr<Storage> storage_;BattleEffectResourceStatus status_=BattleEffectResourceStatus::Closed;
};
} // namespace gf
