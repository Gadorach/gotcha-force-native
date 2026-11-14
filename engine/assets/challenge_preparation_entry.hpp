#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
//Normal fresh Challenge entry ONLY. Source keys are non-callable provenance;
//this is not a replay/resume-state profile or a general common-context codec.
class ChallengePreparationEntryProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="Challenge preparation entry: fresh normal source recipe required";return false;};
        if(!b.data||b.size!=64||b.be32(0)!=0x47465045||b.be32(4)!=1||b.be32(8)!=64||b.be32(12)!=0x800bfdac||b.be32(16)!=0x80195f74||b.be32(20)!=0x80195fe0||b.be32(24)!=6328||b.be32(28)!=0x01000000||b.be32(32)||b.be32(36)||b.be32(40)||b.be32(44)!=0x800284f4||b.be32(48)||b.be32(52)||b.be32(56)||b.be32(60))return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
} // namespace gf
