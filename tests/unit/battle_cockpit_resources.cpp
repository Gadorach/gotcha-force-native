#include <cstdio>
#include <cstdlib>
#include <functional>
#include "engine/gameplay/battle_cockpit_resources.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"cockpit resources: %s\n",why);std::abort();}}
struct Files:gf::IFileSystem {
    std::vector<gf::u8> bytes;unsigned reads=0;std::string path;std::function<void()> stat,read_action;
    bool file_size(const std::string& p,gf::u64* size)override{path=p;*size=bytes.size();if(stat)stat();return true;}
    bool read(const std::string&,std::vector<gf::u8>& out)override{++reads;out=bytes;if(read_action)read_action();return true;}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}bool erase(const std::string&)override{return false;}bool exists(const std::string&)override{return false;}
};}
int main(){
    using namespace gf;std::string error;Files files;files.bytes.resize(32);store_be32(files.bytes.data(),32);std::array<u8,64> raw{};
    const std::array<u32,4> head{0x47464350,1,3,48};for(unsigned i=0;i<4;++i)store_be32(raw.data()+4*i,head[i]);
    for(unsigned i=0;i<3;++i){store_be32(raw.data()+16+16*i,i);store_be32(raw.data()+20+16*i,2707+i);store_be32(raw.data()+24+16*i,32);store_be32(raw.data()+28+16*i,challenge_payload_hash({files.bytes.data(),files.bytes.size()}));}
    BattleCockpitResourceProfile profile;check(profile.load({raw.data(),raw.size()},&error)&&!profile.find(4)&&profile.find(3)==profile.find(1),"exact three profiles and source mode3 alias");
    for(u8 mode=0;mode<4;++mode){BattleCockpitResources owner;const auto reads=files.reads;check(owner.load(profile,mode,files,"owned",&error)&&files.reads==reads+1&&owner.ready()&&!owner.archive_record(),"raw input read, null header, no descriptor allocation");check(files.path=="owned/"+std::to_string(2707+(mode==3?1:mode))+".hsd","mode selects exact member");check(!owner.load(profile,mode,files,"owned",&error)&&files.reads==reads+1,"no reread");const auto* identity=owner.input().data;BattleCockpitResources moved=std::move(owner);check(moved.ready()&&!owner.ready()&&moved.input().data==identity,"stable byte ownership across move");moved.retire();check(!moved.input().data&&!moved.load(profile,mode,files,"owned",&error),"retirement retains one-shot claim");}
    for(unsigned route=0;route<3;++route){BattleCockpitResources owner,moved;const auto reads=files.reads;if(route==0)files.stat=[&](){owner.retire();};if(route==1)files.read_action=[&](){owner.retire();};if(route==2)files.read_action=[&](){moved=std::move(owner);};check(!owner.load(profile,0,files,"owned",&error)&&!owner.ready()&&!moved.ready()&&files.reads==reads+(route==0?0:1),"retirement/move prevents late publication and stat retirement prevents read");files.stat={};files.read_action={};}
    files.bytes.back()=1;BattleCockpitResources corrupt;check(!corrupt.load(profile,0,files,"owned",&error)&&!corrupt.ready(),"wrong source bytes refused");
    for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,32u,36u,40u,48u,52u,56u}){auto bad=raw;bad[at]^=1;BattleCockpitResourceProfile invalid;check(!invalid.load({bad.data(),bad.size()},&error)&&!invalid.ready(),"malformed source recipe/extent");}
    std::printf("battle cockpit resources: %u checks PASS\n",checks);
}
