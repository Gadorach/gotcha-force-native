#pragma once
#include "engine/assets/force_catalog.hpp"
namespace gf {
struct ChallengeRandomForceProfile {
    u32 catalog_hash=0;u16 preferred_model=0,special_model=0,default_model=0;std::array<u8,41> name{};
    bool for_catalog(const ForceCatalog& c)const{return c.ready()&&catalog_hash==c.fingerprint()&&c.find(preferred_model)&&c.find(special_model)&&c.find(default_model)&&name[0]&&std::find(name.begin(),name.end(),u8(0))!=name.end();}
};
inline bool challenge_random_force_load(ByteView b,const ForceCatalog& catalog,ChallengeRandomForceProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="Challenge Random Force: profile/catalog domain";return false;};
    if(!out||!b.data||b.size!=80||b.be32(0)!=0x47465246||b.be32(4)!=1||b.be16(18)!=30||b.be16(20)!=200||b.be16(22)!=2000)return fail();
    for(unsigned i=65;i<80;++i)if(b.data[i])return fail();
    ChallengeRandomForceProfile p;p.catalog_hash=b.be32(8);p.preferred_model=b.be16(12);p.special_model=b.be16(14);p.default_model=b.be16(16);std::copy_n(b.data+24,41,p.name.begin());
    if(!p.for_catalog(catalog))return fail();
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
