#pragma once
#include "engine/assets/battle_common_resources.hpp"
#include "engine/core/backends.hpp"
#include <memory>
#include <exception>
namespace gf {
namespace battle_common_resource_detail {
struct Storage {bool live=true;std::vector<u8> bytes;std::array<std::size_t,4> offsets{},sizes{};};
}
class BattleCommonResourceBinding {
public:
    bool live()const{const auto s=storage_.lock();return s&&s->live;}
    ByteView block(unsigned i)const{const auto s=storage_.lock();return s&&s->live&&i<4?ByteView{s->bytes.data()+s->offsets[i],s->sizes[i]}:ByteView{};}
    bool same_owner(const BattleCommonResourceBinding& other)const{const auto s=storage_.lock();return s&&s->live&&s==other.storage_.lock();}
private:friend class BattleCommonResources;std::weak_ptr<const battle_common_resource_detail::Storage> storage_;
};
//Owns the actual four decoded native byte blocks, not motion players or frame
//authority. No second PZZ/ARAM decoder or host pointer in a guest word.
class BattleCommonResources {
public:
    BattleCommonResources()=default;BattleCommonResources(const BattleCommonResources&)=delete;BattleCommonResources& operator=(const BattleCommonResources&)=delete;
    BattleCommonResources(BattleCommonResources&&)=default;BattleCommonResources& operator=(BattleCommonResources&&)=default;
    ~BattleCommonResources(){retire();}
    bool load(const BattleCommonResourceProfile& profile,IFileSystem& files,const std::string& path,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("battle common resources: ")+why;return false;};
        if(attempted_||!profile.ready()||path.empty()){return reject("fresh owner/source profile/path required");}attempted_=true;
        const auto recipe=profile;
        try{const auto scope=std::make_shared<const ReadScope>();read_scope_=scope;auto current=[&](){return !retired_&&read_scope_==scope;};
            auto next=std::make_shared<battle_common_resource_detail::Storage>();u64 size=0;if(!files.file_size(path,&size)||!current()||size!=recipe.bytes()||!files.read(path,next->bytes)||!current())return reject("missing/short/retired native read; no replay");const ByteView b{next->bytes.data(),next->bytes.size()};if(b.size!=recipe.bytes()||!challenge_native_bundle_valid(b)||b.be32(8)!=4||challenge_payload_hash(b)!=recipe.hash())return reject("native bytes/profile identity mismatch");
            std::size_t at=48;for(unsigned i=0;i<4;++i){if(b.be32(20+8*i)!=recipe.block_bytes(i))return reject("source block extent mismatch");next->offsets[i]=at;next->sizes[i]=recipe.block_bytes(i);at+=next->sizes[i];}storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){return reject("uncertain native common-resource read; no replay");}
    }
    bool ready()const{return storage_&&storage_->live;}ByteView block(unsigned i)const{return ready()&&i<4?ByteView{storage_->bytes.data()+storage_->offsets[i],storage_->sizes[i]}:ByteView{};}
    ByteView context98()const{return block(0);}ByteView context9c()const{return block(1);}
    ByteView common_motion_bytes()const{return block(2);}ByteView common_parameters()const{return block(3);}
    BattleCommonResourceBinding binding()const{BattleCommonResourceBinding result;if(ready())result.storage_=storage_;return result;}
    void retire(){read_scope_.reset();if(storage_)storage_->live=false;storage_.reset();attempted_=retired_=true;}
private:struct ReadScope {};std::shared_ptr<const ReadScope> read_scope_;std::shared_ptr<battle_common_resource_detail::Storage> storage_;bool attempted_=false,retired_=false;
};
} // namespace gf
