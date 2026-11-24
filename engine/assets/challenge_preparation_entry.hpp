#pragma once
#include <array>
#include "engine/core/types.hpp"
#include <string>
namespace gf {
//Normal fresh Challenge entry ONLY. Source keys are non-callable provenance;
//this is not a replay/resume-state profile or a general common-context codec.
class ChallengePreparationEntryProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=dispatch_=mode_=false;auto fail=[&](){if(error)*error="Challenge preparation entry: fresh normal source recipe required";return false;};
        if(!b.contains(0,64)||b.be32(0)!=0x47465045||b.be32(4)<1||b.be32(4)>3)return fail();
        const bool mode=b.be32(4)==3;const unsigned size=mode?96:64;
        if(b.size!=size||b.be32(8)!=size||b.be32(12)!=0x800bfdac||b.be32(16)!=0x80195f74||b.be32(20)!=0x80195fe0||b.be32(24)!=6328||b.be32(28)!=0x01000000||b.be32(32)||b.be32(36)||b.be32(40)||b.be32(44)!=0x800284f4)return fail();
        if(b.be32(4)==1){if(b.be32(48)||b.be32(52)||b.be32(56)||b.be32(60))return fail();}
        else{if(b.be32(48)!=0x80017918||b.be32(52)!=0x80196948||b.be32(56)!=0x01030100||b.be32(60)!=0x802d2e48)return fail();dispatch_=true;}
        if(mode){
            const std::array<u32,8> tail{0x8002821c,0x802cfdc4,0x80195f2c,1,0x801962c4,0x46,0,0x80195fbc};
            for(unsigned i=0;i<tail.size();++i)if(b.be32(64+4*i)!=tail[i])return fail();
            mode_=true;
        }
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    //V2 opts into the enclosing normal menu-mode1 -> battle-mode3 return.
    //V1 preserves UNKNOWN common00; it does not silently gain this lineage.
    bool normal_dispatch()const{return ready_&&dispatch_;}
    //V3 explicitly selects the source table's Challenge index1 BEFORE setup.
    //V1/V2 retain UNKNOWN common32; the Challenge label is not a late setter.
    bool challenge_mode_lineage()const{return ready_&&mode_;}
 private:bool ready_=false,dispatch_=false,mode_=false;
};
} // namespace gf
