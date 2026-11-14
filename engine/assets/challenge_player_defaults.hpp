#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <algorithm>
#include <string>
namespace gf {
//962C4's shared six-slot tail, NOT common+50 mode or an AI-only default.
struct ChallengePlayerDefaults {u8 field20=0,field32=0,field3e=0;bool operator==(const ChallengePlayerDefaults&)const=default;};
class ChallengePlayerDefaultsProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};if(!b.data||b.size!=32||b.be32(0)!=0x47465044||b.be32(4)!=1||b.be32(8)!=32||b.be32(12)||std::any_of(b.data+25,b.data+32,[](u8 v){return v!=0;})){if(error)*error="Challenge player defaults: header/extent/reserved";return false;}
        field20_=b.data[16];field3e_=b.data[17];std::copy_n(b.data+18,7,table_.begin());ready_=true;if(error)error->clear();return true;
    }
    //Source signed team=-1 reads the immediately preceding byte. It is actual
    //retained table DATA, not a sentinel/default-to-zero or active-slot shortcut.
    bool select(unsigned difficulty,s8 team,ChallengePlayerDefaults* out)const{
        if(out)*out={};
        if(!out||!ready_||difficulty>=3||team< -1||team>1)return false;
        *out={field20_,table_[unsigned(int(difficulty)*2+team+1)],field3e_};return true;
    }
    bool ready()const{return ready_;}
private:std::array<u8,7> table_{};u8 field20_=0,field3e_=0;bool ready_=false;
};
} // namespace gf
