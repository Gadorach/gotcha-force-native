#include <cstdio>
#include <cstdlib>
#include <functional>
#include "engine/gameplay/battle_common_resources.hpp"
namespace {unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"common resources: %s\n",why);std::abort();}}
struct Files:gf::IFileSystem {std::vector<gf::u8> data;unsigned reads=0,sizes=0,writes=0;bool missing=false;std::function<void()> during_read;
    bool file_size(const std::string&,gf::u64* n)override{++sizes;if(missing)return false;*n=data.size();return true;}
    bool read(const std::string&,std::vector<gf::u8>& out)override{++reads;out=data;if(during_read)during_read();return true;}
    bool write(const std::string&,const gf::u8*,std::size_t)override{++writes;return false;}bool exists(const std::string&)override{return false;}bool erase(const std::string&)override{++writes;return false;}
};}
int main(){using namespace gf;std::string e;Files files;files.data.resize(48+16);store_be32(files.data.data(),0x47464e42);store_be32(files.data.data()+4,1);store_be32(files.data.data()+8,4);store_be32(files.data.data()+12,files.data.size()-16);for(unsigned i=0;i<4;++i){store_be32(files.data.data()+16+8*i,i);store_be32(files.data.data()+20+8*i,4);store_be32(files.data.data()+48+4*i,0x12340000+i);}std::array<u8,64> raw{};const std::array<u32,16> words{0x4746434f,1,64,696,u32(files.data.size()),challenge_payload_hash({files.data.data(),files.data.size()}),4,4,4,4,0x25f400,0x264c00,0x1f3400,0x1f2c00,0xa49800,0};for(unsigned i=0;i<16;++i)store_be32(raw.data()+4*i,words[i]);BattleCommonResourceProfile p;check(p.load({raw.data(),raw.size()},&e),"owned four-block source recipe");
    BattleCommonResources common;check(common.load(p,files,"owned",&e)&&common.ready()&&files.reads==1&&files.sizes==1&&!files.writes,"actual one-shot IFileSystem byte owner");files.data.back()^=255;for(unsigned i=0;i<4;++i)check(common.block(i).be32(0)==0x12340000+i&&common.block(i).size==4,"source input mutation cannot alter owned blocks");check(common.context98().data==common.block(0).data&&common.context9c().data==common.block(1).data&&common.common_motion_bytes().data==common.block(2).data&&common.common_parameters().data==common.block(3).data&&!common.block(4).data,"exact distinct source role bindings, no header/geometry/pose decode");check(!common.load(p,files,"owned",&e)&&files.reads==1,"no re-read/reset on live owner");BattleCommonResources moved=std::move(common);check(moved.ready()&&!common.ready(),"move keeps exact byte allocation");moved.retire();check(!moved.ready()&&!moved.context98().data&&!moved.load(p,files,"owned",&e),"retirement clears views, claim retained");
    BattleCommonResources wrong;check(!wrong.load(p,files,"wrong",&e)&&!wrong.ready(),"content hash mismatch no partial state");const auto reads=files.reads;check(!wrong.load(p,files,"wrong",&e)&&files.reads==reads,"failed read not replayed");
    files.data.back()^=255;BattleCommonResources reentrant;files.during_read=[&](){reentrant.retire();};check(!reentrant.load(p,files,"reentrant",&e)&&!reentrant.ready(),"retire during read prevents late publication");files.during_read={};
    for(unsigned at:{0u,4u,8u,12u,24u,40u,44u,48u,52u,56u,60u}){auto bad=raw;bad[at]^=1;BattleCommonResourceProfile x;check(!x.load({bad.data(),bad.size()},&e)&&!x.ready(),"bad source recipe/extent refuses");}
    check(!p.load({nullptr,64},&e)&&!p.load({raw.data(),63},&e),"null/short profile");
    BattleCommonResourceProfile linked_profile;check(linked_profile.load({raw.data(),raw.size()},&e),"reload scope recipe");
    BattleCommonResources linked,neighbor;check(linked.load(linked_profile,files,"linked",&e)&&neighbor.load(linked_profile,files,"neighbor",&e),"equal bytes in separate actual owners");
    const auto reference=linked.binding();check(reference.live()&&reference.block(2).data==linked.common_motion_bytes().data&&!reference.same_owner(neighbor.binding()),"scoped common binding distinguishes equal bytes");
    BattleCommonResources relocated=std::move(linked);check(reference.same_owner(relocated.binding())&&!linked.ready(),"binding follows heap-stable move");relocated.retire();check(!reference.live()&&!reference.block(2).data,"binding cannot retain retired common bytes");
    BattleCommonResources reading,receiving;files.during_read=[&](){receiving=std::move(reading);};check(!reading.load(linked_profile,files,"moving",&e)&&!reading.ready()&&!receiving.ready(),"move during read cannot publish through moved-from owner");
    std::printf("battle common resources: %u owned checks PASS\n",checks);
}
