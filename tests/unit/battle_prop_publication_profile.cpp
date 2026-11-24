#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_prop_publication.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"prop publication profile: %s\n",why);std::abort();}}}
int main(){
 using namespace gf;std::array<u8,80> bytes{};std::string error;
 const std::array<u32,20> words{0x47465050,1,80,0,0x80089eb4,0x80087b64,0x80087c84,0x8008b290,0x800097b4,0x8008c0dc,0x8008c1b0,0x80009790,48,30,18,0x17,0x14,0x0b,1,0};
 for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
 BattlePropPublicationProfile profile;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready(),"source prefix operands");
 for(unsigned i=0;i<words.size();++i){auto bad=bytes;bad[4*i]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready(),"changed recipe cannot select a different callback/count/mask");}
 check(!profile.load({},&error),"missing operands refuse");
 std::printf("prop publication source profile: %u checks PASS\n",checks);
}
