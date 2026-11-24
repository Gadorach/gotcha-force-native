#pragma once
#include "engine/assets/startup_auxiliary.hpp"
namespace gf {
class BattleInitialCommonAuxiliaryProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;profile_={};auto fail=[&](){if(error)*error="initial common auxiliary: exact slot3/split-effect/inline2 source recipe required";return false;};
        if(!b.contains(0,240)||b.size!=240||b.be32(0)!=0x47464341||b.be32(4)!=1||b.be32(8)!=240||b.be32(12))return fail();
        const std::array<u32,12> keys{0x800c7664,0x800c76a0,0x80089100,0x80006fb4,0x80042954,0x80040eb4,0x80006c88,0x800c74ec,0x800c79f8,0x800c7954,0x800c77ec,0x800c790c};
        const std::array<u32,4> meta{3,0x615,0x6dd5d3b2,2};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        for(unsigned i=0;i<meta.size();++i)if(b.be32(64+4*i)!=meta[i])return fail();
        const std::array<u32,40> canonical{0x47464155,0x00010002,160,0x6dd5d3b2,0x25ac8,0x00030004,0x28000200,0x3303071c,0x2aaa0000,0x41900000,0x41700000,0xc1d80000,0x3ecccccd,0x3303071c,0x2aaa0000,0xc1900000,0x41700000,0xc1d80000,0x3ecccccd,0x29020200,0x33030000,0x0e380000,0x41900000,0x41700000,0xc1d80000,0x3ecccccd,0x33030000,0x0e380000,0xc1900000,0x41700000,0xc1d80000,0x3ecccccd,0x3f4ccccd,0x3f800000,0x3f4ccccd,0x3f800000,0x3f4ccccd,0x3f800000,0x38c90fdb,0x06150033};
        for(unsigned i=0;i<canonical.size();++i)if(b.be32(80+4*i)!=canonical[i])return fail();
        if(!startup_auxiliary_load({b.data+80,160},&profile_,error))return false;
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const StartupAuxiliaryProfile* auxiliary()const{return ready_?&profile_:nullptr;}
private:bool ready_=false;StartupAuxiliaryProfile profile_;
};
}
